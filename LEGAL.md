# PPSX33 — Legal Notice & Project Position

**Last updated:** 2026-10-13

This document describes what PPSX33 is, what it is not, and the conditions under which you may use this repository and software. **Read it before using Load ELF, Decrypt EBOOT, Build, or Run.**

---

## 1. What PPSX33 is

PPSX33 is an **independent research and engineering project**: a PlayStation 3–oriented **recompiler / HLE runtime toolkit** (GUI, core library, and host runtime). It is developed for technical study, interoperability research, and personal experimentation with **lawfully obtained** software and data.

- The **tool identity** is **PPSX33**.
- The **host window title**, **logo branding**, and **generated game host executable name** are **`PPSX33`** / **`PPSX33.exe`**.
- This branding exists so output is clearly a **PPSX33 tool product**, not a retail PlayStation product and not a claim that the author “officially ported” any commercial title as a finished game release.

Sony Interactive Entertainment, PlayStation, and related marks are property of their respective owners. PPSX33 is **not** affiliated with, endorsed by, or sponsored by Sony.

---

## 2. What PPSX33 is not

- **Not** a piracy tool and **not** a distribution channel for game ISOs, packages, EBOOTs, or save data.
- **Not** a substitute for buying or licensing games.
- **Not** an official emulator or official PC port of any Sony title.
- **Not** a product that claims to replace the PlayStation 3 console or PlayStation Network services.

---

## 3. No proprietary keys, no built-in DRM bypass

**PPSX33 does not contain, use, or distribute any proprietary decryption keys, nor does it possess the capability to bypass Digital Rights Management (DRM) or encryption frameworks.**

In plain terms:

| Claim | Status |
|-------|--------|
| Ships Sony/PlayStation decryption keys | **No** |
| Embeds retail disc/HDD encryption secrets | **No** |
| Built-in “one-click” DRM crack engine | **No** |
| Ability to unlock content you do not already have keys/tools for | **No** (by design of this project) |

Any optional workflow that involves an **EBOOT** or encrypted binary relies on **tools and installations you supply yourself** (for example, a path to software you already installed). PPSX33 **does not promote** decrypting protected binaries, does not ship keys, and does not instruct users to obtain unauthorized dumps.

---

## 4. Your content, your responsibility

You must only use PPSX33 with material you **lawfully own or are licensed to use**, and only in ways that comply with:

- Applicable copyright and computer-misuse law in your jurisdiction  
- The terms of use of any platform or store from which you obtained the software  
- Sony / PlayStation terms where they apply to your use of their software and services  

**Examples of expected user conduct**

- Use dumps / ELFs / data derived from **media or installs you own**.  
- Do **not** request, share, or upload copyrighted game binaries in issues, PRs, or public chats.  
- Do **not** redistribute generated executables bundled with game assets, USRDIR data, or decrypted images.  
- Do **not** use PPSX33 to facilitate piracy or unauthorized multiplayer / service access.

**Use at your own risk.** The authors provide the software **as is**, without warranty of any kind.

---

## 5. No promotion of decryption or piracy

The repository and UI:

- **Do not promote** decrypting `EBOOT.BIN` to ELF as a product feature for the general public.  
- **Do not promote** downloading or sharing pirated games.  
- Treat any external decrypt step as **optional, user-supplied, and user-responsible**.  
- Require an on-screen **legal acknowledgment** before Load ELF / Decrypt flows in the GUI.

If you decline the acknowledgment, those actions are cancelled.

---

## 6. Distribution policy

| Allowed | Not allowed |
|---------|-------------|
| Forking / studying this **open repository code** under its license | Redistributing **game data**, ISOs, pkgs, EBOOTs, RAP/RIF, or decrypted retail images |
| Sharing **patches to PPSX33 source** (no game content) | Selling or hosting “pre-ported” commercial games built with this tool |
| Running **PPSX33.exe** locally with **your** owned data | Claiming the result is an official Sony PC port |

Generated `PPSX33.exe` is a **host binary for your local experiments**. It is not a license to copy or sell the underlying game.

---

## 7. Intellectual property

- **PPSX33** code and original assets in this repository are subject to this project’s license (see `LICENSE` if present).  
- **Game code, art, audio, trademarks, and characters** remain the property of their respective rights holders.  
- HLE stubs and recompilation techniques implement **interfacing behavior** for research; they do not transfer ownership of any game.

---

## 8. Why this project can exist as a public research repo

Public technical projects that:

1. ship **no** proprietary keys,  
2. do **not** implement a self-contained DRM circumvention product,  
3. require **user-owned** inputs,  
4. clearly brand output as **tooling** (here: **PPSX33**), and  
5. discourage redistribution of copyrighted game assets  

are positioned as **development / research tools**, not as commercial re-releases of PlayStation software.

**This is not a guarantee that no one will ever dispute your use.** Laws differ by country. Only a qualified attorney can advise on your specific situation. The maintainers are not your lawyers. Misuse (piracy, redistribution of games, trafficking in keys) remains unlawful regardless of this document.

---

## 9. Enforcement against abuse of *this* project

Project maintainers may:

- Close issues/PRs that request or attach copyrighted game files or keys  
- Refuse support for piracy use cases  
- Update this notice as needed  

---

## 10. Acceptance

By using PPSX33 (including clicking **Yes** on the in-app legal dialog), you confirm that:

1. You will use only lawfully obtained materials.  
2. You understand PPSX33 ships **no** proprietary decrypt keys and **no** built-in DRM bypass.  
3. You will **not** distribute game data or claim an official Sony port.  
4. You accept that the host EXE and window are branded **PPSX33**.  
5. You use the software **at your own risk**.

---

*PPSX33 — independent recompiler toolkit. Not affiliated with Sony Interactive Entertainment.*
