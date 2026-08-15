#!/usr/bin/env python3
from __future__ import annotations

import argparse
import concurrent.futures
import json
import os
import subprocess
import sys
import tempfile
from dataclasses import dataclass
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
OUTPUT_FORMATS = ("text", "report", "json")

PREFIX = """
#include <cstdint>
#include <array>
#include <expected>
#include <functional>
#include <optional>
#include <span>
#include <string_view>
#include <type_traits>
#include <tuple>
#include <utility>
#include <variant>

#include "arc/borrow.hpp"
#include "arc/fanin.hpp"
#include "arc/flow.hpp"
#include "arc/mpsc.hpp"
#include "arc/proof.hpp"
#include "arc/result.hpp"
#include "arc/roles.hpp"
#include "arc/rpc.hpp"
#include "arc/spsc.hpp"

namespace {

struct State {
    std::uint32_t value{};
};

struct Text {
    char value[4]{'a', 'r', 'c', '\\0'};
};

constinit State state{};
constinit State other{};
constinit const State const_state{};
constinit Text text{};

}  // namespace
"""


@dataclass(frozen=True)
class Case:
    name: str
    source: str
    must_contain: str | None = None
    prelude: str = ""


@dataclass(frozen=True)
class CaseResult:
    name: str
    must_contain: str | None
    problem: str | None

    @property
    def passed(self) -> bool:
        return self.problem is None


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
        name="scoped_borrow_returns_span",
        source="""
using Cell = arc::StaticRef<&state, arc::Core::core1>;

void probe()
{
    (void)Cell::with_write([](State& current) {
        return std::span<State, 1>{&current, 1};
    });
}
""",
        must_contain="non-owning view",
    ),
    Case(
        name="scoped_borrow_returns_string_view",
        source="""
using TextCell = arc::StaticRef<&text, arc::Core::core1>;

void probe()
{
    (void)TextCell::with_write([](Text& current) {
        return std::string_view{current.value, 3};
    });
}
""",
        must_contain="non-owning view",
    ),
    Case(
        name="scoped_borrow_returns_tuple_reference",
        source="""
using Cell = arc::StaticRef<&state, arc::Core::core1>;

void probe()
{
    (void)Cell::with_write([](State& current) {
        return std::tuple<State&>{current};
    });
}
""",
        must_contain="standard wrapper",
    ),
    Case(
        name="scoped_borrow_returns_array_pointer",
        source="""
using Cell = arc::StaticRef<&state, arc::Core::core1>;

void probe()
{
    (void)Cell::with_write([](State& current) {
        return std::array<State*, 1>{&current};
    });
}
""",
        must_contain="standard wrapper",
    ),
    Case(
        name="scoped_borrow_returns_optional_pointer",
        source="""
using Cell = arc::StaticRef<&state, arc::Core::core1>;

void probe()
{
    (void)Cell::with_write([](State& current) {
        return std::optional<State*>{&current};
    });
}
""",
        must_contain="standard wrapper",
    ),
    Case(
        name="scoped_borrow_returns_variant_span",
        source="""
using Cell = arc::StaticRef<&state, arc::Core::core1>;

void probe()
{
    (void)Cell::with_write([](State& current) {
        return std::variant<std::span<State, 1>, std::uint32_t>{std::span<State, 1>{&current, 1}};
    });
}
""",
        must_contain="standard wrapper",
    ),
    Case(
        name="scoped_borrow_returns_result_pointer",
        source="""
using Cell = arc::StaticRef<&state, arc::Core::core1>;

void probe()
{
    (void)Cell::with_write([](State& current) {
        return arc::Result<State*>{&current};
    });
}
""",
        must_contain="standard wrapper",
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
        name="scoped_borrow_returns_optional_loan",
        source="""
using Cell = arc::StaticRef<&state, arc::Core::core1>;

void probe()
{
    const auto read = Cell::read<arc::Core::core1>();
    (void)Cell::with_write([&](State&) {
        return std::optional{read};
    });
}
""",
        must_contain="standard wrapper",
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
        name="core_local_returns_span",
        source="""
void probe()
{
    arc::CoreLocal<State, arc::Core::core1> local{};
    (void)local.with<arc::Core::core1>([](State& current) {
        return std::span<State, 1>{&current, 1};
    });
}
""",
        must_contain="non-owning view",
    ),
    Case(
        name="core_local_returns_pair_pointer",
        source="""
void probe()
{
    arc::CoreLocal<State, arc::Core::core1> local{};
    (void)local.with<arc::Core::core1>([](State& current) {
        return std::pair<State*, std::uint32_t>{&current, 1U};
    });
}
""",
        must_contain="standard wrapper",
    ),
    Case(
        name="core_local_returns_optional_pointer",
        source="""
void probe()
{
    arc::CoreLocal<State, arc::Core::core1> local{};
    (void)local.with<arc::Core::core1>([](State& current) {
        return std::optional<State*>{&current};
    });
}
""",
        must_contain="standard wrapper",
    ),
    Case(
        name="core_local_returns_expected_pointer",
        source="""
void probe()
{
    arc::CoreLocal<State, arc::Core::core1> local{};
    (void)local.with<arc::Core::core1>([](State& current) {
        return std::expected<State*, int>{&current};
    });
}
""",
        must_contain="standard wrapper",
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
        name="core_local_pointer_state",
        source="""
using Bad = arc::CoreLocal<State*, arc::Core::core1>;
static_assert(Bad::owner == arc::Core::core1);
""",
        must_contain="cannot carry borrowed storage directly",
    ),
    Case(
        name="core_msg_span_payload",
        source="""
using Bad = arc::CoreMsg<std::span<State, 1>, arc::Core::core1, arc::Core::core0>;
static_assert(Bad::from == arc::Core::core1);
""",
        must_contain="cannot carry borrowed storage directly",
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
        name="core_msg_returns_string_view",
        source="""
void probe()
{
    arc::CoreLocal<Text, arc::Core::core1> local{};
    const auto msg = local.msg<arc::Core::core0>();
    (void)msg.with([](const Text& current) {
        return std::string_view{current.value, 3};
    });
}
""",
        must_contain="non-owning view",
    ),
    Case(
        name="core_msg_returns_tuple_reference",
        source="""
void probe()
{
    arc::CoreLocal<State, arc::Core::core1> local{};
    const auto msg = local.msg<arc::Core::core0>();
    (void)msg.with([](const State& current) {
        return std::tuple<const State&>{current};
    });
}
""",
        must_contain="standard wrapper",
    ),
    Case(
        name="core_msg_returns_optional_reference_wrapper",
        source="""
void probe()
{
    arc::CoreLocal<State, arc::Core::core1> local{};
    const auto msg = local.msg<arc::Core::core0>();
    (void)msg.with([](const State& current) {
        return std::optional{std::cref(current)};
    });
}
""",
        must_contain="standard wrapper",
    ),
    Case(
        name="core_msg_returns_expected_string_view",
        source="""
void probe()
{
    arc::CoreLocal<Text, arc::Core::core1> local{};
    const auto msg = local.msg<arc::Core::core0>();
    (void)msg.with([](const Text& current) {
        return std::expected<std::string_view, int>{std::string_view{current.value, 3}};
    });
}
""",
        must_contain="standard wrapper",
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
        name="spsc_pointer_payload",
        source="""
using Bad = arc::Spsc<State*, 4>;
static_assert(Bad::bytes() > 0U);
""",
        must_contain="payload cannot carry borrowed storage directly",
    ),
    Case(
        name="mpsc_span_payload",
        source="""
using Bad = arc::Mpsc<std::span<State, 1>, 4>;
static_assert(Bad::cap() == 4U);
""",
        must_contain="payload cannot carry borrowed storage directly",
    ),
    Case(
        name="fanin_array_pointer_payload",
        source="""
using Bad = arc::Fanin<std::array<State*, 1>, 4, 2>;
static_assert(Bad::cap() == 4U);
""",
        must_contain="payload cannot carry borrowed storage directly",
    ),
    Case(
        name="rpc_request_pointer_payload",
        source="""
using Bad = arc::RpcLane<std::uint8_t, State*, State, 4>;
static_assert(sizeof(Bad) > 0U);
""",
        must_contain="payload cannot carry borrowed storage directly",
    ),
    Case(
        name="rpc_reply_string_view_payload",
        source="""
using Bad = arc::RpcLane<std::uint8_t, State, std::string_view, 4>;
static_assert(sizeof(Bad) > 0U);
""",
        must_contain="payload cannot carry borrowed storage directly",
    ),
    Case(
        name="flow_pointer_payload",
        source="""
struct PtrProducer {
    [[nodiscard]] bool try_push(State* const&) noexcept { return true; }
};

struct PtrConsumer {
    [[nodiscard]] bool try_pop(State*& out) noexcept
    {
        out = nullptr;
        return true;
    }
};

struct PtrLane {
    using value_type = State*;
    [[nodiscard]] PtrProducer producer() noexcept { return {}; }
    [[nodiscard]] PtrConsumer consumer() noexcept { return {}; }
};

struct PtrSource {
    using value_type = State*;
    static bool read(State*& out) noexcept
    {
        out = nullptr;
        return true;
    }
};

struct PtrSink {
    static bool write(State* const&) noexcept { return true; }
};

using Bad = arc::Flow<PtrSource, PtrLane, PtrSink>;
static_assert(sizeof(Bad) > 0U);
""",
        must_contain="payload cannot carry borrowed storage directly",
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
        name="scoped_role_returns_span",
        source="""
void probe()
{
    arc::Roles<arc::Spsc<int, 4>> roles{};
    (void)roles.with_consumer([](auto& consumer) {
        return std::span{&consumer, 1};
    });
}
""",
        must_contain="non-owning view",
    ),
    Case(
        name="scoped_role_returns_tuple_reference",
        source="""
void probe()
{
    arc::Roles<arc::Spsc<int, 4>> roles{};
    (void)roles.with_consumer([](auto& consumer) {
        return std::tuple<decltype(consumer)>{consumer};
    });
}
""",
        must_contain="standard wrapper",
    ),
    Case(
        name="scoped_role_returns_optional_reference_wrapper",
        source="""
void probe()
{
    arc::Roles<arc::Spsc<int, 4>> roles{};
    (void)roles.with_consumer([](auto& consumer) {
        return std::optional{std::ref(consumer)};
    });
}
""",
        must_contain="standard wrapper",
    ),
    Case(
        name="scoped_role_returns_optional_endpoint",
        source="""
void probe()
{
    arc::Roles<arc::Spsc<int, 4>> roles{};
    (void)roles.with_consumer([](auto& consumer) {
        return std::optional{std::move(consumer)};
    });
}
""",
        must_contain="standard wrapper",
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
        name="scoped_rpc_returns_expected_endpoint",
        source="""
enum class Op : std::uint8_t {
    set,
};

void probe()
{
    arc::Roles<arc::RpcLane<Op, State, State, 4>> roles{};
    (void)roles.with_client([](auto& client) {
        using Client = std::remove_reference_t<decltype(client)>;
        return std::expected<Client, int>{std::move(client)};
    });
}
""",
        must_contain="standard wrapper",
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
    Case(
        name="s31_drive_rejects_s3_dedicated_gpio",
        prelude="#define ARC_TARGET_ESP32S31 1\n",
        source="""
#include "arc/drive.hpp"

using Led = arc::Drive<2, 0>;
static_assert(sizeof(Led) > 0U);
""",
        must_contain="arc::Drive uses ESP32-S3 dedicated GPIO registers and is not implemented for ESP32-S31",
    ),
    Case(
        name="s31_sense_rejects_s3_dedicated_gpio",
        prelude="#define ARC_TARGET_ESP32S31 1\n",
        source="""
#include "arc/sense.hpp"

using Button = arc::Sense<42, 0>;
static_assert(sizeof(Button) > 0U);
""",
        must_contain="arc::Sense uses ESP32-S3 dedicated GPIO registers and is not implemented for ESP32-S31",
    ),
    Case(
        name="s31_trax_rejects_riscv_target",
        prelude="#define ARC_TARGET_ESP32S31 1\n",
        source="""
#include "arc/trax.hpp"

void probe()
{
    (void)arc::Trax::enable();
}
""",
        must_contain="arc::Trax is not available on ESP32-S31/RISC-V",
    ),
    Case(
        name="s31_mask_rejects_xtensa_interrupt_levels",
        prelude="#define ARC_TARGET_ESP32S31 1\n",
        source="""
#include "arc/mask.hpp"

using BadMask = arc::Mask<2>;
static_assert(sizeof(BadMask) > 0U);
""",
        must_contain="arc::Mask on RISC-V only supports global interrupt masking",
    ),
    Case(
        name="s31_raw_vector_rejects_xtensa_vectors",
        prelude="#define ARC_TARGET_ESP32S31 1\n",
        source="""
#include "arc/interrupt_matrix.hpp"

void handler() noexcept {}
using Vector = arc::RawVector<1, handler>;
static_assert(sizeof(Vector) > 0U);
""",
        must_contain="arc::RawVector is Xtensa-only",
    ),
    Case(
        name="s31_bare_core_rejects_unwired_true_amp",
        prelude="#define ARC_TARGET_ESP32S31 1\n",
        source="""
#include "arc/bare_core.hpp"

struct Program {
    static void loop() noexcept {}
};

using Amp = arc::BareCore<Program>;
static_assert(sizeof(Amp) > 0U);
""",
        must_contain="arc::BareCore true AMP is not wired for ESP32-S31",
    ),
    Case(
        name="s31_touch_bus_rejects_s3_capacitive_touch",
        prelude="#define ARC_TARGET_ESP32S31 1\n",
        source="""
#include "arc/touch.hpp"

using Bus = arc::TouchBus<>;
static_assert(sizeof(Bus) > 0U);
""",
        must_contain="arc::TouchBus is ESP32-S3 capacitive touch only",
    ),
    Case(
        name="s31_touch_rejects_s3_capacitive_touch",
        prelude="#define ARC_TARGET_ESP32S31 1\n",
        source="""
#include "arc/touch.hpp"

using Pad = arc::Touch<arc::TouchBus<>, 1>;
static_assert(sizeof(Pad) > 0U);
""",
        must_contain="arc::Touch is ESP32-S3 capacitive touch only",
    ),
)


