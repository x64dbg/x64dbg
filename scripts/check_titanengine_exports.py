#!/usr/bin/env python3
"""Verify that TitanEngine-compatible DLLs export the canonical x64dbg ABI."""

from __future__ import annotations

import argparse
import re
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


DECLARATION_RE = re.compile(
    r"__declspec\(dllexport\)\s+(.+?)\s+(?:TITCALL\s+)?(\w+)\((.*?)\);"
)

ENUM_TYPES = {
    "TitanStructureType",
    "TitanAccessType",
    "TitanEngineVariable",
    "TitanBreakpointRemoveOption",
    "TitanCustomHandler",
    "TitanBreakpointType",
    "TitanSoftwareBreakpointType",
    "TitanMemoryBreakpointType",
    "TitanHardwareBreakpointType",
    "TitanHardwareBreakpointSize",
    "TitanRegister",
}
CALLBACK_TYPES = {
    "TITANCALLBACKARG",
    "TITANCALLBACK",
    "TITANCBCH",
    "TITANCBSTEP",
    "TITANCBSOFTBP",
    "TITANCBHWBP",
    "TITANCBMEMBP",
}


def exported_declarations(path: Path) -> dict[str, tuple[str, tuple[str, ...]]]:
    text = path.read_text(encoding="utf-8-sig")
    result: dict[str, tuple[str, tuple[str, ...]]] = {}
    for return_type, name, raw_arguments in DECLARATION_RE.findall(text):
        arguments = tuple(arg.strip() for arg in raw_arguments.split(",") if arg.strip())
        result[name] = (return_type.strip(), arguments)
    return result


def abi_type(declaration: str, *, return_type: bool = False) -> str:
    """Normalize legacy spelling while preserving the binary call contract.

    The GleeBug and native TitanEngine headers predate the canonical strongly
    typed enum/callback declarations. Their DWORD/LPVOID spellings are ABI
    equivalent, so the conformance check accepts those spellings while still
    checking return class, argument count, pointer/value class and width.
    """
    value = re.sub(r"\b(const|volatile)\b", "", declaration)
    value = re.sub(r"\s+", " ", value).strip()
    # Drop a normal C parameter name. Pointer stars remain part of the type.
    value = re.sub(r"\s+[A-Za-z_]\w*$", "", value).strip()
    words = set(re.findall(r"[A-Za-z_]\w*", value))
    if words & CALLBACK_TYPES:
        return "pointer"
    if "*" in value or value in {
        "HANDLE", "LPVOID", "LPCVOID", "LPWSTR", "LPCWSTR", "LPDWORD",
        "PBOOL", "PDWORD", "PULONG64", "LPFILETIME", "LPTHREAD_START_ROUTINE",
        "PMEMORY_BASIC_INFORMATION",
    }:
        return "pointer"
    if value == "void":
        return "void"
    if value == "bool":
        return "bool"
    if words & ENUM_TYPES or value in {"DWORD", "long", "int", "BOOL"}:
        return "i32"
    if value == "ULONG_PTR":
        return "pointer"
    if value == "SIZE_T":
        return "uintptr"
    # Named structures returned by value must continue to match by name.
    return value


def abi_signature(signature: tuple[str, tuple[str, ...]]) -> tuple[str, tuple[str, ...]]:
    return abi_type(signature[0], return_type=True), tuple(abi_type(arg) for arg in signature[1])


def check_header_conformance(canonical_header: Path, adapter_headers: list[Path], required: set[str]) -> bool:
    canonical = exported_declarations(canonical_header)
    missing_canonical = sorted(required - canonical.keys())
    if missing_canonical:
        print(f"FAIL {canonical_header}: missing declarations for {', '.join(missing_canonical)}")
        return False

    passed = True
    for header in adapter_headers:
        try:
            declarations = exported_declarations(header)
        except OSError as exc:
            print(f"FAIL {header}: {exc}")
            passed = False
            continue
        problems: list[str] = []
        for name in sorted(required):
            actual = declarations.get(name)
            if actual is None:
                problems.append(f"{name} (missing)")
            elif abi_signature(actual) != abi_signature(canonical[name]):
                problems.append(f"{name} (signature)")
        if problems:
            print(f"FAIL {header}: {', '.join(problems)}")
            passed = False
        else:
            print(f"PASS {header}: {len(required)} canonical ABI signatures conform")
    return passed


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
        "bin/x32/DbgEng/TitanEngine.dll",
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
    parser.add_argument(
        "--canonical-header",
        type=Path,
        default=root / "src/dbg/TitanEngine/TitanEngine.h",
        help="Canonical ABI declaration header",
    )
    parser.add_argument(
        "--adapter-header",
        action="append",
        type=Path,
        help="Adapter header to validate (may be repeated; defaults to all in-tree/DbgEng headers)",
    )
    args = parser.parse_args()

    required = canonical_exports(args.definition)
    dlls = args.dll or default_dlls(root)
    if not dlls:
        parser.error("no DLLs supplied and no built engine DLLs were found")

    adapter_headers = args.adapter_header or [
        root / "src/third_party/TitanEngine/TitanEngine/definitions.h",
        root / "src/third_party/GleeBug/TitanEngineEmulator/TitanEngine.h",
        root / "src/third_party/GleeBug/StaticEngine/TitanEngine.h",
        root.parent / "x64dbg-dbgeng/src/TitanEngine/TitanEngine.h",
    ]
    failed = not check_header_conformance(args.canonical_header, adapter_headers, required)
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
