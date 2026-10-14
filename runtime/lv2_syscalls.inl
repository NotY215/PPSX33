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

// XX-L/M: sys_spu_thread_* / image / event queue (GOW boot paths)
// 160 sys_spu_thread_group_create, 161 destroy, 162 start, 163 join, 165 yield
// 169 sys_spu_thread_write_snr, 171 set_priority, 172 connect_event ...
case 160: case 161: case 162: case 163: case 165: case 166: case 167:
case 169: case 170: case 171: case 172: case 173: case 174: case 175:
case 176: case 177: case 178: case 179: case 180: case 181:
    c->gpr[3] = 0; break;
// sys_spu_image_open / close / get_information style (user wrapper often PRX)
case 156: case 157: case 158: case 159:
    c->gpr[3] = 0; break;

// sys_rwlock / cond
case 112: case 113: case 114: case 115:
    c->gpr[3] = 0; break;
