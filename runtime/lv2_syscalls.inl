// XV-M: cases inlined into ps3rt_b.inl ps3rt_syscall (do not #include from game src)
// XV-L: included inside ps3rt_syscall switch
case 1: // getpid
    c->gpr[3] = 1; break;
case 4:
    c->gpr[3] = 0; break;
case 5:
    if (r3 && r3 + 4 <= g_mem_size) wr_be32_guest(r3, 1);
    c->gpr[3] = 0; break;
case 25:
    if (r3 && r3 + 4 <= g_mem_size) wr_be32_guest(r3, 0x00440000);
    c->gpr[3] = 0; break;
case 41: case 43: case 44: case 45: case 46: case 47: {
    static uint32_t s_tid = 1;
    if ((uint32_t)num == 41 && r3 && r3 + 4 <= g_mem_size) wr_be32_guest(r3, s_tid++);
    c->gpr[3] = 0; break;
}
case 48: case 49: case 50: case 51: case 52: case 53:
    c->gpr[3] = 0; break;
case 90: case 91: case 92: case 93: case 94: case 95: case 96:
    c->gpr[3] = 0; break;
case 100: case 101: case 102: case 103: case 104: case 105: case 106: case 107:
    c->gpr[3] = 0; break;
case 112: case 113: case 114: case 115: case 116: case 117:
    c->gpr[3] = 0; break;
case 128: {
    static uint32_t qid = 0x1000;
    if (r3 && r3 + 4 <= g_mem_size) wr_be32_guest(r3, ++qid);
    c->gpr[3] = 0; break;
}
case 129: case 130: case 131: case 132: case 133: case 134: case 135:
case 136: case 137: case 138: case 139:
    c->gpr[3] = 0; break;
case 141: case 142:
#if defined(_WIN32)
    Sleep(0);
#endif
    c->gpr[3] = 0; break;
case 143: {
    static uint32_t fid = 0x8000;
    if (r3 && r3 + 4 <= g_mem_size) wr_be32_guest(r3, ++fid);
    c->gpr[3] = 0; break;
}
case 144: {
    static uint64_t fake_sec = 1000;
    ++fake_sec;
    if (r3 && r3 + 8 <= g_mem_size) wr_be64_guest(r3, fake_sec);
    if (r4 && r4 + 8 <= g_mem_size) wr_be64_guest(r4, 0);
    c->gpr[3] = 0; break;
}
case 145:
    c->gpr[3] = 79800000ull; break;
case 156: case 157: case 158: case 159:
case 160: case 161: case 162: case 163: case 165: case 166: case 167:
case 169: case 170: case 171: case 172: case 173: case 174: case 175:
case 176: case 177: case 178: case 179: case 180: case 181:
    c->gpr[3] = 0; break;
case 330: case 331: case 332: case 333: case 334:
    c->gpr[3] = 0; break;
case 348: case 349: case 350: case 351: case 352: case 353: case 354: {
    uint64_t sz = r4 ? r4 : 0x1000;
    uint64_t al = r5 ? r5 : 0x10000;
    if (al > 0x100000) al = 0x10000;
    uint64_t a = guest_alloc(sz, al);
    if (r3 && r3 + 8 <= g_mem_size) wr_be64_guest(r3, a);
    static int s_m = 0;
    if (s_m < 16) {
        ps3rt_log("[lv2] XV-L mem sc=%u size=0x%llx -> ea=0x%llx\n",
            (unsigned)num, (unsigned long long)sz, (unsigned long long)a);
        ++s_m;
    }
    c->gpr[3] = a ? 0 : (uint64_t)(int64_t)-1;
    break;
}
case 480: case 481: case 482: case 483: case 484: case 485: case 486:
case 487: case 488: case 489: case 490: case 491: case 492: case 493:
case 494: case 496: case 497: case 498: case 499: {
    static uint32_t s_prxid = 0x100;
    if ((uint32_t)num == 480 || (uint32_t)num == 499) {
        if (r4 && r4 + 4 <= g_mem_size) wr_be32_guest(r4, ++s_prxid);
        else if (r3 && r3 + 4 <= g_mem_size) wr_be32_guest(r3, ++s_prxid);
    }
    static int s_prx = 0;
    if (s_prx < 20) {
        ps3rt_log("[lv2] XV-L sys_prx sc=%u r3=0x%llx r4=0x%llx\n",
            (unsigned)num, (unsigned long long)r3, (unsigned long long)r4);
        ++s_prx;
    }
    c->gpr[3] = 0; break;
}
case 801: case 802: case 803: case 804: case 805: case 806: case 807:
case 808: case 809: case 810: case 811: case 812: case 813: case 814:
    c->gpr[3] = 0; break;
case 871: case 872: case 873: case 874: case 875:
    c->gpr[3] = 0; break;
case 986: case 987: case 988: case 989: case 990: case 991:
    c->gpr[3] = 0; break;
