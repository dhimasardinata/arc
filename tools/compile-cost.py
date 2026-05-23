#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Any


@dataclass(frozen=True)
class CostKey:
    category: str
    name: str
    detail: str


@dataclass
class CostRow:
    key: CostKey
    total_us: int = 0
    count: int = 0

    @property
    def ms(self) -> float:
        return self.total_us / 1000.0


def trace_events(doc: object) -> list[dict[str, Any]]:
    if not isinstance(doc, dict):
        return []
    events = doc.get("traceEvents", [])
    if not isinstance(events, list):
        return []
    return [event for event in events if isinstance(event, dict)]


def load_trace_events(path: Path) -> list[dict[str, Any]]:
    try:
        doc = json.loads(path.read_text(encoding="utf-8"))
    except OSError as exc:
        raise ValueError(f"{path}: {exc}") from exc
    except json.JSONDecodeError as exc:
        raise ValueError(f"{path}: invalid JSON at line {exc.lineno}") from exc
    return trace_events(doc)


def event_us(event: dict[str, Any]) -> int:
    if event.get("ph") not in {None, "X"}:
        return 0
    duration = event.get("dur", 0)
    if not isinstance(duration, int | float) or duration <= 0:
        return 0
    return int(duration)


def event_key(event: dict[str, Any]) -> CostKey:
    args = event.get("args", {})
    detail = ""
    if isinstance(args, dict):
        raw = args.get("detail", "")
        if raw is not None:
            detail = str(raw)
    return CostKey(
        category=str(event.get("cat", "")),
        name=str(event.get("name", "")),
        detail=detail,
    )


def aggregate_events(
    events: list[dict[str, Any]],
    *,
    categories: set[str] | None = None,
    min_us: int = 0,
) -> tuple[list[CostRow], int, int]:
    rows: dict[CostKey, CostRow] = {}
    total_us = 0
    matched = 0

    for event in events:
        duration = event_us(event)
        if duration <= 0:
            continue
        key = event_key(event)
        if categories and key.category not in categories:
            continue
        if duration < min_us:
            continue
        row = rows.setdefault(key, CostRow(key=key))
        row.total_us += duration
        row.count += 1
        total_us += duration
        matched += 1

    ranked = sorted(
        rows.values(),
        key=lambda row: (-row.total_us, -row.count, row.key.category, row.key.name, row.key.detail),
    )
    return ranked, total_us, matched


def shorten(text: str, limit: int) -> str:
    if len(text) <= limit:
        return text
    if limit <= 1:
        return text[:limit]
    return f"{text[: limit - 1]}~"


def render(rows: list[CostRow], *, traces: int, events: int, total_us: int, top: int) -> str:
    out = [
        f"arc compile cost: {traces} trace(s), {events} timed event(s), {total_us / 1000.0:.3f} ms",
        f"{'ms':>10} {'count':>5} {'category':<18} {'name':<28} detail",
    ]
    for row in rows[:top]:
        out.append(
            f"{row.ms:10.3f} {row.count:5d} "
            f"{shorten(row.key.category, 18):<18} {shorten(row.key.name, 28):<28} {row.key.detail}"
        )
    return "\n".join(out)


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Rank Clang -ftime-trace JSON events by aggregate duration.",
    )
    parser.add_argument("trace", nargs="+", type=Path, help="trace JSON files emitted by Clang -ftime-trace")
    parser.add_argument("--top", type=int, default=20, help="maximum rows to print")
    parser.add_argument("--min-ms", type=float, default=0.0, help="drop individual events below this duration")
    parser.add_argument("--category", action="append", default=[], help="keep only an exact trace category")
    return parser


def main(argv: list[str] | None = None) -> int:
    parser = build_parser()
    args = parser.parse_args(argv)
    if args.top <= 0:
        parser.error("--top must be positive")
    if args.min_ms < 0:
        parser.error("--min-ms must be non-negative")

    events: list[dict[str, Any]] = []
    for trace in args.trace:
        try:
            events.extend(load_trace_events(trace))
        except ValueError as exc:
            print(exc, file=sys.stderr)
            return 1

    categories = set(args.category) if args.category else None
    rows, total_us, matched = aggregate_events(
        events,
        categories=categories,
        min_us=int(args.min_ms * 1000),
    )
    print(render(rows, traces=len(args.trace), events=matched, total_us=total_us, top=args.top))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
