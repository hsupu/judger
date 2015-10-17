#include "myexec.h"

/**
 * 准备测试样例，成功返回 1 失败返回 0
 */
int prepare_test_case(int number, const char *filename)
{

    size_t l = strlen(filename);
    if (l <= 3 || strcmp(filename + l-3, ".in") != 0) {
        return 0;
    }

    char name[NAME_MAX];
    strncpy(name, filename,  l-3);
    name[l-3] = '\0';

    sprintf(input_path, "%s/%s", testcase_dir, filename);
    sprintf(output_path, "%s/%s.out", testcase_dir, name);
    sprintf(user_output_path, "%s/dst/%ld-%s.out", runtime_dir, run_id, name);

    if (DEBUG) {
        printf("\nTest Case %d:\n\033[0;32m- input: %s\n- output: %s\n- user: %s\033[0m\n\n", number, input_path, output_path, user_output_path);
    }

    return 1;
}

void reopen(int fileno, const char *path, int oflag, mode_t mode)
{
    int tmp_fileno = open(path, oflag, mode);
    if (tmp_fileno == -1) {
        perror("open");
        exit(1);
    }
    dup2(tmp_fileno, fileno);
    close(tmp_fileno);
}

void myexec(long used_time)
{
    // 降低优先级
    nice(20);

    // 改变 std stream 的事应该只发生在子进程
    // 不能使用 freopen 因为它将产生更高的 fno 而 exec 后只保留 0,1,2 而且不会关闭 exec 之前的文件
    reopen(STDIN_FILENO, input_path, O_RDONLY, 0);
    reopen(STDOUT_FILENO, user_output_path, O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU | S_IRGRP | S_IROTH);
    reopen(STDERR_FILENO, error_path, O_WRONLY | O_CREAT | O_APPEND, S_IRWXU | S_IRGRP | S_IROTH);

    if (DEBUG) {
        struct timeval tv;
        gettimeofday(&tv, NULL);
        fprintf(stderr, "[%s] start: %ld.\n", input_path, tv.tv_sec);
    }

    /*
    alarm(0);
    alarm(time_limit * 10);
    */

    struct rlimit LIM;

    // CPU 用时限制
    // 达到软限制会发送 SIGXCPU 信号，达到硬限制会发送 SIGKILL 信号
    LIM.rlim_cur = time_limit - used_time / 1000;
    LIM.rlim_max = LIM.rlim_cur + 1;
    if (setrlimit(RLIMIT_CPU, &LIM) < 0) {
        perror("setrlimit [CPU]");
        exit(1);
    }
    //signal(SIGXCPU, myexit);

    // 避免 sleep() 恶意运行
    alarm(0);
    alarm(LIM.rlim_cur * 10);

    // 输出限制
    LIM.rlim_cur = 32 * MB;
    LIM.rlim_max = LIM.rlim_cur + MB;
    if (setrlimit(RLIMIT_FSIZE, &LIM) < 0) {
        perror("setrlimit [FSIZE]");
        exit(1);
    }
    //signal(SIGXFSZ, myexit);

    // 进程数限制
    if (language == LANG_JAVA) {
        LIM.rlim_max = LIM.rlim_cur = 1000;
    } else {
        LIM.rlim_max = LIM.rlim_cur = 1;
    }
    if (setrlimit(RLIMIT_NPROC, &LIM) < 0) {
        perror("setrlimit [NPROC]");
        exit(1);
    }

    ///*
    // 内存限制
    LIM.rlim_cur = mem_limit;
    LIM.rlim_max = mem_limit * 2;
    // FreeBSD 平台也许需要修改为 RLIMIT_VMEM
    if (setrlimit(RLIMIT_AS, &LIM) < 0) {
        perror("setrlimit [AS]");
        exit(1);
    }
    //*/

    // 堆栈限制，注意系统有特定的硬限制比如 OSX 为 8MB 所以不能无限放宽
    LIM.rlim_max = LIM.rlim_cur = 4 * MB;
    if (setrlimit(RLIMIT_STACK, &LIM) < 0) {
        perror("setrlimit [STACK]");
        exit(1);
    }

    if (DEBUG) {
        fprintf(stderr, "[%s] exec %s now\n", input_path, language_string[language]);
    }

    // trace me
    if (ptrace(PTRACE_TRACEME, 0, NULL, NULL) < 0) {
        perror("ptrace");
        exit(1);
    }

    switch (language) {
        case LANG_C: case LANG_CPP:
            execl(exec_path, exec_path, (char *)NULL);
            break;
        case LANG_JAVA:
            execl("java",
                  "java",
                  "-Xms128M",
                  "-Xms512M",
                  "-Djava.security.manager",
                  "-Djava.security.policy=./java.policy",
                  "-DONLINE_JUDGE=true",
                  "Main",
                  (char *)NULL);
            break;
        case LANG_CSHARP:
            execl("mono", "mono", "main.exe", (char *)NULL);
            break;
        case LANG_PYTHON:
            execl("python", "python", "main.py", (char *)NULL);
            break;
        case LANG_RUBY:
            execl("ruby", "ruby", "main.rb", (char *)NULL);
            break;
        case LANG_PHP:
            execl("php", "php", "main.php", (char *)NULL);
            break;
        default:
            fprintf(stderr, "Unknown language.\n");
            exit(1);
    }
    perror("execl");
    exit(1);
}

