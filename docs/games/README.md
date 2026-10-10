# PPSX33 Game Coverage Records

<p align="center"><img src="https://raw.githubusercontent.com/NotY215/PPSX33/main/assets/ppsx33-logo.png" alt="PPSX33 circular logo" width="120" /></p>


Static lifting snapshots only. Not proof of playability.

**Priority:** GOW3 for runtime. Minecraft, Uncharted 2, and Demon's Souls are opcode / SPU stress titles (**lift only**).

| Game | Record | Instructions | Translated | Unimplemented | Chunks | Static % |
| --- | --- | ---: | ---: | ---: | ---: | ---: |
| <img src="https://commons.wikimedia.org/wiki/Special:FilePath/God_of_War_Logo.png" width="78" alt="God of War logo" /> God of War III | [GOW3](GOW3/GOW3.md) | 1,285,560 | 1,285,560 | 0 | 157 | 100% snapshot |
| <img src="https://commons.wikimedia.org/wiki/Special:FilePath/Minecraft_Logo.png" width="78" alt="Minecraft logo" /> Minecraft (PS3) | [Minecraft](Mineacraft/README.md) | 3,034,530 | 3,034,530 | 0 | 371 | 100% snapshot |
| <img src="https://www.pngkey.com/png/detail/983-9839044_uncharted-2-among-thieves.png" width="48" alt="Uncharted 2 logo artwork" /> Uncharted 2: Among Thieves | [Uncharted2](Uncharted2/UNCHARTED2.md) | 3,485,016 | 3,485,016 | 0 | 426 | 100% snapshot |
| <img src="https://commons.wikimedia.org/wiki/Special:FilePath/Demon%27s_Souls_logo_black.svg" width="78" alt="Demon's Souls logo" /> Demon's Souls | [DemonsSouls](DemonsSouls/DEMONS_SOULS.md) | 6,343,506 | 6,343,506 | 0 | 775 | 100% snapshot |
| <img src="https://commons.wikimedia.org/wiki/Special:FilePath/Grand_Theft_Auto_San_Andreas_logo.svg" width="78" alt="Grand Theft Auto: San Andreas logo" /> Grand Theft Auto: San Andreas | [Grand Theft Auto: San Andreas](Grand%20Theft%20Auto%20-%20San%20Andreas/Grand%20Theft%20Auto%20-%20San%20Andreas.md) | 1,893,428 | 1,893,428 | 0 | 232 | 100% reported snapshot |
| <img src="https://thumb.wikimedia.org/wikipedia/it/thumb/7/76/Grand_Theft_Auto_V_logo.svg/330px-Grand_Theft_Auto_V_logo.svg.png?utm_source=it.wikipedia.org&utm_campaign=index&utm_content=thumbnail&_=20240305183247" width="78" alt="Grand Theft Auto V logo" /> Grand Theft Auto V | [Grand Theft Auto V](Grand%20Theft%20Auto%20V/Grand%20Theft%20Auto%20V.md) | 6,934,214 | 6,934,214 | 0 | 847 | 100% reported snapshot |

## Analysis findings

| Game | PT_LOAD segments | Entry OPD | Symbols | OPDs | PRX hits | SPU images |
| --- | ---: | --- | ---: | ---: | ---: | ---: |
| <img src="https://commons.wikimedia.org/wiki/Special:FilePath/God_of_War_Logo.png" width="78" alt="God of War logo" /> God of War III | 5 | `0x50ddc0` | 0 | 3 | 47 | 8 |
| <img src="https://commons.wikimedia.org/wiki/Special:FilePath/Minecraft_Logo.png" width="78" alt="Minecraft logo" /> Minecraft (PS3) | 5 | `0xbbe520` | 0 | 4,097 | 45 | 11 |
| <img src="https://www.pngkey.com/png/detail/983-9839044_uncharted-2-among-thieves.png" width="48" alt="Uncharted 2 logo artwork" /> Uncharted 2: Among Thieves | 5 | `0xdd8618` | 0 | 116 | 391 | 7 |
| <img src="https://commons.wikimedia.org/wiki/Special:FilePath/Demon%27s_Souls_logo_black.svg" width="78" alt="Demon's Souls logo" /> Demon's Souls | 5 | `0x1916138` | 0 | 4,097 | 134 | 12 |
| <img src="https://commons.wikimedia.org/wiki/Special:FilePath/Grand_Theft_Auto_San_Andreas_logo.svg" width="78" alt="Grand Theft Auto: San Andreas logo" /> Grand Theft Auto: San Andreas | 5 | `0x77b718` | 0 | 4,097 | 23 | 0 |
| <img src="https://thumb.wikimedia.org/wikipedia/it/thumb/7/76/Grand_Theft_Auto_V_logo.svg/330px-Grand_Theft_Auto_V_logo.svg.png?utm_source=it.wikipedia.org&utm_campaign=index&utm_content=thumbnail&_=20240305183247" width="78" alt="Grand Theft Auto V logo" /> Grand Theft Auto V | 5 | `0x1a90b80` | 0 | 4,097 | 56 | 8 |

These are per-input snapshots. GTA: San Andreas report files: [lift report](Grand%20Theft%20Auto%20-%20San%20Andreas/lift_report.txt) and [analysis report](Grand%20Theft%20Auto%20-%20San%20Andreas/analysis_report.txt). GTA V report files: [lift report](Grand%20Theft%20Auto%20V/lift_report.txt) and [analysis report](Grand%20Theft%20Auto%20V/analysis_report.txt). PRX hits and OPD discovery are analysis findings, not proof of complete module resolution or runtime support. See [PPU coverage](../PPU_COVERAGE.md) for interpretation and caveats.
