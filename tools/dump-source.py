#!/usr/bin/env python3

from __future__ import annotations

import shutil
import subprocess
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
DEFAULT_OUT = ROOT / "dump" / "files"
SKIP_PARTS = {
    ".cache",
    ".codex",
    ".espressif",
    ".git",
    ".rustup",
    "build",
    "dump",
    "esp-idf",
    "managed_components",
}
ALLOWED_SUFFIXES = {
    ".c",
    ".cc",
    ".cpp",
    ".cxx",
    ".h",
    ".hh",
    ".hpp",
    ".hxx",
    ".cmake",
    ".txt",
}


def tracked_and_local_files() -> list[Path]:
    try:
        result = subprocess.run(
            ["git", "ls-files", "-co", "--exclude-standard"],
            cwd=ROOT,
            check=True,
            text=True,
            stdout=subprocess.PIPE,
        )
    except (OSError, subprocess.CalledProcessError):
        return [path.relative_to(ROOT) for path in ROOT.rglob("*") if path.is_file()]

    return [Path(line) for line in result.stdout.splitlines() if line]


def skipped(path: Path) -> bool:
    return any(part in SKIP_PARTS or part.startswith("build-") for part in path.parts)


def wanted(path: Path) -> bool:
    name = path.name
    lower = name.lower()

    if skipped(path) or name.startswith("sdkconfig"):
        return False
    if lower == "readme.md" or name == "CMakeLists.txt":
        return True
    if name.startswith("partitions") and path.suffix == ".csv":
        return True
    return path.suffix in ALLOWED_SUFFIXES


def flat_name(path: Path, used: set[str]) -> str:
    candidate = "__".join(path.parts)
    if candidate not in used:
        used.add(candidate)
        return candidate

    stem = Path(candidate).stem
    suffix = Path(candidate).suffix
    index = 2
    while True:
        fallback = f"{stem}__{index}{suffix}"
        if fallback not in used:
            used.add(fallback)
            return fallback
        index += 1


def main() -> int:
    out = Path(sys.argv[1]).expanduser().resolve() if len(sys.argv) > 1 else DEFAULT_OUT
    if out == ROOT or ROOT not in out.parents:
        print("dump output must stay inside this repository", file=sys.stderr)
        return 2

    shutil.rmtree(out, ignore_errors=True)
    out.mkdir(parents=True, exist_ok=True)

    copied: list[tuple[str, Path]] = []
    used: set[str] = set()
    for rel in sorted(set(tracked_and_local_files())):
        src = ROOT / rel
        if not src.is_file() or not wanted(rel):
            continue
        name = flat_name(rel, used)
        dst = out / name
        shutil.copy2(src, dst)
        copied.append((name, rel))

    manifest = out / "MANIFEST.txt"
    manifest.write_text(
        "\n".join(f"{name}\t{path}" for name, path in copied) + "\n",
        encoding="utf-8")
    print(f"dumped {len(copied)} files into {out.relative_to(ROOT)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
