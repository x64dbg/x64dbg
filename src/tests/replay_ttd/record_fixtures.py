"""Record the deterministic TTD fixtures used by the replay_ttd test.

Downloads the official TTD recorder (https://aka.ms/ttd/download), records the
already-built bin/<arch>/tests/replay_ttd.exe for each architecture, preserves
the recorded binaries next to the traces and refreshes the hashes in
fixture-manifest.json. Must be run from an elevated prompt.

The positional manifest entries (first, last, firstMilestoneHit,
handledException, processExitBoundary) are not derived here; update them by
replaying the new trace and re-running the replay_ttd test.
"""
from __future__ import annotations

import argparse
import ctypes
import hashlib
import json
import os
import re
import shutil
import subprocess
import sys
import tempfile
import urllib.request
import xml.etree.ElementTree as ET
import zipfile
from pathlib import Path

REPO = Path(__file__).resolve().parents[3]
MANIFEST = Path(__file__).with_name("fixture-manifest.json")
# x64dbg output arch -> recorder package (the x86 recorder records x32 targets)
PACKAGES = {"x64": "TTD-x64", "x32": "TTD-x86"}


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        while chunk := stream.read(1024 * 1024):
            digest.update(chunk)
    return digest.hexdigest()


def fetch_recorders(recorder_dir: Path, bundle: Path | None) -> None:
    if all((recorder_dir / arch / "TTD.exe").is_file() for arch in PACKAGES):
        return
    with tempfile.TemporaryDirectory() as temp:
        temp = Path(temp)
        if bundle is None:
            print("Downloading TTD from https://aka.ms/ttd/download")
            appinstaller = temp / "ttd.appinstaller"
            urllib.request.urlretrieve("https://aka.ms/ttd/download", appinstaller)
            root = ET.parse(appinstaller).getroot()
            uri = next(e.attrib["Uri"] for e in root.iter() if e.tag.endswith("}MainBundle") or e.tag == "MainBundle")
            bundle = temp / "ttd.msixbundle"
            urllib.request.urlretrieve(uri, bundle)
        with zipfile.ZipFile(bundle) as outer:
            for arch, package in PACKAGES.items():
                msix = next(n for n in outer.namelist() if n.lower() == f"{package}.msix".lower())
                package_path = temp / f"{package}.msix"
                package_path.write_bytes(outer.read(msix))
                target = recorder_dir / arch
                shutil.rmtree(target, ignore_errors=True)
                with zipfile.ZipFile(package_path) as inner:
                    inner.extractall(target)


def record(arch: str, recorder_dir: Path, fixture_dir: Path) -> dict:
    tests = REPO / "bin" / arch / "tests"
    target = tests / "replay_ttd.exe"
    helper = tests / "replay_ttd_helper.dll"
    for path in (target, helper):
        if not path.is_file():
            sys.exit(f"missing {path}; build the replay_ttd tests for {arch} first")

    out_dir = fixture_dir / arch
    shutil.rmtree(out_dir, ignore_errors=True)
    (out_dir / "binaries").mkdir(parents=True)
    trace = out_dir / "replay_ttd.run"
    command = [str(recorder_dir / arch / "TTD.exe"), "-acceptEula", "-noUI", "-out", str(out_dir / "replay_ttd"),
               "-passThroughExit", "-launch", str(target)]
    print(" ".join(command))
    subprocess.run(command, check=True)
    if not trace.is_file():
        sys.exit(f"recorder did not produce {trace}")

    for name in ("replay_ttd.exe", "replay_ttd.pdb", "replay_ttd_helper.dll", "replay_ttd_helper.pdb"):
        shutil.copy2(tests / name, out_dir / "binaries" / name)

    log = (out_dir / "replay_ttd.out").read_text(errors="replace")
    release = re.search(r"Release:\s*(\S+)", log)
    version = re.search(r"TTDRecord\s+(\S+)", log)
    return {
        "trace": f"{arch}/replay_ttd.run",
        "sha256": sha256_file(trace),
        "size": trace.stat().st_size,
        "targetSha256": sha256_file(target),
        "helperSha256": sha256_file(helper),
        "release": release.group(1) if release else None,
        "version": version.group(1) if version else None,
    }


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--arch", action="append", choices=sorted(PACKAGES), help="default: all")
    parser.add_argument("--fixture-dir", type=Path,
                        default=Path(os.environ.get("X64DBG_TTD_FIXTURE_DIR", REPO / "build" / "replay-ttd-fixtures")))
    parser.add_argument("--recorder-dir", type=Path, default=REPO / "build" / "ttd-recorder")
    parser.add_argument("--bundle", type=Path, help="use a local .msixbundle instead of downloading")
    args = parser.parse_args()

    if not ctypes.windll.shell32.IsUserAnAdmin():
        sys.exit("TTD recording requires an elevated prompt")

    fetch_recorders(args.recorder_dir, args.bundle)
    manifest = json.loads(MANIFEST.read_text(encoding="utf-8"))
    for arch in args.arch or sorted(PACKAGES):
        result = record(arch, args.recorder_dir, args.fixture_dir)
        entry = manifest["fixtures"][arch]
        for key in ("trace", "sha256", "size", "targetSha256", "helperSha256"):
            entry[key] = result[key]
        if result["release"]:
            manifest["recorder"]["release"] = result["release"]
        if result["version"]:
            manifest["recorder"]["version"] = result["version"]
        print(f"{arch}: recorded {result['size']} bytes, sha256 {result['sha256']}")
    MANIFEST.write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
    print("Manifest hashes updated. Replay the traces and update first/last/milestone positions if they changed.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