def compiler() -> str:
    return os.environ.get("CXX", "c++")


def compile_pch(tmp: Path) -> dict[str, Path]:
    pch_map: dict[str, Path] = {}

    pch_header = tmp / "arc_prefix.hpp"
    pch_header.write_text(PREFIX, encoding="utf-8")
    pch_file = tmp / "arc_prefix.hpp.gch"
    cmd = [
        compiler(),
        "-std=gnu++23",
        "-x",
        "c++-header",
        "-Itests/host/stubs",
        "-Icomponents/arc/include",
        str(pch_header),
        "-o",
        str(pch_file),
    ]
    if subprocess.run(cmd, cwd=ROOT, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, check=False).returncode == 0:
        pch_map[""] = pch_header

    s31_prelude = "#define ARC_TARGET_ESP32S31 1\n"
    s31_header = tmp / "arc_prefix_s31.hpp"
    s31_header.write_text(s31_prelude + PREFIX, encoding="utf-8")
    s31_file = tmp / "arc_prefix_s31.hpp.gch"
    cmd_s31 = [
        compiler(),
        "-std=gnu++23",
        "-x",
        "c++-header",
        "-Itests/host/stubs",
        "-Icomponents/arc/include",
        str(s31_header),
        "-o",
        str(s31_file),
    ]
    if (
        subprocess.run(cmd_s31, cwd=ROOT, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, check=False).returncode
        == 0
    ):
        pch_map[s31_prelude] = s31_header

    return pch_map


