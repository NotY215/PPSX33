# PPSX33 Roadmap

**Last updated:** 2026-10-15  
**Scope:** Experimental PlayStation 3 static recompiler.

PPSX33 is a **research toolkit**, not a retail emulator. Labels describe in-tree work, not verified commercial playability.

---

## Track overview

```text
I – XIX     Tooling & core pipeline (TESTING)
XX-A – XX-O Packaging + base runtime (DONE)
A – U       Post-XX lettered implementations (DONE)
```

---

## Post-XX lettered implementations

| Sno. | Name | Status | Work |
|------|------|--------|------|
| **A–Q** | Prior lettered work | DONE | Surfaces through pad pressure, save, NID stubs (see prior revisions) |
| **R** | Timing model | DONE | PPU/SPU phase ticks in `progress_hw`; `[time] R` log |
| **S** | PRX modules | DONE | Module registry on sysmodule/first NID hit; `[prx] S load` |
| **T** | RSX textured draws | DONE | Log/tint when draws issue with bound texture unit |
| **U** | Deeper NID behaviour | DONE | GetSystemParamInt/String, CheckCallback concrete returns |

### Optional further work

| Sno. | Name | Status | Work |
|------|------|--------|------|
| **V** | Host texture sample | PLANNED | Upload guest tex to D3D and sample in draws |
| **W** | Full PRX relocate | PLANNED | ELF reloc / export binding |
| **X** | Frame pacing | PLANNED | Host vsync-linked guest time |

---

## Related docs

[README.md](README.md) · [USER_MANUAL.md](USER_MANUAL.md) · [LEGAL.md](LEGAL.md)

---

*No proprietary keys or copyrighted game assets are included.*