// 获取文件大小
long get_file_size(const char *path)
{
    struct stat f_stat;
    if (stat(path, &f_stat) == -1) {
        return 0;
    }
    return (long) f_stat.st_size;
}

// 获取进程状态
long get_proc_status(pid_t pid, const char *mark)
{
    FILE *pf;
    char fn[PATH_MAX+1];
    sprintf(fn, "/proc/%d/status", pid);
    pf = fopen(fn, "r");

    size_t m = strlen(mark);

    char buf[BUFSIZ+1];
    long ret = 0;

    while (pf && fgets(buf, BUFSIZ, pf)) {
        buf[strlen(buf) - 1] = '\0';
        if (strncmp(buf, mark, m) == 0) {
            sscanf(buf + m + 1, "%ld", &ret);
            break;
        }
    }

    if (pf) {
        fclose(pf);
    }

    /*
    if (DEBUG) {
        printf("%s %ld\n", mark, ret);
    }
    */
    return ret;
}

// 获取页面错误
// JAVA use PAGEFAULT
long get_page_fault_mem(struct rusage &ru, pid_t &pid)
{
    long m_minflt = ru.ru_minflt * getpagesize();

    if (DEBUG) {
        long m_vmpeak = get_proc_status(pid, "VmPeak:");
        long m_vmdata = get_proc_status(pid, "VmData:");
        printf("VmPeak:%ld KB VmData:%ld KB minflt:%lu KB\n", m_vmpeak, m_vmdata, m_minflt >> 10);
    }
    return m_minflt;
}

