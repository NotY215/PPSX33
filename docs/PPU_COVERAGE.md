# PPSX33 PPU instruction coverage

This document tracks PowerPC PPU instruction families implemented by PPSX33 and the remaining work. Per-game translation counts are recorded separately under [games](games/README.md). Counts represent instruction instances in a specific game's lift report, not the number of distinct PowerPC opcodes.

## Implemented instruction families

- Integer arithmetic, logical operations, shifts, rotates, and mask operations (including `rlwinm`, `rldicl`, `rldicr`, `rldic`, `rldimi`, `rldcl`, `rldcr`, and `rlwnm`)
- Loads and stores, including update and indexed forms and `lmw/stmw`
- Branches, condition-register logic, `mfcr/mtcrf`, and `mfspr/mtspr` for LR/CTR/XER
- Floating-point memory operations, including single/double precision and indexed forms
- A subset of floating-point arithmetic and conversion operations, including `fadd`, `fsub`, `fmul`, `fdiv`, fused operations, `fmr`, `fcmpu`, `frsp`, `fctiwz`, `fabs`, and `fneg`

This list describes implemented families, not complete coverage of every instruction variant or edge case.

## Current game sample: God of War III

Latest recorded lift statistics:

| Metric | Count |
| --- | ---: |
| Instruction instances | 1,285,560 |
| Translated | 1,103,166 |
| Unimplemented | 182,394 |
| Chunks | 157 |
| Translation coverage | 85.81% |

The percentage is calculated as translated instances divided by total instruction instances. See [the GOW3 record](games/GOW3.md) for context. Successful translation does not by itself prove runtime correctness or that the game is playable.

## Known high-priority gaps

The GoW3 report previously highlighted these remaining areas; treat the opcode-family breakdown as an approximate diagnostic until refreshed from the latest lift report:

| Opcode key | Approximate instances | Notes |
| --- | ---: | --- |
| `op=4` | 76k | VMX / AltiVec vector instructions |
| `op=0` | 39k | Illegal instructions or padding in code segments |
| `op=31 xo=103` | 19k | Needs decoding / implementation |
| `op=9` | 13k | Needs decoding / implementation |
| `op=6` | 10k | Needs decoding / implementation |
| Other | — | Remaining FPU variants, atomics, and uncommon XO forms |

## Still to implement or validate

- Complete XER carry, overflow, and summary-overflow behavior
- Reservation-based atomics (`lwarx/stwcx./ldarx/stdcx.`)
- Remaining floating-point operations and rounding behavior (`fres`, `frsqrte`, `fsel`, `mffs`, `mtfsf`, and others)
- VMX / AltiVec instruction support, with a suitable host implementation
- Trap instruction semantics
- Function-level code emission instead of fixed-size chunks
- Re-run and refresh per-game coverage metrics after meaningful lifter changes

## Reporting rules

- Keep instruction-instance counts separate from unique opcode counts.
- Record the source ELF's game name and the date or commit associated with each measurement when known.
- Do not commit game executables, decrypted ELFs, keys, or other copyrighted game data.
