# z1m Docs

These docs are the working spec for `z1m`.

They are meant to lock down direction early, keep the codebase explicit, and
avoid re-litigating basic project assumptions every time implementation starts
moving.

Read these first:

- `project-goals.md`: what the project is trying to be
- `game-scope.md`: what must exist in the shipped game
- `multiplayer-model.md`: how coop networking should behave
- `architecture.md`: code and module boundaries
- `roadmap.md`: implementation order

Read these as supporting docs:

- `reference-policy.md`: how we use the ROM, decomps, and outside references
- `open-questions.md`: a short list of items that are still intentionally open
- `overworld-debug-status.md`: current status of the overworld loader/debug work

The rule for now is simple: if implementation and docs disagree, update one of
them immediately instead of letting the mismatch sit around.
