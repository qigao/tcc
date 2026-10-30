#!/usr/bin/env python3
"""Strict PE/COFF physical-section audit of the original ARM64 VM code arrays.

No pefile, disassembler, SDK, installer or new consumer project is needed.
Compute two known source sites independently of the C macros, locate their
COMPLETE encoded arrays in the actual PE sections, and reject writable or
executable storage and relocation overlap.

This certifies neither OS CFG enforcement nor SEH; structural CFG metadata
checks remain separate from native indirect-call target admission.
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
GUARD_CF = 0x4000
CF_INSTRUMENTED = 0x100
CF_FUNCTION_TABLE_PRESENT = 0x400
LOAD_CONFIG_DIRECTORY = 10
EXCEPTION_DIRECTORY = 3
ARM64_XDATA_HAS_HANDLER = 1 << 20
ARM64_XDATA_SINGLE_EPILOG = 1 << 21
LOAD_CONFIG_GUARD_OFFSET = 0x90



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


def u64(data: bytes, at: int) -> int:
    require(0 <= at <= len(data) - 8, "PE u64 out of file range")
    return struct.unpack_from("<Q", data, at)[0]


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
            "dll_characteristics": dll, "image_base": u64(data, opt + 24),
            "size_of_image": u32(data, opt + 56)}


def raw_offset_for_rva(sections: list[dict], rva: int, size: int) -> int:
    for sec in sections:
        rel = rva - sec["rva"]
        if rel >= 0 and rel + size <= sec["raw_size"]:
            return sec["raw_at"] + rel
    raise AuditError(f"unmapped RVA range {rva:08x}+{size}")


def audit_cfg_metadata(data: bytes, pe: dict, require_cfg: bool = False) -> dict:
    """GuardCF *structure* only; never call this runtime CFG admission.

    The linker must populate the PE64 load-config table and a sorted GFIDS
    array of address-taken ARM64 functions. The bit in DllCharacteristics
    alone is not sufficient. Even complete GFIDS do not prove the compiler
    inserts checks before the indirect BLR or that the OS enforces them.
    """
    declared = bool(pe["dll_characteristics"] & GUARD_CF)
    if not declared:
        require(not require_cfg,
                "CFG admission RED: GUARD_CF absent from actual TinyCC ARM64 PE")
        return {"state": "not_advertised", "guard_cf": False,
                "gfids_count": 0, "runtime_enforced": False}
    require(len(pe["directories"]) > LOAD_CONFIG_DIRECTORY,
            "CFG admission RED: GUARD_CF without Load Config directory")
    rva, directory_size = pe["directories"][LOAD_CONFIG_DIRECTORY]
    require(rva and directory_size >= LOAD_CONFIG_GUARD_OFFSET + 4,
            "CFG admission RED: missing/truncated Load Config")
    off = raw_offset_for_rva(pe["sections"], rva, LOAD_CONFIG_GUARD_OFFSET + 4)
    require(u32(data, off) >= LOAD_CONFIG_GUARD_OFFSET + 4,
            "CFG admission RED: Load Config declared size too small")
    flags = u32(data, off + LOAD_CONFIG_GUARD_OFFSET)
    require(flags & CF_INSTRUMENTED and flags & CF_FUNCTION_TABLE_PRESENT,
            "CFG admission RED: missing instrumentation/GFIDS GuardFlags")
    check_va = u64(data, off + 0x70)
    table_va = u64(data, off + 0x80)
    count = u64(data, off + 0x88)
    image_base = pe["image_base"]
    image_size = pe["size_of_image"]
    stride_extra = (flags >> 28) & 0x0f
    require(stride_extra <= 1,
            "CFG admission RED: unsupported GFIDS extended entry stride")
    require(0 < count <= 1000000 and image_size > 0,
            "CFG admission RED: invalid/beyond-budget GFIDS count")
    require(image_base <= check_va and image_base <= table_va,
            "CFG admission RED: pointer/table below preferred image base")
    check_rva = check_va - image_base
    table_rva = table_va - image_base
    entry_size = 4 + stride_extra
    require(check_rva <= image_size - 8 and
            count * entry_size <= image_size and
            table_rva <= image_size - count * entry_size,
            "CFG admission RED: Load Config address/count outside image")
    raw_offset_for_rva(pe["sections"], check_rva, 8)
    table_off = raw_offset_for_rva(pe["sections"], table_rva,
                                   count * entry_size)
    check_sections = [sec for sec in pe["sections"]
                      if sec["rva"] <= check_rva and
                      check_rva + 8 <= sec["rva"] + sec["raw_size"]]
    require(len(check_sections) == 1 and
            check_sections[0]["flags"] & READ and
            not check_sections[0]["flags"] & (WRITE | EXECUTE),
            "CFG admission RED: loader check pointer is not read-only data")
    previous = -1
    for i in range(count):
        at = table_off + i * entry_size
        target = u32(data, at)
        require(target > previous and target % 4 == 0 and target < image_size,
                "CFG admission RED: GFIDS unsorted/duplicate/misaligned RVA")
        require(any(sec["flags"] & EXECUTE and
                    not sec["flags"] & WRITE and
                    sec["rva"] <= target < sec["rva"] + sec["virtual_size"]
                    for sec in pe["sections"]),
                "CFG admission RED: GFIDS target outside executable section")
        if stride_extra:
            require(data[at + 4] & ~3 == 0,
                    "CFG admission RED: invalid GFIDS extra flags")
        previous = target
    return {"state": "structural_only", "guard_cf": True,
            "guard_flags": f"{flags:08x}",
            "gfids_count": count, "check_pointer_rva": f"{check_rva:08x}",
            "runtime_enforced": False}


def audit_arm64_exception_metadata(data: bytes, pe: dict,
                                   require_handler: bool = False) -> dict:
    """Inspect actual ARM64 .pdata/.xdata, not source macros or exceptions.

    The pinned TinyCC tccpe.c emits X=0 for its ARM64 unwind records.
    This audit proves the *presence or absence* of handler-bearing xdata,
    not Windows phase-one dispatch or __try/__finally cleanup. In particular,
    X=1 + an RVA does not establish an authentic language scope table.
    """
    dirs = pe["directories"]
    require(len(dirs) > EXCEPTION_DIRECTORY,
            "ARM64 SEH metadata RED: missing exception directory entry")
    rva, size = dirs[EXCEPTION_DIRECTORY]
    require(rva and size and size % 8 == 0 and size <= 8 * 100000,
            "ARM64 SEH metadata RED: empty/misaligned/oversized .pdata directory")
    section = next((s for s in pe["sections"]
                    if s["rva"] <= rva and
                    rva + size <= s["rva"] + s["raw_size"]), None)
    require(section is not None and
            (section["flags"] & READ) and
            not (section["flags"] & (WRITE | EXECUTE)),
            "ARM64 SEH metadata RED: .pdata is not immutable mapped data")
    at = raw_offset_for_rva(pe["sections"], rva, size)
    count = size // 8
    prev_begin = -1
    xdata_records = 0
    handler_records = 0
    packed_records = 0
    link_records = 0
    header_versions = set()
    handler_targets = []
    for n in range(count):
        begin_rva = u32(data, at + n * 8)
        unwind_word = u32(data, at + n * 8 + 4)
        flavor = unwind_word & 3
        require(begin_rva > prev_begin and begin_rva % 4 == 0 and
                begin_rva < pe["size_of_image"] and
                any((sec["flags"] & EXECUTE) and
                    not (sec["flags"] & WRITE) and
                    sec["rva"] <= begin_rva <
                    sec["rva"] + sec["virtual_size"]
                    for sec in pe["sections"]),
                "ARM64 SEH metadata RED: unsorted/non-executable .pdata start")
        prev_begin = begin_rva
        if flavor in (1, 2):
            packed_records += 1
            continue
        if flavor == 3:
            target = unwind_word & ~3
            require(rva <= target and target < rva + size and
                    (target - rva) % 8 == 0,
                    "ARM64 SEH metadata RED: invalid .pdata link")
            link_records += 1
            continue
        # flavor 0: the second DWORD is the 4-byte-aligned xdata RVA.
        xdata_rva = unwind_word
        xsec = next((sec for sec in pe["sections"]
                     if sec["rva"] <= xdata_rva and
                     xdata_rva + 4 <= sec["rva"] + sec["raw_size"]), None)
        require(xdata_rva and xdata_rva % 4 == 0 and
                xsec is not None and (xsec["flags"] & READ) and
                not (xsec["flags"] & (WRITE | EXECUTE)),
                "ARM64 SEH metadata RED: xdata outside immutable section")
        xoff = raw_offset_for_rva(pe["sections"], xdata_rva, 4)
        hdr = u32(data, xoff)
        function_words = hdr & 0x3ffff
        version = (hdr >> 18) & 3
        has_handler = bool(hdr & ARM64_XDATA_HAS_HANDLER)
        single_epilog = bool(hdr & ARM64_XDATA_SINGLE_EPILOG)
        epilog_field = (hdr >> 22) & 31
        code_words = (hdr >> 27) & 31
        require(version == 0 and function_words > 0 and
                begin_rva + 4 * function_words <= pe["size_of_image"],
                "ARM64 SEH metadata RED: unsupported xdata version/length")
        header_versions.add(version)
        xdata_records += 1
        # An extension word is present ONLY when both compact fields are 0.
        ext = (epilog_field == 0 and code_words == 0)
        header_words = 2 if ext else 1
        if ext:
            extension = u32(data, raw_offset_for_rva(
                pe["sections"], xdata_rva + 4, 4))
            epilog_field = extension & 0xffff
            code_words = (extension >> 16) & 0xff
            require((extension >> 24) == 0,
                    "ARM64 SEH metadata RED: nonzero extended reserved bits")
        require(code_words > 0 and code_words <= 255 and
                (not single_epilog or epilog_field < 4 * code_words),
                "ARM64 SEH metadata RED: bad unwind words/epilog index")
        scope_words = 0 if single_epilog else epilog_field
        record_bytes = (header_words + scope_words + code_words +
                        int(has_handler)) * 4
        require(record_bytes <= 1 << 20,
                "ARM64 SEH metadata RED: excessive unwind record")
        record_off = raw_offset_for_rva(pe["sections"], xdata_rva,
                                        record_bytes)
        # The unwind codes and the optional handler RVA must occupy actual
        # file bytes; do not accept a flag set on a truncated section.
        if has_handler:
            handler_rva = u32(data, record_off + record_bytes - 4)
            require(handler_rva % 4 == 0 and
                    handler_rva < pe["size_of_image"] and
                    any((sec["flags"] & EXECUTE) and
                        not (sec["flags"] & WRITE) and
                        sec["rva"] <= handler_rva <
                        sec["rva"] + sec["virtual_size"]
                        for sec in pe["sections"]),
                    "ARM64 SEH metadata RED: invalid handler RVA/section")
            handler_records += 1
            handler_targets.append(f"0x{handler_rva:08x}")
    require(not require_handler or handler_records > 0,
            "ARM64 SEH metadata RED: no handler-bearing .xdata (X=1) records")
    return {"pdata_entries": count, "xdata_records": xdata_records,
            "packed_records": packed_records, "linked_records": link_records,
            "xdata_versions": sorted(header_versions),
            "handler_records": handler_records,
            "handler_target_rvas": handler_targets[:16],
            "scope_cleanup_verified": False,
            "automatic_handler_dispatch_verified": False}


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
    parser.add_argument("--require-cfg", action="store_true",
                        help="fail closed unless actual PE CFG metadata is structurally valid")
    parser.add_argument("--require-seh-metadata", action="store_true",
                        help="require actual handler-bearing ARM64 .xdata (not a cleanup/dispatch proof)")
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
    cfg = audit_cfg_metadata(data, pe, args.require_cfg)
    seh = audit_arm64_exception_metadata(data, pe, args.require_seh_metadata)
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
        "guard_cf_advertised": bool(pe["dll_characteristics"] & GUARD_CF),
        "cfg_metadata": cfg,
        "seh_metadata": seh
    }
    print("ARM64_PE_VM_SECTIONS_PASS: " + json.dumps(report, sort_keys=True))


if __name__ == "__main__":
    try:
        main()
    except (AuditError, OSError, struct.error) as exc:
        print(f"ARM64_PE_VM_SECTIONS_RED: {exc}", file=sys.stderr)
        sys.exit(1)
