#!/usr/bin/env python3
"""Build-time fixture encoder for the standalone ARM64 feasibility test."""
import argparse
from pathlib import Path

MASK = 0xFFFFFFFF


def mix32(value: int) -> int:
    value = (value ^ (value >> 16)) * 0x7FEB352D & MASK
    value = (value ^ (value >> 15)) * 0x846CA68B & MASK
    return value ^ (value >> 16)


def fnv1a(data: bytes) -> int:
    value = 0x811C9DC5
    for byte in data:
        value = ((value ^ byte) * 0x01000193) & MASK
    return value


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--seed", required=True, type=lambda x: int(x, 0))
    parser.add_argument("--text", default="arm64-hidden-poc")
    parser.add_argument("--output", required=True, type=Path)
    args = parser.parse_args()
    if not 0 <= args.seed <= MASK:
        parser.error("seed must fit unsigned 32 bits")
    plain = args.text.encode("utf-8") + b"\0"
    state = args.seed ^ 0x41524D36
    encoded = bytearray()
    for index, byte in enumerate(plain):
        state = mix32((state + (index + 1) * 0x9E3779B9) & MASK)
        encoded.append(byte ^ (state & 255))
    values = ", ".join(f"0x{byte:02x}" for byte in encoded)
    body = (
        "/* GENERATED: obfus ARM64 experimental encoded fixture. */\n"
        "#ifndef OBFH_ARM64_FIXTURE_H\n#define OBFH_ARM64_FIXTURE_H\n"
        f"#define OBFH_ARM64_FIXTURE_SEED 0x{args.seed:08x}u\n"
        "#if OBFH_ARM64_FIXTURE_SEED != OBFH_SEED\n"
        "#error \"Encoded fixture seed differs from target compiler seed\"\n"
        "#endif\n"
        f"static const unsigned char obfh_arm64_fixture_bytes[] = {{ {values} }};\n"
        f"#define OBFH_ARM64_FIXTURE_HASH 0x{fnv1a(plain):08x}u\n"
        "#endif\n"
    )
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(body, encoding="utf-8")


if __name__ == "__main__":
    main()
