#!/usr/bin/env python3

from __future__ import annotations

import json
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable

from PIL import Image


SCRIPT_DIR = Path(__file__).resolve().parent
PROJECT_DIR = SCRIPT_DIR.parent
ROM_PATH = PROJECT_DIR.parent / "Legend of Zelda, The (USA) (Rev 1).nes"
ASSET_ROOT = PROJECT_DIR / "assets" / "sprites"

TILE_BYTES = 16
TILE_COUNT = 256
COMMON_SPRITE_OFFSET = 32911
COMMON_SPRITE_LENGTH = 1792
OVERWORLD_SPRITE_OFFSET = 53611
OVERWORLD_SPRITE_LENGTH = 1824
OVERWORLD_SPRITE_START = 0x8E * TILE_BYTES
UNDERWORLD_COMMON_OFFSET = 56523
UNDERWORLD_COMMON_LENGTH = 256
UNDERWORLD_COMMON_START = 0x8E * TILE_BYTES
UNDERWORLD_SPECIAL_START = 0x9E * TILE_BYTES
UNDERWORLD_GROUP_127_OFFSET = 56779
UNDERWORLD_GROUP_127_LENGTH = 544
UNDERWORLD_GROUP_358_OFFSET = 55435
UNDERWORLD_GROUP_358_LENGTH = 544

OBJ_ANIMATIONS = [
    0x00, 0x08, 0x0B, 0x0F, 0x13, 0x17, 0x5C, 0x60, 0x1B, 0x1B, 0x21, 0x21, 0x64, 0x6A,
    0x27, 0x29, 0x2B, 0x35, 0x3F, 0x70, 0x74, 0x76, 0x76, 0x78, 0x7A, 0x7E, 0x80, 0x49,
    0x82, 0x84, 0x86, 0x4B, 0x4F, 0x4F, 0x51, 0x51, 0x88, 0x8C, 0x90, 0x90, 0x92, 0x94,
    0x96, 0x98, 0x99, 0x99, 0x99, 0x53, 0x54, 0x9A, 0x9B, 0x9B, 0xA5, 0xA5, 0xAB, 0xAB,
    0xAC, 0xAE, 0xAE, 0xAF, 0xAF, 0xB2, 0xB8, 0xB8, 0x08, 0x08, 0xC6, 0xC6, 0xC6, 0xC6,
    0xC6, 0xC6, 0xC8, 0xC8, 0xC9, 0xC9, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA,
    0x09, 0x09, 0x0A, 0x0A, 0x0B, 0x0B, 0x0B, 0x0B, 0x0B, 0x0B, 0xCB, 0x55, 0x55, 0x55,
    0x55, 0x55, 0x55, 0x55, 0x56, 0x57, 0x57, 0xCB, 0xCC, 0x58, 0x58, 0x58, 0x58, 0x58,
    0x58, 0x58, 0x58, 0x58, 0x59, 0x59, 0x59, 0x59, 0x5A, 0x5A, 0x5A, 0x5A, 0x5B, 0x5B,
    0x5B,
]

