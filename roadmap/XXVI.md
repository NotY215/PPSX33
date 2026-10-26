# XXVI — Native host UI

**Status:** ACTIVE (2026-10-26)

Replace WebView2 HTML shell with a pure WinForms UI that matches the website design system.

## Why

WebView2 caused:

- `JsonDocument` disposed-object crashes on message marshaling
- WindowsBase version conflicts (WPF package assets)
- Slower cold start and extra runtime dependency

## Subphases

| ID | Status | Work |
|----|--------|------|
| **A** | DONE | Remove WebView2 package and bridge |
| **B** | DONE | Sidebar workflow, logo PNG, cyan ring animation (~30fps timer) |
| **C** | DONE | Tabs (Console/Report/Roadmap/Requirements/Controller), status bar |
| **D** | PLANNED | Further transition polish without blocking recompile work |

## Notes

- Window icon may remain `.ico`; **in-UI logo uses `ppsx33-logo.png`**
- Host builds still use **system Visual Studio** (`redist/` = installer + guide only)
- All workflow actions stay on existing `Native.*` / partial methods
