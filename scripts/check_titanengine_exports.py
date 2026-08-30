#!/usr/bin/env python3
"""Verify that TitanEngine-compatible DLLs export the canonical x64dbg ABI."""

from __future__ import annotations

import argparse
import struct
import sys
from pathlib import Path


def canonical_exports(definition: Path) -> set[str]:
    exports: set[str] = set()
    in_exports = False
    for raw_line in definition.read_text(encoding="utf-8-sig").splitlines():
        line = raw_line.split(";", 1)[0].strip()
        if not line:
            continue
        if line.upper() == "EXPORTS":
            in_exports = True
            continue
        if in_exports:
            exports.add(line.split()[0].split("=", 1)[0])
    if not exports:
        raise ValueError(f"No exports found in {definition}")
    return exports


def pe_exports(path: Path) -> set[str]:
    data = path.read_bytes()

    def unpack(fmt: str, offset: int):
        return struct.unpack_from(fmt, data, offset)

    if data[:2] != b"MZ":
        raise ValueError("missing DOS signature")
    pe_offset = unpack("<I", 0x3C)[0]
    if data[pe_offset : pe_offset + 4] != b"PE\0\0":
        raise ValueError("missing PE signature")

    file_header = pe_offset + 4
    _, section_count, _, _, _, optional_size, _ = unpack("<HHIIIHH", file_header)
    optional = file_header + 20
    magic = unpack("<H", optional)[0]
    if magic == 0x10B:
        directories = optional + 96
    elif magic == 0x20B:
        directories = optional + 112
    else:
        raise ValueError(f"unknown optional-header magic 0x{magic:x}")

    export_rva, export_size = unpack("<II", directories)
    if not export_rva or not export_size:
        return set()

    sections: list[tuple[int, int, int, int]] = []
    section_table = optional + optional_size
    for index in range(section_count):
        offset = section_table + index * 40
        virtual_size, virtual_address, raw_size, raw_offset = unpack("<IIII", offset + 8)
        sections.append((virtual_address, max(virtual_size, raw_size), raw_offset, raw_size))

    def rva_offset(rva: int) -> int:
        for virtual_address, mapped_size, raw_offset, raw_size in sections:
            if virtual_address <= rva < virtual_address + mapped_size:
                relative = rva - virtual_address
                if relative >= raw_size:
                    raise ValueError(f"RVA 0x{rva:x} has no raw-file data")
                return raw_offset + relative
        if rva < optional + optional_size:
            return rva
        raise ValueError(f"RVA 0x{rva:x} is outside all sections")

    export_offset = rva_offset(export_rva)
    number_of_names = unpack("<I", export_offset + 24)[0]
    address_of_names = unpack("<I", export_offset + 32)[0]
    names_offset = rva_offset(address_of_names)

    result: set[str] = set()
    for index in range(number_of_names):
        name_rva = unpack("<I", names_offset + index * 4)[0]
        name_offset = rva_offset(name_rva)
        end = data.find(b"\0", name_offset)
        if end < 0:
            raise ValueError("unterminated export name")
        result.add(data[name_offset:end].decode("ascii"))
    return result


def default_dlls(root: Path) -> list[Path]:
    relative = [
        "bin/x64/TitanEngine.dll",
        "bin/x64/GleeBug/TitanEngine.dll",
        "bin/x64/StaticEngine/TitanEngine.dll",
        "bin/x64/DbgEng/TitanEngine.dll",
        "bin/x32/TitanEngine.dll",
        "bin/x32/GleeBug/TitanEngine.dll",
        "bin/x32/StaticEngine/TitanEngine.dll",
    ]
    return [root / item for item in relative if (root / item).is_file()]


def main() -> int:
    script_dir = Path(__file__).resolve().parent
    root = script_dir.parent

    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "dll",
        nargs="*",
        type=Path,
        help="DLLs to check (defaults to built in-tree engine DLLs)",
    )
    parser.add_argument(
        "--definition",
        type=Path,
        default=root / "src/dbg/TitanEngine/TitanEngine.def",
        help="Canonical module definition file",
    )
    args = parser.parse_args()

    required = canonical_exports(args.definition)
    dlls = args.dll or default_dlls(root)
    if not dlls:
        parser.error("no DLLs supplied and no built engine DLLs were found")

    failed = False
    for dll in dlls:
        try:
            actual = pe_exports(dll)
        except (OSError, ValueError, struct.error) as exc:
            print(f"FAIL {dll}: {exc}")
            failed = True
            continue
        missing = sorted(required - actual)
        if missing:
            print(f"FAIL {dll}: missing {', '.join(missing)}")
            failed = True
        else:
            print(f"PASS {dll}: {len(required)} required exports present")
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
