# PPSX33 Game Record: God of War III (GOW3)

## Lift statistics

| Metric | Count |
| --- | ---: |
| Instructions | 1,285,560 |
| Translated | 1,103,166 |
| Unimplemented | 182,394 |
| Chunks | 157 |
| Translation coverage | 85.81% |

Translation coverage is calculated as:

`1,103,166 / 1,285,560 × 100 ≈ 85.81%`

## Interpretation

These figures measure static instruction translation only. They do not demonstrate successful boot, correct rendering, or full-game compatibility.

The current MSVC build produces `game.exe` and `ps3rt.dll`. Launching the output starts execution, but the guest PC then leaves the recompiled range. This runtime failure is separate from the static translation percentage. System calls, PRX imports, SPU workloads, RSX graphics, and guest control-flow correctness remain incomplete.
