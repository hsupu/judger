#ifndef JUDGER_SYSCALL_CHECKER_H
#define JUDGER_SYSCALL_CHECKER_H

// NULL
#include <stdio.h>

// memset
#include <memory.h>

// ptrace
#include <sys/types.h>
#include <sys/ptrace.h>

// PTRACE_GETREGS
#include <sys/user.h>

#include "syscall_table.h"

#ifdef __i386
// x86
#define REG_SYSCALL orig_eax
#define REG_RET eax
#define REG_ARG0 ebx
#define REG_ARG1 ecx
#else
// x64
#define REG_SYSCALL orig_rax
#define REG_RET rax
#define REG_ARG0 rdi
#define REG_ARG1 rsi
#endif	//__i386

void init_syscall_limit();
unsigned long get_syscall(pid_t);
int check_syscall(unsigned long);

#endif //JUDGER_SYSCALL_CHECKER_H
