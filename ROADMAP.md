# PPSX33 roadmap

**Current host version:** 1.1.0-Alpha  

Primary goal: **full PS3 → native Windows recompilation** (lift + HLE + RSX host path). Not every title is playable yet.

## Done (recent)

| Item | Notes |
|------|--------|
| Qt6 host UI | Replaces former C# UI |
| Unified `ps3rt.dll` | No modular api/rsx/render/resolution split; no `-render` workbench |
| Vulkan 1.2 present | Clear + guest RGBA staging; 720×576 default |
| GCM flip/label + control GET@+8 | Display buffer → present color |
| FIFO drain/publish + method classify | Jump/call/return walk |
| Dispatch sink | Method → surface/viewport/flip → present queue |
| cellAudio ring parity | GetPortConfig + guest ring pull → XAudio2 |
| Sticky island table | Shared `ppu_island_table.h` + soft-continue re-rank |
| Version resource + game EXE icon | CMD/console `-version` |
| Shader **bind** path | VP/FP offset track; SPIR-V program body still planned |

## In progress

| Item | Notes |
|------|--------|
| Title FIFO geometry | Real draws beyond assist/clear |
| Sticky recovery under GOW load | Re-test after full patch set |
| SPIR-V from guest ucode | Bind ready; compiler path next |

## Planned

| Item | Notes |
|------|--------|
| Fullscreen SPIR-V blit pipeline | Roadmap carry-over |
| Deeper cellGcmSys / sysutil | Title-driven |
| SPU / SPURS depth | Job assist exists; expand as needed |
| OpenGL 4.3 secondary path | UI may expose later; Vulkan default |

## Non-goals

- Shipping decryption keys or EBOOT decryptors  
- Bulk import of third-party emulator trees into the product tree  
- Claiming “playable” from lift success alone  

## Testing policy

Apply the full post-analysis patch series, then validate end-to-end (host build → game build → run log).
