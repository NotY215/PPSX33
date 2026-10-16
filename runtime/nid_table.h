// Real PS3 module NID table (common cell*/sys_* exports).
// Expanded for GOW3, Demon's Souls, Uncharted 2 PRX hits.
#pragma once
#include <cstdint>
#include <cstring>

struct NidEntry {
    uint32_t    nid;
    const char* module;
    const char* name;
    int         kind; // 0 generic, 1 gcm, 2 fs, 3 sysmodule, 4 audio, 5 spurs, 6 np, 7 sysutil, 8 io, 9 vdec, 10 game, 11 save
};

static const NidEntry kNidTable[] = {
    // --- cellGcmSys (XX-I full HLE table) ---
    { 0x15BAE46B, "cellGcmSys", "cellGcmInitBody", 1 },
    { 0x4524FE95, "cellGcmSys", "cellGcmGetConfiguration", 1 },
    { 0x55A14C05, "cellGcmSys", "cellGcmGetControlRegister", 1 },
    { 0x5F909B17, "cellGcmSys", "cellGcmGetLabelAddress", 1 },
    { 0x21AC3697, "cellGcmSys", "cellGcmAddressToOffset", 1 },
    { 0x2A6FBA9C, "cellGcmSys", "cellGcmOffsetToAddress", 1 },
    { 0x63387071, "cellGcmSys", "cellGcmGetDisplayBuffer", 1 },
    { 0x21397818, "cellGcmSys", "cellGcmSetDisplayBuffer", 1 },
    { 0xA547ADDE, "cellGcmSys", "cellGcmGetFlipStatus", 1 },
    { 0xB2E761D4, "cellGcmSys", "cellGcmResetFlipStatus", 1 },
    { 0xD81D0D2D, "cellGcmSys", "cellGcmSetFlip", 1 },
    { 0x05E548B9, "cellGcmSys", "cellGcmSetFlipCommand", 1 },
    { 0xF80196C0, "cellGcmSys", "cellGcmSetWaitFlip", 1 },
    { 0xDC09357B, "cellGcmSys", "cellGcmSetFlipMode", 1 },
    { 0x0E4F32E0, "cellGcmSys", "cellGcmSetFlipImmediate", 1 },
    { 0xA53D12AE, "cellGcmSys", "cellGcmSetFlipWithWaitLabel", 1 },
    { 0x72A577CE, "cellGcmSys", "cellGcmGetFlipStatus2", 1 },
    { 0x0F9BB8AA, "cellGcmSys", "cellGcmSetDefaultCommandBuffer", 1 },
    { 0xA53D12AF, "cellGcmSys", "cellGcmSetDefaultFifoSize", 1 },
    { 0x9DC3136B, "cellGcmSys", "cellGcmGetCurrentDisplayBufferId", 1 },
    { 0x983FB9AA, "cellGcmSys", "cellGcmSetSecondVFrequency", 1 },
    { 0x4AE8D215, "cellGcmSys", "cellGcmSetVBlankFrequency", 1 },
    { 0xA114EC67, "cellGcmSys", "cellGcmMapMainMemory", 1 },
    { 0xA547ADDF, "cellGcmSys", "cellGcmMapEaIoAddress", 1 },
    { 0x3A33F0FD, "cellGcmSys", "cellGcmMapEaIoAddressWithFlags", 1 },
    { 0x05AE417E, "cellGcmSys", "cellGcmUnmapEaIoAddress", 1 },
    { 0xDB23DA22, "cellGcmSys", "cellGcmUnmapIoAddress", 1 },
    { 0x6340CECC, "cellGcmSys", "cellGcmMapLocalMemory", 1 },
    { 0x21AC3698, "cellGcmSys", "cellGcmIoOffsetToAddress", 1 },
    { 0x6571A277, "cellGcmSys", "cellGcmGetLastFlipTime", 1 },
    { 0xD34B796E, "cellGcmSys", "cellGcmGetLastSecondVTime", 1 },
    { 0x0FB8471B, "cellGcmSys", "cellGcmGetVBlankCount", 1 },
    { 0xCFC88E2D, "cellGcmSys", "cellGcmSetDebugOutputLevel", 1 },
    { 0xBC982946, "cellGcmSys", "cellGcmSetTile", 1 },
    { 0x9A0159AF, "cellGcmSys", "cellGcmSetTileInfo", 1 },
    { 0xBD2F6DD7, "cellGcmSys", "cellGcmBindTile", 1 },
    { 0xD0B1D189, "cellGcmSys", "cellGcmUnbindTile", 1 },
    { 0x9A7920AA, "cellGcmSys", "cellGcmSetZcull", 1 },
    { 0xD34B796F, "cellGcmSys", "cellGcmBindZcull", 1 },
    { 0xA75640E8, "cellGcmSys", "cellGcmUnbindZcull", 1 },
    { 0x527C6439, "cellGcmSys", "cellGcmSetInvalidateTile", 1 },
    { 0xE4F1324B, "cellGcmSys", "cellGcmSetUserHandler", 1 },
    { 0xFA981D20, "cellGcmSys", "cellGcmSetUserCommand", 1 },
    { 0x8C7FF6FE, "cellGcmSys", "cellGcmSetFlipHandler", 1 },
    { 0x4D7CE993, "cellGcmSys", "cellGcmSetGraphicsHandler", 1 },
    { 0xA41EF5C6, "cellGcmSys", "cellGcmSetQueueHandler", 1 },
    { 0xFFEB4B8C, "cellGcmSys", "cellGcmSetVBlankHandler", 1 },
    { 0x6B91DB6A, "cellGcmSys", "cellGcmCursorEnable", 1 },
    { 0xC5F1D5E3, "cellGcmSys", "cellGcmCursorDisable", 1 },
    { 0x1BBD6B5C, "cellGcmSys", "cellGcmCursorSetImage", 1 },
    { 0x6CD0F95D, "cellGcmSys", "cellGcmCursorSetPosition", 1 },
    { 0x069704B6, "cellGcmSys", "cellGcmSetPrepareFlip", 1 },
    { 0xACEDC629, "cellGcmSys", "cellGcmGetReportDataAddressLocation", 1 },
    { 0x99D397B2, "cellGcmSys", "cellGcmGetReportDataAddress", 1 },
    { 0x5A41C10F, "cellGcmSys", "cellGcmGetTimeReportLocation", 1 },
    { 0x3B9BD5BD, "cellGcmSys", "cellGcmSysGetLastError", 1 },
    { 0x718BF5F8, "cellFs", "cellFsOpen", 2 },
    { 0x4D5B9EF2, "cellFs", "cellFsRead", 2 },
    { 0xECDCF2AB, "cellFs", "cellFsWrite", 2 },
    { 0x2CB51F0F, "cellFs", "cellFsClose", 2 },
    { 0xA397D042, "cellFs", "cellFsLseek", 2 },
    { 0x7F13FC8C, "cellFs", "cellFsStat", 2 },
    { 0xEF3EFA34, "cellFs", "cellFsFstat", 2 },
    { 0x63FF6D4A, "cellSysmodule", "cellSysmoduleInitialize", 3 },
    { 0x3A495ADE, "cellSysmodule", "cellSysmoduleLoadModule", 3 },
    { 0x63A2A3C3, "cellSysmodule", "cellSysmoduleUnloadModule", 3 },
    // cellAudio
    { 0x0B168F92, "cellAudio", "cellAudioInit", 4 },
    { 0x566966C0, "cellAudio", "cellAudioCreatePort", 4 },
    { 0x5B1E2C73, "cellAudio", "cellAudioPortOpen", 4 },
    { 0x65C27676, "cellAudio", "cellAudioPortStart", 4 },
    { 0xDDC2AB95, "cellAudio", "cellAudioPortStop", 4 },
    { 0x4129FE2A, "cellAudio", "cellAudioQuit", 4 },
    { 0xC20EE39D, "cellAudio", "cellAudioSetNotifyEventQueue", 4 },
    { 0x2B9BD9AD, "cellAudio", "cellAudioAddData", 4 },
    { 0x377E0CD9, "cellAudio", "cellAudioAdd2chData", 4 },
    { 0x07ADD381, "cellAudio", "cellAudioGetPortConfig", 4 },
    { 0x2BEF9CDE, "cellAudio", "cellAudioCreateNotifyEventQueue", 4 },
    // cellSaveData (PS3-native only — no emulator savestates)
    { 0xFCEC7CDC, "cellSaveData", "cellSaveDataList", 11 },
    { 0x273D116A, "cellSaveData", "cellSaveDataListAuto", 11 },
    { 0x52AAC4FA, "cellSaveData", "cellSaveDataFixed", 11 },
    { 0xE7FA817A, "cellSaveData", "cellSaveDataAutoSave2", 11 },
    { 0x8B7ED64B, "cellSaveData", "cellSaveDataAutoLoad2", 11 },
    { 0x2141DBDA, "cellSaveData", "cellSaveDataListSave2", 11 },
    { 0x1DFBFDD6, "cellSaveData", "cellSaveDataListLoad2", 11 },
    { 0x4A69CE7D, "cellSaveData", "cellSaveDataFixedSave2", 11 },
    { 0x3855058B, "cellSaveData", "cellSaveDataFixedLoad2", 11 },
    { 0xF8A35856, "cellSaveData", "cellSaveDataDelete2", 11 },
    { 0x6AC93485, "cellSaveData", "cellSaveDataUserListSave2", 11 },
    { 0x91F2FDB1, "cellSaveData", "cellSaveDataUserListLoad2", 11 },
    { 0xAC6B8901, "cellSpurs", "cellSpursInitialize", 5 },
    { 0xCAFFF3C0, "cellSpurs", "cellSpursFinalize", 5 },
    { 0x5EF96465, "cellSpurs", "cellSpursAttachLv2EventQueue", 5 },
    { 0x82A1FDB2, "cellSpurs", "cellSpursDetachLv2EventQueue", 5 },
    { 0x7CB33C2E, "cellSpurs", "cellSpursCreateJobChain", 5 },
    { 0x0EB323C0, "cellSpurs", "cellSpursJoinJobChain", 5 },
    { 0x2C9B9C1C, "cellSpurs", "cellSpursRunJobChain", 5 },
    { 0x39C173FB, "cellSpurs", "cellSpursShutdownJobChain", 5 },
    { 0xB9BC6207, "cellSpurs", "cellSpursAddWorkload", 5 },
    { 0x0E54C3A6, "cellSpurs", "cellSpursRemoveWorkload", 5 },
    { 0x1D46DEA1, "cellSpurs", "cellSpursStartWorkload", 5 },
    { 0xFF7A7BD2, "cellSpurs", "cellSpursStopWorkload", 5 },
    { 0xC2AC4828, "cellSpurs", "cellSpursWaitForWorkload", 5 },
    { 0x4DC1DEE4, "cellSpurs", "cellSpursCreateTaskset", 5 },
    { 0xA839A4D9, "cellSpurs", "cellSpursJoinTaskset", 5 },
    { 0x52CC6C82, "cellSpurs", "cellSpursCreateTask", 5 },
    { 0xE717AC73, "cellSpurs", "cellSpursSendSignal", 5 },
    { 0x16394A4A, "cellSpurs", "cellSpursGetInfo", 5 },
    { 0x8F788547, "sysPrxForUser", "sys_spu_image_close", 0 },
    { 0xB9959ADF, "sysPrxForUser", "sys_spu_image_get_information", 0 },
    { 0xEBE5F72F, "sysPrxForUser", "sys_spu_image_import", 0 },
    { 0xE0DA3BD3, "sysPrxForUser", "sys_spu_thread_initialize", 0 },

    { 0xCFAD36C9, "cellSysutil", "cellSysutilRegisterCallback", 7 },
    { 0x189A74DA, "cellSysutil", "cellSysutilCheckCallback", 7 },
    { 0x0BAE8772, "cellVideoOut", "cellVideoOutConfigure", 7 },
    { 0xE558748D, "cellVideoOut", "cellVideoOutGetState", 7 },
    { 0xBD28FDB1, "sceNp", "sceNpInit", 6 },
    { 0x1C936E6D, "sys_io", "cellPadInit", 8 },
    { 0x3AAAD464, "sys_io", "cellPadGetData", 8 },
    { 0x0D5F2C14, "sys_io", "cellPadEnd", 8 },
    { 0xA703A51D, "sys_io", "cellPadGetInfo2", 8 },
    { 0x3F797DFF, "sys_io", "cellPadClearBuf", 8 },
    { 0xDA9883AA, "cellGame", "cellGameDataCheck", 10 },
    { 0xB7A0F6C8, "cellGame", "cellGameBootCheck", 10 },
    { 0xA45362F6, "sysPrxForUser", "sys_initialize_tls", 0 },

    // --- O: expanded library stubs (return success / no-op) ---
    { 0x2C03F51C, "cellSysutilRegisterCallback", "cellSysutil", 0 },
    { 0x9D98AFA0, "cellSysutilUnregisterCallback", "cellSysutil", 0 },
    { 0x189A74DA, "cellSysutilCheckCallback", "cellSysutil", 0 },
    { 0x40E895D3, "cellSysutilGetSystemParamInt", "cellSysutil", 0 },
    { 0x938013A3, "cellSysutilGetSystemParamString", "cellSysutil", 0 },
    { 0xCFDD8E87, "cellSysmoduleLoadModule", "cellSysmodule", 0 },
    { 0xA193EA16, "cellSysmoduleUnloadModule", "cellSysmodule", 0 },
    { 0x5A59E273, "cellSysmoduleIsLoaded", "cellSysmodule", 0 },
    { 0x3A52CF82, "cellSysmoduleInitialize", "cellSysmodule", 0 },
    { 0x63FF6C11, "cellPadInit", "cellPad", 0 },
    { 0xA703A51D, "cellPadEnd", "cellPad", 0 },
    { 0x1CF98800, "cellPadGetInfo2", "cellPad", 0 },
    { 0x3AAAD464, "cellPadGetData", "cellPad", 0 },
    { 0x0D5F2C14, "cellPadClearBuf", "cellPad", 0 },
    { 0xF54172B3, "cellPadSetPortSetting", "cellPad", 0 },
    { 0x0EB1F0BA, "cellKbInit", "cellKb", 0 },
    { 0xBFCE3285, "cellKbEnd", "cellKb", 0 },
    { 0x1F71ECBE, "cellMouseInit", "cellMouse", 0 },
    { 0x3EF332C8, "cellMouseEnd", "cellMouse", 0 },
    { 0x45EC29DB, "cellSpursInitialize", "cellSpurs", 0 },
    { 0xCA64789A, "cellSpursFinalize", "cellSpurs", 0 },
    { 0xACFC2024, "cellSpursAttachLv2EventQueue", "cellSpurs", 0 },

    // XXIII-Q: additional GCM / RESC / sysutil NIDs commonly hit during boot draws
    { 0xFE47E4A0, "cellGcmSys", "cellGcmSetFragmentProgramOffset", 1 },
    { 0xA91B3B70, "cellGcmSys", "cellGcmSetVertexProgram", 1 },
    { 0x21CEE584, "cellGcmSys", "cellGcmSetFragmentProgram", 1 },
    { 0x51C9D62B, "cellGcmSys", "cellGcmSetVertexProgramLoad", 1 },
    { 0x876153B8, "cellGcmSys", "cellGcmSetFragmentProgramLoad", 1 },
    { 0x0C4524F2, "cellGcmSys", "cellGcmSetVertexDataArrayFormat", 1 },
    { 0x8BDE5EBF, "cellGcmSys", "cellGcmSetVertexDataArrayOffset", 1 },
    { 0xD9A0A879, "cellGcmSys", "cellGcmSetTexture", 1 },
    { 0x7CE45E09, "cellGcmSys", "cellGcmSetTextureAddress", 1 },
    { 0x650E2403, "cellGcmSys", "cellGcmSetTextureControl", 1 },
    { 0x3746F642, "cellGcmSys", "cellGcmSetTextureFilter", 1 },
    { 0xA7E03D81, "cellGcmSys", "cellGcmSetDrawInlineArray", 1 },
    { 0x4A62F756, "cellGcmSys", "cellGcmSetDrawIndexArray", 1 },
    { 0xC47D0812, "cellSysutil", "cellSysutilRegisterCallback", 3 },
    { 0x9D98AFA0, "cellSysutil", "cellSysutilCheckCallback", 3 },
    { 0x189A74DA, "cellSysutil", "cellSysutilUnregisterCallback", 3 },
};

