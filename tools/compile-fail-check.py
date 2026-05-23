#!/usr/bin/env python3
from __future__ import annotations

import os
import subprocess
import sys
import tempfile
from dataclasses import dataclass
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]

PREFIX = """
#include <cstdint>
#include <utility>

#include "arc/borrow.hpp"

namespace {

struct State {
    std::uint32_t value{};
};

constinit State state{};
constinit State other{};
constinit const State const_state{};

}  // namespace
"""


@dataclass(frozen=True)
class Case:
    name: str
    source: str
    must_contain: str | None = None


CASES = (
    Case(
        name="wrong_core_static_read",
        source="""
using Cell = arc::StaticRef<&state, arc::Core::core1>;

void probe()
{
    (void)Cell::read<arc::Core::core0>();
}
""",
    ),
    Case(
        name="owner_bound_arrow",
        source="""
using Cell = arc::StaticRef<&state, arc::Core::core1>;

void probe()
{
    auto loan = Cell::read<arc::Core::core1>();
    (void)loan.operator->();
}
""",
    ),
    Case(
        name="mutable_const_loan",
        source="""
using Bad = arc::StaticLoan<&const_state, arc::Core::any, arc::BorrowMode::mut>;
static_assert(Bad::mode == arc::BorrowMode::mut);
""",
        must_contain="mutable mode cannot wrap const storage",
    ),
    Case(
        name="conflicting_loan_pack",
        source="""
using Cell = arc::StaticRef<&state, arc::Core::core1>;
using Bad = arc::LoanPack<Cell::Read, Cell::Write>;
static_assert(Bad::count == 2U);
""",
        must_contain="static borrow conflict",
    ),
    Case(
        name="conflicting_static_edit",
        source="""
using Cell = arc::StaticRef<&state, arc::Core::core1>;
using Bad = arc::StaticEdit<Cell, Cell>;
static_assert(Bad::count == 2U);
""",
        must_contain="static borrow conflict",
    ),
    Case(
        name="move_mutable_loan",
        source="""
using Cell = arc::StaticRef<&state, arc::Core::core1>;

void probe()
{
    auto loan = Cell::write<arc::Core::core1>();
    auto moved = std::move(loan);
    static_cast<void>(moved);
}
""",
    ),
)


def compiler() -> str:
    return os.environ.get("CXX", "c++")


def compile_case(case: Case, tmp: Path) -> str | None:
    source = tmp / f"{case.name}.cpp"
    source.write_text(PREFIX + "\n" + case.source, encoding="utf-8")

    cmd = [
        compiler(),
        "-std=gnu++23",
        "-fsyntax-only",
        "-Itests/host/stubs",
        "-Icomponents/arc/include",
        str(source),
    ]
    result = subprocess.run(
        cmd,
        cwd=ROOT,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
        check=False,
    )
    output = result.stdout + result.stderr
    if result.returncode == 0:
        return f"{case.name}: expected compile failure, but compilation succeeded"
    if case.must_contain is not None and case.must_contain not in output:
        return f"{case.name}: expected diagnostic containing {case.must_contain!r}\n{output}"
    return None


def main() -> int:
    problems: list[str] = []
    with tempfile.TemporaryDirectory(prefix="arc-compile-fail-") as tmp_dir:
        tmp = Path(tmp_dir)
        for case in CASES:
            problem = compile_case(case, tmp)
            if problem is not None:
                problems.append(problem)

    if problems:
        for problem in problems:
            print(f"arc compile-fail check failed: {problem}", file=sys.stderr)
        return 1

    print(f"arc compile-fail check: OK ({len(CASES)} cases)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
