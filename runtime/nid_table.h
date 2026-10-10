// Real PS3 module NID table (common cell*/sys_* exports).
#pragma once
#include <cstdint>

struct NidEntry {
    uint32_t    nid;
    const char* module;
    const char* name;
    int         kind; // 0 generic, 1 gcm, 2 fs, 3 sysmodule, 4 audio, 5 spurs, 6 np, 7 sysutil, 8 io
};

static const NidEntry kNidTable[] = {
    { 0x15BAE46B, "cellGcmSys", "cellGcmInitBody", 1 },
    { 0x21AC3697, "cellGcmSys", "cellGcmAddressToOffset", 1 },
    { 0x4524FE95, "cellGcmSys", "cellGcmGetConfiguration", 1 },
    { 0x55A14C05, "cellGcmSys", "cellGcmGetControlRegister", 1 },
    { 0x5F909B17, "cellGcmSys", "cellGcmGetLabelAddress", 1 },
    { 0x63387071, "cellGcmSys", "cellGcmGetDisplayBuffer", 1 },
    { 0x72A577CE, "cellGcmSys", "cellGcmGetCurrentField", 1 },
    { 0x98396A7C, "cellGcmSys", "cellGcmSetFlipMode", 1 },
    { 0xA53D12AE, "cellGcmSys", "cellGcmSetDisplayBuffer", 1 },
    { 0xA547ADDE, "cellGcmSys", "cellGcmGetFlipStatus", 1 },
    { 0xB2E761D4, "cellGcmSys", "cellGcmResetFlipStatus", 1 },
    { 0xD81D0D2D, "cellGcmSys", "cellGcmSetFlip", 1 },
    { 0xE315A0B2, "cellGcmSys", "cellGcmGetLastFrameId", 1 },
    { 0xF80196C0, "cellGcmSys", "cellGcmSetWaitFlip", 1 },
    { 0x718BF5F8, "cellFs", "cellFsOpen", 2 },
    { 0x4D5B9EF2, "cellFs", "cellFsRead", 2 },
    { 0xECDCF2AB, "cellFs", "cellFsWrite", 2 },
    { 0x2CB51F0F, "cellFs", "cellFsClose", 2 },
    { 0x7DE6DCED, "cellFs", "cellFsStat", 2 },
    { 0xA397D042, "cellFs", "cellFsLseek", 2 },
    { 0xBA901FE6, "cellFs", "cellFsMkdir", 2 },
    { 0x2796FDF3, "cellFs", "cellFsRmdir", 2 },
    { 0x7F4677A8, "cellFs", "cellFsUnlink", 2 },
    { 0xF12EECC8, "cellFs", "cellFsRename", 2 },
    { 0x3F61245C, "cellFs", "cellFsOpendir", 2 },
    { 0x5C74903D, "cellFs", "cellFsReaddir", 2 },
    { 0xFF42DCC3, "cellFs", "cellFsClosedir", 2 },
    { 0x63FF6D4A, "cellSysmodule", "cellSysmoduleInitialize", 3 },
    { 0x96CDFA20, "cellSysmodule", "cellSysmoduleFinalize", 3 },
    { 0x3A495ADE, "cellSysmodule", "cellSysmoduleLoadModule", 3 },
    { 0x63A2A3C3, "cellSysmodule", "cellSysmoduleUnloadModule", 3 },
    { 0x5A59E258, "cellSysmodule", "cellSysmoduleIsLoaded", 3 },
    { 0x0B168F92, "cellAudio", "cellAudioInit", 4 },
    { 0x566966C0, "cellAudio", "cellAudioCreatePort", 4 },
    { 0x5B1E2C73, "cellAudio", "cellAudioPortOpen", 4 },
    { 0x65C27676, "cellAudio", "cellAudioPortStart", 4 },
    { 0xDDC2AB95, "cellAudio", "cellAudioPortStop", 4 },
    { 0xAC6B8901, "cellSpurs", "cellSpursInitialize", 5 },
    { 0xCAF8AB41, "cellSpurs", "cellSpursFinalize", 5 },
    { 0x4A5EEA63, "cellSpurs", "cellSpursCreateTaskset", 5 },
    { 0xC2ACDF43, "cellSpurs", "cellSpursJoinTaskset", 5 },
    { 0xCFAD36C9, "cellSysutil", "cellSysutilRegisterCallback", 7 },
    { 0x9D98AFA0, "cellSysutil", "cellSysutilUnregisterCallback", 7 },
    { 0x189A74DA, "cellSysutil", "cellSysutilCheckCallback", 7 },
    { 0x40E895D3, "cellSysutil", "cellSysutilGetSystemParamInt", 7 },
    { 0xBD28FDB1, "sceNp", "sceNpInit", 6 },
    { 0x39D4A3A8, "sceNp", "sceNpTerm", 6 },
    { 0x5F08F86B, "sceNpTrophy", "sceNpTrophyInit", 6 },
    { 0x1C936E6D, "sys_io", "cellPadInit", 8 },
    { 0x3AAAD464, "sys_io", "cellPadGetData", 8 },
    { 0x0D5F2C14, "sys_io", "cellPadEnd", 8 },
};
static constexpr int kNidTableCount = (int)(sizeof(kNidTable) / sizeof(kNidTable[0]));
static inline const NidEntry* nid_lookup(uint32_t nid) {
    for (int i = 0; i < kNidTableCount; ++i)
        if (kNidTable[i].nid == nid) return &kNidTable[i];
    return nullptr;
}
