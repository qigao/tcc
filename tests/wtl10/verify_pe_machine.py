#!/usr/bin/env python3
"""Fail if a cross-compiler produced an ELF, Mach-O or wrong PE machine."""
import pathlib
import struct
import sys

if len(sys.argv) != 3 or sys.argv[2] not in ("x86_64", "aarch64"):
    raise SystemExit("verify_pe_machine.py FILE x86_64|aarch64")
data = pathlib.Path(sys.argv[1]).read_bytes()
if len(data) < 128 or data[:2] != b"MZ":
    raise SystemExit("Expected a Windows MZ executable")
offset = struct.unpack_from("<I", data, 0x3C)[0]
if offset < 64 or offset + 6 >= len(data) or data[offset:offset+4] != b"PE\0\0":
    raise SystemExit("Bad PE/COFF header")
machine = struct.unpack_from("<H", data, offset + 4)[0]
expected = {"x86_64": 0x8664, "aarch64": 0xAA64}[sys.argv[2]]
if machine != expected:
    raise SystemExit(f"Incorrect PE machine {machine:#x} expected {expected:#x}")
print(f"Verified Windows PE {sys.argv[2]} at {sys.argv[1]}")
