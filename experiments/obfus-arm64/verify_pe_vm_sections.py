#!/usr/bin/env python3
"""Strict PE/COFF physical-section audit of the original ARM64 VM code arrays.

No pefile, disassembler, SDK, installer or new consumer project is needed.
Compute two known source sites independently of the C macros, locate their
COMPLETE encoded arrays in the actual PE sections, and reject writable or
executable storage and relocation overlap.

This does not certify ASLR, CFG, unwind/SEH or resistance to reversing.
"""
import argparse
import json
import struct
import sys
from pathlib import Path

MASK32 = 0xffffffff
READ = 0x40000000
WRITE = 0x80000000
EXECUTE = 0x20000000


class AuditError(ValueError):
    pass


def require(ok: bool, message: str) -> None:
    if not ok:
        raise AuditError(message)


def u16(data: bytes, at: int) -> int:
    require(0 <= at <= len(data) - 2, "PE u16 out of file range")
    return struct.unpack_from("<H", data, at)[0]


def u32(data: bytes, at: int) -> int:
    require(0 <= at <= len(data) - 4, "PE u32 out of file range")
    return struct.unpack_from("<I", data, at)[0]


def parse_pe(data: bytes) -> dict:
    require(len(data) >= 512 and data[:2] == b"MZ", "not a DOS/PE image")
    pe = u32(data, 0x3c)
    require(pe >= 64 and pe + 24 < len(data), "bad e_lfanew")
    require(data[pe:pe + 4] == b"PE\0\0", "missing PE signature")
    coff = pe + 4
    require(u16(data, coff) == 0xaa64, "not a native ARM64 PE (machine != AA64)")
    number = u16(data, coff + 2)
    opt_size = u16(data, coff + 16)
    opt = coff + 20
    require(1 <= number <= 96, "invalid PE section count")
    require(opt_size >= 112 and opt + opt_size <= len(data), "invalid PE64 optional header")
    require(u16(data, opt) == 0x20b, "not PE32+")
    dll = u16(data, opt + 70)
    n_dirs = u32(data, opt + 108)
    directories = []
    for i in range(min(n_dirs, 16)):
        offset = opt + 112 + i * 8
        require(offset + 8 <= opt + opt_size, "truncated PE data directory")
        directories.append((u32(data, offset), u32(data, offset + 4)))
    sections = []
    table = opt + opt_size
    require(table + number * 40 <= len(data), "section table outside image")
    for i in range(number):
        at = table + i * 40
        name = data[at:at + 8].split(b"\0", 1)[0].decode("ascii", "replace")
        virtual_size, rva, raw_size, raw_at = struct.unpack_from("<IIII", data, at + 8)
        flags = u32(data, at + 36)
        require(not raw_size or raw_at + raw_size <= len(data),
                f"section {name} outside image")
        require(not (flags & EXECUTE and flags & WRITE),
                f"W+X section detected: {name} flags={flags:08x}")
        sections.append({"name": name, "rva": rva, "virtual_size": virtual_size,
                         "raw_size": raw_size, "raw_at": raw_at, "flags": flags})
    require(any(s["flags"] & EXECUTE for s in sections), "no executable code section")
    return {"sections": sections, "directories": directories,
            "dll_characteristics": dll}


def raw_offset_for_rva(sections: list[dict], rva: int, size: int) -> int:
    for sec in sections:
        rel = rva - sec["rva"]
        if rel >= 0 and rel + size <= sec["raw_size"]:
            return sec["raw_at"] + rel
    raise AuditError(f"unmapped RVA range {rva:08x}+{size}")


def base_relocations(data: bytes, pe: dict) -> list[tuple[int, int]]:
    directories = pe["directories"]
    if len(directories) <= 5:
        return []
    rva, size = directories[5]
    if rva == 0 or size == 0:
        require(rva == 0 and size == 0, "incomplete base relocation data directory")
        return []
    start = raw_offset_for_rva(pe["sections"], rva, size)
    end = start + size
    entries = []
    pos = start
    while pos < end:
        require(pos + 8 <= end, "truncated base relocation block")
        page, block_size = struct.unpack_from("<II", data, pos)
        require(block_size >= 8 and block_size % 2 == 0 and pos + block_size <= end,
                "malformed base relocation block")
        for entry in range(pos + 8, pos + block_size, 2):
            raw = u16(data, entry)
            reloc_type = raw >> 12
            if reloc_type:
                reloc_rva = page + (raw & 0xfff)
                raw_offset_for_rva(pe["sections"], reloc_rva, 1)
                entries.append((reloc_rva, reloc_type))
        pos += block_size
    return entries


