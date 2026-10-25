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

// XXIII-S: sys_ppu_thread_* — record create; success so workers can proceed
case 41: case 43: case 44: case 45: case 46: case 47: {
    static uint32_t s_tid = 1;
    static int s_th = 0;
    if ((c->gpr[11] & 0xFFFFFFFF) == 41 || (c->gpr[11] & 0xFFFF) == 41) {
        // create-ish: return a fake thread id in *r3 if pointer
        uint64_t p = c->gpr[3];
        if (p && p + 4 <= g_mem_size) {
            uint32_t tid = s_tid++;
            g_mem[p] = (uint8_t)(tid >> 24); g_mem[p+1] = (uint8_t)(tid >> 16);
            g_mem[p+2] = (uint8_t)(tid >> 8); g_mem[p+3] = (uint8_t)tid;
        }
    }
    if (s_th < 16) {
        ps3rt_log("[lv2] XXIII-S ppu_thread sc=%llu r3=0x%llx
",
            (unsigned long long)(c->gpr[11] & 0xFFFFFFFF), (unsigned long long)c->gpr[3]);
        ++s_th;
    }
    c->gpr[3] = 0; break;
}

// H: sys_memory_*
case 348: case 349: case 350: case 351: case 352: case 353:
    c->gpr[3] = 0; break;

// H: sys_process_exit / getpid
case 1: case 3:
    c->gpr[3] = 0; break;

// H: sys_fs_* LV2-level
case 801: case 802: case 803: case 804: case 808: case 809: case 811:
    c->gpr[3] = 0; break;



// ============================================================================
// XXV-G depth — LV2 contracts (documented errors, not silent success for all)
// CELL_OK = 0; CELL_ENOSYS = 0x80010005; CELL_EINVAL = 0x80010002
// ============================================================================

// sys_process_* (common boot)
case 1: case 2: case 3: case 4: case 5: case 6: case 7: case 8: case 9:
case 10: case 11: case 12: case 13: case 14: case 15: case 16: case 17:
case 18: case 19: case 20: case 21: case 22: case 23: case 24: case 25:
    {
        static int s_proc = 0;
        if (s_proc < 16) {
            ps3rt_log("[lv2] XXV-G process sc=%llu -> OK\n", (unsigned long long)(c->gpr[11] & 0xFFFFFFFFull));
            ++s_proc;
        }
        c->gpr[3] = 0; break;
    }

// sys_memory_allocate / free / get_page_attribute (approx numbers used by many titles)
case 348: case 349: case 350: case 351: case 352: case 353: case 354: case 355:
case 356: case 357: case 358: case 359: case 360: case 361: case 362:
    {
        // allocate-style: write a guest pointer if r4 is out-ptr
        uint64_t out = r4;
        if (out && out + 4 <= g_mem_size) {
            static uint32_t s_heap = 0x20000000u;
            uint32_t blk = s_heap;
            s_heap += 0x10000u;
            if (s_heap > 0x28000000u) s_heap = 0x20000000u;
            wr_be32_guest(out, blk);
        }
        c->gpr[3] = 0; break;
    }

// sys_fs open/read/write/close/lseek/stat (subset; detailed FS is cellFs HLE)
case 801: case 802: case 803: case 804: case 805: case 806: case 807: case 808:
case 809: case 810: case 811: case 812: case 813: case 814: case 815: case 816:
    {
        static int s_fs = 0;
        if (s_fs < 20) {
            ps3rt_log("[lv2] XXV-G fs sc=%llu r3=0x%llx -> OK/stub\n",
                (unsigned long long)(c->gpr[11] & 0xFFFFFFFFull), (unsigned long long)r3);
            ++s_fs;
        }
        c->gpr[3] = 0; break;
    }

// sys_prx_load_module / start / stop / unload (approx)
case 480: case 481: case 482: case 483: case 484: case 485: case 486: case 487:
case 488: case 490: case 492: case 493: case 494: case 495: case 496: case 497:
    {
        static int s_prx = 0;
        if (s_prx < 20) {
            ps3rt_log("[lv2] XXV-G sys_prx sc=%llu -> OK\n", (unsigned long long)(c->gpr[11] & 0xFFFFFFFFull));
            ++s_prx;
        }
        // Prefer module id in *r3 when looks like out pointer
        if (r3 && r3 + 4 <= g_mem_size) {
            static uint32_t s_mid = 0x1000;
            wr_be32_guest(r3, ++s_mid);
        }
        c->gpr[3] = 0; break;
    }

// sys_ss / game data / unknown high — ENOSYS when strict
case 870: case 871: case 872: case 873: case 874: case 875: case 876: case 877:
case 878: case 879: case 880:
    {
        static int s_strict = -1;
        if (s_strict < 0) {
            const char* e = std::getenv("PS3RT_STRICT_LV2");
            s_strict = (e && e[0]=='1') ? 1 : 0;
        }
        if (s_strict) {
            c->gpr[3] = 0x80010005ull; // CELL_ENOSYS
            static int s_e = 0;
            if (s_e < 12) {
                ps3rt_log("[lv2] XXV-G ENOSYS sc=%llu (STRICT)\n", (unsigned long long)(c->gpr[11] & 0xFFFFFFFFull));
                ++s_e;
            }
        } else {
            c->gpr[3] = 0;
        }
        break;
    }

