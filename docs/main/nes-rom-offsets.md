# NES ROM File Offsets

## The iNES Header

Every `.nes` ROM file begins with a 16-byte iNES header. This header is **not**
part of the game's PRG-ROM or CHR-ROM data -- it's metadata added by the
emulation community to describe the cartridge hardware.

```
Offset  Size  Contents
------  ----  --------
0x00    4     Magic: "NES\x1a"
0x04    1     PRG-ROM size in 16KB units
0x05    1     CHR-ROM size in 8KB units (0 = CHR-RAM)
0x06    1     Flags 6 (mapper low, mirroring, battery, trainer)
0x07    1     Flags 7 (mapper high, format)
0x08-0F 8     Extended flags (often all zeros in older dumps)
```

PRG-ROM data starts at byte **16** (0x10). This is easy to forget.

## Offset Formula

Zelda 1 (MMC1) has 8 banks of 16KB PRG-ROM, no CHR-ROM (uses CHR-RAM).

To convert a CPU address to a ROM file offset:

```
rom_offset = 16 + (bank_number * 0x4000) + (cpu_address - 0x8000)
```

Or equivalently, given a bank's base ROM offset:

```
bank_rom_start = 16 + (bank_number * 0x4000)
```

The `+ 16` accounts for the iNES header. Without it, every read is shifted by
16 bytes into whatever data or code precedes the intended location.

## Bank Layout (Zelda 1 Rev 1)

| Bank | ROM File Offset | CPU Address Range |
|------|-----------------|-------------------|
| 0    | 0x00010         | $8000-$BFFF       |
| 1    | 0x04010         | $8000-$BFFF       |
| 2    | 0x08010         | $8000-$BFFF       |
| 3    | 0x0C010         | $8000-$BFFF       |
| 4    | 0x10010         | $8000-$BFFF       |
| 5    | 0x14010         | $8000-$BFFF       |
| 6    | 0x18010         | $8000-$BFFF       |
| 7    | 0x1C010         | $C000-$FFFF (fixed) |

## What Went Wrong

All hardcoded ROM offsets in the project were originally computed as
`bank * 0x4000 + offset_in_bank`, missing the `+ 16`. This shifted every ROM
read by 16 bytes, causing:

- **Level block attributes**: read 16 bytes of 0xFF padding before the real
  data, cross-contaminating all 6 attribute tables (A-F). Each room's
  unique_room_id, palette selectors, and flags were wrong.
- **Room layouts**: read 16 bytes of 6502 machine code (the tail of the
  preceding function) before the real layout table. Each room's column
  descriptors pointed to the wrong compressed data.
- **CHR tile patterns**: read from 16 bytes before each pattern section,
  producing garbled tile graphics.

The combined level block + room layout shift made the overworld appear shifted
by roughly one screen down and one screen to the right (~0x11 rooms), with
ocean missing from edges and some rooms in the wrong place.

## How To Add New ROM Offsets

When adding a new ROM offset, either:

1. Read the address pointer table from the ROM itself (the game's own pointer
   tables are already at the correct ROM positions once bank start is right).
2. Use the formula: `rom_offset = 16 + bank * 0x4000 + (cpu_addr - 0x8000)`.
3. Search for known byte patterns in the ROM to verify.

Never use `bank * 0x4000 + offset` without the header adjustment.
