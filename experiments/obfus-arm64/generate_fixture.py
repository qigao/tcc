#!/usr/bin/env python3
"""Build-time ciphertext fixtures for ARM64 obfus transport tests.

This is fixture generation, NOT an implementation of arbitrary HIDE_STRING(str).
C sources receive only encoded bytes and fixed independent length/hash receipts.
"""
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


def encode(seed: int, plain: bytes) -> bytes:
    state = seed ^ 0x41524D36
    encoded = bytearray()
    for index, byte in enumerate(plain):
        state = mix32((state + (index + 1) * 0x9E3779B9) & MASK)
        encoded.append(byte ^ (state & 255))
    return bytes(encoded)


def fixture_lines(name: str, plain: bytes, seed: int) -> str:
    assert plain.endswith(b"\0")
    cipher = encode(seed, plain)
    assert cipher != plain
    values = ", ".join(f"0x{byte:02x}" for byte in cipher)
    return (
        f"static const unsigned char obfh_arm64_fixture_{name}_bytes[] = "
        f"{{ {values} }};\n"
        f"#define OBFH_ARM64_FIXTURE_{name.upper()}_HASH 0x{fnv1a(plain):08x}u\n"
        f"#define OBFH_ARM64_FIXTURE_{name.upper()}_COUNT {len(plain)}u\n"
    )


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--seed", required=True, type=lambda x: int(x, 0))
    parser.add_argument("--text", default="arm64-hidden-poc")
    parser.add_argument("--output", required=True, type=Path)
    args = parser.parse_args()
    if not 0 <= args.seed <= MASK:
        parser.error("seed must fit unsigned 32 bits")
    plain = args.text.encode("utf-8") + b"\0"
    if not plain:
        parser.error("empty input is invalid")
    body = (
        "/* GENERATED: obfus ARM64 experimental encoded fixtures. */\n"
        "#ifndef OBFH_ARM64_FIXTURE_H\n#define OBFH_ARM64_FIXTURE_H\n"
        f"#define OBFH_ARM64_FIXTURE_SEED 0x{args.seed:08x}u\n"
        "#if OBFH_ARM64_FIXTURE_SEED != OBFH_SEED\n"
        "#error \"Encoded fixture seed differs from target compiler seed\"\n"
        "#endif\n"
        + fixture_lines("", plain, args.seed)
        + fixture_lines("utf8", "文字列-日本語-🔒".encode("utf-8") + b"\0", args.seed)
        + fixture_lines("embedded", b"first\0second\0", args.seed)
        + fixture_lines("empty", b"\0", args.seed)
        + "#endif\n"
    )
    # Preserve the existing fixture symbol/receipt names for the old smoke gate.
    body = body.replace("obfh_arm64_fixture__bytes", "obfh_arm64_fixture_bytes")
    body = body.replace("OBFH_ARM64_FIXTURE__HASH", "OBFH_ARM64_FIXTURE_HASH")
    body = body.replace("OBFH_ARM64_FIXTURE__COUNT", "OBFH_ARM64_FIXTURE_COUNT")
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(body, encoding="utf-8")


if __name__ == "__main__":
    main()