OBJ_ANIM_FRAME_HEAP = [
    0x00, 0x04, 0x08, 0x0C, 0x10, 0x10, 0x14, 0x18, 0x5C, 0x9E, 0x44, 0xCE, 0xD2, 0xD6,
    0xDA, 0xCE, 0xD2, 0xD6, 0xDA, 0xF0, 0xF4, 0xF8, 0xFC, 0xF0, 0xF4, 0xF8, 0xFC, 0xB4,
    0xB0, 0xB0, 0xB8, 0xB2, 0xB2, 0xB4, 0xB0, 0xB0, 0xB8, 0xB2, 0xB2, 0xCA, 0xCC, 0xCA,
    0xCC, 0xBC, 0xBE, 0xC0, 0xC0, 0xC2, 0xC4, 0xC0, 0xC0, 0xBC, 0xBE, 0xBC, 0xBE, 0xC0,
    0xC0, 0xC2, 0xC4, 0xC0, 0xC0, 0xBC, 0xBE, 0xBC, 0xBE, 0xEC, 0xEE, 0xEC, 0xEE, 0xEC,
    0xEE, 0xBC, 0xBE, 0xC6, 0xC8, 0xA0, 0xA8, 0xA4, 0xAC, 0x90, 0xE8, 0xE4, 0xE0, 0x94,
    0xF3, 0xC9, 0xBD, 0xC1, 0x98, 0x9A, 0x9C, 0xF8, 0xB8, 0xBC, 0xB0, 0xB4, 0xB8, 0xBC,
    0xB0, 0xB4, 0xB8, 0xAC, 0xB4, 0xBC, 0xB0, 0xB4, 0xB8, 0xAC, 0xB4, 0xBC, 0xB0, 0xB4,
    0xAC, 0xAE, 0xB0, 0xB2, 0xA8, 0xAA, 0x92, 0x94, 0xA0, 0xA2, 0xA6, 0xA4, 0xA2, 0xA4,
    0xD8, 0xDA, 0x00, 0x00, 0x9A, 0x9C, 0x9A, 0x9C, 0x9A, 0x9C, 0xB4, 0xB8, 0xBC, 0xBE,
    0xB4, 0xB8, 0xBC, 0xBE, 0xFC, 0xFE, 0xAC, 0x9C, 0xA0, 0xA4, 0xA0, 0xA4, 0xA8, 0x8E,
    0xA4, 0xDC, 0xE0, 0xE4, 0xE8, 0xEC, 0xF0, 0xF4, 0xF8, 0xFA, 0xFE, 0xF4, 0xF6, 0xFE,
    0xFC, 0xF0, 0xF8, 0xB0, 0xF6, 0xF0, 0xD4, 0xFC, 0xFE, 0xF8, 0xE8, 0xEA, 0xE0, 0xE4,
    0xEC, 0xEC, 0xD0, 0xD4, 0xD8, 0xDC, 0xE0, 0xE4, 0xC0, 0xC8, 0xC4, 0xCC, 0xE8, 0xEA,
    0x72, 0x74, 0xDE, 0xEE, 0xF8, 0x96, 0x98, 0xB1,
]

OBJ_ANIM_ATTR_HEAP = [
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x01, 0x01, 0x01,
    0x01, 0x02, 0x02, 0x02, 0x02, 0x03, 0x03, 0x03, 0x03, 0x02, 0x02, 0x02, 0x02, 0x02,
    0x82, 0x02, 0x02, 0x82, 0x02, 0x01, 0x81, 0x01, 0x01, 0x81, 0x01, 0x01, 0x01, 0x02,
    0x02, 0x02, 0x02, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 0x02, 0x02, 0x02, 0x02,
    0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03,
    0x03, 0x03, 0x03, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x01, 0x01, 0x01,
    0x02, 0x03, 0x03, 0x03, 0x02, 0x02, 0x00, 0x02, 0x01, 0x01, 0x01, 0x01, 0x02, 0x02,
    0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x01, 0x01, 0x01, 0x01, 0x03, 0x03, 0x03, 0x03, 0x00, 0x00, 0x02, 0x02, 0x02, 0x02,
    0x03, 0x03, 0x03, 0x03, 0x01, 0x01, 0x02, 0x02, 0x03, 0x03, 0x01, 0x01, 0x01, 0x01,
    0x02, 0x02, 0x02, 0x02, 0x01, 0x01, 0x01, 0x01, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,
    0x01, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x01, 0x01, 0x01,
    0x01, 0x01, 0x01, 0x02, 0x00, 0x00, 0x03, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x01, 0x01, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03,
    0x03, 0x03, 0x02, 0x02, 0x01, 0x01, 0x02, 0x03,
]

NES_COLORS = {
    0x07: (60, 24, 0, 255),
    0x0F: (0, 0, 0, 0),
    0x15: (172, 124, 0, 255),
    0x22: (80, 160, 255, 255),
    0x26: (216, 120, 0, 255),
    0x27: (252, 160, 68, 255),
    0x29: (0, 200, 88, 255),
    0x30: (252, 252, 252, 255),
}

SPRITE_PALETTES = {
    "green": [0x0F, 0x29, 0x27, 0x07],
    "blue": [0x0F, 0x22, 0x27, 0x07],
    "red": [0x0F, 0x26, 0x27, 0x07],
    "link": [0x0F, 0x15, 0x27, 0x30],
}

