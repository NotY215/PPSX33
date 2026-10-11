# PPSX33 Development Roadmap

Last updated: 2026-10-11  
Current implementation checkpoint: **Phase N complete**  
Next milestone: **Phase O, real import/NID resolution**

PPSX33 is an experimental PlayStation 3 ELF analysis and static PPU lifting/recompilation project. This roadmap records the problem each phase addresses, the implementation logic, why the change was needed, and the remaining limits. A phase marked **Done** means its described code change is present in the repository, not that every PS3 title works or that every heuristic is semantically correct.

## Status overview

| Phase | Status | Main area |
|---|---|---|
| A | Done | Project pipeline and host application foundation |
| B | Done | Recover from invalid PPU program counters and return addresses |
| C | Done | Broaden LV2 system-call/HLE coverage |
| D | Done | PRX import/NID filtering and lookup foundations |
| E | Done | Null-call loop escape and TOC restoration |
| F | Done | Table-walk/null-loop recovery, OPD soft resolution, runtime split |
| G | Done | Guest GCM state, NID filtering, deeper return recovery |
| H | Done | Detect PPU waits and advance basic RSX/GCM progress |
| I | Done | Stabilize SPU load bounds and Phase H build integration |
| J | Done | More forceful progress pokes and wait-loop escape |
| K | Done | Break internal translated-chunk spins through branch handling |
| L | Done | Early link-register blacklisting for repeated bad returns |
| M | Done | Periodic hardware progress from guest memory activity |
| N | Done | Prevent repeated escape to the same failed return target |
| **O** | **Next** | Resolve real NIDs from TOC/OPD and import metadata |
| P | Planned | RSX method decoding and draw-command correctness |
| Q | Planned | SPU instruction execution and scheduling |

---

## Phase A: Establish the project pipeline

**Status: Done**

**Problem:** A recompiler needs a repeatable path from an input ELF to a named project, generated code, host build files, and an executable artifact. Without clear project boundaries, paths and output names can become disconnected from the selected game.

**Implementation logic:**
- Validate that the selected input ELF exists and that the project name is usable.
- Sanitize filesystem-invalid characters in project names.
- Create separate `input/`, `codebase/`, and `output/` directories.
- Preserve the source ELF as `input/EBOOT.elf`.
- Use the native `ps3core` library for ELF/lifting/project work, with CLI and GUI front ends using the core API.
- Place CMake outputs under `build/dist` so the generated native components share a predictable output location.

**Why it was done:** It provides a stable boundary between source data, generated code, and build artifacts, and makes projects easier to inspect and reproduce.

**Remaining limits:** A successful project creation or host build only proves that the pipeline processed its input. It does not prove that generated PPU code is semantically complete or that the game boots.

## Phase B: Recover invalid PPU control flow

**Status: Done**

**Problem:** Translated code can branch or return to an address that is not inside any generated PPU chunk. If the runtime treats every such address as a permanent stop, execution may halt early even when a usable return address remains in the link register, count register, or guest stack.

**Implementation logic:**
- Check whether the current PC belongs to a known translated chunk.
- When it does not, inspect plausible return locations such as LR/CTR and aligned values found on the guest stack.
- Accept a recovered target only if it points into a known chunk.
- Bound and log recovery attempts rather than assuming an arbitrary address is executable.

**Why it was done:** It gives the runtime a way to recover from missing external-call implementations and incomplete control-flow coverage.

**Remaining limits:** A stack value that resembles a code address may be data, and choosing a plausible return can corrupt control flow. Recovery is a diagnostic heuristic, not a substitute for proper call/import resolution.

## Phase C: Expand LV2 system-call/HLE foundations

**Status: Done**

**Problem:** PS3 code frequently depends on LV2 services for process state, memory, threads, synchronization, filesystem operations, SPU setup, PRX handling, and related kernel services. Unhandled calls can return incorrect values, fail to initialize expected structures, or leave guest code waiting indefinitely.

**Implementation logic:**
- Extend the runtime syscall dispatcher with additional service paths.
- Read guest arguments from the expected PPU registers and return status/results through guest registers.
- Use guest-memory bounds checks before reading or writing structures.
- Provide basic allocation and identifier behavior where full kernel semantics are not implemented.

**Why it was done:** Basic HLE coverage allows more guest code to progress far enough to reveal the next missing dependency.

**Remaining limits:** These are partial compatibility implementations. Synthetic IDs, simplified synchronization, incomplete filesystem behavior, and stubbed services may not match real LV2 semantics or timing.

## Phase D: PRX imports and NID filtering

**Status: Done**

**Problem:** Calls into unresolved PRX imports may land at synthetic or unresolved import addresses. In addition, ordinary pointer-like values can resemble NIDs, producing false import matches and incorrect dispatch.

