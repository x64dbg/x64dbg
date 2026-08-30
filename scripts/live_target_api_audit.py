# /// script
# requires-python = ">=3.11"
# dependencies = [
#   "multilspy @ git+https://github.com/microsoft/multilspy.git@b9a0ef1bc221e5575787be80493059b01f03c37a",
#   "pefile==2024.8.26",
# ]
# ///
"""Inventory live process/thread API use in x64dbg and build reverse call graphs.

Run from the repository root:

    uv run scripts/live_target_api_audit.py

The analyzer deliberately launches clangd with --compile-commands-dir=<repo>/build.
It refuses to run without a usable compile database containing BUILD_DBG commands;
falling back to clangd's header guessing would make the results incomplete.

The PE import table is a seed/cross-check, not the source of call sites. Source
locations and caller edges come from clangd through MultiLSPy. Dynamic APIs and
TitanEngine's process/thread-facing surface are included explicitly.
"""

from __future__ import annotations

import argparse
import asyncio
import contextlib
import dataclasses
import hashlib
import json
import logging
import os
import pathlib
import re
import shutil
import subprocess
import sys
import time
from collections import Counter, defaultdict, deque
from typing import Any, Iterable

import pefile
from multilspy.language_server import LanguageServer
from multilspy.language_servers.clangd_language_server.clangd_language_server import ClangdLanguageServer
from multilspy.lsp_protocol_handler.server import ProcessLaunchInfo
from multilspy.multilspy_config import Language, MultilspyConfig
from multilspy.multilspy_logger import MultilspyLogger

# This catalog is intentionally semantic, rather than a broad "name contains
# Process" filter.  Context-sensitive handle APIs are retained, but called out
# so that target and debugger-internal/file/event uses are not conflated.
# module, category, confidence/scope note
API_GROUPS: dict[str, tuple[str, str, str]] = {}


def api(module: str, category: str, names: str, note: str = "direct target operation") -> None:
    for name in names.split():
        API_GROUPS[name] = (module, category, note)


api("KERNEL32.dll", "memory.read", "ReadProcessMemory")
api("KERNEL32.dll", "memory.query", "VirtualQueryEx K32GetMappedFileNameW GetMappedFileNameW QueryWorkingSetEx")
api("KERNEL32.dll", "memory.allocate", "VirtualAllocEx VirtualFreeEx")
api("KERNEL32.dll", "memory.protect", "VirtualProtectEx")
api("KERNEL32.dll", "process.open", "OpenProcess")
api("KERNEL32.dll", "thread.open", "OpenThread")
api("KERNEL32.dll", "process.query", "CheckRemoteDebuggerPresent GetProcessId IsWow64Process K32GetModuleFileNameExW GetModuleFileNameExW K32GetProcessImageFileNameW GetProcessImageFileNameW GetProcessDEPPolicy")
api("KERNEL32.dll", "thread.query", "GetProcessIdOfThread GetThreadId GetThreadPriority GetThreadTimes QueryThreadCycleTime GetThreadDescription")
api("KERNEL32.dll", "thread.context", "GetThreadContext")
api("KERNEL32.dll", "thread.control", "SuspendThread ResumeThread SetThreadPriority")
api("KERNEL32.dll", "thread.create", "CreateRemoteThread")
api("KERNEL32.dll", "process.control", "DebugBreakProcess TerminateProcess")
api("KERNEL32.dll", "thread.control", "TerminateThread")
api("KERNEL32.dll", "enumeration", "CreateToolhelp32Snapshot Process32First Process32Next Thread32First Thread32Next")
api("KERNEL32.dll", "handle.remote", "DuplicateHandle")
api("KERNEL32.dll", "handle.context-sensitive", "CloseHandle GetHandleInformation WaitForSingleObject WaitForMultipleObjects", "context-sensitive: includes target, debugger worker, file, event, and semaphore handles")
api("USER32.dll", "enumeration", "GetWindowThreadProcessId")
api("USER32.dll", "thread.control", "PostThreadMessageA")
api("USER32.dll", "window.context-sensitive", "EnumWindows EnumChildWindows GetClassLongPtrA GetClassLongPtrW GetClassNameW GetForegroundWindow GetParent GetWindow GetWindowRect GetWindowTextW IsWindow IsWindowEnabled IsWindowUnicode IsWindowVisible EnableWindow", "live host/window state; some sites inspect or control debuggee windows, so keep outside dump/TTD target-state providers")
api("ADVAPI32.dll", "process.token", "OpenProcessToken GetTokenInformation AdjustTokenPrivileges LookupPrivilegeValueW")
api("PSAPI.dll", "memory.query", "QueryWorkingSetEx")
api("IPHLPAPI.dll", "process.network", "GetTcpTable2 GetTcp6Table2", "system live-state enumeration filtered by owning PID; unavailable for dump/TTD state")