LINK_HEAD_TILES = [0x02, 0x06, 0x08, 0x0A]
LINK_HEAD_MAGIC_SHIELD_TILES = [0x80, 0x54, 0x60, 0x60]


@dataclass(frozen=True)
class ClipMetadata:
    world_width: float
    world_height: float
    offset_x: float
    offset_y: float
    aabb_width: float
    aabb_height: float
    frame_ms: int = 120


PLAYER_METADATA = ClipMetadata(1.0, 1.0, 0.0, -0.04, 0.65, 0.75)
ENEMY_METADATA = ClipMetadata(0.8, 0.8, 0.0, 0.0, 0.65, 0.65)


def color(index: int) -> tuple[int, int, int, int]:
    return NES_COLORS.get(index, (252, 216, 168, 255))


def load_overworld_sprite_pattern_data(rom_path: Path) -> bytes:
    rom = rom_path.read_bytes()
    pattern_data = bytearray(TILE_COUNT * TILE_BYTES)
    pattern_data[:COMMON_SPRITE_LENGTH] = rom[COMMON_SPRITE_OFFSET:COMMON_SPRITE_OFFSET + COMMON_SPRITE_LENGTH]
    pattern_data[OVERWORLD_SPRITE_START:OVERWORLD_SPRITE_START + OVERWORLD_SPRITE_LENGTH] = (
        rom[OVERWORLD_SPRITE_OFFSET:OVERWORLD_SPRITE_OFFSET + OVERWORLD_SPRITE_LENGTH]
    )
    return bytes(pattern_data)


def load_underworld_sprite_pattern_data(rom_path: Path, special_offset: int, special_length: int) -> bytes:
    rom = rom_path.read_bytes()
    pattern_data = bytearray(TILE_COUNT * TILE_BYTES)
    pattern_data[:COMMON_SPRITE_LENGTH] = rom[COMMON_SPRITE_OFFSET:COMMON_SPRITE_OFFSET + COMMON_SPRITE_LENGTH]
    pattern_data[UNDERWORLD_COMMON_START:UNDERWORLD_COMMON_START + UNDERWORLD_COMMON_LENGTH] = (
        rom[UNDERWORLD_COMMON_OFFSET:UNDERWORLD_COMMON_OFFSET + UNDERWORLD_COMMON_LENGTH]
    )
    pattern_data[UNDERWORLD_SPECIAL_START:UNDERWORLD_SPECIAL_START + special_length] = (
        rom[special_offset:special_offset + special_length]
    )
    return bytes(pattern_data)


def lookup_animation_frame(animation_index: int, frame_index: int) -> tuple[int, int]:
    heap_index = OBJ_ANIMATIONS[animation_index] + frame_index
    return OBJ_ANIM_FRAME_HEAP[heap_index], OBJ_ANIM_ATTR_HEAP[heap_index]


def lookup_object_frame(object_type: int, frame_index: int) -> tuple[int, int]:
    animation_index = object_type + 1
    return lookup_animation_frame(animation_index, frame_index)


def render_sprite_cell(pattern_data: bytes, tile: int, palette_name: str, attributes: int) -> Image.Image:
    image = Image.new("RGBA", (8, 16), (0, 0, 0, 0))
    pixels = image.load()
    palette = SPRITE_PALETTES[palette_name]
    tile_index = tile & 0xFE
    for half in range(2):
        tile_offset = (tile_index + half) * TILE_BYTES
        for row in range(8):
            plane0 = pattern_data[tile_offset + row]
            plane1 = pattern_data[tile_offset + row + 8]
            for column in range(8):
                shift = 7 - column
                color_index = ((plane0 >> shift) & 1) | (((plane1 >> shift) & 1) << 1)
                rgba = color(palette[color_index])
                pixels[column, half * 8 + row] = rgba

    if attributes & 0x40:
        image = image.transpose(Image.FLIP_LEFT_RIGHT)
    if attributes & 0x80:
        image = image.transpose(Image.FLIP_TOP_BOTTOM)
    return image


