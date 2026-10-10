# <img src="https://commons.wikimedia.org/wiki/Special:FilePath/Minecraft_Logo.png" width="140" alt="Minecraft logo" /> Minecraft (PS3) static lift record

**Role:** Opcode / coverage stress (lift only). Runtime compatibility has not been established.

## Recorded static lift

| Metric | Value |
| --- | ---: |
| Instruction instances | 3,034,530 |
| Translated instances reported | 3,034,530 |
| Unimplemented instances reported | 0 |
| Chunks | 371 |
| Reported static translation coverage | 100% for this snapshot |

## ELF analysis

| Metric | Value |
| --- | ---: |
| PT_LOAD segments | 5 |
| Entry OPD | `0xbbe520` |
| Detected symbols | 0 |
| Detected OPD entries | 4,097 |
| PRX/module string hits | 45 |
| Embedded SPU images | 11 |

See [lift_report.txt](lift_report.txt) and [analysis_report.txt](analysis_report.txt) for the archived reports. The analysis report lists sample function descriptors, PRX/module strings, and embedded SPU image offsets and sizes.

## Interpretation and follow-up

The report indicates that all instruction instances in this input snapshot were classified as translated. This is not proof that every emitted operation is semantically correct, that all imports resolve, or that the game runs. Approximate operations and no-op fallbacks may still be counted as translated. The 11 embedded SPU images are detected artifacts, not proof that SPU programs execute correctly.

1. Review the lift report for approximate operations and fallback paths.
2. Add focused synthetic regression tests for unsupported or approximate instruction families.
3. Validate PRX/NID resolution separately from PPU translation.
4. Keep Minecraft as a lift-only stress target while GOW3 remains the primary runtime target.

Do not commit commercial-game executables, keys, or copyrighted game assets.