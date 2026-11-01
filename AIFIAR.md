# AIFIAR — how AI assistants should work on PPSX33

PPSX33 expects coding assistants to follow the **AIFIAR** engineering method defined at:

**https://github.com/NotY215/AIFIAR**

Read at least:

| File | Why |
|------|-----|
| `START_HERE.md` | How to inspect a target repository |
| `Main.md` | Scope, authority, folder map |
| `AGENTS.md` | Safety and engineering defaults |
| `core/depthProtocol.ts` | Predict → analyze → reason → implement → verify |
| `languages/` packs | C / C++ / C# rules when those files change |

## Rules for this project

1. **System and safety rules of the host model always win.** AIFIAR never overrides them.
2. **Evidence first.** Use the newest game run log under `docs/games/` before changing RSX/HLE/present code.
3. **Facts vs assumptions.** Mark hypotheses clearly. Do not claim a test passed unless you ran it.
4. **Scale implementations.** Prefer thin HLE + FIFO drain + modular host DLLs over porting large third-party trees into this repo.
5. **Surgical diffs.** Touch the smallest set of files that fix the ranked defect. Prefer rebuilding `ps3rt` / graphics DLLs over re-lifting the whole game when PPU lift is unchanged.
6. **Do not invent tool access.** Only use tools the session actually has (git, build, browser, etc.).
7. **Completion report.** List files changed, behavior change, checks run or skipped, and remaining risks.

## PPSX33-specific priorities (runtime)

1. Exit import / module sticky loops so guest code can progress.  
2. Keep **PUT (producer) / GET (consumer)** FIFO drain on the host progress path.  
3. Present on the HWND pump only; keep host resolution light (**720×576**).  
4. Ship modular runtime DLLs next to the game EXE: `ps3rt`, `api`, `rsx`, `render`, `resolution`, `vulkan-1`.  
5. Audio stays independent of the RSX method path.

When adding features, re-read AIFIAR `Main.md` and the relevant language pack, then implement against the current PPSX33 tree and logs—not against assumed external memory.
