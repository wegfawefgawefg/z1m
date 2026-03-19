#!/usr/bin/env python3

from __future__ import annotations

import argparse
from pathlib import Path

from PIL import Image, ImageDraw


REPO_ROOT = Path(__file__).resolve().parent.parent
WORKSPACE_ROOT = REPO_ROOT.parent
CONTENT_PATH = REPO_ROOT / "content" / "overworld_q1.txt"
OUTPUT_PATH = REPO_ROOT / "artifacts" / "generated_overworld_debug.png"
ROM_PATH = WORKSPACE_ROOT / "Legend of Zelda, The (USA) (Rev 1).nes"
SCREEN_COLUMNS = 16
SCREEN_ROWS = 8
SCREEN_TILE_WIDTH = 32
SCREEN_TILE_HEIGHT = 22
CHR_TILE_SIZE = 8
COMMON_BACKGROUND_OFFSET = 34703
COMMON_BACKGROUND_LENGTH = 1792
COMMON_MISC_OFFSET = 36495
COMMON_MISC_LENGTH = 224
OVERWORLD_BACKGROUND_OFFSET = 51531
OVERWORLD_BACKGROUND_LENGTH = 2080


def parse_screens() -> list[tuple[int, list[int], list[int]]]:
    tokens = CONTENT_PATH.read_text(encoding="utf-8").split()
    token_index = 0

    token_index += 2
    for _ in range(5):
        token_index += 2

    screens: list[tuple[int, list[int], list[int]]] = []
    for _ in range(SCREEN_COLUMNS * SCREEN_ROWS):
        room_id = int(tokens[token_index + 1], 16)
        token_index += 2
        attrs = [int(value, 16) for value in tokens[token_index + 1 : token_index + 7]]
        token_index += 7
        token_index += 2
        token_index += 1
        tiles = [int(value, 16) for value in tokens[token_index : token_index + 704]]
        token_index += 704
        token_index += 1
        screens.append((room_id, attrs, tiles))

    return screens


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--output",
        type=Path,
        default=OUTPUT_PATH,
    )
    parser.add_argument(
        "--room-id-offset",
        type=lambda value: int(value, 0),
        default=0,
    )
    parser.add_argument(
        "--wrap-room-ids",
        action="store_true",
    )
    return parser.parse_args()


def color_for_nes_index(index: int) -> tuple[int, int, int, int]:
    colors = {
        0x00: (92, 92, 92, 255),
        0x12: (0, 88, 248, 255),
        0x17: (200, 76, 12, 255),
        0x1A: (0, 168, 0, 255),
        0x30: (252, 252, 252, 255),
        0x37: (252, 188, 60, 255),
    }
    return colors.get(index, (252, 216, 168, 255))


def palette_for_selector(selector: int) -> list[tuple[int, int, int, int]]:
    backdrop = (252, 216, 168, 255)
    if selector == 0:
        return [
            backdrop,
            color_for_nes_index(0x30),
            color_for_nes_index(0x00),
            color_for_nes_index(0x12),
        ]
    if selector == 2:
        return [
            backdrop,
            color_for_nes_index(0x1A),
            color_for_nes_index(0x37),
            color_for_nes_index(0x12),
        ]
    return [
        backdrop,
        color_for_nes_index(0x17),
        color_for_nes_index(0x37),
        color_for_nes_index(0x12),
    ]


def get_screen_palette_selector(attrs: list[int], local_tile_x: int, local_tile_y: int) -> int:
    outer_selector = attrs[0] & 0x03
    inner_selector = attrs[1] & 0x03
    attr_x = local_tile_x // 4
    attr_y = local_tile_y // 4
    if attr_x <= 0 or attr_x >= 7:
        return outer_selector
    if 1 <= attr_y <= 3:
        return inner_selector
    if attr_y == 4 and (local_tile_y % 4) < 2:
        return inner_selector
    return outer_selector


