# <img src="https://commons.wikimedia.org/wiki/Special:FilePath/Demon%27s_Souls_logo_black.svg" width="150" alt="Demon's Souls logo" /> Demon's Souls — lift record

- **Date:** 2026-10
- **Input:** Decrypted EBOOT.ELF (RPCS3 `--decrypt`)
- **ELF:** 5 PT_LOAD segments, entry OPD `0x1916138`
- **Analysis:** symbols=0, opds=4097, prx_hits=134, spu_images=12
- **Lift:** 6,343,506 instructions translated, 0 unimplemented, 775 chunks
- **Artifacts:** `codebase/lift_report.txt`, `codebase/analysis_report.txt`, extracted `spu_image_XX.bin`

## Why this title

Large commercial binary with heavy SPU usage and many PRX imports. Used to expand coverage of the static lifter and to feed additional module/NID strings into the PRX heuristic table. Not a runtime target.
