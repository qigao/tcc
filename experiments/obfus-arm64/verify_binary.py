#!/usr/bin/env python3
"""Binary plaintext-residual gate for fixed native ARM64 fixtures.

Checks literal byte sequences only: this is NOT a proof of cryptographic
security, no-debug-copy guarantee, or resistance to disassembly.
"""
import argparse
from pathlib import Path


def main() -> None:
    p = argparse.ArgumentParser()
    p.add_argument("--binary", required=True, type=Path)
    p.add_argument("--forbid", action="append", default=[])
    p.add_argument("--forbid-hex", action="append", default=[])
    args = p.parse_args()
    sequences = [text.encode("utf-8") for text in args.forbid]
    for hex_sequence in args.forbid_hex:
        try:
            sequences.append(bytes.fromhex(hex_sequence))
        except ValueError as exc:
            p.error(f"invalid hexadecimal fixture: {exc}")
    if not sequences or any(not sequence for sequence in sequences):
        p.error("at least one nonempty forbidden byte sequence is required")
    blob = args.binary.read_bytes()
    if not blob:
        p.error("binary is empty")
    for index, sequence in enumerate(sequences, 1):
        if sequence in blob:
            p.error(
                f"plaintext pattern {index} of {len(sequence)} bytes "
                "appears in produced binary"
            )
    print(f"NO_PLAINTEXT_GATE_PASS: {args.binary} ({len(blob)} bytes, "
          f"{len(sequences)} patterns)")


if __name__ == "__main__":
    main()
