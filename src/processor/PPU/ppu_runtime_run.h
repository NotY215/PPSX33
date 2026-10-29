// WORK: PPU run support (ppu_runtime_run.h).


// HLE status dump is fully header-local (no hle_status.h required for game builds).
static inline void ps3rt_log_hle_status_once(void) {
    static int done = 0;
    if (done) return;
    done = 1;
    static const char* const k_names[] = {
        "sys_ppu_thread/mutex/cond", "sys_memory/mmapper", "cellFs", "cellPad",
        "cellAudio", "cellGcmSys", "cellSysutil/sysmodule", "cellSpurs", "RSX/FIFO"
    };
    static const char* const k_notes[] = {
        "subset", "subset", "path map + host I/O", "XInput/host keys",
        "host path", "control/FIFO kick", "common NIDs", "job assist",
        "assist draws proven; title stream pending"
    };
    for (int i = 0; i < 9; ++i)
        ps3rt_log("[hle] %s = partial (%s)\n", k_names[i], k_notes[i]);
}

// PLACEHOLDER_FULL_FILE