def build_flippable_pair(left_tile: int, attributes: int, flip_h: bool) -> tuple[tuple[int, int], tuple[int, int]]:
    left = [left_tile, attributes]
    right = [left_tile + 2, attributes]
    if flip_h:
        left, right = right, left
        left[1] ^= 0x40
        right[1] ^= 0x40
    return (left[0], left[1]), (right[0], right[1])


def build_mirrored_pair(left_tile: int, attributes: int) -> tuple[tuple[int, int], tuple[int, int]]:
    return (left_tile, attributes), (left_tile, attributes ^ 0x40)


def compose_pair_image(
    pattern_data: bytes,
    palette_name: str,
    left: tuple[int, int],
    right: tuple[int, int],
) -> Image.Image:
    image = Image.new("RGBA", (16, 16), (0, 0, 0, 0))
    image.paste(render_sprite_cell(pattern_data, left[0], palette_name, left[1]), (0, 0))
    image.paste(render_sprite_cell(pattern_data, right[0], palette_name, right[1]), (8, 0))
    return image


def generic_walker_frame(direction: str, walk_frame: int) -> tuple[int, bool]:
    if direction == "left":
        return walk_frame, True
    if direction == "right":
        return walk_frame, False
    if direction == "down":
        return 2, walk_frame == 1
    return 3, walk_frame == 1


def octorok_frame(direction: str, walk_frame: int) -> tuple[int, bool, bool]:
    if direction == "left":
        return (0, False, False) if walk_frame == 0 else (3, False, False)
    if direction == "right":
        return (0, True, False) if walk_frame == 0 else (3, True, False)
    if direction == "up":
        return (1, False, True) if walk_frame == 0 else (4, False, True)
    return (2, False, True) if walk_frame == 0 else (5, False, True)


def darknut_frame(direction: str, walk_frame: int) -> tuple[int, bool]:
    if direction == "left":
        return (0, True) if walk_frame == 0 else (3, True)
    if direction == "right":
        return (0, False) if walk_frame == 0 else (3, False)
    if direction == "down":
        return (1, False) if walk_frame == 0 else (4, False)
    if walk_frame == 0:
        return 2, False
    return 5, True


def patch_link_pair(direction: str, shielded: bool, left: list[int], right: list[int]) -> None:
    if shielded:
        target = right if direction == "right" else left
        if target[0] in LINK_HEAD_TILES:
            replacement = LINK_HEAD_MAGIC_SHIELD_TILES[LINK_HEAD_TILES.index(target[0])]
            original = target[0]
            target[0] = replacement
            if original == 0x0A:
                target[1] &= 0x0F
        return

    if direction == "down" and left[0] < 0x0B:
        original = left[0]
        left[0] += 0x50
        if original == 0x0A:
            left[1] &= 0x0F


def save_clip(path_stem: str, frames: Iterable[Image.Image], metadata: ClipMetadata) -> None:
    frame_list = list(frames)
    if not frame_list:
        raise ValueError(f"clip {path_stem} has no frames")

    width = frame_list[0].width
    height = frame_list[0].height
    strip = Image.new("RGBA", (width * len(frame_list), height), (0, 0, 0, 0))
    for index, frame in enumerate(frame_list):
        strip.paste(frame, (index * width, 0))

    png_path = ASSET_ROOT / f"{path_stem}.png"
    json_path = ASSET_ROOT / f"{path_stem}.json"
    png_path.parent.mkdir(parents=True, exist_ok=True)
    strip.save(png_path)
    json_path.write_text(
        json.dumps(
            {
                "frame_width": width,
                "frame_height": height,
                "frame_count": len(frame_list),
                "frame_ms": metadata.frame_ms,
                "world_width": metadata.world_width,
                "world_height": metadata.world_height,
                "offset_x": metadata.offset_x,
                "offset_y": metadata.offset_y,
                "aabb_width": metadata.aabb_width,
                "aabb_height": metadata.aabb_height,
            },
            indent=2,
        )
        + "\n"
    )


