# Phase XXII — Pipeline correctness

| ID | Status | Work |
|----|--------|------|
| **XXII-A** | DONE | RSX present / color-buffer |
| **XXII-B** | DONE | PPU soft-recovery |
| **XXII-C** | DONE | SPU/MFC tag & stall (case fix) |
| **XXII-D** | DONE | LV2/HLE visibility |
| **XXII-E** | DONE | Modules/PRX/FS ordered load |
| **XXII-F** | DONE | Timing / audio surface |
| **XXII-G** | DONE | RSX draw classification (tex vs none) |
| **XXII-H** | DONE | PPU recovery surface depth |
| **XXII-I** | DONE | SPU channel path continuity |
| **XXII-J** | DONE | LV2 contracts note / upstream log |
| **XXII-K** | PLANNED | Save / pad depth |
| **XXII-L** | PLANNED | OpenGL probe prep |

## Fixes in this drop

- `spu_stub.cpp`: `CH_MFC_RdTagStat` brace was inside a comment → cases outside switch / `ch_write` not found. Fixed.
- Controller tab: `StyleBtn` Dock=Top broke absolute layout; rebuilt with `TableLayoutPanel` + `StyleBtnNoDock`.