def compile_case(case: Case, tmp: Path, pchs: dict[str, Path]) -> str | None:
    source = tmp / f"{case.name}.cpp"
    pch = pchs.get(case.prelude)
    if pch is not None:
        source.write_text(case.source, encoding="utf-8")
        cmd = [
            compiler(),
            "-std=gnu++23",
            "-fsyntax-only",
            "-Itests/host/stubs",
            "-Icomponents/arc/include",
            f"-I{tmp}",
            "-include",
            pch.name,
            str(source),
        ]
    else:
        source.write_text(case.prelude + PREFIX + "\n" + case.source, encoding="utf-8")
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


def run_cases(cases: tuple[Case, ...] = CASES, workers: int | None = None) -> list[CaseResult]:
    with tempfile.TemporaryDirectory(prefix="arc-compile-fail-") as tmp_dir:
        tmp = Path(tmp_dir)
        pchs = compile_pch(tmp)
        max_workers = workers if workers is not None else min(32, (os.cpu_count() or 1) + 4)
        with concurrent.futures.ThreadPoolExecutor(max_workers=max_workers) as executor:
            future_to_case = {executor.submit(compile_case, case, tmp, pchs): case for case in cases}
            results_by_case: dict[str, CaseResult] = {}
            for future in concurrent.futures.as_completed(future_to_case):
                case = future_to_case[future]
                problem = future.result()
                results_by_case[case.name] = CaseResult(name=case.name, must_contain=case.must_contain, problem=problem)
        return [results_by_case[case.name] for case in cases]


