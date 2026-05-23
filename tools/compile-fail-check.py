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
#include <functional>
#include <utility>

#include "arc/borrow.hpp"
#include "arc/proof.hpp"
#include "arc/roles.hpp"
#include "arc/rpc.hpp"
#include "arc/spsc.hpp"

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
        name="wrong_core_static_member_write",
        source="""
using Cell = arc::StaticRef<&state, arc::Core::core1>;

void probe()
{
    Cell::with_write<arc::Core::core0>([](State&) {});
}
""",
    ),
    Case(
        name="wrong_core_static_snapshot",
        source="""
using Cell = arc::StaticRef<&state, arc::Core::core1>;

void probe()
{
    (void)Cell::snapshot<arc::Core::core0>();
}
""",
    ),
    Case(
        name="wrong_core_static_set",
        source="""
using Cell = arc::StaticRef<&state, arc::Core::core1>;

void probe()
{
    Cell::set<arc::Core::core0>(State{});
}
""",
    ),
    Case(
        name="wrong_core_static_free_set",
        source="""
using Cell = arc::StaticRef<&state, arc::Core::core1>;

void probe()
{
    arc::set<Cell, arc::Core::core0>(State{});
}
""",
    ),
    Case(
        name="const_static_set",
        source="""
using ConstCell = arc::StaticRef<&const_state>;

void probe()
{
    ConstCell::set(State{});
}
""",
    ),
    Case(
        name="wrong_core_loan_snapshot",
        source="""
using Cell = arc::StaticRef<&state, arc::Core::core1>;

void probe()
{
    const auto loan = Cell::read<arc::Core::core1>();
    (void)loan.snapshot<arc::Core::core0>();
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
        name="conflicting_member_edit",
        source="""
using Cell = arc::StaticRef<&state, arc::Core::core1>;
using Bad = Cell::Edit<Cell>;
static_assert(Bad::count == 2U);
""",
        must_contain="static borrow conflict",
    ),
    Case(
        name="const_member_edit",
        source="""
using ConstCell = arc::StaticRef<&const_state>;
using Bad = ConstCell::Edit<>;
static_assert(Bad::count == 1U);
""",
    ),
    Case(
        name="wrong_core_static_member_edit",
        source="""
using Cell = arc::StaticRef<&state, arc::Core::core1>;
using OtherCell = arc::StaticRef<&other, arc::Core::core1>;

void probe()
{
    Cell::with_edit<arc::Core::core0, OtherCell>([](State&, const State&) {});
}
""",
    ),
    Case(
        name="mixed_owner_inferred_reads",
        source="""
using Core1Cell = arc::StaticRef<&state, arc::Core::core1>;
using Core0Cell = arc::StaticRef<&other, arc::Core::core0>;

void probe()
{
    arc::with_reads<Core1Cell, Core0Cell>([](const State&, const State&) {});
}
""",
    ),
    Case(
        name="wrong_core_static_member_reads",
        source="""
using Cell = arc::StaticRef<&state, arc::Core::core1>;
using OtherCell = arc::StaticRef<&other, arc::Core::core1>;

void probe()
{
    Cell::with_reads<arc::Core::core0, OtherCell>([](const State&, const State&) {});
}
""",
    ),
    Case(
        name="mixed_owner_inferred_snapshots",
        source="""
using Core1Cell = arc::StaticRef<&state, arc::Core::core1>;
using Core0Cell = arc::StaticRef<&other, arc::Core::core0>;

void probe()
{
    (void)arc::snapshots<Core1Cell, Core0Cell>();
}
""",
    ),
    Case(
        name="mixed_owner_member_snapshots",
        source="""
using Core1Cell = arc::StaticRef<&state, arc::Core::core1>;
using Core0Cell = arc::StaticRef<&other, arc::Core::core0>;

void probe()
{
    (void)Core1Cell::snapshots<Core0Cell>();
}
""",
    ),
    Case(
        name="wrong_core_static_snapshots",
        source="""
using Cell = arc::StaticRef<&state, arc::Core::core1>;
using OtherCell = arc::StaticRef<&other, arc::Core::core1>;

void probe()
{
    (void)arc::snapshots<arc::Core::core0, Cell, OtherCell>();
}
""",
    ),
    Case(
        name="wrong_core_static_member_snapshots",
        source="""
using Cell = arc::StaticRef<&state, arc::Core::core1>;
using OtherCell = arc::StaticRef<&other, arc::Core::core1>;

void probe()
{
    (void)Cell::snapshots<arc::Core::core0, OtherCell>();
}
""",
    ),
    Case(
        name="scoped_borrow_returns_reference",
        source="""
using Cell = arc::StaticRef<&state, arc::Core::core1>;

void probe()
{
    (void)Cell::with_write([](State& current) -> State& {
        return current;
    });
}
""",
        must_contain="callback cannot return a reference or pointer",
    ),
    Case(
        name="scoped_borrow_returns_pointer",
        source="""
using Cell = arc::StaticRef<&state, arc::Core::core1>;
using OtherCell = arc::StaticRef<&other, arc::Core::core1>;

void probe()
{
    (void)arc::with_reads<Cell, OtherCell>([](const State& current, const State&) {
        return &current;
    });
}
""",
        must_contain="callback cannot return a reference or pointer",
    ),
    Case(
        name="scoped_borrow_returns_reference_wrapper",
        source="""
using Cell = arc::StaticRef<&state, arc::Core::core1>;

void probe()
{
    (void)Cell::with_write([](State& current) {
        return std::ref(current);
    });
}
""",
        must_contain="reference wrapper",
    ),
    Case(
        name="scoped_borrow_returns_loan",
        source="""
using Cell = arc::StaticRef<&state, arc::Core::core1>;

void probe()
{
    const auto read = Cell::read<arc::Core::core1>();
    (void)Cell::with_write([&](State&) {
        return read;
    });
}
""",
        must_contain="callback cannot return a reference or pointer or static loan",
    ),
    Case(
        name="scoped_borrow_pack_returns_loan",
        source="""
using Cell = arc::StaticRef<&state, arc::Core::core1>;
using OtherCell = arc::StaticRef<&other, arc::Core::core1>;

void probe()
{
    const auto read = OtherCell::read<arc::Core::core1>();
    (void)arc::with_reads<Cell, OtherCell>([&](const State&, const State&) {
        return read;
    });
}
""",
        must_contain="callback cannot return a reference or pointer or static loan",
    ),
    Case(
        name="scoped_member_reads_returns_pointer",
        source="""
using Cell = arc::StaticRef<&state, arc::Core::core1>;
using OtherCell = arc::StaticRef<&other, arc::Core::core1>;

void probe()
{
    (void)Cell::with_reads<OtherCell>([](const State& current, const State&) {
        return &current;
    });
}
""",
        must_contain="callback cannot return a reference or pointer",
    ),
    Case(
        name="scoped_member_edit_returns_reference",
        source="""
using Cell = arc::StaticRef<&state, arc::Core::core1>;
using OtherCell = arc::StaticRef<&other, arc::Core::core1>;

void probe()
{
    (void)Cell::with_edit<OtherCell>([](State& current, const State&) -> State& {
        return current;
    });
}
""",
        must_contain="callback cannot return a reference or pointer",
    ),
    Case(
        name="core_local_returns_reference",
        source="""
void probe()
{
    arc::CoreLocal<State, arc::Core::core1> local{};
    (void)local.with<arc::Core::core1>([](State& current) -> State& {
        return current;
    });
}
""",
        must_contain="callback cannot return a reference or pointer",
    ),
    Case(
        name="core_local_returns_reference_wrapper",
        source="""
void probe()
{
    arc::CoreLocal<State, arc::Core::core1> local{};
    (void)local.with<arc::Core::core1>([](State& current) {
        return std::ref(current);
    });
}
""",
        must_contain="reference wrapper",
    ),
    Case(
        name="wrong_core_local_snapshot",
        source="""
void probe()
{
    arc::CoreLocal<State, arc::Core::core1> local{};
    (void)local.snapshot<arc::Core::core0>();
}
""",
    ),
    Case(
        name="wrong_core_local_set",
        source="""
void probe()
{
    arc::CoreLocal<State, arc::Core::core1> local{};
    local.set<arc::Core::core0>(State{});
}
""",
    ),
    Case(
        name="proof_zero_subject_fact",
        source="""
using Bad = arc::proof::Deadline<0U, 100U>;
static_assert(Bad::bound == 100U);
""",
        must_contain="Fact needs a subject",
    ),
    Case(
        name="proof_zero_subject_query",
        source="""
using Claims = arc::proof::Pack<
    100U,
    arc::proof::Deadline<17U, 100U>>;
static_assert(Claims::has<arc::proof::Kind::deadline, 0U>());
""",
        must_contain="subject query needs a non-zero subject",
    ),
    Case(
        name="wrong_core_msg_snapshot",
        source="""
void probe()
{
    arc::CoreLocal<State, arc::Core::core1> local{};
    const auto msg = local.msg<arc::Core::core0>();
    (void)msg.snapshot<arc::Core::core1>();
}
""",
    ),
    Case(
        name="core_local_msg_any",
        source="""
using Local = arc::CoreLocal<State, arc::Core::core1>;
using Bad = Local::Msg<arc::Core::any>;
static_assert(Bad::to == arc::Core::any);
""",
        must_contain="destination must be a concrete core",
    ),
    Case(
        name="core_local_incoming_any",
        source="""
using Local = arc::CoreLocal<State, arc::Core::core1>;
using Bad = Local::Incoming<arc::Core::any>;
static_assert(Bad::from == arc::Core::any);
""",
        must_contain="source must be a concrete core",
    ),
    Case(
        name="wrong_core_msg_with",
        source="""
void probe()
{
    arc::CoreLocal<State, arc::Core::core1> local{};
    const auto msg = local.msg<arc::Core::core0>();
    msg.with<arc::Core::core1>([](const State&) {});
}
""",
    ),
    Case(
        name="core_msg_returns_reference",
        source="""
void probe()
{
    arc::CoreLocal<State, arc::Core::core1> local{};
    const auto msg = local.msg<arc::Core::core0>();
    (void)msg.with([](const State& current) -> const State& {
        return current;
    });
}
""",
        must_contain="callback cannot return a reference or pointer",
    ),
    Case(
        name="core_msg_returns_reference_wrapper",
        source="""
void probe()
{
    arc::CoreLocal<State, arc::Core::core1> local{};
    const auto msg = local.msg<arc::Core::core0>();
    (void)msg.with([](const State& current) {
        return std::cref(current);
    });
}
""",
        must_contain="reference wrapper",
    ),
    Case(
        name="core_local_returns_pointer",
        source="""
void probe()
{
    const arc::CoreLocal<State, arc::Core::core1> local{};
    (void)local.with<arc::Core::core1>([](const State& current) {
        return &current;
    });
}
""",
        must_contain="callback cannot return a reference or pointer",
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
    Case(
        name="scoped_role_returns_endpoint",
        source="""
void probe()
{
    arc::Roles<arc::Spsc<int, 4>> roles{};
    (void)roles.with_producer([](auto& producer) {
        return std::move(producer);
    });
}
""",
        must_contain="scoped role callback cannot return an endpoint",
    ),
    Case(
        name="scoped_role_returns_other_endpoint",
        source="""
void probe()
{
    arc::Roles<arc::Spsc<int, 4>> roles{};
    auto consumer = roles.consumer();
    (void)roles.with_producer([&](auto&) {
        return std::move(consumer);
    });
}
""",
        must_contain="scoped role callback cannot return an endpoint",
    ),
    Case(
        name="scoped_role_returns_reference_wrapper",
        source="""
void probe()
{
    arc::Roles<arc::Spsc<int, 4>> roles{};
    (void)roles.with_consumer([](auto& consumer) {
        return std::ref(consumer);
    });
}
""",
        must_contain="reference wrapper",
    ),
    Case(
        name="scoped_split_returns_endpoint",
        source="""
void probe()
{
    arc::Roles<arc::Spsc<int, 4>> roles{};
    (void)roles.with_split([](auto& producer, auto&) {
        return std::move(producer);
    });
}
""",
        must_contain="scoped role callback cannot return an endpoint",
    ),
    Case(
        name="scoped_rpc_returns_other_endpoint",
        source="""
enum class Op : std::uint8_t {
    set,
};

void probe()
{
    arc::Roles<arc::RpcLane<Op, State, State, 4>> roles{};
    auto server = roles.server();
    (void)roles.with_client([&](auto&) {
        return std::move(server);
    });
}
""",
        must_contain="scoped role callback cannot return an endpoint",
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
