; Optional MASM acceleration for BE32 load (x64). Linked when ASM_MASM is on.
OPTION CASEMAP:NONE
.code
PUBLIC ps3rt_be32_load_asm
ps3rt_be32_load_asm PROC
    mov eax, DWORD PTR [rcx]
    bswap eax
    ret
ps3rt_be32_load_asm ENDP
END