**Implementation logic:**
- Expand the NID lookup table and PRX import detection paths.
- Filter values that look like guest addresses rather than genuine import identifiers.
- Provide controlled import-stub behavior when a target cannot yet be resolved.
- Preserve logs for unresolved import paths so missing services can be identified.

**Why it was done:** Incorrectly treating pointers as NIDs can dispatch the wrong HLE function; failing to handle an unresolved import can leave execution at a non-code address.

**Remaining limits:** A larger table and filtering rules do not provide complete PRX metadata. Accurate resolution still requires real import/export information and correct TOC/OPD interpretation. Phase O addresses that gap.

## Phase E: Break null-call loops and restore TOC

**Status: Done**

**Problem:** A missing import or invalid call target can repeatedly return execution to a null-like address. PPU code also relies on register r2 as the table-of-contents (TOC) pointer; if it becomes zero after an escape, later global-data accesses can fail.

**Implementation logic:**
- Track repeated null-like exits and soft-break counts.
- Search alternate stack return candidates and, when appropriate, try nearby return positions.
- Restore the entry TOC if r2 becomes zero.
- Add logging around loop escape decisions.

**Why it was done:** It prevents some immediate hangs and avoids continuing with an obviously lost TOC pointer.

**Remaining limits:** Nearby-address skipping is not a true return operation and may skip instructions or land in data. Restoring the entry TOC is only a fallback; functions may legitimately use different TOCs. Phase F and Phase N refine recovery, but proper import resolution is preferred.

## Phase F: Harden table-walk recovery and split the runtime

**Status: Done**

**Problem:** Some repeated null-call sites are table-walk or import-thunk patterns. The original loop breaker could revisit the same small region, fail to escape a larger table, or make the runtime header difficult to maintain. A split-header refactor also risks breaking generated projects if the build driver copies only the old header.

**Implementation logic:**
- Treat a small PC window as one repeated null-call site.
- Increase skip distance gradually, from small strides through larger table regions, after repeated failures.
- Clear stale sticky-return state after repeated unsuccessful escapes.
- Attempt soft OPD resolution by reading an entry point and TOC from guest memory when a plausible OPD is available.
- Split the PPU runtime into `ppu_runtime.h`, `ppu_runtime_core.h`, `ppu_runtime_core_a.h`, `ppu_runtime_core_b.h`, and `ppu_runtime_run.h`.
- Update `src/core/project.cpp` so all required runtime headers are staged into generated output.

**Why it was done:** It targets loops that survive a single return recovery and keeps runtime helpers manageable while preserving the generated-project build.

**Remaining limits:** Aggressive table skipping can bypass valid work. Soft OPD resolution is only as reliable as the pointer and descriptor checks. The split headers are a single logical runtime and must be shipped together. See [the historical Phase F note](PHASE_F.md) and [runtime architecture guide](runtime/README.md).

## Phase G: Add guest GCM state and stronger return recovery

**Status: Done**

**Problem:** RSX/GCM-related guest code expects valid control-register and label areas, while unresolved NID dispatch and repeated null-loop return targets can still cause bad jumps or endless recovery cycles.

**Implementation logic:**
- Allocate and initialize guest memory for basic `cellGcmSys` control-register and label state.
- Add handlers for selected control-register, label-address, initialization, flip-status, and main-memory mapping operations.
- Filter pointer-like values from NID matching.
- Scan a wider guest-stack range when recovering a return.
- Track exhausted link-register targets and avoid repeatedly selecting a known failing target.

**Why it was done:** Basic GCM state lets guest code interact with expected structures, and deeper return recovery offers alternatives when the nearest stack values are not useful.

**Remaining limits:** The GCM state and handlers are partial HLE, not a complete RSX driver. Wider stack scans increase the chance of selecting a false return. See [runtime architecture](runtime/README.md).

## Phase H: Detect waits and add hardware progress

**Status: Done**

**Problem:** Guest code may wait for RSX/GCM work to complete. A pure translated CPU loop cannot make progress if the simulated hardware state that the loop is polling never changes.

**Implementation logic:**
- Detect repeated PPU PCs and wait-like cases such as PC matching LR.
- Add `ps3rt_progress_hw()` as a runtime helper.
- When the GCM control block is valid, inspect PUT/GET state and advance the basic emulated command position when work is pending.
- Connect the helper to the partial RSX flush/present path and provide bounded diagnostic logging.

**Why it was done:** It introduces a minimal way for guest wait loops to observe hardware-side progress instead of spinning forever on unchanged state.

**Remaining limits:** Advancing GET toward PUT is a simplified progress model. It does not mean every RSX command was decoded or executed correctly, and automatic flip/present behavior is not equivalent to real GPU synchronization.

