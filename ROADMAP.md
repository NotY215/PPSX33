# PPSX33 Roadmap — God of War 3 (GOW_3)

**Last updated:** 2026-10-12 (Phase XI)

Phases use **Roman numerals**. Status: Done / Partial / Next / Planned.

---

## Full phase table

| Phase | Status | Focus |
|-------|--------|-------|
| **I** | Done | Boot image map, entry PC/TOC, basic PPU loop |
| **II** | Done | Import stub surface at 0x39800000, null-call survival |
| **III** | Done | Soft-break / stride skip on null loops |
| **IV** | Done | LV2 mmapper (sc 352) + guest alloc |
| **V** | Done | cellGcmSys stub surface, PRX kind dispatch |
| **VI** | Done | GCM control / labels / FIFO allocation |
| **VII** | Done | D3D11 host present + flip path |
| **VIII** | Done | RSX FIFO put/get modulo (stop method storm) |
| **IX** | Done | Sticky LR / region escape, stack recover |
| **X** | Done | Correct NV method packets; known-method decode; CLEAR=draw; inject FIFO |
| **XI** | **Done** | Stop region-ban address-walk; prefer high last_good; no low-image bans |
| **XII** | Next | Real libgcm put from guest; fewer synthetic injects |
| **XIII** | Planned | Texture / surface setup HLE |
| **XIV** | Planned | Vertex/index draw path (real draws beyond CLEAR) |
| **XV** | Planned | Shader decode / host shader stubs |
| **XVI** | Planned | cellFs path + disc/content access |
| **XVII** | Planned | SPU task realism beyond mailbox HLE |
| **XVIII** | Planned | Audio port HLE |
| **XIX** | Planned | Pad/input wired to host |
| **XX** | Planned | Stability / save / long-run |

---

## Phase XI (this commit)

**Bug:** After healthy flips, null-import path region-banned **low image** pages (`0x18000`, `0x20000`, …) and *walked* upward until halt at `0x97fb8`.

**Fix:**
1. **Never** `region-ban` addresses `< 0x100000` (code image).
2. Removed sequential **chunk-start** fallback (it fed the walk).
3. Stack recover prefers `0x300000–0x500000` (active GOW band).
4. **Walk detector**: if leave targets advance by ≤0x10000 three times → snap to `last_good` / `0x33b1a0`.
5. Force-out of banned regions only when PC is already in a high banned window.

---

## Success signals (current)

```
[rsx] flip #N draws=N clears=N methods=… known=… unknown=0|small
[ps3] heartbeat … pc=0x33… or 0x32…   (not walking 0x18000..0x90000)
```

Closing the RSX window only hides it; the process keeps running (Phase X window policy).

---

## Layout reminder

Keep build outputs in:

```
build/dist/projects/GOW_3/output/
  GOW_3.exe
  ps3rt.dll / runtime DLLs
  guest_image.bin
```

Do not copy into the original game install until cellFs / content paths are real (Phase XVI).

---

## Apply

```bat
cd /d E:\PPSX33
git pull origin main
```

Rebuild All → Build → Run.

Expect: no more `region-ban 0x18000` … `0x90000` chains; after any null storm, snap back toward `0x33xxxx` / last_good instead of walking the image.
