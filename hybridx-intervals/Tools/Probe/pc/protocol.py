"""BLE File Transfer Service (FTS) wire encoding -- classic (v4) commands only.

Pure struct packing/unpacking against una-sdk/Docs/BLE-File-Transfer-Service.md.
No BLE dependency here (that's send_plan.py, which imports this module), so
the byte layout can be checked with plain `unittest` and no hardware
(test_protocol.py) -- the one part of P0 this container CAN verify.

Only the commands hybridx-intervals PLAN Step 3 needs: MKDIR, the classic
WRITE flow, and LISTDIR. DELETE, MOVE, READ and the version-5 extensions are
out of scope for this throwaway probe.
"""

from __future__ import annotations

import struct
from dataclasses import dataclass

# GATT UUIDs (Docs/BLE-File-Transfer-Service.md "Service and characteristics").
# The characteristic IDs are UNA-specific (ADAF0001/ADAF0002), not Adafruit's
# upstream ADAF0100/ADAF0200 -- these exact UUIDs are what the watch exposes.
FTS_SERVICE_UUID = "0000febb-0000-1000-8000-00805f9b34fb"
VERSION_CHAR_UUID = "adaf0001-4669-6c65-5472-616e73666572"
RAW_TRANSFER_CHAR_UUID = "adaf0002-4669-6c65-5472-616e73666572"

# Commands.
CMD_WRITE = 0x20
CMD_WRITE_PACING = 0x21
CMD_WRITE_DATA = 0x22
CMD_MKDIR = 0x40
CMD_MKDIR_STATUS = 0x41
CMD_LISTDIR = 0x50
CMD_LISTDIR_ENTRY = 0x51

# Status codes (the `status` byte in responses).
STATUS_OK = 0x01
STATUS_ERROR = 0x02
STATUS_ERROR_NO_FILE = 0x03
STATUS_ERROR_PROTOCOL = 0x04
STATUS_ERROR_READ_ONLY = 0x05

STATUS_NAMES = {
    STATUS_OK: "OK",
    STATUS_ERROR: "ERROR",
    STATUS_ERROR_NO_FILE: "ERROR_NO_FILE",
    STATUS_ERROR_PROTOCOL: "ERROR_PROTOCOL",
    STATUS_ERROR_READ_ONLY: "ERROR_READ_ONLY",
}


def status_name(status: int) -> str:
    return STATUS_NAMES.get(status, "UNKNOWN(0x%02X)" % status)


def _path_bytes(path: str) -> bytes:
    return path.encode("utf-8")


# ---------------------------------------------------------------------------
# MKDIR 0x40 -> 0x41
# ---------------------------------------------------------------------------

def encode_mkdir(path: str, current_time_ns: int) -> bytes:
    """request {command, reserved, pathLength, reserved(4), currentTime(8)} + path"""
    p = _path_bytes(path)
    header = struct.pack("<BBHIQ", CMD_MKDIR, 0, len(p), 0, current_time_ns)
    return header + p


@dataclass
class MkdirStatus:
    command: int
    status: int
    truncated_time_ns: int


def decode_mkdir_status(data: bytes) -> MkdirStatus:
    """response {command, status, reserved(6), truncatedTime(8)} -- 16 bytes"""
    if len(data) < 16:
        raise ValueError("MKDIR status too short: %d bytes" % len(data))
    command, status, truncated_time_ns = struct.unpack_from("<BB6xQ", data, 0)
    return MkdirStatus(command=command, status=status, truncated_time_ns=truncated_time_ns)


# ---------------------------------------------------------------------------
# WRITE 0x20 -> WRITE_PACING 0x21 (ack), WRITE_DATA 0x22 (request, carries data)
# ---------------------------------------------------------------------------

def encode_write(path: str, offset: int, current_time_ns: int, total_size: int) -> bytes:
    """request {command, reserved, pathLength, offset(4), currentTime(8), totalSize(4)} + path"""
    p = _path_bytes(path)
    header = struct.pack("<BBHIQI", CMD_WRITE, 0, len(p), offset, current_time_ns, total_size)
    return header + p


@dataclass
class WritePacing:
    command: int
    status: int
    offset: int
    truncated_time_ns: int
    free_space: int


def decode_write_pacing(data: bytes) -> WritePacing:
    """response {command, status, reserved(2), offset(4), truncatedTime(8), freeSpace(4)} -- 20 bytes"""
    if len(data) < 20:
        raise ValueError("WRITE_PACING too short: %d bytes" % len(data))
    command, status, offset, truncated_time_ns, free_space = struct.unpack_from(
        "<BB2xIQI", data, 0
    )
    return WritePacing(
        command=command,
        status=status,
        offset=offset,
        truncated_time_ns=truncated_time_ns,
        free_space=free_space,
    )


def encode_write_data(offset: int, chunk: bytes) -> bytes:
    """request {command, status=0x01, reserved(2), offset(4), dataSize(4)} + data"""
    header = struct.pack("<BBHII", CMD_WRITE_DATA, STATUS_OK, 0, offset, len(chunk))
    return header + chunk


# ---------------------------------------------------------------------------
# LISTDIR 0x50 -> 0x51 (one notification per entry, then a pathLength==0 terminator)
# ---------------------------------------------------------------------------

def encode_listdir(path: str) -> bytes:
    """request {command, reserved, pathLength} + path"""
    p = _path_bytes(path)
    header = struct.pack("<BBH", CMD_LISTDIR, 0, len(p))
    return header + p


@dataclass
class ListEntry:
    command: int
    status: int
    entry_number: int
    total_entries: int
    flags: int
    modification_time_ns: int
    file_size: int
    name: str

    @property
    def is_dir(self) -> bool:
        return bool(self.flags & 0x1)

    @property
    def is_terminator(self) -> bool:
        return self.name == "" and self.entry_number == self.total_entries


def decode_listdir_entry(data: bytes) -> ListEntry:
    """response, 28-byte header + name (Docs/BLE-File-Transfer-Service.md LISTDIR 0x51)"""
    if len(data) < 28:
        raise ValueError("LISTDIR entry too short: %d bytes" % len(data))
    (
        command,
        status,
        path_length,
        entry_number,
        total_entries,
        flags,
        modification_time_ns,
        file_size,
    ) = struct.unpack_from("<BBHIIIQI", data, 0)
    name_bytes = data[28 : 28 + path_length]
    if len(name_bytes) < path_length:
        raise ValueError(
            "LISTDIR entry name truncated: wanted %d, got %d" % (path_length, len(name_bytes))
        )
    return ListEntry(
        command=command,
        status=status,
        entry_number=entry_number,
        total_entries=total_entries,
        flags=flags,
        modification_time_ns=modification_time_ns,
        file_size=file_size,
        name=name_bytes.decode("utf-8"),
    )
