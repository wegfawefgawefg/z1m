#!/usr/bin/env python3

from __future__ import annotations

from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parent.parent
WORKSPACE_ROOT = REPO_ROOT.parent
ROM_PATH = WORKSPACE_ROOT / "Legend of Zelda, The (USA) (Rev 1).nes"
Z05_PATH = WORKSPACE_ROOT / "zelda1-disassembly" / "src" / "Z_05.asm"
OUTPUT_PATH = REPO_ROOT / "content" / "overworld_q1.txt"

ROOM_LAYOUTS_OFFSET = 87064
ROOM_LAYOUTS_LENGTH = 1936
LEVEL_BLOCK_OFFSET = 99328
LEVEL_BLOCK_LENGTH = 768

SCREEN_COUNT = 128
SCREEN_COLUMNS = 16
SCREEN_ROWS = 8
UNIQUE_ROOM_COLUMN_COUNT = 16
SQUARE_ROWS_PER_SCREEN = 11
SCREEN_TILE_WIDTH = 32
SCREEN_TILE_HEIGHT = 22


def parse_byte_blocks(source: str) -> dict[str, list[int]]:
    blocks: dict[str, list[int]] = {}
    current_label: str | None = None

    for raw_line in source.splitlines():
        line = raw_line.split(";", 1)[0].strip()
        if not line:
            continue

        if line.endswith(":"):
            current_label = line[:-1]
            continue

        if not line.startswith(".BYTE") or current_label is None:
            continue

        values_text = line.removeprefix(".BYTE").strip()
        values = [int(token.strip().removeprefix("$"), 16) for token in values_text.split(",")]
        blocks.setdefault(current_label, []).extend(values)

    return blocks


def build_square_tiles(square_index: int, primary_squares: list[int], secondary_squares: list[int]) -> list[int]:
    if square_index >= 0x10:
        return [
            primary_squares[square_index],
            square_index + 1,
            square_index + 2,
            square_index + 3,
        ]

    base_index = square_index * 4
    return secondary_squares[base_index : base_index + 4]


def expand_screen(
    room_id: int,
    room_layouts: bytes,
    attrs_a: bytes,
    attrs_b: bytes,
    attrs_c: bytes,
    attrs_d: bytes,
    attrs_e: bytes,
    attrs_f: bytes,
    primary_squares: list[int],
    secondary_squares: list[int],
    column_heap_data: list[int],
    column_heap_offsets: list[int],
) -> dict[str, object]:
    unique_room_id = attrs_d[room_id] & 0x3F
    room_layout_offset = unique_room_id * UNIQUE_ROOM_COLUMN_COUNT
    room_layout = room_layouts[room_layout_offset : room_layout_offset + UNIQUE_ROOM_COLUMN_COUNT]

    tiles = [0x24] * (SCREEN_TILE_WIDTH * SCREEN_TILE_HEIGHT)

    for column_index, descriptor in enumerate(room_layout):
        heap_index = descriptor >> 4
        heap_column_index = descriptor & 0x0F
        search_index = -1
        square_pointer = column_heap_offsets[heap_index]

        while square_pointer < len(column_heap_data):
            if column_heap_data[square_pointer] & 0x80:
                search_index += 1
                if search_index == heap_column_index:
                    break
            square_pointer += 1

        if square_pointer >= len(column_heap_data):
            continue

        repeat_state = 0

        for square_row in range(SQUARE_ROWS_PER_SCREEN):
            if square_pointer >= len(column_heap_data):
                break

            square_descriptor = column_heap_data[square_pointer]
            square_index = square_descriptor & 0x3F
            tile_values = build_square_tiles(square_index, primary_squares, secondary_squares)

            tile_x = column_index * 2
            tile_y = square_row * 2
            tiles[tile_y * SCREEN_TILE_WIDTH + tile_x] = tile_values[0]
            tiles[(tile_y + 1) * SCREEN_TILE_WIDTH + tile_x] = tile_values[1]
            tiles[tile_y * SCREEN_TILE_WIDTH + tile_x + 1] = tile_values[2]
            tiles[(tile_y + 1) * SCREEN_TILE_WIDTH + tile_x + 1] = tile_values[3]

            if square_descriptor & 0x40:
                repeat_state ^= 0x40
                if repeat_state == 0:
                    square_pointer += 1
            else:
                square_pointer += 1

    return {
        "room_id": room_id,
        "unique_room_id": unique_room_id,
        "attrs": [
            attrs_a[room_id],
            attrs_b[room_id],
            attrs_c[room_id],
            attrs_d[room_id],
            attrs_e[room_id],
            attrs_f[room_id],
        ],
        "tiles": tiles,
    }


def write_text_file(screens: list[dict[str, object]]) -> None:
    with OUTPUT_PATH.open("w", encoding="utf-8") as output:
        output.write("z1m_overworld_q1 1\n")
        output.write(f"screen_count {SCREEN_COUNT}\n")
        output.write(f"screen_columns {SCREEN_COLUMNS}\n")
        output.write(f"screen_rows {SCREEN_ROWS}\n")
        output.write(f"screen_tile_width {SCREEN_TILE_WIDTH}\n")
        output.write(f"screen_tile_height {SCREEN_TILE_HEIGHT}\n\n")

        for screen in screens:
            room_id = int(screen["room_id"])
            unique_room_id = int(screen["unique_room_id"])
            attrs = list(screen["attrs"])
            tiles = list(screen["tiles"])

            output.write(f"screen {room_id:02x}\n")
            output.write("attrs " + " ".join(f"{value:02x}" for value in attrs) + "\n")
            output.write(f"unique_room {unique_room_id:02x}\n")
            output.write("tiles\n")

            for row in range(SCREEN_TILE_HEIGHT):
                start = row * SCREEN_TILE_WIDTH
                end = start + SCREEN_TILE_WIDTH
                output.write(" ".join(f"{value:02x}" for value in tiles[start:end]) + "\n")

            output.write("end\n\n")


def main() -> None:
    rom = ROM_PATH.read_bytes()
    z05_source = Z05_PATH.read_text(encoding="utf-8")
    blocks = parse_byte_blocks(z05_source)

    room_layouts = rom[ROOM_LAYOUTS_OFFSET : ROOM_LAYOUTS_OFFSET + ROOM_LAYOUTS_LENGTH]
    level_block = rom[LEVEL_BLOCK_OFFSET : LEVEL_BLOCK_OFFSET + LEVEL_BLOCK_LENGTH]

    attrs_a = level_block[0:128]
    attrs_b = level_block[128:256]
    attrs_c = level_block[256:384]
    attrs_d = level_block[384:512]
    attrs_e = level_block[512:640]
    attrs_f = level_block[640:768]

    primary_squares = blocks["PrimarySquaresOW"]
    secondary_squares = blocks["SecondarySquaresOW"]
    column_heap_labels = [f"ColumnHeapOW{value:X}" for value in range(16)]
    column_heap_offsets: list[int] = []
    column_heap_data: list[int] = []
    for label in column_heap_labels:
        column_heap_offsets.append(len(column_heap_data))
        column_heap_data.extend(blocks[label])

    screens = [
        expand_screen(
            room_id,
            room_layouts,
            attrs_a,
            attrs_b,
            attrs_c,
            attrs_d,
            attrs_e,
            attrs_f,
            primary_squares,
            secondary_squares,
            column_heap_data,
            column_heap_offsets,
        )
        for room_id in range(SCREEN_COUNT)
    ]

    write_text_file(screens)


if __name__ == "__main__":
    main()
