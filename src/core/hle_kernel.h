#pragma once
// WORK: Intercepts native OS syscalls (threads, mutexes, file IO) and PRX imports.
struct PPUContext;
namespace ppsx33::core {
void hle_kernel_note();
}
