namespace PS3Recomp.Gui;

/// <summary>Embedded LEGAL.md — Legal tab uses MarkdownHtml styling.</summary>
public static class EmbeddedLegal
{
    public const string Markdown = @"
# PPSX33 — Legal Notice & Project Position

**Last updated:** 2026-10-28

This document describes what PPSX33 is, what it is not, and the conditions under which you may use this repository and software. **Read it before using Load ELF, Decrypt EBOOT, Build, or Run.**

---

## Summary

- Independent research recompiler / HLE toolkit — **not** affiliated with Sony Interactive Entertainment or any PlayStation brand.
- No bundled decryption keys; optional external RPCS3 decrypt tools are **user-supplied**.
- Current builds load **decrypted ELF** only on the load path.
- Use only **lawfully obtained** materials; do **not** redistribute game data.
- Host branding is **PPSX33** (tooling, not a retail port).

---

## What PPSX33 is

1. An engineering toolkit for studying PS3 executables via static lift + HLE runtime.
2. A host-side presentation path (Vulkan 1.2 default, OpenGL secondary).
3. Open-source research software under the project LICENSE.

## What PPSX33 is not

1. Not an official PlayStation emulator product.
2. Not a service that distributes games, keys, or firmware.
3. Not a warranty of compatibility with any specific title.

---

## User responsibilities

- You are responsible for the legality of any game data you process in your jurisdiction.
- Do not use PPSX33 to infringe intellectual property or violate export rules.
- Reports and logs may contain local paths and addresses — review before publishing.

---

## No warranty

THE SOFTWARE IS PROVIDED ""AS IS"", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.

## Limitation of liability

IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM OR DAMAGES ARISING FROM USE OF THE SOFTWARE.

---

Canonical sources: `docs/legal/LEGAL.md`, `docs/legal/TERMS.md`, and the repository LICENSE.
";
}
