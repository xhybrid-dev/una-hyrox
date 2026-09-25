#!/usr/bin/env python3
"""hybridx-intervals PLAN P0, Step 3: write a small test file into the
Intervals Probe app's own "Plans/" folder over the watch's BLE File Transfer
Service, then list that folder back as a self-check.

Classic (protocol version 4) flow only -- MKDIR, WRITE/WRITE_DATA/WRITE_PACING,
LISTDIR (una-sdk/Docs/BLE-File-Transfer-Service.md). No windowing, no DIGEST:
the test payload is one small chunk, well under any plausible MTU.

`SDK::Kernel::fs` is sandbox-rooted per app (Docs/app-config-fields.md:27), so
the probe app's own "Plans/" folder is reached over BLE at the absolute path
/Apps/<APP_FILE_NAME>/Plans/ -- APP_FILE_NAME "HXIntervalsProbe" for this
probe (Software/Apps/Probe-CMake/CMakeLists.txt).

Usage:
    pip install -r requirements.txt
    python3 send_plan.py                       # scans for the watch by FTS service UUID
    python3 send_plan.py --name "My Watch"      # or match by advertised name
    python3 send_plan.py --address AA:BB:CC:DD:EE:FF

The watch must already be paired/bonded via the OS's own Bluetooth settings
first (docs/PROBE.md) -- this script does not attempt to drive pairing itself,
since how that's triggered is exactly one of the two things this probe exists
to find out.
"""

from __future__ import annotations

import argparse
import asyncio
import struct
import sys
import time
from datetime import datetime, timezone

from bleak import BleakClient, BleakScanner

import protocol as fts

APP_DIR = "/Apps/HXIntervalsProbe"
PLANS_DIR = f"{APP_DIR}/Plans"
DEFAULT_FILE = "probe_plan.json"

SCAN_TIMEOUT_S = 10.0
RESPONSE_TIMEOUT_S = 10.0


def log(msg: str) -> None:
    print(f"[send_plan] {msg}", flush=True)


class ResponseWaiter:
    """Collects Raw Transfer notifications so the async command flow can await one."""

    def __init__(self) -> None:
        self.queue: asyncio.Queue[bytes] = asyncio.Queue()

    def handle_notification(self, _sender, data: bytearray) -> None:
        self.queue.put_nowait(bytes(data))

    async def next(self, timeout: float = RESPONSE_TIMEOUT_S) -> bytes:
        return await asyncio.wait_for(self.queue.get(), timeout=timeout)


async def find_watch(name: str | None, address: str | None):
    if address:
        log(f"connecting directly to address {address}")
        return address

    log(f"scanning for {SCAN_TIMEOUT_S:.0f}s "
        f"(matching {'name ' + repr(name) if name else 'the File Transfer Service UUID'})...")
    devices = await BleakScanner.discover(timeout=SCAN_TIMEOUT_S)
    for d in devices:
        if name and d.name and name.lower() in d.name.lower():
            log(f"found by name: {d.name} ({d.address})")
            return d.address
    if not name:
        for d in devices:
            uuids = (d.metadata or {}).get("uuids", []) if hasattr(d, "metadata") else []
            if any(fts.FTS_SERVICE_UUID.lower() == u.lower() for u in uuids):
                log(f"found by service UUID: {d.name} ({d.address})")
                return d.address

    log("no match found. Devices seen:")
    for d in devices:
        log(f"  {d.address}  {d.name!r}")
    return None


async def do_mkdir(client: BleakClient, waiter: ResponseWaiter, path: str) -> None:
    log(f"MKDIR {path}")
    packet = fts.encode_mkdir(path, current_time_ns=time.time_ns())
    await client.write_gatt_char(fts.RAW_TRANSFER_CHAR_UUID, packet, response=False)
    raw = await waiter.next()
    result = fts.decode_mkdir_status(raw)
    log(f"  -> {fts.status_name(result.status)}")
    if result.status not in (fts.STATUS_OK, fts.STATUS_ERROR):
        # A pre-existing directory is an acceptable response per the doc; the
        # exact status byte for "already exists" is unconfirmed -- a finding
        # for docs/NOTES.md once Jon has run this against real hardware.
        log("  (unexpected status -- note this in docs/NOTES.md)")