// 监视运行
void watch(pid_t pid, enum judge_status &judge_status, long &max_used_mem, long &used_time, long error_size)
{
    // 之后将只能获取到子进程的运行总时间
    long init_used_time = used_time;

    // 为获取子进程的使用内存做准备
    long used_mem;

    // 为 OLE 判断做准备
    //long output_filesize = get_file_size(output_path);
    //long output_limit = output_filesize * 2;

    // 为限制系统调用做准备
    init_syscall_limit();

    int status;
    struct rusage ru;
    while (1) {
        pid_t pid_ret = wait4(pid, &status, 0, &ru);
        if (pid_ret != pid) {
            perror("wait4");
            exit(1);
        }

        /*
        if (DEBUG) {
            printf("Time used: %ld ms -> %ld s %ld us\n", init_used_time, ru.ru_utime.tv_sec, ru.ru_utime.tv_usec);
        }
        */

        // TLE 判断
        used_time = init_used_time + ru.ru_utime.tv_sec * 1000 + ru.ru_utime.tv_usec / 1000;
        if (used_time >= time_limit * 1000) {
            judge_status = JUDGE_TLE;
            goto send_kill;
        }

        // MLE 判断
        if (language == LANG_JAVA) {
            // JVM GC ask VM before need, so used kernel page fault times and page size.
            used_mem = get_page_fault_mem(ru, pid);
        } else {
            // other use VmPeak
            used_mem = get_proc_status(pid, "VmPeak:") << 10;
        }
        if (used_mem > max_used_mem) {
            max_used_mem = used_mem;
        }
        if (max_used_mem > mem_limit) {
            judge_status = JUDGE_MLE;
            goto send_kill;
        }

        if (!DEBUG) {
            // RE 判断 stderr
            if (get_file_size(error_path) > error_size) {
                judge_status = JUDGE_RE;
                goto send_kill;
            }
        }

        /* OLE 输出超限判断，不这么做是因为运行有多余的空格
        if (get_file_size(user_path) > output_limit) {
            judge_status = JUDGE_OLE;
            goto send_kill;
        }
        */

        if (WIFEXITED(status)) {
            // 进程退出
            int exitcode = WEXITSTATUS(status);
            if (DEBUG) {
                printf("child exited with code: %d.\n", exitcode);
            }
            if (exitcode != 0) {
                // 返回值错误认定为 RE
                judge_status = JUDGE_RE;
            }
            break;
        }

        if (WIFSIGNALED(status)) {
            // 未捕捉信号而终止
            int sigcode = WTERMSIG(status);
            if (DEBUG) {
                fprintf(stderr, "child exited with signal: %d ", sigcode);
                psignal(sigcode, NULL);
            }
            break;
        }

        if (WIFSTOPPED(status)) {
            // 未捕捉信号而暂停
            // 子进程被追踪(ptrace)或调用WUN-TRACED时才可能发生
            int sigcode = WSTOPSIG(status);
            if (DEBUG && sigcode != SIGTRAP) {
                fprintf(stderr, "child stopped with signal: %d ", sigcode);
                psignal(sigcode, NULL);
            }

            switch (sigcode) {
                case SIGSTOP:
                    // JAVA 使用 SIGSTOP 等待下次 CPU 运行
                    if (language != LANG_JAVA) {
                        judge_status = JUDGE_RE;
                        goto send_kill;
                    }
                case SIGTRAP:
                    // c++ case 语句中不能出现带初始化的变量声明
                    unsigned long syscall;
                    syscall = get_syscall(pid);

                    switch (check_syscall(syscall)) {
                        case 0:
                            if (DEBUG) {
                                printf("System call: %lu\n", syscall);
                            }
                            judge_status = JUDGE_RE;
                            goto send_kill;
                        case 1:
                            ptrace(PTRACE_SYSCALL, pid, NULL, NULL);
                            break;
                        case 2:
                            //ptrace(PTRACE_SETREGS, pid, NULL, NULL);
                            break;
                    }
                    break;
                case SIGSEGV:
                    judge_status = JUDGE_MLE;
                    goto send_kill;
                case SIGCHLD:
                case SIGALRM:
                    alarm(0);
                case SIGXCPU:
                case SIGKILL:
                    judge_status = JUDGE_TLE;
                    goto send_kill;
                case SIGXFSZ:
                    judge_status = JUDGE_OLE;
                    goto send_kill;
                default:
                    judge_status = JUDGE_FAIL;
                    goto send_kill;
            }

        }
    }
    return ;
    send_kill:
        ptrace(PTRACE_KILL, pid, NULL, NULL);

}

/* 不使用该函数是因为需要判定 PE
void seek_nonspace(FILE* &f, int &c)
{
    // 空白字符包括 空格符、\t 制表符、\n 换行符、\v 垂直制表符、\f 换页符、\r 回车符
    while (c != EOF || isspace(c)) {
        c = fgetc(f);
    }
}
*/

// 除空行外，遇到不同的空白符判定为 PE
enum judge_status find_next_nonspace(int &c1, int &c2, FILE * &f1, FILE * &f2)
{
    c1 = fgetc(f1), c2 = fgetc(f2);
    while ((isspace(c1)) || (isspace(c2))) {
        if (c1 != c2) {
            if (c2 == EOF) {
                do {
                    c1 = fgetc(f1);
                } while (isspace(c1));
                break;
            } else if (c1 == EOF) {
                do {
                    c2 = fgetc(f2);
                } while (isspace(c2));
                break;
            } else if ((c1 == '\r' && c2 == '\n')) {
                c1 = fgetc(f1);
            } else if ((c2 == '\r' && c1 == '\n')) {
                c2 = fgetc(f2);
            } else {
                return JUDGE_PE;
            }
        }
        if (isspace(c1)) c1 = fgetc(f1);
        if (isspace(c2)) c2 = fgetc(f2);
    }
    return JUDGE_RJ;
}

// 检查用户输出
enum judge_status check_user_output()
{
    if (DEBUG) {
        printf("Checking output...\n");
    }

