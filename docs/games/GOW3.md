# God of War III (GOW3)

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

These are static translation metrics only. They do not establish that the generated output builds successfully, runs correctly, or makes the full game playable. Runtime support for PS3 system calls, PRX imports, SPU workloads, and RSX graphics remains separate work.

## Follow-up

- Refresh these numbers after significant lifter changes.
- Compare the unimplemented breakdown against the current lift report to prioritize missing instructions.
- Keep the original game data and decrypted ELF out of this repository. This record stores only summary statistics.