api("ntdll.dll", "memory.read", "NtReadVirtualMemory NtWow64ReadVirtualMemory64")
api("ntdll.dll", "memory.write", "NtWriteVirtualMemory NtWow64WriteVirtualMemory64")
api("ntdll.dll", "process.query", "NtQueryInformationProcess NtWow64QueryInformationProcess64")
api("ntdll.dll", "thread.query", "NtQueryInformationThread")
api("ntdll.dll", "thread.control", "NtSuspendThread")
api("ntdll.dll", "system.enumeration", "NtQuerySystemInformation")
api("ntdll.dll", "handle.remote", "NtQueryObject")
api("ntdll.dll", "debug.event.file", "NtFsControlFile", "operates on LOAD_DLL_DEBUG_INFO.hFile; live-debug event compatibility dependency")

api("dbghelp.dll", "dump", "MiniDumpWriteDump")
api("dbghelp.dll", "stack", "StackWalk64")
api("dbghelp.dll", "symbols", "SymGetModuleInfoW64 SymGetSearchPathW SymLoadModuleExW SymSetSearchPathW SymUnloadModule64")

api("TitanEngine.dll", "memory.read", "MemoryReadSafe")
api("TitanEngine.dll", "memory.write", "MemoryWriteSafe")
api("TitanEngine.dll", "process.query", "GetPEBLocation")
api("TitanEngine.dll", "process.open", "TitanOpenProcess")
api("TitanEngine.dll", "thread.open", "TitanOpenThread")
api("TitanEngine.dll", "thread.query", "GetTEBLocation")
api("TitanEngine.dll", "thread.context", "GetFullContextDataEx SetFullContextDataEx GetContextDataEx SetContextDataEx GetAVXContext SetAVXContext GetAVX512Context SetAVX512Context")
api("TitanEngine.dll", "debug.session", "InitDebugW StopDebug DebugLoop AttachDebugger DetachDebuggerEx IsFileBeingDebugged GetDebugData SetCustomHandler SetNextDbgContinueStatus")
api("TitanEngine.dll", "breakpoint", "SetBPXOptions IsBPXEnabled SetBPX DeleteBPX SetMemoryBPXEx RemoveMemoryBPX GetUnusedHardwareBreakPointRegister SetHardwareBreakPoint DeleteHardwareBreakPoint RemoveAllBreakPoints")
api("TitanEngine.dll", "execution", "StepInto StepOver")
api("TitanEngine.dll", "engine.configuration", "SetEngineVariable EngineCheckStructAlignment", "engine configuration rather than a target access; retained as an engine-boundary dependency")

# APIs resolved by name are absent from the PE import table.  alias_patterns
# identify actual pointer calls where practical. Resolution sites are always
# emitted as evidence even if the pointer subsequently flows through a helper.
DYNAMIC_APIS: dict[str, list[str]] = {
    "NtWow64QueryInformationProcess64": [r"\bquery64\s*\("],
    "NtWow64ReadVirtualMemory64": [],
    "NtWow64WriteVirtualMemory64": [],
    "QueryWorkingSetEx": [r"\bfnQueryWorkingSetEx\s*\("],
    "GetProcessDEPPolicy": [r"\bGPDP\s*\("],
    "GetThreadDescription": [r"\b_GetThreadDescription\s*\("],
}

SOURCE_SUFFIXES = {".c", ".cc", ".cpp", ".cxx", ".h", ".hh", ".hpp", ".hxx"}
FUNCTION_KINDS = {6, 9, 12}  # Method, Constructor, Function (LSP SymbolKind)


@dataclasses.dataclass(frozen=True)
class Pos:
    path: str
    line: int
    character: int


@dataclasses.dataclass
class Symbol:
    id: str
    name: str
    path: str
    kind: int
    range: dict[str, Any]
    selection: dict[str, Any]


class QuietLogger(MultilspyLogger):
    def __init__(self, verbose: bool) -> None:
        super().__init__()
        self.verbose = verbose
        if not verbose:
            self.logger.disabled = True


