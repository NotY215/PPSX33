# Demon's Souls (PS3)

**Status:** Initial static lift recorded  
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
5. Preserve the lift report at [`DemonsSouls_lift_report.txt`](DemonsSouls_lift_report.txt); analysis metadata not present in the lift report remains unrecorded.

## Recorded lift metrics

| Metric | Value |
| --- | ---: |
| Segments | — |
| Entry OPD | — |
| Symbols | — |
| OPD entries | — |
| PRX string hits | — |
| Embedded SPU images | — |
| Instruction instances | 6,343,506 |
| Translated | 6,343,506 |
| Unimplemented | 0 |
| Chunks | 775 |
| Reported static coverage | 100% for this snapshot |

Source: [`DemonsSouls_lift_report.txt`](DemonsSouls_lift_report.txt). The report contains instruction, translation, unimplemented, and chunk counts only. Other metadata is not provided. Zero unimplemented entries does not prove semantic correctness or playability.

## Follow-up

1. Archive the full analysis report under this folder when available; the lift report is already archived.
2. Compare approximate/fallback ops with GOW3 and Uncharted 2.
3. Exercise any embedded SPU images against the SPU interpreter.
4. Expand PRX/NID HLE from module strings that appear in the analysis report.

See [DEMONS_SOULS.md](DEMONS_SOULS.md) and [ROADMAP](../../../ROADMAP.md).
