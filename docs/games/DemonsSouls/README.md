# Demon's Souls (PS3)

**Status:** Planned for static analysis / lift (same track as Uncharted 2)  
**Runtime compatibility:** Not established — **lift only** until GOW3 runtime is further along.

## Role

| Title | Role |
| --- | --- |
| God of War III | Primary **runtime** target |
| Uncharted 2 | Opcode / SPU stress (**lift only**) |
| Demon's Souls | Additional static coverage + SPU/PRX samples (**lift only**) |

Do not prioritize native `game.exe` build/run for Demon's Souls yet. Use decrypt → decompile → inspect reports.

## Workflow (same as Uncharted 2)

1. In the GUI, set **RPCS3** path (OPTIONS → Choose rpcs3.exe) if not already saved.
2. **Decrypt EBOOT** — selects `rpcs3.exe` (if needed) then `EBOOT.BIN`, runs `rpcs3 --decrypt`.
3. Create project (suggested name: `DemonsSouls` or `DEMONS SOULS`).
4. **Decompile** only; skip **Build** / **Run** for this title for now.
5. Copy or summarize `codebase/lift_report.txt` and `codebase/analysis_report.txt` here when you have numbers.

## Metrics (fill after first lift)

| Metric | Value |
| --- | ---: |
| Segments | — |
| Entry OPD | — |
| Symbols | — |
| OPD entries | — |
| PRX string hits | — |
| Embedded SPU images | — |
| Instruction instances | — |
| Translated | — |
| Unimplemented | — |
| Chunks | — |
| Reported static coverage | — |

Record values only from real tool output. Do not invent counts.

## Follow-up

1. Archive full lift/analysis reports under this folder (redacted paths OK).
2. Compare approximate/fallback ops with GOW3 and Uncharted 2.
3. Exercise any embedded SPU images against the SPU interpreter.
4. Expand PRX/NID HLE from module strings that appear in the analysis report.

See [DEMONS_SOULS.md](DEMONS_SOULS.md) and [ROADMAP](../../../ROADMAP.md).
