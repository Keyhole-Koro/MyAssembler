#!/usr/bin/env python3
import argparse
import os
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[1]
PROJECT_ROOT = REPO_ROOT.parents[1]
TESTS_DIR = REPO_ROOT / "tests"
ASM_PATH = REPO_ROOT / "build" / "myas"
LINKER_PATH = PROJECT_ROOT / "toolchain" / "MyLinker" / "mllinker"
EMU_PATH = PROJECT_ROOT / "runtime" / "MyEmulator" / "build" / "myemu"

TESTCASES = [
    ("simpleChar", ["succeed/memory/simpleChar.masm"], "R1", 72),
    ("importFrom", ["succeed/function/importFrom_main.masm", "succeed/function/importFrom_helper.masm"], "R1", 11),
    ("localDataReloc", ["succeed/memory/localDataReloc_main.masm", "succeed/memory/localDataReloc_helper.masm"], "R1", 77),
    ("localLabelCollision", ["succeed/memory/localLabelCollision_main.masm", "succeed/memory/localLabelCollision_a.masm", "succeed/memory/localLabelCollision_b.masm"], "R1", 42),
]
VERBOSE = False


def colored(text, color_code):
    return f"\033[{color_code}m{text}\033[0m"


def status_line(label, message, color="36"):
    print(colored(f"[{label}]", color), message)


def run(command, cwd=None):
    return subprocess.run(command, cwd=cwd, check=True, capture_output=True, text=True)


def build_all():
    status_line("SETUP", "build assembler, linker, and emulator")
    for cmd, desc in [
        (["make", "-C", str(REPO_ROOT), "all"], "Build MyAssembler"),
        (["make", "-C", str(PROJECT_ROOT / 'toolchain' / 'MyLinker'), "all"], "Build MyLinker"),
        (["make", "-C", str(PROJECT_ROOT / 'runtime' / 'MyEmulator'), "all"], "Build MyEmulator"),
    ]:
        result = subprocess.run(cmd, check=False, capture_output=True, text=True)
        if result.returncode != 0:
            sys.stderr.write(result.stdout)
            sys.stderr.write(result.stderr)
            status_line("FATAL", f"{desc} failed", "31")
            raise SystemExit(1)


def run_tests(selected=None):
    cases = TESTCASES
    if selected:
        cases = [case for case in TESTCASES if case[0] == selected]
        if not cases:
            print(f"[ERROR] Test case '{selected}' not found.")
            raise SystemExit(1)
    status_line("RUN", f"{len(cases)} case(s)")
    temp_root = Path(tempfile.mkdtemp(prefix="myassembler-integration-tests-"))
    failures = []
    try:
        for basename, sources, reg, expected in cases:
            case_dir = temp_root / basename
            case_dir.mkdir(parents=True, exist_ok=True)
            linked_bin = case_dir / f"{basename}.bin"
            try:
                obj_paths = []
                for src_rel in sources:
                    asm_path = TESTS_DIR / src_rel
                    stem = Path(src_rel).stem
                    prelink_bin = case_dir / f"{stem}.prelink.bin"
                    obj_path = case_dir / f"{stem}.mobj"
                    run([str(ASM_PATH), str(asm_path), str(prelink_bin), "--obj", str(obj_path)], cwd=case_dir)
                    obj_paths.append(str(obj_path))

                run([str(LINKER_PATH), str(linked_bin)] + obj_paths, cwd=case_dir)
                output = run([str(EMU_PATH), "-i", str(linked_bin), "--reg", reg], cwd=case_dir).stdout
                lines = [line.strip() for line in output.splitlines() if line.strip()]
                actual = int(lines[-1], 0)
                if actual != expected:
                    failures.append((basename, f"expected {expected}, got {actual}"))
                    status_line("FAIL", f"{basename} expected {expected}, got {actual}", "31")
                else:
                    status_line("PASS", f"{basename} {reg} = {actual} (expected)", "32")
            except Exception as exc:
                failures.append((basename, str(exc)))
                status_line("FAIL", f"{basename} {exc}", "31")
    finally:
        shutil.rmtree(temp_root, ignore_errors=True)
    status_line("DONE", f"Summary: {len(cases) - len(failures)} passed, {len(failures)} failed", "32" if not failures else "33")
    if failures:
        raise SystemExit(1)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Run MyAssembler integration tests.")
    parser.add_argument("test", nargs="?", help="Optional test case name or source filename")
    parser.add_argument("--verbose", action="store_true", help="Reserved for future detailed logging")
    args = parser.parse_args()
    VERBOSE = args.verbose
    selected = None
    if args.test:
        base, ext = os.path.splitext(args.test)
        selected = base if ext else args.test
    build_all()
    run_tests(selected)