def export_link_clips(pattern_data: bytes) -> None:
    for shielded, prefix in ((False, "shield"), (True, "magic_shield")):
        for direction in ("down", "up", "left", "right"):
            frames = []
            for walk_frame in (0, 1):
                frame_index, flip_h = generic_walker_frame(direction, walk_frame)
                left_tile, attributes = lookup_animation_frame(0, frame_index)
                left, right = build_flippable_pair(left_tile, attributes, flip_h)
                left_list = [left[0], left[1]]
                right_list = [right[0], right[1]]
                patch_link_pair(direction, shielded, left_list, right_list)
                frames.append(
                    compose_pair_image(
                        pattern_data,
                        "link",
                        (left_list[0], left_list[1]),
                        (right_list[0], right_list[1]),
                    )
                )
            save_clip(f"link/{prefix}_{direction}", frames, PLAYER_METADATA)


def export_generic_family(pattern_data: bytes, base_name: str, object_type: int, palette_name: str) -> None:
    for direction in ("down", "up", "left", "right"):
        frames = []
        for walk_frame in (0, 1):
            frame_index, flip_h = generic_walker_frame(direction, walk_frame)
            left_tile, attributes = lookup_object_frame(object_type, frame_index)
            left, right = build_flippable_pair(left_tile, attributes, flip_h)
            frames.append(compose_pair_image(pattern_data, palette_name, left, right))
        save_clip(f"enemies/{base_name}_{direction}", frames, ENEMY_METADATA)


def export_octorok_family(pattern_data: bytes, base_name: str, object_type: int, palette_name: str) -> None:
    for direction in ("down", "up", "left", "right"):
        frames = []
        for walk_frame in (0, 1):
            frame_index, flip_h, mirrored = octorok_frame(direction, walk_frame)
            left_tile, attributes = lookup_object_frame(object_type, frame_index)
            left, right = (
                build_mirrored_pair(left_tile, attributes)
                if mirrored
                else build_flippable_pair(left_tile, attributes, flip_h)
            )
            frames.append(compose_pair_image(pattern_data, palette_name, left, right))
        save_clip(f"enemies/{base_name}_{direction}", frames, ENEMY_METADATA)


def export_darknut_family(pattern_data: bytes, base_name: str, object_type: int, palette_name: str) -> None:
    for direction in ("down", "up", "left", "right"):
        frames = []
        for walk_frame in (0, 1):
            frame_index, flip_h = darknut_frame(direction, walk_frame)
            left_tile, attributes = lookup_object_frame(object_type, frame_index)
            left, right = build_flippable_pair(left_tile, attributes, flip_h)
            frames.append(compose_pair_image(pattern_data, palette_name, left, right))
        save_clip(f"enemies/{base_name}_{direction}", frames, ENEMY_METADATA)


def main() -> None:
    overworld_pattern_data = load_overworld_sprite_pattern_data(ROM_PATH)
    underworld_127_pattern_data = load_underworld_sprite_pattern_data(
        ROM_PATH, UNDERWORLD_GROUP_127_OFFSET, UNDERWORLD_GROUP_127_LENGTH
    )
    underworld_358_pattern_data = load_underworld_sprite_pattern_data(
        ROM_PATH, UNDERWORLD_GROUP_358_OFFSET, UNDERWORLD_GROUP_358_LENGTH
    )

    export_link_clips(overworld_pattern_data)
    export_octorok_family(overworld_pattern_data, "octorok_red", 0x07, "red")
    export_octorok_family(overworld_pattern_data, "octorok_blue", 0x09, "green")
    export_generic_family(overworld_pattern_data, "moblin_red", 0x04, "red")
    export_generic_family(overworld_pattern_data, "moblin_blue", 0x03, "green")
    export_generic_family(overworld_pattern_data, "lynel_red", 0x02, "red")
    export_generic_family(overworld_pattern_data, "lynel_blue", 0x01, "green")
    export_generic_family(underworld_127_pattern_data, "goriya_red", 0x06, "red")
    export_generic_family(underworld_127_pattern_data, "goriya_blue", 0x05, "green")
    export_darknut_family(underworld_358_pattern_data, "darknut_red", 0x0C, "red")
    export_darknut_family(underworld_358_pattern_data, "darknut_blue", 0x0B, "green")
    print(f"wrote sprite clips to {ASSET_ROOT}")


if __name__ == "__main__":
    main()
