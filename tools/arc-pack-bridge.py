#!/usr/bin/env python3

import argparse
import json
import struct
import sys
from dataclasses import dataclass


TYPES = {
    "u8": ("B", 1),
    "i8": ("b", 1),
    "u16": ("H", 2),
    "i16": ("h", 2),
    "u32": ("I", 4),
    "i32": ("i", 4),
    "u64": ("Q", 8),
    "i64": ("q", 8),
    "f32": ("f", 4),
    "f64": ("d", 8),
}


@dataclass(frozen=True)
class Field:
    kind: str
    name: str

    @property
    def size(self) -> int:
        return TYPES[self.kind][1]

    @property
    def fmt(self) -> str:
        return TYPES[self.kind][0]


def parse_schema(schema: str) -> list[Field]:
    fields: list[Field] = []
    for item in schema.split(","):
        spec = item.strip()
        if not spec:
            continue
        if ":" not in spec:
            raise SystemExit(f"schema field lacks name: {spec}")
        kind, name = spec.split(":", 1)
        if kind not in TYPES:
            raise SystemExit(f"unsupported field type: {kind}")
        fields.append(Field(kind, name))
    if not fields:
        raise SystemExit("schema must contain at least one field")
    return fields


def decode_frame(fields: list[Field], endian: str, frame: bytes) -> dict[str, object]:
    prefix = ">" if endian == "big" else "<"
    out: dict[str, object] = {}
    pos = 0
    for field in fields:
        value = struct.unpack_from(prefix + field.fmt, frame, pos)[0]
        out[field.name] = value
        pos += field.size
    return out


def main() -> int:
    parser = argparse.ArgumentParser(description="Decode fixed arc::pack frames from stdin.")
    parser.add_argument("--schema", required=True, help="Comma list like u32:seq,i16:temp")
    parser.add_argument("--endian", choices=("big", "little"), default="big")
    parser.add_argument("--topic", default="/arc/telemetry")
    parser.add_argument(
        "--foxglove-jsonl", action="store_true", help="Wrap each decoded frame in a Foxglove-style JSONL envelope"
    )
    args = parser.parse_args()

    fields = parse_schema(args.schema)
    frame_size = sum(field.size for field in fields)
    payload = sys.stdin.buffer.read()
    if len(payload) % frame_size != 0:
        raise SystemExit(f"input length {len(payload)} is not a multiple of frame size {frame_size}")

    for index in range(0, len(payload), frame_size):
        decoded = decode_frame(fields, args.endian, payload[index : index + frame_size])
        if args.foxglove_jsonl:
            decoded = {
                "topic": args.topic,
                "receive_time": index // frame_size,
                "message": decoded,
            }
        print(json.dumps(decoded, separators=(",", ":")))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
