# PPSX33 Game Coverage Records


Static lifting snapshots only. Not proof of playability.

**Priority:** GOW3 for runtime. Minecraft, Uncharted 2, and Demon's Souls are opcode / SPU stress titles (**lift only**).

| Game | Record | Instructions | Translated | Unimplemented | Chunks | Static % |
| --- | --- | ---: | ---: | ---: | ---: | ---: |
| God of War III | [GOW3](GOW3/GOW3.md) | 1,285,560 | 1,285,560 | 0 | 157 | 100% snapshot |
| Minecraft (PS3) | [Minecraft](Mineacraft/README.md) | 3,034,530 | 3,034,530 | 0 | 371 | 100% snapshot |
| Uncharted 2: Among Thieves | [Uncharted2](Uncharted2/UNCHARTED2.md) | 3,485,016 | 3,485,016 | 0 | 426 | 100% snapshot |
| Demon's Souls | [DemonsSouls](DemonsSouls/DEMONS_SOULS.md) | 6,343,506 | 6,343,506 | 0 | 775 | 100% snapshot |
| Grand Theft Auto: San Andreas | [Grand Theft Auto: San Andreas](Grand%20Theft%20Auto%20-%20San%20Andreas/Grand%20Theft%20Auto%20-%20San%20Andreas.md) | 1,893,428 | 1,893,428 | 0 | 232 | 100% reported snapshot |
| Grand Theft Auto V | [Grand Theft Auto V](Grand%20Theft%20Auto%20V/Grand%20Theft%20Auto%20V.md) | 6,934,214 | 6,934,214 | 0 | 847 | 100% reported snapshot |

## Analysis findings

| Game | PT_LOAD segments | Entry OPD | Symbols | OPDs | PRX hits | SPU images |
| --- | ---: | --- | ---: | ---: | ---: | ---: |
| God of War III | 5 | `0x50ddc0` | 0 | 3 | 47 | 8 |
| Minecraft (PS3) | 5 | `0xbbe520` | 0 | 4,097 | 45 | 11 |
| Uncharted 2: Among Thieves | 5 | `0xdd8618` | 0 | 116 | 391 | 7 |
| Demon's Souls | 5 | `0x1916138` | 0 | 4,097 | 134 | 12 |
| Grand Theft Auto: San Andreas | 5 | `0x77b718` | 0 | 4,097 | 23 | 0 |
| Grand Theft Auto V | 5 | `0x1a90b80` | 0 | 4,097 | 56 | 8 |

These are per-input snapshots. GTA: San Andreas report files: [lift report](Grand%20Theft%20Auto%20-%20San%20Andreas/lift_report.txt) and [analysis report](Grand%20Theft%20Auto%20-%20San%20Andreas/analysis_report.txt). GTA V report files: [lift report](Grand%20Theft%20Auto%20V/lift_report.txt) and [analysis report](Grand%20Theft%20Auto%20V/analysis_report.txt). PRX hits and OPD discovery are analysis findings, not proof of complete module resolution or runtime support. See [PPU coverage](../PPU_COVERAGE.md) for interpretation and caveats.


## Publication and rights note

Game titles are used only to identify technical test inputs; this page does not imply endorsement, affiliation, or permission to distribute those games. The listed counts are static analysis measurements, not compatibility claims. Do not commit commercial executables, decrypted ELFs, EBOOT files, firmware, keys, extracted assets, or unsanitized logs. Before publishing any report, check that it contains no proprietary input data, private local paths, usernames, or secrets. Remove or replace third-party artwork unless its license and redistribution permission have been verified.
