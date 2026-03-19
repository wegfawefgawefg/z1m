# Overworld Debug Status

This is the current state of the Quest 1 overworld loader/debug work.

The short version:

- the map is still wrong
- the remaining bug is not just "bad colors"
- I do not think I am one small edit away from the final fix

## What Seems Correct

- The `2 x 2` square write order matches the original game's `WriteSquareOW`
  behavior from `zelda1-disassembly`.
- The overworld column heap directory we build from `ColumnHeapOW0..F` matches
  the disassembly addresses, so the column table concatenation itself does not
  look wrong.
- Many individual room shapes match an independent exported overworld tile dump
  exactly once tile IDs are normalized by shape instead of color.

That last point matters: a lot of room-local structure is real. The current
problem is not simply "every room was decoded into nonsense."

## What Seems Wrong

- The exported/runtime world is not in the same screen order as the continuous
  overworld map we expect to walk around.
- A `source_room = (world_room + 0x11) & 0x7f` remap matches a large chunk of
  the map much better than raw row-major placement.
- That remap fixes obvious center-map failures, including the broken lake area.
- Even after that shift, a non-trivial set of edge screens and some repeated
  room patterns are still wrong.

So there are likely two separate issues:

1. A world-screen ordering/origin problem.
2. A smaller remaining subset of screens whose layout/palette selection is
   still wrong even after the big shift.

## Evidence Behind That

- Against an independent overworld tile dump from
  `asweigart/nes_zelda_map_data`, the current raw room order matches `0/128`
  screens exactly by normalized room shape.
- Applying a `+0x11` source-room offset raises that to `88/128` exact matches.
- Screens like `x=5,y=2` looked wrong in the renderer, but their raw square
  structure actually matched the independent dump. That means the renderer and
  palette path made the diagnosis look worse than it was.
- The failures that remain are clustered around edge/ocean screens and a few
  duplicated-room cases where multiple rooms share very similar shapes.

## What I Do Not Think The Main Bug Is

- I do not think the main problem is the `2 x 2` square orientation.
- I do not think this is a simple `x/y` swap.
- I do not think the column directory concatenation is the main bug.
- I do not think palette alone explains the full world-order failure.

Palette is still wrong in places, but palette bugs do not explain the room
shift behavior.

## What I Think The Actual Issue Is

The current loader/export path is still too close to "raw room-id order" and
not yet correctly producing the continuous world-space ordering we want in
`z1m`.

There is probably also a smaller remaining decode problem on top of that.
Possible places:

- wrong source-room basis for some edge screens
- a remaining misunderstanding around how a subset of overworld screens select
  their room layout
- an incomplete handling of the metadata attached to those screens

The user suspicion about a page/boundary bug is plausible for the remaining
subset, but it does not explain the full-map `+0x11` shift pattern by itself.

## Practical Conclusion

I know the class of bug now better than before, but I do not have the final
solution yet.

The next productive move is not more blind tweaking. It is one of these:

1. Find a second ROM-aware exported data source in machine-readable form
   (`json`, `txt`, `csv`, anything better than screenshot comparison).
2. Make `z1m` store a clean separation between:
   - continuous world-screen position
   - original ROM source room id
3. Solve the remaining bad edge screens with a focused diff against a trusted
   external dump.

## References Used

- `zelda1-disassembly`
- `asweigart/nes_zelda_map_data`
- the local cropped reference map in `artifacts/`