def load_chr_tiles() -> list[list[int]]:
    rom = ROM_PATH.read_bytes()
    pattern_data = bytearray(256 * 16)
    pattern_data[0:COMMON_BACKGROUND_LENGTH] = rom[
        COMMON_BACKGROUND_OFFSET : COMMON_BACKGROUND_OFFSET + COMMON_BACKGROUND_LENGTH
    ]
    pattern_data[0x70 * 16 : 0x70 * 16 + OVERWORLD_BACKGROUND_LENGTH] = rom[
        OVERWORLD_BACKGROUND_OFFSET : OVERWORLD_BACKGROUND_OFFSET + OVERWORLD_BACKGROUND_LENGTH
    ]
    pattern_data[0xF2 * 16 : 0xF2 * 16 + COMMON_MISC_LENGTH] = rom[
        COMMON_MISC_OFFSET : COMMON_MISC_OFFSET + COMMON_MISC_LENGTH
    ]

    tiles: list[list[int]] = []
    for tile_index in range(256):
        tile_pixels = [0] * (CHR_TILE_SIZE * CHR_TILE_SIZE)
        base = tile_index * 16
        for row in range(CHR_TILE_SIZE):
            plane0 = pattern_data[base + row]
            plane1 = pattern_data[base + row + 8]
            for column in range(CHR_TILE_SIZE):
                shift = 7 - column
                value = ((plane0 >> shift) & 0x01) | (((plane1 >> shift) & 0x01) << 1)
                tile_pixels[row * CHR_TILE_SIZE + column] = value
        tiles.append(tile_pixels)
    return tiles


def get_source_screen(
    screens_by_room_id: dict[int, tuple[int, list[int], list[int]]],
    world_room_id: int,
    room_id_offset: int,
    wrap_room_ids: bool,
) -> tuple[int, list[int], list[int]] | None:
    source_room_id = world_room_id + room_id_offset
    if wrap_room_ids:
        source_room_id &= 0x7F

    return screens_by_room_id.get(source_room_id)


def main() -> None:
    args = parse_args()
    screens = parse_screens()
    screens_by_room_id = {room_id: (room_id, attrs, tiles) for room_id, attrs, tiles in screens}
    chr_tiles = load_chr_tiles()
    image = Image.new(
        "RGBA",
        (
            SCREEN_COLUMNS * SCREEN_TILE_WIDTH * CHR_TILE_SIZE,
            SCREEN_ROWS * SCREEN_TILE_HEIGHT * CHR_TILE_SIZE,
        ),
        (16, 24, 34, 255),
    )
    draw = ImageDraw.Draw(image)

    for world_room_id in range(SCREEN_COLUMNS * SCREEN_ROWS):
        screen = get_source_screen(
            screens_by_room_id,
            world_room_id,
            args.room_id_offset,
            args.wrap_room_ids,
        )
        if screen is None:
            continue

        _, attrs, tiles = screen
        room_x = world_room_id % SCREEN_COLUMNS
        room_y = world_room_id // SCREEN_COLUMNS
        origin_x = room_x * SCREEN_TILE_WIDTH * CHR_TILE_SIZE
        origin_y = room_y * SCREEN_TILE_HEIGHT * CHR_TILE_SIZE

        for tile_y in range(SCREEN_TILE_HEIGHT):
            for tile_x in range(SCREEN_TILE_WIDTH):
                tile = tiles[tile_y * SCREEN_TILE_WIDTH + tile_x]
                palette = palette_for_selector(get_screen_palette_selector(attrs, tile_x, tile_y))
                tile_pixels = chr_tiles[tile]
                left = origin_x + tile_x * CHR_TILE_SIZE
                top = origin_y + tile_y * CHR_TILE_SIZE

                for py in range(CHR_TILE_SIZE):
                    for px in range(CHR_TILE_SIZE):
                        color = palette[tile_pixels[py * CHR_TILE_SIZE + px]]
                        draw.rectangle(
                            [
                                left + px,
                                top + py,
                                left + px,
                                top + py,
                            ],
                            fill=color,
                        )

    for screen_x in range(SCREEN_COLUMNS + 1):
        x = screen_x * SCREEN_TILE_WIDTH * CHR_TILE_SIZE
        draw.line((x, 0, x, image.height), fill=(255, 255, 255, 96), width=1)

    for screen_y in range(SCREEN_ROWS + 1):
        y = screen_y * SCREEN_TILE_HEIGHT * CHR_TILE_SIZE
        draw.line((0, y, image.width, y), fill=(255, 255, 255, 96), width=1)

    args.output.parent.mkdir(parents=True, exist_ok=True)
    image.save(args.output)
    print(args.output)


if __name__ == "__main__":
    main()
