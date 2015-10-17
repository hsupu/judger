#include "syscall_checker.h"

int syscall_counter[sizeof(syscall_limit)];

void init_syscall_limit() {
    memcpy(syscall_counter, syscall_limit, sizeof(syscall_limit));
}

unsigned long get_syscall(pid_t pid)
{
    struct user_regs_struct regs;
    ptrace(PTRACE_GETREGS, pid, NULL, &regs);
    return regs.REG_SYSCALL;
}

int check_syscall(unsigned long syscall)
{
    switch (syscall_table[syscall]) {
        case 0: case 1: case 2:
            return syscall_table[syscall];
        case 3:
            if (syscall_counter[syscall] > 0) {
                syscall_counter[syscall]--;
                return 1;
            } else {
                return 0;
            }
        default:
            return 0;
    }
}

