// XXIII-G: LV2 wait/signal contracts
// XXII-J: LV2 contracts — prefer documented return where known; else 0 with log upstream
// XXII-D: LV2 / HLE — log first use; prefer non-zero for unknown high-risk syscalls
// lv2_syscalls.inl — HLE with first-hit logging (accuracy: stubs still incomplete)\n// lv2_syscalls.inl — H: expanded LV2 HLE (included inside ps3rt_syscall switch)

// XXIII-G: lwmutex
case 48: case 49: case 50: case 51: {
    static int s_mtx = 0;
    if (s_mtx < 12) { ps3rt_log("[lv2] XXIII-G lwmutex sc r3=0x%llx
", (unsigned long long)r3); ++s_mtx; }
    c->gpr[3] = 0; break;
}

// XXIII-G: timer sleep yields host slice
case 141: case 142:
#if defined(_WIN32)
    Sleep(0);
#endif
    c->gpr[3] = 0; break;

// sys_time_get_current_time — r3=&sec, r4=&nsec
case 144: {
    static uint64_t fake_sec = 1000;
    ++fake_sec;
    if (r3 && r3 + 8 <= g_mem_size) wr_be64_guest(r3, fake_sec);
    if (r4 && r4 + 8 <= g_mem_size) wr_be64_guest(r4, 0);
    c->gpr[3] = 0; break;
}

// sys_time_get_timebase_frequency
case 145:
    c->gpr[3] = 79800000ull; break;

// sys_spu_thread_* / image
case 160: case 161: case 162: case 163: case 165: case 166: case 167:
case 169: case 170: case 171: case 172: case 173: case 174: case 175:
case 176: case 177: case 178: case 179: case 180: case 181:
    c->gpr[3] = 0; break;
case 156: case 157: case 158: case 159:
    c->gpr[3] = 0; break;

// sys_rwlock / cond
case 112: case 113: case 114: case 115:
    c->gpr[3] = 0; break;

// H: sys_mutex_*
case 100: case 101: case 102: case 103: case 104: case 105: case 106: case 107:
    c->gpr[3] = 0; break;

// H: sys_event_queue_*
case 128: case 129: case 130: case 131: case 132: case 133: case 134: case 135:
case 136: case 137:
    c->gpr[3] = 0; break;

// H: sys_semaphore_*
case 90: case 91: case 92: case 93: case 94: case 95: case 96:
    c->gpr[3] = 0; break;

// H: sys_ppu_thread_*
case 41: case 43: case 44: case 45: case 46: case 47:
    c->gpr[3] = 0; break;

// H: sys_memory_*
case 348: case 349: case 350: case 351: case 352: case 353:
    c->gpr[3] = 0; break;

// H: sys_process_exit / getpid
case 1: case 3:
    c->gpr[3] = 0; break;

// H: sys_fs_* LV2-level
case 801: case 802: case 803: case 804: case 808: case 809: case 811:
    c->gpr[3] = 0; break;
