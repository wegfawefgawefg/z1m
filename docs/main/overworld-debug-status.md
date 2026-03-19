# Overworld Debug Status

## Resolved

The overworld extraction and rendering pipeline is working correctly.

The root cause of all observed issues (room shift, wrong palettes, garbled
tiles, missing ocean) was a single bug: every hardcoded ROM file offset was
missing the 16-byte iNES header adjustment. This shifted all ROM reads by 16
bytes, corrupting level block attributes, room layouts, and CHR tile patterns
simultaneously.

See `nes-rom-offsets.md` for the full explanation and the correct offset formula.

### What was fixed

- `extract_overworld.py`: `ROOM_LAYOUTS_OFFSET` 87064 -> 87080,
  `LEVEL_BLOCK_OFFSET` 99328 -> 99344
- `export_overworld_png.py`: all three CHR offsets +16
- `debug_tileset.cpp`: all three CHR offsets +16

### What was already correct

- Column heap concatenation from `ColumnHeapOW0..F`
- Square-to-tile expansion (2x2 primary/secondary squares)
- Column heap navigation (upper/lower nibble indexing)
- Repeat/compress flag handling (bit 6 toggle)
- Room ID to screen position mapping (row-major, 16 columns)
- Unique room ID resolution (`attrs_d[room_id] & 0x7F`)
- CHR tile decoding (2-bitplane NES format)
- Pattern table assembly (common_bg + overworld_bg + common_misc)
- Palette selection (inner/outer selectors from attrs A/B)

The extraction logic, decompression algorithm, and rendering pipeline were all
correct. Only the ROM file offsets were wrong.
