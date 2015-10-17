#ifndef JUDGER_MYEXEC_H
#define JUDGER_MYEXEC_H

/**
 * run with limited resource
 */

#include <stdio.h>
#include <stdlib.h>

// va_*
#include <stdarg.h>

// PATH_MAX, FILE_MAX
#ifdef __APPLE__
#include <sys/syslimits.h>
#elif defined(__LINUX__)
#include <linux/limits.h>
#endif

// string
#include <string.h>

// isspace
#include <ctype.h>

// fork
// exec
// chroot
#include <unistd.h>

// open
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

// getuid
#include <unistd.h>
#include <sys/types.h>

// wait4
#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/wait.h>

// signal
// psignal
#include <sys/time.h>
#include <signal.h>

// setrlimit
#include <sys/time.h>
#include <sys/resource.h>

// stat
#include <sys/stat.h>

// dir
#include <dirent.h>

// ptrace
#include <sys/types.h>
#include <sys/ptrace.h>
//#include "ptrace_macfix.h"

// syscall checker
#include "syscall_checker.h"

#define DEBUG 1
#define MB (1048576)

enum judge_status {
    JUDGE_FAIL = -1,
    JUDGE_PD = 0, // Pending
    JUDGE_RJ,     // Running & judging
    JUDGE_CE,     // Compile Error
    JUDGE_AC,     // Accepted
    JUDGE_RE,     // Runtime Error
    JUDGE_WA,     // Wrong Answer
    JUDGE_TLE,    // Time Limit Exceeded
    JUDGE_MLE,    // Memory Limit Exceeded
    JUDGE_OLE,    // Output Limit Exceeded
    JUDGE_PE,     // Presentation Error
};

char judge_string[][25] = {
    "Pending",
    "Runing & Judging",
    "Compile Error",
    "Accepted",
    "Runtime Error",
    "Wrong Answer",
    "Time Limit Exceeded",
    "Memory Limit Exceeded",
    "Output Limit Exceeded",
    "Presentation Error"
};

enum language {
    LANG_NONE = 0,
    LANG_C,
    LANG_CPP,
    LANG_JAVA,
    LANG_CSHARP,
    LANG_PYTHON,
    LANG_RUBY,
    LANG_PHP
};

char language_string[][10] = {
    "unknown",
    "c",
    "c++",
    "java",
    "csharp",
    "python",
    "ruby",
    "php"
};

long question_id, run_id, time_limit, mem_limit;
enum language language;

// 当前工作目录
char work_dir[PATH_MAX];

// 测试样例目录
char testcase_dir[PATH_MAX];
// 输入样例路径
char input_path[PATH_MAX];
// 输出样例路径
char output_path[PATH_MAX];

// 运行时目录
char runtime_dir[PATH_MAX];
// 源文件目录
char src_dir[PATH_MAX];
// 待测文件路径
char exec_path[PATH_MAX];
// 目标目录
char dst_dir[PATH_MAX];
// 用户输出路径
char user_output_path[PATH_MAX];
// 错误日志路径
char error_path[PATH_MAX];

#endif //JUDGER_MYEXEC_H