static constexpr int kNidTableCount = (int)(sizeof(kNidTable) / sizeof(kNidTable[0]));

static inline const NidEntry* nid_lookup(uint32_t nid) {
    for (int i = 0; i < kNidTableCount; ++i)
        if (kNidTable[i].nid == nid) return &kNidTable[i];
    return nullptr;
}

static inline int prx_module_kind(const char* mod) {
    if (!mod) return 0;
    if (strstr(mod, "Gcm") || strstr(mod, "gcm")) return 1;
    if (strstr(mod, "Fs") || strstr(mod, "fs")) return 2;
    if (strstr(mod, "Sysmodule") || strstr(mod, "sysmodule")) return 3;
    if (strstr(mod, "Audio") || strstr(mod, "audio")) return 4;
    if (strstr(mod, "Spurs") || strstr(mod, "spurs")) return 5;
    if (strstr(mod, "Np") || strstr(mod, "np")) return 6;
    if (strstr(mod, "Sysutil") || strstr(mod, "sysutil")) return 7;
    if (strstr(mod, "io") || strstr(mod, "Pad") || strstr(mod, "pad")) return 8;
    if (strstr(mod, "vdec") || strstr(mod, "Vdec")) return 9;
    if (strstr(mod, "Game") || strstr(mod, "game")) return 10;
    if (strstr(mod, "Save") || strstr(mod, "save")) return 11;
    return 0;
}
