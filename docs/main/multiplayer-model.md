# Multiplayer Model

## Session Shape

The default session model is host-plus-clients with join-in-progress.

- One host owns the current shared world state.
- Other players can connect at any time.
- New players enter the host's current world, not a separate fork.
- Players are free to travel to completely different parts of the map.

## Networking Priorities

The multiplayer model is intentionally coop-biased and latency-tolerant.

Priority order:

1. No obvious local input lag for the acting player
2. Shared campaign progression that remains understandable
3. Eventual consistency for remote actors
4. Strict agreement only where it materially matters

## Sync Philosophy

We do not want lockstep simulation.

The default approach is:

- Each player runs most movement, combat, and local presentation immediately.
- The network layer exchanges important events plus coarse state updates.
- Remote actors are allowed to look delayed, approximate, or slightly wrong.
- Small disagreements in enemy position, exact knockback, or timing are
  acceptable when they do not break shared progression.

Examples of events that should replicate reliably:

- player joined or left
- enemy killed
- boss defeated
- room puzzle solved
- door opened
- item spawned or consumed when that matters to world state

Examples of state that can be soft-synced:

- remote player position
- remote player facing and weapon animation
- enemy transient position
- short-lived projectile placement

## Authority Model

Working authority split:

- Host-authoritative:
  - shared world flags
  - durable room state
  - player roster
  - session settings
- Locally authoritative at input time:
  - self movement
  - self attack timing
  - self immediate presentation
- Reconciled only when needed:
  - world-mutating events
  - state that would block progression

## Player Interaction

Player-to-player interaction is a feature, but it should stay optional and
session-configurable.

Working default:

- player collision and push: enabled
- player damage from other players: disabled
- items affecting other players: enabled where fun and readable

That means bombs, sword swings, and similar actions may interact with teammates
for knockback, disruption, or support effects without making griefing the
default behavior.

## Scale Target

- Initial target: `16` players
- Design rule: no deep assumption that the game only supports `2-4`
- Relevance filtering should be based on region, room, and event importance so
  distant players do not force full-fidelity updates everywhere