async def do_write(client: BleakClient, waiter: ResponseWaiter, path: str, payload: bytes) -> None:
    log(f"WRITE {path} ({len(payload)} bytes)")
    write_packet = fts.encode_write(path, offset=0, current_time_ns=time.time_ns(), total_size=len(payload))
    await client.write_gatt_char(fts.RAW_TRANSFER_CHAR_UUID, write_packet, response=False)

    raw = await waiter.next()
    ack = fts.decode_write_pacing(raw)
    log(f"  WRITE_PACING -> {fts.status_name(ack.status)}, freeSpace={ack.free_space}")
    if ack.status != fts.STATUS_OK:
        raise RuntimeError(f"WRITE rejected: {fts.status_name(ack.status)}")

    bytes_acked = len(payload) - ack.free_space
    while ack.free_space > 0:
        chunk = payload[bytes_acked:]
        data_packet = fts.encode_write_data(offset=bytes_acked, chunk=chunk)
        await client.write_gatt_char(fts.RAW_TRANSFER_CHAR_UUID, data_packet, response=False)

        raw = await waiter.next()
        ack = fts.decode_write_pacing(raw)
        bytes_acked = len(payload) - ack.free_space
        log(f"  WRITE_DATA -> {fts.status_name(ack.status)}, "
            f"acked={bytes_acked}/{len(payload)}, freeSpace={ack.free_space}")
        if ack.status != fts.STATUS_OK:
            raise RuntimeError(f"WRITE_DATA rejected: {fts.status_name(ack.status)}")

    log("  WRITE complete (freeSpace == 0)")


async def do_listdir(client: BleakClient, waiter: ResponseWaiter, path: str) -> list[fts.ListEntry]:
    log(f"LISTDIR {path}")
    packet = fts.encode_listdir(path)
    await client.write_gatt_char(fts.RAW_TRANSFER_CHAR_UUID, packet, response=False)

    entries: list[fts.ListEntry] = []
    while True:
        raw = await waiter.next()
        entry = fts.decode_listdir_entry(raw)
        if entry.is_terminator:
            break
        entries.append(entry)
        kind = "dir " if entry.is_dir else "file"
        log(f"  [{entry.entry_number + 1}/{entry.total_entries}] {kind} {entry.name} ({entry.file_size} bytes)")
    log(f"  -> {len(entries)} entries")
    return entries


async def run(name: str | None, address: str | None, filename: str) -> int:
    target = await find_watch(name, address)
    if not target:
        log("could not find the watch -- pass --name or --address. Is it paired and advertising?")
        return 1

    async with BleakClient(target) as client:
        log(f"connected to {target}")

        try:
            version_bytes = await client.read_gatt_char(fts.VERSION_CHAR_UUID)
            (version,) = struct.unpack("<I", version_bytes[:4])
            log(f"FTS protocol version: {version} (this script only uses classic v4 commands)")
        except Exception as exc:  # noqa: BLE001 -- purely informational
            log(f"could not read Version characteristic ({exc}); continuing with classic commands anyway")

        waiter = ResponseWaiter()
        await client.start_notify(fts.RAW_TRANSFER_CHAR_UUID, waiter.handle_notification)

        try:
            await do_mkdir(client, waiter, PLANS_DIR)

            sent_at = datetime.now(timezone.utc).isoformat()
            payload = ('{"probe": true, "sentAt": "%s"}' % sent_at).encode("utf-8")
            file_path = f"{PLANS_DIR}/{filename}"
            await do_write(client, waiter, file_path, payload)

            entries = await do_listdir(client, waiter, PLANS_DIR)
            found = [e for e in entries if e.name == filename]
            if found:
                log(f"self-check OK: {filename} is listed, {found[0].file_size} bytes")
            else:
                log(f"self-check FAILED: {filename} not found in LISTDIR of {PLANS_DIR}")
                return 1
        finally:
            await client.stop_notify(fts.RAW_TRANSFER_CHAR_UUID)

    log("done. Now open the Intervals Probe app on the watch and check what it reports.")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--name", help="substring of the watch's advertised BLE name")
    parser.add_argument("--address", help="connect directly by BLE address, skipping the scan")
    parser.add_argument("--file", default=DEFAULT_FILE, help=f"file name to write (default: {DEFAULT_FILE})")
    args = parser.parse_args()

    try:
        return asyncio.run(run(args.name, args.address, args.file))
    except KeyboardInterrupt:
        return 130


if __name__ == "__main__":
    sys.exit(main())