class CompileDbClangd(ClangdLanguageServer):
    """MultiLSPy clangd adapter that makes the build compile DB mandatory."""

    def __init__(self, config: MultilspyConfig, logger: MultilspyLogger, root: str, compile_db_dir: str):
        clangd = self.setup_runtime_dependencies(logger, config)
        cmd = [
            clangd,
            f"--compile-commands-dir={compile_db_dir}",
            "--background-index",
            "--pch-storage=memory",
            "--header-insertion=never",
        ]
        LanguageServer.__init__(
            self,
            config,
            logger,
            root,
            ProcessLaunchInfo(cmd=cmd, cwd=root),
            "cpp",
        )
        self.server_ready = asyncio.Event()
        self.index_started = asyncio.Event()
        self.index_finished = asyncio.Event()
        self.index_progress: dict[str, Any] = {}
        self.inactive_regions: dict[str, list[dict[str, Any]]] = {}
        self.inactive_region_events: dict[str, asyncio.Event] = defaultdict(asyncio.Event)
        self.inactive_region_attempted: set[str] = set()

    def _get_initialize_params(self, repository_absolute_path: str):
        params = super()._get_initialize_params(repository_absolute_path)
        params.setdefault("capabilities", {}).setdefault("textDocument", {})["inactiveRegionsCapabilities"] = {
            "inactiveRegions": True
        }
        return params

    async def inactive_ranges(self, absolute_path: pathlib.Path, timeout: float = 2.0) -> list[dict[str, Any]]:
        uri = absolute_path.resolve().as_uri()
        if uri not in self.inactive_regions and uri not in self.inactive_region_attempted:
            self.inactive_region_attempted.add(uri)
            try:
                await asyncio.wait_for(self.inactive_region_events[uri].wait(), timeout=timeout)
            except asyncio.TimeoutError:
                # Older/non-conforming clangd builds may omit the extension.
                return []
        return self.inactive_regions.get(uri, [])

    @contextlib.asynccontextmanager
    async def start_server(self):
        async def ignore(_params):
            return None

        async def progress(params):
            value = params.get("value", {})
            token = str(params.get("token", ""))
            title = str(value.get("title", ""))
            text = (token + " " + title).lower()
            if "index" in text:
                self.index_progress = params
                if value.get("kind") == "begin":
                    self.index_started.set()
                elif value.get("kind") == "end":
                    self.index_finished.set()

        async def log_message(params):
            if getattr(self.logger, "verbose", False):
                self.logger.log(f"clangd: {params}", logging.INFO)

        async def inactive_regions(params):
            uri = params.get("uri") or params.get("textDocument", {}).get("uri", "")
            self.inactive_regions[uri] = params.get("regions", [])
            self.inactive_region_events[uri].set()

        self.server.on_request("client/registerCapability", ignore)
        self.server.on_notification("language/status", ignore)
        self.server.on_notification("window/logMessage", log_message)
        self.server.on_request("workspace/executeClientCommand", lambda _params: asyncio.sleep(0, result=[]))
        self.server.on_notification("$/progress", progress)
        self.server.on_notification("textDocument/publishDiagnostics", ignore)
        self.server.on_notification("language/actionableNotification", ignore)
        self.server.on_notification("experimental/serverStatus", ignore)
        self.server.on_notification("textDocument/inactiveRegions", inactive_regions)

        async with LanguageServer.start_server(self):
            await self.server.start()
            response = await self.server.send.initialize(self._get_initialize_params(self.repository_root_path))
            if "capabilities" not in response:
                raise RuntimeError(f"unexpected clangd initialize response: {response}")
            self.server.notify.initialized({})
            self.completions_available.set()
            self.server_ready.set()
            try:
                yield self
            finally:
                await self.server.shutdown()
                await self.server.stop()

    async def wait_for_background_index(self, timeout: float) -> str:
        """Wait for clangd's index progress, with a cache-quiescence fallback."""
        try:
            await asyncio.wait_for(self.index_started.wait(), timeout=min(5.0, timeout))
        except asyncio.TimeoutError:
            # A warm index often emits no begin notification. Give clangd time
            # to load the shards; callers also perform a reference stability probe.
            await asyncio.sleep(2.0)
            return "warm/no-progress-notification"
        remaining = max(1.0, timeout - 5.0)
        try:
            await asyncio.wait_for(self.index_finished.wait(), timeout=remaining)
            return "background-index-finished"
        except asyncio.TimeoutError as exc:
            raise RuntimeError(f"clangd background index did not finish within {timeout}s; last progress={self.index_progress}") from exc


