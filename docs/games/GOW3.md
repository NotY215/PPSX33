# PPSX33 Game Record: God of War III (GOW3)

## Lift statistics

| Metric | Count |
| --- | ---: |
| Instructions | 1,285,560 |
| Translated | 1,103,166 |
| Unimplemented | 182,394 |
| Chunks | 157 |
| Translation coverage | 85.81% |

The translated and unimplemented counts sum to the total instruction count. Translation coverage is calculated as:

`1,103,166 / 1,285,560 × 100 ≈ 85.81%`

## What these numbers mean

- **Instructions** is the total instruction instances reported for this lift.
- **Translated** is the number of instruction instances the current lifter translated.
- **Unimplemented** is the number of instruction instances still unsupported by the lifter.
- **Chunks** is the number of emitted instruction chunks.

These are static translation metrics only. They do not establish that the game is playable. The current MSVC build produces `game.exe` and `ps3rt.dll`, but launching the output starts execution and then leaves the recompiled guest-code range. This runtime failure is separate from the 85.81% static translation figure. PS3 system calls, PRX imports, SPU workloads, RSX graphics, and guest control-flow correctness remain incomplete.

## Follow-up

- Refresh these numbers after significant lifter changes.
- Compare the unimplemented breakdown against the current lift report to prioritize missing instructions.
- Keep the original game data and decrypted ELF out of this repository. This record stores only summary statistics.
