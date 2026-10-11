// lv2_syscalls.inl — Phase P extra LV2 HLE (included inside ps3rt_syscall switch)
// Keep this file focused on syscall numbers only.

// sys_lwmutex_lock / unlock family — succeed immediately
case 48: case 49: case 50: case 51:
    c->gpr[3] = 0; break;

// sys_timer_usleep (141) / sys_timer_sleep (142)
case 141: case 142:
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

// sys_spu_thread_* / event stubs seen in GOW boot
case 169: case 160: case 161: case 162:
    c->gpr[3] = 0; break;

// sys_rwlock / cond
case 112: case 113: case 114: case 115:
    c->gpr[3] = 0; break;
