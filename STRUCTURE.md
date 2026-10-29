# PPSX33 source layout

```
src/
  core/           loaders/, fs/, memory, HLE kernel (former runtime/ps3rt*)
  processor/
    PPU/          lifter, runtime headers, interpreter/JIT surface
    SPU/          interpreter (former spu_stub), local store, JIT future
  RSX/            device (former rsx_stub), FIFO/state/memory/shaders
    Backends/
      DX11/       default
      OpenGL/     future phase 2
      Vulkan/     future phase 3
  games-stuff/
    Audio/        device (former audio_save), drivers, decoders
    Video/        PAMF, demux, codecs, present
  UI/             WinForms GUI (former src/gui) + future C++ panels
```

There is **no** top-level `runtime/` source tree. Prebuilt `ps3rt.dll` and run headers still install under `build/dist/runtime/` for game linking.

Website: https://ppsx33.xo.je/