def case_group(name: str) -> str:
    if name.startswith(("wrong_core_static", "wrong_core_loan", "mixed_owner", "const_static", "owner_bound")):
        return "static_borrow_access"
    if name.startswith(("mutable_const", "conflicting_", "const_member", "move_mutable_loan")):
        return "static_borrow_alias"
    if name.startswith(("scoped_borrow", "scoped_member")):
        return "scoped_borrow_result"
    if name.startswith(("wrong_core_local", "core_local")):
        return "core_local"
    if name.startswith(("wrong_core_msg", "core_msg")):
        return "core_msg"
    if name.startswith(("spsc_", "mpsc_", "fanin_", "rpc_", "flow_")):
        return "payload_boundary"
    if name.startswith(("scoped_role", "scoped_split", "scoped_rpc")):
        return "role_scope"
    if name.startswith("proof_"):
        return "proof_metadata"
    return "other"


def group_summary(results: list[CaseResult]) -> dict[str, dict[str, int]]:
    groups: dict[str, dict[str, int]] = {}
    for result in results:
        group = groups.setdefault(case_group(result.name), {"cases": 0, "passed": 0, "failed": 0})
        group["cases"] += 1
        if result.passed:
            group["passed"] += 1
        else:
            group["failed"] += 1
    return groups


