# PPSX33 Roadmap

**Last updated:** 2026-10-15  
**Scope:** Experimental PlayStation 3 static recompiler — tooling, packaging, runtime, post-XX lettered work.

PPSX33 is a **research toolkit**, not a retail emulator. Labels describe in-tree engineering work, not verified commercial playability.

---

## Track overview

```text
I – XIX     Tooling & core pipeline (TESTING)
XX-A – XX-G Packaging & host UX (DONE)
XX-H – XX-O Runtime playability base (DONE)
A – Q       Post-XX lettered implementations (DONE)
R+          Remaining gaps
```

---

## Phases I – XIX (TESTING)

Project shell through RSX depth/FIFO, GUI workflow, pad, audio/save hooks. See repository history for per-phase detail. Static lift metrics are not playability proof.

---

## XX-A … XX-G (DONE) · XX-H … XX-O (DONE)

Packaging (game EXE, USRDIR copy, TERMS, logs) and base runtime (GCM, PSARC, SPU/MFC/SPURS core, audio, ICON0).

---

## Post-XX lettered implementations

| Sno. | Name | Status | Work |
|------|------|--------|------|
| **A** | RSX surfaces | DONE | Color/depth surface fields |
| **B** | PPU accuracy helpers | DONE | mul/div/rotate; LR recovery |
| **C** | SPU ISA expansion | DONE | Logic, float, rotates |
| **D** | MFC / DMA fidelity | DONE | List layout, tags, queue |
| **E** | SPURS jobchain | DONE | ready, chain_next |
| **F** | cellFs directories | DONE | opendir/readdir/closedir |
| **G** | Audio ring / notify | DONE | Ring indices, notify key |
| **H** | LV2 syscalls | DONE | Mutex, event, sem, thread, mem |
| **I** | RSX draw / shader | DONE | Bind counts, program id |
| **J** | PPU lifter helpers | DONE | Shared helpers surface |
| **K** | Audio multi-port | DONE | Up to 8 ports |
| **L** | RSX texture unit | DONE | Offset/format/size tracking |
| **M** | Lifter helper surface | DONE | PS3_LIFTER_HELPERS markers |
| **N** | cellFs errno / SELF | DONE | Errno mapping; SELF reject |
| **O** | Library NID HLE | DONE | sysutil/sysmodule/pad/kb/mouse/spurs stubs |
| **P** | Save data lifecycle | DONE | Create/list/load/save/delete HLE |
| **Q** | Pad pressure / sixaxis | DONE | Pressure array; accel stubs; sensor toggle |

### Remaining gaps

| Sno. | Name | Status | Work |
|------|------|--------|------|
| **R** | Timing model | PARTIAL | Coordinated PPU/SPU/RSX timing |
| **S** | PRX modules | PARTIAL | Load, relocate, import/export |
| **T** | RSX textured draws | PARTIAL | Sample textures in host draws |
| **U** | Deeper NID behaviour | PARTIAL | Beyond success stubs |

---

## Related docs

[README.md](README.md) · [USER_MANUAL.md](USER_MANUAL.md) · [LEGAL.md](LEGAL.md) / [TERMS.md](TERMS.md)

---

*No proprietary keys or copyrighted game assets are included.*
