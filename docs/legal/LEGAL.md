# PPSX33 Legal Notice and Project Information

**Last updated:** 2026-11-01  
**Project:** PPSX33  
**Repository:** [github.com/NotY215/PPSX33](https://github.com/NotY215/PPSX33)  
**Website:** [ppsx33.xo.je](https://ppsx33.xo.je/)

This document explains the project's identity, licensing boundaries, treatment of game-related materials, and important legal limitations. It is informational and is not legal advice. Read it with [TERMS.md](TERMS.md), [PRIVACY.md](PRIVACY.md), [THIRD_PARTY.md](THIRD_PARTY.md), [NOTICE](NOTICE), and the root [LICENSE](../../LICENSE).

## 1. Project identity

PPSX33 is an independent, open-source engineering project focused on researching and developing a native Windows PlayStation 3 recompiler and the runtime services needed by generated builds.

The project is not:

- An official Sony or PlayStation product.
- Affiliated with, sponsored by, or endorsed by Sony Interactive Entertainment, Sony Group, or any game publisher.
- A source of commercial PlayStation games, firmware, proprietary keys, or licensed game content.
- A promise that any particular game is supported, bootable, or playable.

"PlayStation", "PlayStation 3", "PS3", and other third-party names or marks are used only to identify the platform and technologies discussed by the project. All such marks remain the property of their respective owners.

## 2. Copyright and project license

2.1. The PPSX33 source code and other repository materials expressly covered by the project license are distributed under the **Apache License, Version 2.0**. Read the complete [LICENSE](../../LICENSE) file for the binding license terms.

2.2. The Apache License 2.0 grants the permissions stated in that license, subject to its conditions. It includes copyright and patent license provisions, redistribution requirements, a trademark limitation, and warranty and liability provisions.

2.3. The project license does not grant rights to third-party game code, game assets, firmware, console software, trademarks, proprietary SDKs, or other materials merely because PPSX33 can read, reference, process, or generate output from them.

2.4. Third-party dependencies and assets retain their own copyright and license terms. A project-level license does not relicense them. Review the relevant license and notices before redistribution.

2.5. Contributions accepted into the project are handled under the contribution terms described in [CONTRIBUTING.md](../../CONTRIBUTING.md) and the applicable project license, unless a separate written agreement states otherwise.

## 3. Game data and user-supplied materials

PPSX33 is intended to process input supplied by the user. The user remains responsible for the provenance, authorization, and lawful use of all input materials.

This includes, where applicable:

- Decrypted ELF files and other executable inputs.
- Game data directories and extracted files.
- Firmware, system modules, libraries, and symbols.
- Textures, audio, images, video, fonts, and other assets.
- Save data, logs, memory captures, and generated project files.

The project does not provide a license to use any commercial game or console content. Do not commit or publish game binaries, firmware, encryption keys, decrypted game files, proprietary assets, or other protected material unless you have the necessary rights and authorization.

A public repository, a successful download, or the technical ability to process a file does not establish permission to use or redistribute that file.

## 4. Decryption, interoperability, and reverse engineering

The current documented ELF loader expects a supported decrypted ELF64 big-endian PowerPC input. PPSX33 does not provide proprietary encryption keys or grant permission to access protected content.

Some project documentation or builds may describe optional interaction with external tools. Those tools are separate products, are not made part of PPSX33 by reference, and remain governed by their own licenses and terms. Their availability or use does not establish that a particular operation is authorized.

Rules governing interoperability, preservation, reverse engineering, decryption, technological protection measures, and personal backups vary by country and may depend on specific facts and exceptions. This project does not determine whether a user's conduct is lawful. You must check the law that applies to you and obtain qualified advice when necessary.

## 5. Generated code and executable output

PPSX33 may translate or otherwise incorporate information from user-provided inputs into generated source code, executables, reports, and other artifacts.

- Generated output is not automatically free of third-party rights.
- The Apache License 2.0 applies only to material it covers. It does not grant a license to embedded or reproduced third-party content.
- You are responsible for examining generated files before publishing, sharing, selling, or distributing them.
- Do not distribute generated files that contain protected game code or assets unless you have the necessary authorization.
- A successful lift, build, or launch is a technical result, not a legal conclusion or compatibility certification.

## 6. Technical status and no compatibility promise

PPSX33 is actively developed. PPU translation, runtime services, SPU behavior, RSX/GCM graphics, system-library coverage, and end-to-end title compatibility may be incomplete or experimental.

Project documentation and roadmap entries describe engineering status and available evidence. They are not a warranty, service-level commitment, or promise of future functionality.

In particular:

- Recognizing or translating instructions does not prove that their behavior is correct.
- A successful compile does not prove that a generated executable will run correctly.
- A visible window or diagnostic/assist draw does not prove that a game's own graphics commands are supported.
- A game report must be read with its stated test conditions and limitations.
- Compatibility claims should be based on reproducible end-to-end evidence, not instruction counts alone.

## 7. Third-party software, dependencies, and assets

PPSX33 may use, reference, or interoperate with third-party development tools, compilers, SDKs, graphics APIs, drivers, libraries, or optional applications. Their names and marks identify the relevant technologies only.

Each third-party component remains subject to its own license, notices, terms, and redistribution rules. Users and distributors must review the versions actually used by their build or release. The project documentation is not a substitute for the license text supplied by the component's owner.

The project does not claim ownership of third-party marks or imply that their owners approve, support, or endorse PPSX33.

See [THIRD_PARTY.md](THIRD_PARTY.md) for the project's third-party inventory notes. That inventory may need to be checked against the exact release and its dependency manifest.

## 8. Privacy and diagnostic information

The repository's stated design does not intentionally implement a PPSX33 account system, advertising profile, or project-operated analytics service. PPSX33 processes local inputs and outputs selected or created by the user. This statement is limited to the project code and does not describe every third-party tool, operating-system service, driver, hosting provider, modified build, or optional integration.

Input paths, reports, logs, generated projects, and crash information may disclose local usernames, directory structures, file names, symbols, module names, or content derived from user-supplied files.

Review diagnostic material before sharing it. Remove secrets, tokens, private keys, personal information, and unauthorized game data. GitHub and other external services process information under their own privacy policies.

See [PRIVACY.md](PRIVACY.md) for further detail.

## 9. Security and safe handling

Treat unknown input files and generated executables as untrusted. Use backups and appropriate system security controls. Do not run content if you do not trust its origin or understand the risks.

Security issues should be reported using the private vulnerability reporting route described in [SECURITY.md](../../SECURITY.md), where available. Do not publicly disclose exploitable details or upload confidential game files, keys, or personal data as part of a report.

## 10. No warranty and limitation of liability

PPSX33 is provided under the terms of the root [Apache License 2.0](../../LICENSE). The license includes the applicable warranty disclaimer and limitation of liability for the work it covers.

To the maximum extent permitted by applicable law, the project makes no additional promise that the software will be accurate, uninterrupted, secure, compatible with a particular title, or fit for a particular purpose. Nothing in this notice excludes rights or liabilities that cannot lawfully be excluded.

This document does not amend or replace the Apache License 2.0.

## 11. Legal documents

| Document | Purpose |
| --- | --- |
| [TERMS.md](TERMS.md) | Conditions and responsibilities for using PPSX33 |
| [PRIVACY.md](PRIVACY.md) | Project privacy and diagnostic-data notes |
| [THIRD_PARTY.md](THIRD_PARTY.md) | Third-party software and asset notes |
| [NOTICE](NOTICE) | Project copyright and attribution notice |
| [LICENSE](../../LICENSE) | Apache License 2.0 |
| [SECURITY.md](../../SECURITY.md) | Security reporting policy |
| [CONTRIBUTING.md](../../CONTRIBUTING.md) | Contribution workflow and contribution terms |

## 12. Contact and legal advice

For project-specific questions, use the repository's public issue tracker for non-sensitive matters. Do not post private legal documents, personal information, game dumps, keys, or confidential material in public.

For advice about copyright, reverse engineering, preservation, circumvention, export rules, or other legal questions, consult a qualified professional in the relevant jurisdiction. The project maintainers cannot determine your legal rights or provide legal advice about your particular use.
