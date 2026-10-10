# Demon's Souls (BLES00632 / BLUS30443 family)

**Role:** Lift-only / opcode + SPU stress (not primary runtime target).

## Static lift snapshot (2026-10)

| Metric | Value |
|--------|-------|
| Segments | 5 |
| Entry OPD | 0x1916138 |
| Symbols | 0 |
| OPD entries | 4097 |
| PRX / module string hits | 134 |
| Embedded SPU images | 12 |
| Instructions | 6,343,506 |
| Translated | 6,343,506 (100%) |
| Unimplemented | 0 |
| Chunks | 775 |

See `lift_report.txt` and `analysis_report.txt` produced by the analysis pass.

## Notes

- Extremely large static image; good stress for the PPU lifter and SPU image extraction.
- 134 PRX hits provide additional NID/module strings for the heuristic table.
- Runtime is **not** targeted; GOW3 remains the primary early-boot target.
