/* This file is part of STonX, the Atari ST Emulator for Unix/X
 * ============================================================
 * STonX is free software and comes with NO WARRANTY - read the file
 * COPYING for details
 */
#ifndef SYSCALLS_H
#define SYSCALLS_H

#define TRACE_SYSCALLS 1

#if TRACE_SYSCALLS
#define TRACE_SYSCALL(_x) syscall(_x)
extern void syscall(int nex);
#else
#define TRACE_SYSCALL(_x)
#endif

#endif /* SYSCALLS_H */
