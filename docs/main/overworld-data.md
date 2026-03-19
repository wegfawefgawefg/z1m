# Overworld Data

The current overworld content path is intentionally simple:

1. `tools/extract_overworld.py` reads the original ROM plus a few reference
   tables from `zelda1-disassembly`
2. it expands quest-1 overworld screens into `content/overworld_q1.txt`
3. `z1m` loads that text file directly at runtime

This keeps the runtime independent from the neighboring reference repos while
still letting us regenerate content from local sources.

The first goal here is correctness of world layout, not final rendering. The
text format stores:

- screen id
- unique room id
- six raw level-block attribute bytes for the screen
- a fully expanded `32 x 22` tile grid for each overworld screen