## Phase I: Stabilize SPU load bounds and build integration

**Status: Done**

**Problem:** SPU context and image-loading foundations are needed by guest runtime paths, but incorrect load-size handling or inconsistent declarations can break builds or write beyond the local store. Splitting helpers can also cause duplicate type definitions or missing symbols.

**Implementation logic:**
- Clamp SPU image copies to the available local-store capacity.
- Keep the SPU context definition in one header to avoid duplicate definitions.
- Repair declarations and runtime integration needed by the Phase H progress helper.
- Preserve the hardware-progress path while fixing the build.

**Why it was done:** Runtime features are not useful if the project cannot compile consistently or a malformed image size can overrun the target buffer.

**Remaining limits:** SPU allocation, image loading, run/stop, mailbox, and basic MFC/DMA paths are foundations, not a complete SPU instruction interpreter or scheduler.

## Phase J: Increase progress pokes and escape PC/LR waits

**Status: Done**

**Problem:** Some waits occur inside translated chunks, so the external-chunk recovery path never runs. A guest can remain at a PC equal to LR, while GCM label waits remain blocked even though the host is still executing instructions.

**Implementation logic:**
- Invoke hardware progress more aggressively from detected wait/spin paths.
- Synchronize the simplified GCM PUT/GET state.
- Clear a bounded label region as a heuristic to release some label-polling waits.
- Add a forced forward step for persistent PC/LR waits and clear the relevant return register to reduce immediate repetition.

**Why it was done:** The earlier helper ran only in limited circumstances and could not release all internal waits.

**Remaining limits:** Clearing labels or forcing PC forward can violate guest synchronization and skip required work. This phase is a liveness workaround, not a correctness implementation. These behaviors need per-title traces and targeted tests before being trusted.

## Phase K: Break internal translated-chunk spins

**Status: Done**

**Problem:** A branch can repeatedly choose the same destination inside a translated chunk. Since the PC never leaves the chunk, external-call recovery cannot detect the stall.

**Implementation logic:**
- Add a branch-condition helper (`bc_taken`) that can observe branch decisions and participate in the progress logic.
- Connect the branch path to hardware progress checks.
- Correct declarations, including the forward declaration needed before use.

**Why it was done:** Detecting only the outer dispatch loop misses loops that are entirely represented by generated code.

**Remaining limits:** Branch-level intervention can alter valid guest loops. It must be limited to clearly detected stalled cases and validated against the PowerPC condition-register/CTR semantics.

## Phase L: Ban repeated bad link-register targets early

**Status: Done**

**Problem:** When a recovered LR target repeatedly returns to a null or stalled site, waiting for many later retries wastes time and can produce another recovery cycle.

**Implementation logic:**
- Record link-register targets implicated in repeated null/sticky-return behavior.
- Add those targets to a bounded ban list earlier in the recovery sequence.
- Prefer an alternate stack-derived target or hardware-progress path instead of choosing a known failing LR again.

**Why it was done:** Phase L reduces repeated attempts at a return address that has already shown itself unproductive.

**Remaining limits:** An LR can be valid in a different context. Bans are runtime heuristics and should be logged, bounded, and revisited as proper call/import resolution improves.

## Phase M: Trigger hardware progress from memory activity

**Status: Done**

**Problem:** A guest can perform a large amount of memory polling or ordinary lifted memory traffic without revisiting a wait detector that triggers hardware progress. The host then continues executing while the emulated hardware state remains stale.

**Implementation logic:**
- Add `ps3rt_touch()` as a hook reached by lifted guest memory operations.
- Count memory touches and periodically call `ps3rt_progress_hw()` rather than doing the heavier progress work on every access.
- Lower selected spin/recovery thresholds so a detected stall is handled sooner.
- Keep logging bounded to avoid flooding output.

**Why it was done:** Memory activity becomes another opportunity to advance the partial hardware model, including when the guest polls a state field through repeated loads.

**Remaining limits:** Periodic progress is heuristic and may add overhead. The memory hook must be present in generated code to help; the touch counter does not itself prove that a particular guest wait is safe to release.

## Phase N: Prevent last-good escape ping-pong

**Status: Done**

**Problem:** A recovery could escape to a remembered `last_good_ret`, return to a null loop, and then choose the same remembered target again. This creates an `escape -> last_good -> null -> escape` cycle that looks active but makes no useful progress.

**Implementation logic:**
- Track `failed_escape` and refuse to select that address again immediately.
- Count failed escapes; after repeated failure, blacklist the target and clear stale `last_good_ret`.
- Prefer stack-based recovery before reusing a remembered target.
- Promote `last_good_ret` only from non-null, non-import return paths.
- Keep a bounded ban list for LR targets and avoid known failing targets.
- For persistent internal spins, periodically poke the hardware model, attempt a different recovered PC, and use a final forced step only in the defined PC/LR wait case.
- Emit Phase N diagnostics so repeated targets and escape choices can be recognized.

