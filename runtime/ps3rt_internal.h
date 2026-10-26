#pragma once
// Runtime modules: ps3rt.cpp (boot/LV2/PRX/GCM), rsx_stub.cpp (RSX/D3D),
// spu_stub.cpp, ppu_runtime_run.h (PPU loop), nid_table.h, ps3rt_stage.h
// Stages: BOOT -> MEM -> GCM -> RSX -> PPU run -> SPU -> EXIT

PS3RT_API void ps3rt_vmx_dispatch(PPUContext* c, unsigned xo11, unsigned vd, unsigned va, unsigned vb, unsigned vc);