    FILE *f1, *f2;
    f1 = fopen(user_output_path, "r");
    f2 = fopen(output_path, "r");
    if (!f1 || !f2) {
        perror("fopen");
        exit(1);
    }

    enum judge_status ret = JUDGE_RJ;

    int c1, c2;
    do {
        ret = find_next_nonspace(c1, c2, f1, f2);
        if (ret != JUDGE_RJ || (c1 == EOF && c2 == EOF)) break;
        if (c1 != c2) ret = JUDGE_WA;
    } while (ret == JUDGE_RJ);

    fclose(f1);
    fclose(f2);

    return ret;
}

//执行命令
int systemf(const char * fmt, ...)
{
    char cmd[BUFSIZ];
    int ret = 0;
    va_list ap;
    va_start(ap, fmt);
    vsprintf(cmd, fmt, ap);
    ret = system(cmd);
    va_end(ap);
    return ret;
}

int main(int argc, char **argv)
{
    if (argc < 4) {
        fprintf(stderr, "\033[0;39;1mUsage: myexec [question_id] [language_code] [run_id] [time_limit](s) [mem_limit](MB)\033[0m\n");
        exit(1);
    }

    if (getuid() != 0) {
        fprintf(stderr, "Error: judger should be run as root.\n");
        exit(1);
    }

    question_id = atoi(argv[1]);
    language = (enum language)atoi(argv[2]);
    run_id = atoi(argv[3]);
    time_limit = atoi(argv[4]);
    mem_limit = atoi(argv[5]) * MB;

    // 因为众所周知的原因，JAVA 拥有更宽松的判定环境
    if (language == LANG_JAVA) {
        time_limit = time_limit * 2;
        mem_limit = mem_limit * 2;
        systemf("cp /etc/java-8-openjdk/security/java.policy %s/java.policy", src_dir);
    }

    getcwd(work_dir, sizeof(work_dir));

    sprintf(testcase_dir, "%ld", question_id);
    sprintf(runtime_dir, "runtime");
    sprintf(src_dir, "%s/src", runtime_dir);
    sprintf(dst_dir, "%s/dst", runtime_dir);

    sprintf(exec_path, "%s/%ld", src_dir, run_id);
    sprintf(error_path, "%s/%ld.err", dst_dir, run_id);

    if (DEBUG) {
        printf("\nInit Info:\n");
        printf("\033[0;33m- Language: %s\n- QuestionID: %ld\n- RunID: %ld\n- Timelimit: %ld s\n- Memlimit: %ld MB\n- Workdir: %s\n- Testcase dir: %s\n- Runtime dir: %s\n- Exec path:%s\n- Error log: %s\033[0m\n", language_string[language], question_id, run_id, time_limit, mem_limit >> 20, work_dir, testcase_dir, runtime_dir, exec_path, error_path);
    }

    DIR *dp;
    struct dirent *dirp;
    if ((dp = opendir(testcase_dir)) == NULL) {
        perror("opendir");
        exit(1);
    }

    enum judge_status judge_status = JUDGE_RJ;
    long used_time    = 0; // ms
    long max_used_mem = 0; // B
    long error_size   = 0; // B
    int  test_count   = 0;

    while (judge_status == JUDGE_RJ && (dirp = readdir(dp)) != NULL) {

        if (prepare_test_case(test_count, dirp->d_name) == 0) continue;

        // 在子进程运行前取值
        error_size = get_file_size(error_path);

        // init_syscalls_limits(language);

        pid_t pid = fork();
        if (pid == 0) {
            // child
            myexec(used_time);
            exit(0);
        } else {
            // parent
            watch(pid, judge_status, max_used_mem, used_time, error_size);

            if (DEBUG) {
                printf("Test %d: %ldms %ldKB\n", test_count, used_time, max_used_mem >> 10);
            }

            if (judge_status == JUDGE_RJ) {
                judge_status = check_user_output();
            }
            test_count += 1;
        }
    }

    if (judge_status == JUDGE_RJ) {
        judge_status = JUDGE_AC;
    }

    if (DEBUG) {
        printf("\nJudge finished:\n- result: [%d] %s\n- time used: %ld ms\n- mem used: %ld KB\n", judge_status, judge_string[judge_status], used_time, max_used_mem >> 10);
    } else {
        printf("%d,%ld,%ld", judge_status, used_time, max_used_mem);
    }

    return 0;
}
