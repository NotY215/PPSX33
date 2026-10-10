# <img src="https://commons.wikimedia.org/wiki/Special:FilePath/God_of_War_Logo.png" width="150" alt="God of War logo" /> God of War III (GOW3): Lift and runtime record

## Static lift statistics

| Metric | Count |
| --- | ---: |
| Instruction instances | 1,285,560 |
| Translated | 1,285,560 |
| Unimplemented | 0 |
| Chunks | 157 |
| Segments | 5 |
| Entry OPD | 0x50ddc0 |
| Symbols | 0 |
| OPD entries (heuristic) | 3 |
| PRX string hits | 47 |
| Embedded SPU images | 8 |
| Reported static coverage | 100% (this snapshot) |

Raw totals: [GOW3_lift_report.txt](GOW3_lift_report.txt) when archived.

## Analysis notes

From the project decompile log (same metrics as analysis_report.txt):

- No symbol table on this commercial ELF (expected).
- Few OPD discoveries (3) vs large code size: function discovery is incomplete; control flow relies on branch targets inside lifted chunks.
- 47 PRX/module string hits: game links many system modules; imports are not fully resolved at lift time.
- 8 embedded SPU images: useful for Phase 4 SPU interpreter testing.

There is no separate `docs/games/GOW3/analysis_report.txt` checked into the repo by default (reports are generated per project under the game folder). Prefer copying a redacted analysis_report.txt here when you want a permanent archive.

## Runtime diagnostics (current)

Entry after OPD resolve:

```text
entry pc=0x10230 toc=0x52d6c8
```

Soon after start, execution repeatedly hits non-translated PCs:

| Observation | Meaning |
| --- | --- |
| `pc=0x39800000` | Not valid guest code in the lifted image; treated as external/import stub |
| Return via `lr` (e.g. 0x103ac, 0x33aec8, 0x336660) | Stub path returns CELL_OK and continues |
| `pc=0` with `lr=0x199c78` | Null call target; same stub path |
| `r2=0x658c0050` | TOC often stable; sometimes cleared on bad paths |

Conclusion: the bottleneck is **missing PRX / function-pointer resolution**, not missing PPU opcode decode. Static coverage is already 100% for this ELF.

## Next runtime steps

1. Resolve or HLE the callees that produce `0x39800000` / null (module imports, init tables). Partial: external stub returns CELL_OK via LR (rate-limited).
2. Expand LV2 syscalls used during CRT/startup. Done for common set from analysis_report: FS (cellFs*), ppu_thread, lwmutex/lwcond, timers, memory, sysmodule/prx, SPU thread group, GCM-ish hooks.
3. Exercise the 8 SPU images under the SPU interpreter (major ISA families present; full vector/FP still partial).
4. GCM FIFO core + flip path present in rsx_stub; next is host draw backend.

## Reproduction

1. Decrypt EBOOT with PPSX33 (RPCS3 `--decrypt`) or load EBOOT.ELF.
2. Decompile + Build.
3. Use **Run game.exe** in the UI (streams console; auto-stops after 4s without output).

Do not commit the ELF, EBOOT, assets, or keys.
