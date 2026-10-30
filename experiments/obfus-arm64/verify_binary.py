#!/usr/bin/env python3
"""Minimal binary plaintext leakage gate; not an obfuscation-strength metric."""
import argparse
from pathlib import Path


def main() -> None:
    p = argparse.ArgumentParser()
    p.add_argument("--binary", required=True, type=Path)
    p.add_argument("--forbid", required=True)
    args = p.parse_args()
    blob = args.binary.read_bytes()
    if args.forbid.encode("utf-8") in blob:
        p.error("unprotected plaintext sequence appears in produced binary")
    if not blob:
        p.error("binary is empty")
    print(f"NO_PLAINTEXT_GATE_PASS: {args.binary} ({len(blob)} bytes)")


if __name__ == "__main__":
    main()
