"""Round-trip / offset checks for protocol.py against the documented byte
layout (una-sdk/Docs/BLE-File-Transfer-Service.md). No BLE hardware needed --
this is the part of P0 verifiable in this container.

    python3 -m unittest test_protocol.py -v
"""

import struct
import unittest

import protocol as p


class MkdirTest(unittest.TestCase):
    def test_encode_header_and_path(self):
        packet = p.encode_mkdir("/Apps/HXIntervalsProbe/Plans", current_time_ns=1234567890123456789)
        path = b"/Apps/HXIntervalsProbe/Plans"

        self.assertEqual(packet[0], p.CMD_MKDIR)
        self.assertEqual(packet[1], 0)  # reserved
        (path_length,) = struct.unpack_from("<H", packet, 2)
        self.assertEqual(path_length, len(path))
        (reserved4,) = struct.unpack_from("<I", packet, 4)
        self.assertEqual(reserved4, 0)
        (current_time,) = struct.unpack_from("<Q", packet, 8)
        self.assertEqual(current_time, 1234567890123456789)
        self.assertEqual(packet[16:], path)
        self.assertEqual(len(packet), 16 + len(path))

    def test_decode_status(self):
        # command, status, 6 reserved bytes, truncatedTime
        raw = struct.pack("<BB6xQ", p.CMD_MKDIR_STATUS, p.STATUS_OK, 999)
        result = p.decode_mkdir_status(raw)
        self.assertEqual(result.command, p.CMD_MKDIR_STATUS)
        self.assertEqual(result.status, p.STATUS_OK)
        self.assertEqual(result.truncated_time_ns, 999)

    def test_decode_status_too_short_raises(self):
        with self.assertRaises(ValueError):
            p.decode_mkdir_status(b"\x41\x01\x00")


class WriteTest(unittest.TestCase):
    def test_encode_write_header_offsets(self):
        path = "/Apps/HXIntervalsProbe/Plans/probe_plan.json"
        packet = p.encode_write(path, offset=0, current_time_ns=42, total_size=49)

        self.assertEqual(packet[0], p.CMD_WRITE)
        (path_length,) = struct.unpack_from("<H", packet, 2)
        self.assertEqual(path_length, len(path.encode("utf-8")))
        (offset,) = struct.unpack_from("<I", packet, 4)
        self.assertEqual(offset, 0)
        (current_time,) = struct.unpack_from("<Q", packet, 8)
        self.assertEqual(current_time, 42)
        (total_size,) = struct.unpack_from("<I", packet, 16)
        self.assertEqual(total_size, 49)
        self.assertEqual(packet[20:], path.encode("utf-8"))

    def test_decode_write_pacing(self):
        # command, status, reserved(2), offset, truncatedTime, freeSpace
        raw = struct.pack("<BBHIQI", p.CMD_WRITE_PACING, p.STATUS_OK, 0, 0, 42, 49)
        ack = p.decode_write_pacing(raw)
        self.assertEqual(ack.status, p.STATUS_OK)
        self.assertEqual(ack.offset, 0)
        self.assertEqual(ack.truncated_time_ns, 42)
        self.assertEqual(ack.free_space, 49)

    def test_terminal_ack_has_zero_free_space(self):
        raw = struct.pack("<BBHIQI", p.CMD_WRITE_PACING, p.STATUS_OK, 0, 49, 42, 0)
        ack = p.decode_write_pacing(raw)
        self.assertEqual(ack.free_space, 0)

    def test_encode_write_data(self):
        chunk = b'{"probe": true}'
        packet = p.encode_write_data(offset=0, chunk=chunk)

        self.assertEqual(packet[0], p.CMD_WRITE_DATA)
        self.assertEqual(packet[1], p.STATUS_OK)
        (offset,) = struct.unpack_from("<I", packet, 4)
        self.assertEqual(offset, 0)
        (data_size,) = struct.unpack_from("<I", packet, 8)
        self.assertEqual(data_size, len(chunk))
        self.assertEqual(packet[12:], chunk)
        self.assertEqual(len(packet), 12 + len(chunk))

    def test_bytes_acked_is_total_minus_free_space(self):
        # Docs: bytesAcked = totalSize - freeSpace.
        total_size = 100
        raw = struct.pack("<BBHIQI", p.CMD_WRITE_PACING, p.STATUS_OK, 0, 60, 0, 40)
        ack = p.decode_write_pacing(raw)
        bytes_acked = total_size - ack.free_space
        self.assertEqual(bytes_acked, 60)


class ListDirTest(unittest.TestCase):
    def test_encode_request(self):
        packet = p.encode_listdir("/Apps/HXIntervalsProbe/Plans")
        path = b"/Apps/HXIntervalsProbe/Plans"

        self.assertEqual(packet[0], p.CMD_LISTDIR)
        (path_length,) = struct.unpack_from("<H", packet, 2)
        self.assertEqual(path_length, len(path))
        self.assertEqual(packet[4:], path)

    def test_decode_entry(self):
        name = b"probe_plan.json"
        header = struct.pack(
            "<BBHIIIQI",
            p.CMD_LISTDIR_ENTRY,
            p.STATUS_OK,
            len(name),
            0,  # entryNumber
            2,  # totalEntries
            0,  # flags (not a directory)
            1234567890123456789,
            49,  # fileSize
        )
        entry = p.decode_listdir_entry(header + name)

        self.assertEqual(entry.name, "probe_plan.json")
        self.assertEqual(entry.entry_number, 0)
        self.assertEqual(entry.total_entries, 2)
        self.assertFalse(entry.is_dir)
        self.assertEqual(entry.file_size, 49)
        self.assertFalse(entry.is_terminator)

    def test_decode_directory_flag(self):
        name = b"nested"
        header = struct.pack("<BBHIIIQI", p.CMD_LISTDIR_ENTRY, p.STATUS_OK, len(name), 1, 2, 0x1, 0, 0)
        entry = p.decode_listdir_entry(header + name)
        self.assertTrue(entry.is_dir)

    def test_decode_terminator(self):
        header = struct.pack("<BBHIIIQI", p.CMD_LISTDIR_ENTRY, p.STATUS_OK, 0, 2, 2, 0, 0, 0)
        entry = p.decode_listdir_entry(header)
        self.assertTrue(entry.is_terminator)
        self.assertEqual(entry.name, "")

    def test_decode_entry_truncated_name_raises(self):
        header = struct.pack("<BBHIIIQI", p.CMD_LISTDIR_ENTRY, p.STATUS_OK, 10, 0, 1, 0, 0, 0)
        with self.assertRaises(ValueError):
            p.decode_listdir_entry(header + b"short")


class StatusNameTest(unittest.TestCase):
    def test_known_status(self):
        self.assertEqual(p.status_name(p.STATUS_OK), "OK")
        self.assertEqual(p.status_name(p.STATUS_ERROR_NO_FILE), "ERROR_NO_FILE")

    def test_unknown_status(self):
        self.assertIn("UNKNOWN", p.status_name(0xFE))


if __name__ == "__main__":
    unittest.main()