def print_report(results: list[CaseResult]) -> None:
    payload = report(results)
    summary = payload["summary"]
    print("arc compile-fail report")
    print(f"- compiler: {payload['compiler']}")
    print(f"- cases: {summary['cases']}")
    print(f"- passed: {summary['passed']}")
    print(f"- failed: {summary['failed']}")
    print("- groups:")
    for group, counts in sorted(summary["groups"].items()):
        print(f"  - {group}: {counts['passed']}/{counts['cases']} passed")
        names = [result.name for result in results if case_group(result.name) == group]
        print(f"    cases: {', '.join(names)}")


def report(results: list[CaseResult]) -> dict[str, object]:
    failed = [result for result in results if not result.passed]
    return {
        "ok": not failed,
        "compiler": compiler(),
        "summary": {
            "cases": len(results),
            "passed": len(results) - len(failed),
            "failed": len(failed),
            "groups": group_summary(results),
        },
        "cases": [
            {
                "name": result.name,
                "group": case_group(result.name),
                "must_contain": result.must_contain,
                "status": "failed_as_expected" if result.passed else "problem",
                **({} if result.problem is None else {"problem": result.problem}),
            }
            for result in results
        ],
    }


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description="Check Arc negative compile contracts")
    parser.add_argument(
        "--format",
        choices=OUTPUT_FORMATS,
        default="text",
        help="output style: text status, grouped human report, or JSON case summary",
    )
    args = parser.parse_args(argv)

    results = run_cases()
    problems = [result.problem for result in results if result.problem is not None]
    if args.format == "json":
        print(json.dumps(report(results), indent=2, sort_keys=True))
    elif args.format == "report":
        print_report(results)
    if problems:
        if args.format == "text":
            for problem in problems:
                print(f"arc compile-fail check failed: {problem}", file=sys.stderr)
        return 1

    if args.format == "text":
        print(f"arc compile-fail check: OK ({len(CASES)} cases)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