**Why it was done:** Phase N addresses the observed repeated escape to the same address (for example, `0x336588`) and makes recovery state reflect previously failed attempts rather than treating every retry as new.

**Remaining limits:** A forced step or alternate return can still break program semantics. The key success criterion is not merely that the log changes address; the guest must make meaningful progress and produce consistent results. Validate against the same input ELF and retain the log.

---

## Phase O: Resolve real NIDs using TOC/OPD and import metadata

**Status: Next**

**Problem to solve:** Current NID filtering and import stubs can keep execution moving, but they do not guarantee that a call resolves to the correct PRX export. Incorrect imports can return the wrong values or silently corrupt state.

**Planned implementation:**
- Parse available import/export and PRX metadata instead of relying primarily on string hits or a broad NID table.
- Resolve NIDs against known module/library identity where metadata is available.
- Validate candidate OPDs, including entry-point alignment, guest-memory bounds, executable-range membership, and TOC plausibility.
- Keep unresolved imports explicit and distinguish a real resolved call from a soft fallback.
- Add tests for false positives, missing exports, invalid descriptors, and pointer values that resemble NIDs.

**Completion criteria:** A reproducible test fixture resolves known imports to expected module/function entries, rejects invalid OPDs, and reports unresolved imports without silently treating them as verified.

## Phase P: RSX method decoding and draw correctness

**Status: Planned**

**Problem to solve:** Advancing GCM PUT/GET or presenting a frame is not the same as interpreting the RSX command stream. Unknown methods, incorrect state transitions, surfaces, textures, shaders, and draw arguments can prevent rendering or produce incorrect output.

**Planned implementation:**
- Decode command headers and method payloads with bounds checks.
- Maintain RSX state for supported methods rather than skipping arbitrary command ranges.
- Implement and test a documented subset of surface, viewport, texture, shader, and draw operations.
- Track unsupported methods and invalid command buffers with explicit diagnostics.
- Build small synthetic command streams with known expected state transitions and output.

**Completion criteria:** Supported methods have deterministic unit tests and renderer tests. Unsupported methods remain visible in logs. Do not claim full RSX or commercial-game compatibility from a successful present call.

## Phase Q: SPU instruction execution and scheduling

**Status: Planned**

**Problem to solve:** Current SPU context, local store, load/run/stop, mailbox, and DMA/MFC APIs do not execute a complete SPU program. Titles that depend on SPU work may wait forever or consume incomplete output.

**Planned implementation:**
- Define a testable SPU instruction execution loop and PC/branch semantics.
- Add instruction families incrementally with correct vector and scalar behavior.
- Implement local-store bounds, mailbox/signal behavior, and DMA ordering checks.
- Introduce explicit SPU scheduling and synchronization with PPU and RSX work.
- Track unsupported instructions and validate against small reference fixtures before adding larger workloads.

**Completion criteria:** Instruction tests cover supported opcodes, state transitions, and edge cases; scheduling tests demonstrate deterministic mailbox/DMA progress. Do not mark the SPU complete based solely on API stubs or successful context creation.

---

## Cross-phase validation policy

A phase is considered implemented when its code and focused tests are present, but it is not considered behaviorally validated until its effects are reproducible.

For runtime/control-flow changes:
1. Build from a clean checkout using the documented toolchain.
2. Run the synthetic pipeline smoke test.
3. Re-run the same ELF/input that exposed the issue.
4. Save the complete runtime log and compare PCs, recovery targets, and progress counters.
5. Confirm the change avoids the old failure without introducing a new loop or invalid state.
6. Record exact input/build provenance and distinguish static-lift counts from execution correctness.

For game reports, **100% translated instructions means only that the current lifting pass emitted output for every counted instruction**. It does not prove semantic correctness, import completeness, RSX/SPU support, bootability, rendering, or playability.

## Developer workflow

From a Windows checkout:

```bat
git pull origin main
```

Rebuild the solution using the repository's documented Visual Studio/CMake setup, then run the relevant smoke test and focused input. Preserve the logs when reporting a regression. Do not use a forced PC advance or a recovery log line alone as evidence that the title is working.

## Related documentation

- [Runtime architecture](runtime/README.md)
- [Developer guide](docs/DEVELOPER_GUIDE.md)
- [PPU lifting coverage](docs/PPU_COVERAGE.md)
- [Game analysis and lift reports](docs/games/README.md)
- [Phase F historical notes](PHASE_F.md)
