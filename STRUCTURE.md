# PPSX33 source layout

```
src/core/          Host tools: ELF, lifter, project API
src/UI/            GUI target path (sources still in src/gui until path flip)
src/gui/           Current WinForms project
Processor/PPU/     PPU module surface
Processor/SPU/     SPU / MFC / SPURS surface
GPU/               RSX HAL, memory, shaders
GPU/dx11/          Default Direct3D 11 backend
GPU/gl/            Next OpenGL backend
GPU/vk/            Future Vulkan backend
Game-Stuff/Audio/  cellAudio, mix, codecs, host backends
Game-Stuff/Video/  PAMF, demux, decode, present, AV sync
runtime/           Active ps3rt bodies (rsx_stub, spu_stub, audio_save) until absorbed
```

**Migration:** new code in module folders. Monoliths stay linked until each symbol moves. Assist FIFO is probe-only; title PUT required for real scenes.

Site: https://ppsx33.xo.je/
