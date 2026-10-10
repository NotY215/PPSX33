# PPSX33 Game Coverage Records

<p align="center"><img src="https://raw.githubusercontent.com/NotY215/PPSX33/main/assets/ppsx33-logo.png" alt="PPSX33 circular logo" width="120" /></p>


Static lifting snapshots only. Not proof of playability.

**Priority:** GOW3 for runtime. Uncharted 2 and Demon's Souls for opcode stress / SPU image samples (**lift only**).

| Game | Record | Instructions | Translated | Unimplemented | Chunks | Static % |
| --- | --- | ---: | ---: | ---: | ---: | ---: |
| God of War III | [GOW3](GOW3/GOW3.md) | 1,285,560 | 1,285,560 | 0 | 157 | 100% snapshot |
| Uncharted 2 | [Uncharted2](Uncharted2/UNCHARTED2.md) | 3,485,016 | 3,485,016 | 0 | 426 | 100% snapshot |
| Demon's Souls | [DemonsSouls](DemonsSouls/DEMONS_SOULS.md) | 6,343,506 | 6,343,506 | 0 | 775 | 100% snapshot |

## Analysis findings

| Game | PT_LOAD segments | Entry OPD | Symbols | OPDs | PRX hits | SPU images |
| --- | ---: | --- | ---: | ---: | ---: | ---: |
| God of War III | 5 | `0x50ddc0` | 0 | 3 | 47 | 8 |
| Uncharted 2 | 5 | `0xdd8618` | 0 | 116 | 391 | 7 |
| Demon's Souls | 5 | `0x1916138` | 0 | 4,097 | 134 | 12 |

These are per-input snapshots. PRX hits and OPD discovery are analysis findings, not proof of complete module resolution or runtime support. See [PPU coverage](../PPU_COVERAGE.md) for interpretation and caveats.