def u32s(*items: int) -> bytes:
    return b"".join(struct.pack("<I", word & MASK32) for word in items)


def original_reference_word(seed: int, key: int, pc: int, op: int,
                            d: int, a: int, b: int, immediate: int) -> int:
    """Independent transcription of upstream OBFH_V_PACK/ENCODE_RAW."""
    field_shifts = ((8, 10, 12), (8, 12, 10), (10, 12, 8),
                    (10, 8, 12), (12, 8, 10), (12, 10, 8))
    mul = ((key >> 8) & 255) | 1
    salt = key & 255
    mode = (seed >> 24) % 6
    encoded_opcode = (
        op * mul + salt if mode == 0 else
        (op * mul) ^ salt if mode == 1 else
        (op ^ salt) * mul if mode == 2 else
        (op + salt) * mul if mode == 3 else
        op * mul - salt if mode == 4 else
        (op - salt) * mul
    ) & 255
    dshift, ashift, bshift = field_shifts[(seed >> 14) % 6]
    packed = (encoded_opcode | (d << dshift) | (a << ashift) |
              (b << bshift) | (immediate << 14)) & MASK32
    layout = (key >> 29) & 3
    if layout == 1:
        packed = ((packed & 255) << 24) | (((packed >> 8) & 3) << 18) | (
            ((packed >> 10) & 3) << 20) | (((packed >> 12) & 3) << 22) | (packed >> 14)
    elif layout == 2:
        packed = ((packed & 255) << 6) | ((packed >> 8) & 3) | (
            ((packed >> 10) & 3) << 2) | (((packed >> 12) & 3) << 4) | (packed & 0xffffc000)
    elif layout == 3:
        packed = (packed & 255) | (((packed >> 8) & 3) << 12) | (
            ((packed >> 10) & 3) << 8) | (((packed >> 12) & 3) << 10) | (packed & 0xffffc000)
    packed &= MASK32
    rotate = key % 31 + 1
    word = ((packed >> rotate) | (packed << (32 - rotate))) & MASK32
    wm = ((seed >> 6) & 15) % 9
    delta = (pc * 0x9e3779b9) & MASK32
    mask = (key + delta if wm % 3 == 0 else
            key ^ delta if wm % 3 == 1 else key - delta) & MASK32
    return (word ^ mask if wm // 3 == 0 else
            word + mask if wm // 3 == 1 else word - mask) & MASK32


def reference_site(seed: int, variant: int, layout: int, salt: int) -> bytes:
    """Two actual site declarations in vm_upstream_static.c, no new fixtures."""
    key = ((layout << 29) | (variant << 24) |
           ((seed ^ salt) & 0x00ffffff)) & MASK32
    va = key & 3
    vb_offset = 1 + ((key >> 3) % 3)
    vc_pick = 1 + ((key >> 5) & 1)
    vc_offset = vc_pick + (vc_pick >= vb_offset)
    vb = (va + vb_offset) & 3
    vc = (va + vc_offset) & 3
    vd = (va + 6 - vb_offset - vc_offset) & 3
    op0 = 5 if variant == 0 else 22  # UOP_ADD or UOP_BSHL
    op1 = 14  # upstream CXOR (used as a no-op on result)
    imm = 0 if variant == 0 else 12
    if variant == 0:
        instruction = [(24, va, vb, 0, 0), (op0, vc, va, vb, 0),
                       (op1, vc, va, 0, imm), (26, 0, vc, 0, 0)]
    elif variant == 7:
        bit64 = bool(key & 64)
        instruction = [
            (24, va, vb, 0, 0), (12, 0, va, 0, 0), (17, 0, 1, 0, 6),
            (15 if bit64 else 3, 1 if bit64 else vd, 0 if bit64 else va, 0, 7 if bit64 else 0),
            (3 if bit64 else 15, vd if bit64 else 1, va if bit64 else 0, 0, 0 if bit64 else 7),
            (16, 0, 0, 0, 8), (3, vd, va, 0, 0), (14, 1, 0, 0, 57),
            (op0, vc, vd, vb, 0), (op1, vc, vd, 0, imm), (12, 0, vc, 0, 0),
            (17, 0, 1, 0, 14), (3, va, vc, 0, 0), (26, 0, va, 0, 0),
            (26, 0, vc, 0, 0)
        ]
    else:
        raise AuditError("site profile is not an original qualified declaration")
    return u32s(*(original_reference_word(seed, key, i, *word)
                  for i, word in enumerate(instruction)))


def locate_code(data: bytes, sections: list[dict], label: str,
                expected: bytes) -> tuple[dict, int]:
    matches = []
    for sec in sections:
        if sec["raw_size"] == 0:
            continue
        raw = data[sec["raw_at"]:sec["raw_at"] + sec["raw_size"]]
        at = raw.find(expected)
        while at >= 0:
            matches.append((sec, at))
            at = raw.find(expected, at + 1)
    require(matches, f"{label}: original {len(expected)}-byte program NOT FOUND in any PE section")
    legal = [(sec, at) for sec, at in matches
             if (sec["flags"] & READ and not sec["flags"] & (WRITE | EXECUTE))]
    require(legal, f"{label}: source program found only in W/X sections: " +
            ", ".join(sec["name"] + f":{sec['flags']:08x}" for sec, _ in matches))
    require(len(legal) == 1, f"{label}: expected exactly 1 protected byte stream, got {len(legal)}")
    sec, offset = legal[0]
    return sec, sec["rva"] + offset


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--binary", required=True, type=Path)
    parser.add_argument("--seed", required=True, type=lambda x: int(x, 0))
    args = parser.parse_args()
    require(0 <= args.seed <= MASK32, "seed outside uint32")
    data = args.binary.read_bytes()
    pe = parse_pe(data)
    print("ARM64_PE_SECTION_HEADER: " + json.dumps({
        "seed": f"0x{args.seed:08x}",
        "sections": [{"name": item["name"],
                      "flags": f"{item['flags']:08x}",
                      "raw": item["raw_size"]} for item in pe["sections"]],
        "dll_characteristics": f"{pe['dll_characteristics']:04x}"
    }, sort_keys=True), flush=True)
    relocations = base_relocations(data, pe)
    sites = {
        "site0": reference_site(args.seed, 0, 0, 0x10100),
        "site7": reference_site(args.seed, 7, 3, 0x808e0),
        "site0_second": reference_site(args.seed, 0, 1, 0x91911)
    }
    protected = []
    for name, expected in sites.items():
        sec, rva = locate_code(data, pe["sections"], name, expected)
        require(rva % 4 == 0, f"{name}: encoded word program not four-byte aligned")
        for reloc_rva, relocation_type in relocations:
            # Do not allow relocation to overwrite ANY encoded instruction
            # data, including an architecture-dependent multi-byte relocation.
            if rva - 8 <= reloc_rva < rva + len(expected):
                raise AuditError(f"{name}: PE base relocation type {relocation_type} "
                                 f"overlaps protected VM bytecode near RVA {reloc_rva:x}")
        protected.append({"site": name, "section": sec["name"], "rva": f"{rva:08x}",
                          "bytes": len(expected), "flags": f"{sec['flags']:08x}"})
    report = {
        "machine": "AA64", "seed": f"0x{args.seed:08x}",
        "sections": [{"name": s["name"], "flags": f"{s['flags']:08x}"}
                     for s in pe["sections"]],
        "protected_programs": protected,
        "base_relocation_count": len(relocations),
        "dynamic_base": bool(pe["dll_characteristics"] & 0x0040),
        "nx_compatible": bool(pe["dll_characteristics"] & 0x0100),
        "guard_cf_advertised": bool(pe["dll_characteristics"] & 0x4000)
    }
    print("ARM64_PE_VM_SECTIONS_PASS: " + json.dumps(report, sort_keys=True))


if __name__ == "__main__":
    try:
        main()
    except (AuditError, OSError, struct.error) as exc:
        print(f"ARM64_PE_VM_SECTIONS_RED: {exc}", file=sys.stderr)
        sys.exit(1)