class Analyzer:
    def __init__(self, args: argparse.Namespace):
        self.args = args
        self.root = pathlib.Path(args.root).resolve()
        self.source_root = (self.root / args.source_root).resolve()
        self.compile_db = (self.root / args.compile_commands).resolve()
        self.binary = (self.root / args.binary).resolve()
        self.output = (self.root / args.output).resolve()
        self.poc = (self.root / args.poc).resolve()
        self.files: dict[str, str] = {}
        self.lines: dict[str, list[str]] = {}
        self.symbols_by_file: dict[str, list[Symbol]] = {}
        self.symbols: dict[str, Symbol] = {}
        self.refs_cache: dict[str, list[dict[str, Any]]] = {}
        self.direct_sites: list[dict[str, Any]] = []
        self.graph_edges: list[dict[str, Any]] = []
        self.graph_depth: dict[str, int] = {}
        self.imports: dict[str, set[str]] = defaultdict(set)
        self.compile_entries: list[dict[str, Any]] = []
        self.compile_files: set[str] = set()
        self.lsp: CompileDbClangd | None = None

    def rel(self, path: str | pathlib.Path) -> str:
        p = pathlib.Path(path)
        if not p.is_absolute():
            p = self.root / p
        try:
            return p.resolve().relative_to(self.root).as_posix()
        except ValueError:
            return p.resolve().as_posix()

    def read_text(self, rel: str) -> str:
        rel = rel.replace("\\", "/")
        if rel not in self.files:
            self.files[rel] = (self.root / rel).read_text(encoding="utf-8", errors="replace")
            self.lines[rel] = self.files[rel].splitlines()
        return self.files[rel]

    def validate_inputs(self) -> dict[str, Any]:
        if not self.compile_db.is_file():
            raise RuntimeError(f"required compile database is missing: {self.compile_db}")
        self.compile_entries = json.loads(self.compile_db.read_text(encoding="utf-8"))
        for entry in self.compile_entries:
            file = self.rel(entry["file"])
            command = entry.get("command", " ".join(entry.get("arguments", [])))
            if file.startswith(self.rel(self.source_root) + "/") and ("BUILD_DBG" in command or self.args.allow_non_dbg_commands):
                self.compile_files.add(file)
        if not self.compile_files:
            raise RuntimeError(
                f"{self.compile_db} has no {self.rel(self.source_root)} entries with BUILD_DBG; refusing clangd fallback parsing"
            )
        if not self.binary.is_file():
            raise RuntimeError(f"seed binary is missing: {self.binary}")
        clangd = pathlib.Path(self.args.clangd) if self.args.clangd else pathlib.Path(shutil.which("clangd") or "")
        if not clangd.is_file():
            raise RuntimeError("clangd was not found; pass --clangd")
        self.args.clangd = str(clangd.resolve())
        return {
            "compile_commands": self.rel(self.compile_db),
            "compile_entries": len(self.compile_entries),
            "dbg_translation_units": len(self.compile_files),
            "clangd": self.args.clangd,
            "binary": self.rel(self.binary),
            "binary_sha256": hashlib.sha256(self.binary.read_bytes()).hexdigest(),
            "binary_mtime": self.binary.stat().st_mtime,
            "source_head": subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=self.root, text=True).strip(),
        }

    def load_imports(self) -> None:
        pe = pefile.PE(str(self.binary), fast_load=True)
        pe.parse_data_directories(
            directories=[
                pefile.DIRECTORY_ENTRY["IMAGE_DIRECTORY_ENTRY_IMPORT"],
                pefile.DIRECTORY_ENTRY["IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT"],
            ]
        )
        for attr in ("DIRECTORY_ENTRY_IMPORT", "DIRECTORY_ENTRY_DELAY_IMPORT"):
            for desc in getattr(pe, attr, []):
                module = desc.dll.decode(errors="replace")
                for item in desc.imports:
                    name = item.name.decode(errors="replace") if item.name else f"ordinal:{item.ordinal}"
                    self.imports[module].add(name)

    def source_files(self) -> list[str]:
        result = []
        for path in self.source_root.rglob("*"):
            if path.is_file() and path.suffix.lower() in SOURCE_SUFFIXES:
                result.append(self.rel(path))
        return sorted(result)

    @staticmethod
    def offset_to_line_col(text: str, offset: int) -> tuple[int, int]:
        line = text.count("\n", 0, offset)
        start = text.rfind("\n", 0, offset) + 1
        return line, offset - start

    @staticmethod
    def contains(rng: dict[str, Any], line: int, char: int) -> bool:
        start, end = rng["start"], rng["end"]
        return (line, char) >= (start["line"], start["character"]) and (line, char) <= (end["line"], end["character"])

    async def file_symbols(self, rel: str) -> list[Symbol]:
        rel = rel.replace("\\", "/")
        if rel in self.symbols_by_file:
            return self.symbols_by_file[rel]
        assert self.lsp
        try:
            raw, _ = await self.lsp.request_document_symbols(rel)
        except Exception as exc:
            print(f"warning: document symbols failed for {rel}: {exc}", file=sys.stderr)
            self.symbols_by_file[rel] = []
            return []
        result = []
        for item in raw:
            if item.get("kind") not in FUNCTION_KINDS:
                continue
            # clangd commonly returns flat SymbolInformation (location/range),
            # while MultiLSPy also supports DocumentSymbol (range/selectionRange).
            # Derive a name selection for SymbolInformation so references are
            # requested on the identifier, not on the return type.
            if "range" in item:
                rng = item["range"]
                selection = item.get("selectionRange", {"start": rng["start"], "end": rng["start"]})
            elif "location" in item and "range" in item["location"]:
                rng = item["location"]["range"]
                name = item["name"].split("(")[0].split("::")[-1]
                selection = self.find_symbol_selection(rel, rng, name)
            else:
                continue
            sel = selection["start"]
            sid = f"{rel}:{sel['line'] + 1}:{sel['character'] + 1}:{item['name']}"
            sym = Symbol(sid, item["name"], rel, item["kind"], rng, selection)
            self.symbols[sid] = sym
            result.append(sym)
        self.symbols_by_file[rel] = result
        return result

    def find_symbol_selection(self, rel: str, rng: dict[str, Any], name: str) -> dict[str, Any]:
        lines = self.read_text(rel).splitlines()
        first = rng["start"]["line"]
        last = min(rng["end"]["line"], first + 12, len(lines) - 1)
        pattern = re.compile(r"\b" + re.escape(name) + r"\b") if re.match(r"^[A-Za-z_]\w*$", name) else re.compile(re.escape(name))
        for line_no in range(first, last + 1):
            start_char = rng["start"]["character"] if line_no == first else 0
            match = pattern.search(lines[line_no], start_char)
            if match:
                return {
                    "start": {"line": line_no, "character": match.start()},
                    "end": {"line": line_no, "character": match.end()},
                }
        return {"start": rng["start"], "end": rng["start"]}

    async def enclosing_symbol(self, rel: str, line: int, char: int) -> Symbol | None:
        candidates = [s for s in await self.file_symbols(rel) if self.contains(s.range, line, char)]
        if not candidates:
            return None
        return min(
            candidates,
            key=lambda s: (
                s.range["end"]["line"] - s.range["start"]["line"],
                s.range["end"]["character"] - s.range["start"]["character"],
            ),
        )

    def occurrence_kind(self, rel: str, line: int, char: int, token: str) -> str:
        text_line = self.lines.get(rel, self.read_text(rel).splitlines())[line] if line < len(self.lines[rel]) else ""
        tail = text_line[char + len(token):]
        if re.match(r"\s*\(", tail):
            return "call"
        return "reference"

    @staticmethod
    def code_mask(text: str) -> str:
        """Blank comments and quoted literals while preserving every offset."""
        chars = list(text)
        state = "code"
        i = 0
        while i < len(chars):
            c = chars[i]
            n = chars[i + 1] if i + 1 < len(chars) else ""
            if state == "code":
                if c == "/" and n == "/":
                    chars[i] = chars[i + 1] = " "
                    state = "line-comment"
                    i += 2
                    continue
                if c == "/" and n == "*":
                    chars[i] = chars[i + 1] = " "
                    state = "block-comment"
                    i += 2
                    continue
                if c in {'\"', "'"}:
                    chars[i] = " "
                    state = "string" if c == '\"' else "char"
                    i += 1
                    continue
            elif state == "line-comment":
                if c == "\n":
                    state = "code"
                else:
                    chars[i] = " "
            elif state == "block-comment":
                if c == "*" and n == "/":
                    chars[i] = chars[i + 1] = " "
                    state = "code"
                    i += 2
                    continue
                if c != "\n":
                    chars[i] = " "
            else:  # string or character literal
                quote = '\"' if state == "string" else "'"
                if c == "\\":
                    chars[i] = " "
                    if i + 1 < len(chars) and chars[i + 1] != "\n":
                        chars[i + 1] = " "
                    i += 2
                    continue
                if c == quote:
                    chars[i] = " "
                    state = "code"
                elif c != "\n":
                    chars[i] = " "
            i += 1
        return "".join(chars)

    def lexical_occurrences(self) -> dict[str, list[Pos]]:
        names = sorted(API_GROUPS, key=len, reverse=True)
        pattern = re.compile(r"\b(" + "|".join(map(re.escape, names)) + r")\b")
        result: dict[str, list[Pos]] = defaultdict(list)
        for rel in self.source_files():
            # Local declaration mirrors are not call sites.
            if rel in {"src/dbg/ntdll/ntdll.h", "src/dbg/dbghelp/dbghelp.h", "src/dbg/TitanEngine/TitanEngine.h"}:
                continue
            text = self.read_text(rel)
            masked = self.code_mask(text)
            for match in pattern.finditer(masked):
                line, char = self.offset_to_line_col(masked, match.start())
                # Require call syntax. Dynamic resolution is handled separately.
                if re.match(r"\s*\(", masked[match.end():match.end() + 32]):
                    result[match.group(1)].append(Pos(rel, line, char))
        return result

    async def collect_direct_sites(self) -> None:
        occurrences = self.lexical_occurrences()
        seen: set[tuple[str, str, int, int, str]] = set()
        for name, positions in sorted(occurrences.items()):
            module, category, note = API_GROUPS[name]
            for pos in positions:
                definitions = []
                if not self.args.skip_lsp_site_validation:
                    assert self.lsp
                    try:
                        definitions = await self.lsp.request_definition(pos.path, pos.line, pos.character)
                    except Exception as exc:
                        print(f"warning: definition validation failed for {name} at {pos.path}:{pos.line + 1}: {exc}", file=sys.stderr)
                    if not definitions:
                        continue
                    inactive = await self.lsp.inactive_ranges(self.root / pos.path)
                    if any(self.contains(rng, pos.line, pos.character) for rng in inactive):
                        continue
                sym = await self.enclosing_symbol(pos.path, pos.line, pos.character)
                if not sym:
                    continue
                # Ignore a function's own declaration/definition name.
                sel = sym.selection["start"]
                if sym.name.split("(")[0].split("::")[-1] == name and (pos.line, pos.character) == (sel["line"], sel["character"]):
                    continue
                key = (name, pos.path, pos.line, pos.character, "direct")
                if key in seen:
                    continue
                seen.add(key)
                imported_as = self.imported_alias(name)
                self.direct_sites.append({
                    "api": name,
                    "module": module,
                    "category": category,
                    "note": note,
                    "dispatch": "direct",
                    "imported_as": imported_as,
                    "definitions": [
                        {
                            "path": str(d.get("relativePath", d.get("absolutePath", ""))).replace("\\", "/"),
                            "line": d["range"]["start"]["line"] + 1,
                            "character": d["range"]["start"]["character"] + 1,
                        }
                        for d in definitions
                    ],
                    "location": {"path": pos.path, "line": pos.line + 1, "character": pos.character + 1},
                    "function": sym.id,
                })

        # GetProcAddress evidence and known pointer invocation aliases.
        for rel in self.source_files():
            text = self.read_text(rel)
            for api_name, alias_patterns in DYNAMIC_APIS.items():
                for match in re.finditer(r"[\"']" + re.escape(api_name) + r"[\"']", text):
                    line, char = self.offset_to_line_col(text, match.start() + 1)
                    if not self.args.skip_lsp_site_validation:
                        assert self.lsp
                        inactive = await self.lsp.inactive_ranges(self.root / rel)
                        if any(self.contains(rng, line, char) for rng in inactive):
                            continue
                    sym = await self.enclosing_symbol(rel, line, char)
                    if sym:
                        key = (api_name, rel, line, char, "dynamic-resolution")
                        if key not in seen:
                            seen.add(key)
                            module, category, note = API_GROUPS[api_name]
                            self.direct_sites.append({
                                "api": api_name,
                                "module": module,
                                "category": category,
                                "note": note,
                                "dispatch": "dynamic-resolution",
                                "imported_as": None,
                                "location": {"path": rel, "line": line + 1, "character": char + 1},
                                "function": sym.id,
                            })
                for alias_pattern in alias_patterns:
                    for match in re.finditer(alias_pattern, text):
                        line, char = self.offset_to_line_col(text, match.start())
                        if not self.args.skip_lsp_site_validation:
                            assert self.lsp
                            inactive = await self.lsp.inactive_ranges(self.root / rel)
                            if any(self.contains(rng, line, char) for rng in inactive):
                                continue
                        sym = await self.enclosing_symbol(rel, line, char)
                        if sym:
                            key = (api_name, rel, line, char, "function-pointer")
                            if key not in seen:
                                seen.add(key)
                                module, category, note = API_GROUPS[api_name]
                                self.direct_sites.append({
                                    "api": api_name,
                                    "module": module,
                                    "category": category,
                                    "note": note,
                                    "dispatch": "function-pointer",
                                    "imported_as": None,
                                    "location": {"path": rel, "line": line + 1, "character": char + 1},
                                    "function": sym.id,
                                })

    def imported_alias(self, name: str) -> str | None:
        aliases = {
            "GetMappedFileNameW": "K32GetMappedFileNameW",
            "GetModuleFileNameExW": "K32GetModuleFileNameExW",
            "GetProcessImageFileNameW": "K32GetProcessImageFileNameW",
        }
        needle = aliases.get(name, name).lower()
        for module, names in self.imports.items():
            for imported in names:
                if imported.lower() == needle:
                    return f"{module}!{imported}"
        return None

    async def references(self, sym: Symbol) -> list[dict[str, Any]]:
        if sym.id in self.refs_cache:
            return self.refs_cache[sym.id]
        assert self.lsp
        start = sym.selection["start"]
        try:
            raw = await self.lsp.request_references(sym.path, start["line"], start["character"])
        except Exception as exc:
            print(f"warning: references failed for {sym.id}: {exc}", file=sys.stderr)
            raw = []
        self.refs_cache[sym.id] = raw
        return raw

    async def build_reverse_graph(self) -> None:
        seeds = sorted({site["function"] for site in self.direct_sites})
        queue = deque((sid, 0) for sid in seeds)
        for sid in seeds:
            self.graph_depth[sid] = 0
        edge_keys: set[tuple[str, str, str, int, int]] = set()
        processed: set[str] = set()

        while queue:
            callee_id, depth = queue.popleft()
            if callee_id in processed or depth >= self.args.max_depth:
                continue
            if len(self.symbols) >= self.args.max_nodes:
                print(f"warning: stopping call graph at --max-nodes={self.args.max_nodes}", file=sys.stderr)
                break
            processed.add(callee_id)
            callee = self.symbols[callee_id]
            for ref in await self.references(callee):
                rel = str(ref.get("relativePath", "")).replace("\\", "/")
                if not rel.startswith(self.rel(self.source_root) + "/"):
                    continue
                start = ref["range"]["start"]
                caller = await self.enclosing_symbol(rel, start["line"], start["character"])
                if not caller or caller.id == callee_id:
                    continue
                kind = self.occurrence_kind(rel, start["line"], start["character"], callee.name.split("(")[0].split("::")[-1])
                key = (caller.id, callee_id, kind, start["line"], start["character"])
                if key in edge_keys:
                    continue
                edge_keys.add(key)
                self.graph_edges.append({
                    "caller": caller.id,
                    "callee": callee_id,
                    "kind": kind,
                    "location": {"path": rel, "line": start["line"] + 1, "character": start["character"] + 1},
                })
                next_depth = depth + 1
                if caller.id not in self.graph_depth or next_depth < self.graph_depth[caller.id]:
                    self.graph_depth[caller.id] = next_depth
                    queue.append((caller.id, next_depth))

    def import_crosscheck(self) -> dict[str, Any]:
        relevant_imports = []
        uncatalogued = []
        catalog_lower = {name.lower() for name in API_GROUPS}
        broad = re.compile(r"(?:Process|Thread|Virtual|Debug|Context|WorkingSet|StackWalk|MiniDump|^Nt)", re.I)
        for module, names in sorted(self.imports.items(), key=lambda item: item[0].lower()):
            for name in sorted(names):
                row = {"module": module, "name": name}
                if name.lower() in catalog_lower or any(self.imported_alias(api_name) == f"{module}!{name}" for api_name in API_GROUPS):
                    relevant_imports.append(row)
                elif broad.search(name):
                    uncatalogued.append(row)
        return {"relevant": relevant_imports, "review_candidates_not_in_catalog": uncatalogued}

    def poc_inventory(self) -> dict[str, Any]:
        cpp = self.poc / "src/TitanEngine/TitanEngine.cpp"
        if not cpp.is_file():
            return {"path": self.rel(self.poc), "error": "POC TitanEngine.cpp not found"}
        text = cpp.read_text(encoding="utf-8", errors="replace")
        methods = sorted(set(re.findall(r"gDebug(?:Client|Control|DataSpaces|Registers|Symbols|SystemObjects)->([A-Za-z_]\w*)", text)))
        export_details: dict[str, Any] = {}
        signature = re.compile(r"__declspec\(dllexport\)\s+[^\n{]+?\b([A-Za-z_]\w*)\s*\([^;{]*\)\s*\{")
        for match in signature.finditer(text):
            name = match.group(1)
            open_brace = text.find("{", match.start(), match.end())
            depth = 0
            end = open_brace
            for end in range(open_brace, len(text)):
                if text[end] == "{":
                    depth += 1
                elif text[end] == "}":
                    depth -= 1
                    if depth == 0:
                        break
            body = text[open_brace + 1:end]
            compact = re.sub(r"//[^\n]*|/\*.*?\*/|\s+", " ", body, flags=re.S).strip()
            obvious_stub = bool(
                re.fullmatch(r"(?:return (?:0|\{\}); )?__debugbreak\(\); return \{\};", compact)
                or re.fullmatch(r"__debugbreak\(\); return;", compact)
            )
            if obvious_stub:
                status = "stub"
            elif "__debugbreak()" in body or re.search(r"TODO:\s*(?:implement|support|switch|how to continue)", body, re.I):
                status = "partial"
            else:
                status = "implemented"
            export_details[name] = {
                "status": status,
                "line": text.count("\n", 0, match.start()) + 1,
                "dbgeng_methods": sorted(set(re.findall(r"gDebug(?:Client|Control|DataSpaces|Registers|Symbols|SystemObjects)->([A-Za-z_]\w*)", body))),
            }
        return {
            "path": self.rel(self.poc),
            "dbgeng_methods": methods,
            "defined_titan_exports": export_details,
            "status_counts": dict(sorted(Counter(v["status"] for v in export_details.values()).items())),
        }

    def shortest_stacks(self) -> list[dict[str, Any]]:
        incoming: dict[str, list[dict[str, Any]]] = defaultdict(list)
        outgoing: dict[str, list[dict[str, Any]]] = defaultdict(list)
        for edge in self.graph_edges:
            incoming[edge["callee"]].append(edge)
            outgoing[edge["caller"]].append(edge)
        roots = {sid for sid in self.symbols if not incoming.get(sid)}
        # "reference" edges (callbacks/registration) are real reachability hints,
        # but are marked in each stack rather than represented as direct calls.
        results = []
        sites_by_function: dict[str, list[dict[str, Any]]] = defaultdict(list)
        for site in self.direct_sites:
            sites_by_function[site["function"]].append(site)
        for seed, sites in sorted(sites_by_function.items()):
            q = deque([(seed, [seed], [])])
            seen = {seed}
            found: list[tuple[list[str], list[str]]] = []
            while q and len(found) < self.args.max_stacks_per_function:
                node, path, kinds = q.popleft()
                callers = incoming.get(node, [])
                if not callers or node in roots:
                    found.append((list(reversed(path)), list(reversed(kinds))))
                    continue
                for edge in callers:
                    caller = edge["caller"]
                    if caller in seen:
                        continue
                    seen.add(caller)
                    q.append((caller, path + [caller], kinds + [edge["kind"]]))
            results.append({
                "target_function": seed,
                "apis": sorted({s["api"] for s in sites}),
                "stacks": [{"functions": path, "edge_kinds": kinds} for path, kinds in found],
            })
        return results

    def write_results(self, metadata: dict[str, Any], index_status: str) -> None:
        self.output.mkdir(parents=True, exist_ok=True)
        symbols_json = {
            sid: {
                "name": sym.name,
                "path": sym.path,
                "kind": sym.kind,
                "selection": sym.selection,
                "reverse_depth": self.graph_depth.get(sid),
            }
            for sid, sym in sorted(self.symbols.items())
        }
        inventory = {
            "metadata": {
                **metadata,
                "index_status": index_status,
                "inactive_region_files": len(self.lsp.inactive_regions) if self.lsp else 0,
                "lsp_site_validation": not self.args.skip_lsp_site_validation,
                "generated_at": time.time(),
            },
            "scope": {
                "source_root": self.rel(self.source_root),
                "binary_is_seed_only": True,
                "compile_database_required": True,
                "limitations": [
                    "Static LSP graph: runtime-only dispatch and unresolved function pointers require manual review.",
                    "Context-sensitive handle APIs include non-target uses and must be classified by argument/ownership.",
                    "Dynamic APIs are found from GetProcAddress strings plus known aliases; arbitrary pointer propagation is not inferred.",
                ],
            },
            "api_catalog": {name: {"module": data[0], "category": data[1], "note": data[2]} for name, data in sorted(API_GROUPS.items())},
            "imports": {module: sorted(names) for module, names in sorted(self.imports.items())},
            "import_crosscheck": self.import_crosscheck(),
            "sites": sorted(self.direct_sites, key=lambda s: (s["api"], s["location"]["path"], s["location"]["line"])),
            "poc": self.poc_inventory(),
        }
        callgraph = {
            "symbols": symbols_json,
            "edges": self.graph_edges,
            "shortest_root_stacks": self.shortest_stacks(),
        }
        (self.output / "inventory.json").write_text(json.dumps(inventory, indent=2), encoding="utf-8")
        (self.output / "callgraph.json").write_text(json.dumps(callgraph, indent=2), encoding="utf-8")
        (self.output / "summary.md").write_text(self.render_summary(inventory, callgraph), encoding="utf-8")

    def render_summary(self, inventory: dict[str, Any], callgraph: dict[str, Any]) -> str:
        by_category: dict[str, list[dict[str, Any]]] = defaultdict(list)
        for site in inventory["sites"]:
            by_category[site["category"]].append(site)
        out = [
            "# Generated live target API inventory",
            "",
            "> Generated by `uv run scripts/live_target_api_audit.py`. The PE import table is only a seed/cross-check; locations and caller edges are source/LSP-derived.",
            "",
            f"- Source HEAD: `{inventory['metadata']['source_head']}`",
            f"- Compile DB: `{inventory['metadata']['compile_commands']}` ({inventory['metadata']['dbg_translation_units']} BUILD_DBG translation units)",
            f"- Seed binary: `{inventory['metadata']['binary']}` (`{inventory['metadata']['binary_sha256'][:16]}…`)",
            f"- API sites: **{len(inventory['sites'])}**",
            f"- Source functions in graph: **{len(callgraph['symbols'])}**",
            f"- Reverse caller edges: **{len(callgraph['edges'])}**",
            "",
        ]
        for category, sites in sorted(by_category.items()):
            out += [f"## {category}", ""]
            by_api: dict[str, list[dict[str, Any]]] = defaultdict(list)
            for site in sites:
                by_api[site["api"]].append(site)
            for api_name, api_sites in sorted(by_api.items()):
                out.append(f"### `{api_name}` ({len(api_sites)} site{'s' if len(api_sites) != 1 else ''})")
                for site in api_sites:
                    loc = site["location"]
                    func = self.symbols.get(site["function"])
                    out.append(f"- `{loc['path']}:{loc['line']}` — `{func.name if func else site['function']}` [{site['dispatch']}]")
                out.append("")
        out += [
            "## Import cross-check requiring manual review",
            "",
            "Broadly process/thread/debug-looking imports not in the semantic catalog:",
            "",
        ]
        for row in inventory["import_crosscheck"]["review_candidates_not_in_catalog"]:
            out.append(f"- `{row['module']}!{row['name']}`")
        out += ["", "## Artifacts", "", "- `inventory.json`: complete site/import/POC data", "- `callgraph.json`: normalized symbols, caller edges, and shortest root stacks", ""]
        return "\n".join(out)

    async def run(self) -> None:
        metadata = self.validate_inputs()
        self.load_imports()
        config = MultilspyConfig(code_language=Language.CPP, server_binary=self.args.clangd)
        logger = QuietLogger(self.args.verbose)
        self.lsp = CompileDbClangd(config, logger, str(self.root), str(self.compile_db.parent))
        async with self.lsp.start_server():
            print(f"clangd started with compile DB: {self.compile_db}")
            index_status = await self.lsp.wait_for_background_index(self.args.index_timeout)
            print(f"clangd index status: {index_status}")
            await self.collect_direct_sites()
            if not self.args.skip_lsp_site_validation and not self.lsp.inactive_regions:
                raise RuntimeError("clangd did not provide textDocument/inactiveRegions; refusing an inventory that may include the wrong compile-time branches")
            print(f"collected {len(self.direct_sites)} API sites in {len({s['function'] for s in self.direct_sites})} functions")
            await self.build_reverse_graph()
            print(f"collected {len(self.graph_edges)} reverse caller edges across {len(self.symbols)} functions")
        self.write_results(metadata, index_status)
        print(f"wrote {self.output / 'summary.md'}")
        print(f"wrote {self.output / 'inventory.json'}")
        print(f"wrote {self.output / 'callgraph.json'}")


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--root", default=".")
    parser.add_argument("--source-root", default="src/dbg")
    parser.add_argument("--compile-commands", default="build/compile_commands.json")
    parser.add_argument("--binary", default="bin/x64/x64dbg.dll")
    parser.add_argument("--poc", default="../x64dbg-dbgeng")
    parser.add_argument("--output", default="build/live-target-api-audit")
    parser.add_argument("--clangd", default=None)
    parser.add_argument("--index-timeout", type=float, default=240.0)
    parser.add_argument("--max-depth", type=int, default=16)
    parser.add_argument("--max-nodes", type=int, default=5000)
    parser.add_argument("--max-stacks-per-function", type=int, default=8)
    parser.add_argument("--allow-non-dbg-commands", action="store_true", help="accept compile DB entries without BUILD_DBG (not recommended)")
    parser.add_argument("--skip-lsp-site-validation", action="store_true", help="faster but includes inactive preprocessor branches; not recommended for final evidence")
    parser.add_argument("--verbose", action="store_true")
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    try:
        asyncio.run(Analyzer(args).run())
        return 0
    except KeyboardInterrupt:
        return 130
    except Exception as exc:
        print(f"error: {exc}", file=sys.stderr)
        if args.verbose:
            raise
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
