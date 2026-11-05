/*
 * shiro_trace.c - source-level crash traces for Shiro programs.
 *
 * Link this file together with assembly produced by
 * `shiro --crash-trace`:
 *
 *     ./shiro prog.shiro --crash-trace -o prog.s
 *     gcc -no-pie prog.s runtime/shiro_trace.c -o prog
 *
 * A constructor installs handlers for SIGSEGV, SIGABRT, SIGILL and
 * SIGFPE. When the program crashes, the handler resolves the faulting
 * address and the rbp call chain against the location/function tables
 * the compiler embedded in .rodata (__shiro_loc_table /
 * __shiro_fn_table) and prints a trace like:
 *
 *     shiro crash trace: SIGFPE (8), faulting instruction 0x401136
 *       #0 boom (prog.shiro:2)
 *       #1 middle (prog.shiro:5)
 *       #2 main (prog.shiro:8)
 *
 * The handler then re-raises the signal with the default disposition,
 * so the process dies exactly as it would without this runtime (same
 * exit status / core dump behaviour).
 *
 * The tables are referenced as weak symbols: if the program was
 * compiled without --crash-trace they are absent and the handler only
 * prints the signal line before re-raising.
 *
 * Everything the handler does uses write(2) and hand-rolled formatting
 * only - no printf, no malloc - so it stays async-signal-safe.
 */

#define _GNU_SOURCE
#include <signal.h>
#include <stddef.h>
#include <stdint.h>
#include <ucontext.h>
#include <unistd.h>

/* Table layouts emitted by the compiler (src/x86gen.cpp). */
struct shiro_loc_entry {
    const void *addr;
    const char *file;
    uint32_t line;
    uint32_t reserved;
};

struct shiro_fn_entry {
    const void *addr;
    const char *name;
};

extern const struct shiro_loc_entry __shiro_loc_table[]
    __attribute__((weak));
extern const struct shiro_loc_entry __shiro_loc_table_end[]
    __attribute__((weak));
extern const struct shiro_fn_entry __shiro_fn_table[]
    __attribute__((weak));
extern const struct shiro_fn_entry __shiro_fn_table_end[]
    __attribute__((weak));
/* Label placed just past the last generated function; only its
 * address is used, to bound table lookups to Shiro-compiled code. */
extern const char __shiro_text_end[] __attribute__((weak));

/* ------------------------------------------------------------------ */
/* async-signal-safe output helpers                                    */

static void t_write_str(const char *s) {
    size_t n = 0;
    while (s[n] != '\0') {
        n++;
    }
    while (n > 0) {
        ssize_t w = write(STDERR_FILENO, s, n);
        if (w <= 0) {
            break;
        }
        s += (size_t)w;
        n -= (size_t)w;
    }
}

static void t_write_u64(uint64_t v) {
    char buf[20];
    int i = (int)sizeof(buf);
    if (v == 0) {
        buf[--i] = '0';
    }
    while (v > 0) {
        buf[--i] = (char)('0' + (v % 10));
        v /= 10;
    }
    ssize_t unused = write(STDERR_FILENO, buf + i, (size_t)((int)sizeof(buf) - i));
    (void)unused;
}

static void t_write_hex(uint64_t v) {
    char buf[16];
    int i = (int)sizeof(buf);
    t_write_str("0x");
    if (v == 0) {
        buf[--i] = '0';
    }
    while (v > 0) {
        int d = (int)(v & 0xf);
        buf[--i] = (char)(d < 10 ? '0' + d : 'a' + (d - 10));
        v >>= 4;
    }
    ssize_t unused = write(STDERR_FILENO, buf + i, (size_t)((int)sizeof(buf) - i));
    (void)unused;
}

/* ------------------------------------------------------------------ */
/* table lookup                                                        */

/* Tables are sorted by address (emission order); the last entry whose
 * address is <= pc wins, which also resolves ties at one address to
 * the most recently emitted entry. */
static const struct shiro_fn_entry *find_fn(const void *pc) {
    const struct shiro_fn_entry *best = NULL;
    long i, count;
    if (__shiro_fn_table == NULL || __shiro_fn_table_end == NULL) {
        return NULL;
    }
    if (pc >= (const void *)__shiro_text_end) {
        return NULL; /* past the last Shiro function: libc / CRT code */
    }
    count = __shiro_fn_table_end - __shiro_fn_table;
    for (i = 0; i < count; i++) {
        if (__shiro_fn_table[i].addr <= pc) {
            best = &__shiro_fn_table[i];
        }
    }
    return best;
}

static const struct shiro_loc_entry *find_loc(const void *pc) {
    const struct shiro_loc_entry *best = NULL;
    long i, count;
    if (__shiro_loc_table == NULL || __shiro_loc_table_end == NULL) {
        return NULL;
    }
    if (pc >= (const void *)__shiro_text_end) {
        return NULL;
    }
    count = __shiro_loc_table_end - __shiro_loc_table;
    for (i = 0; i < count; i++) {
        if (__shiro_loc_table[i].addr <= pc) {
            best = &__shiro_loc_table[i];
        }
    }
    return best;
}

static const char *signal_name(int sig) {
    switch (sig) {
        case SIGSEGV:
            return "SIGSEGV";
        case SIGABRT:
            return "SIGABRT";
        case SIGILL:
            return "SIGILL";
        case SIGFPE:
            return "SIGFPE";
        default:
            return "SIG";
    }
}

static void print_frame(int index, const void *pc) {
    const struct shiro_fn_entry *fn = find_fn(pc);
    const struct shiro_loc_entry *loc = find_loc(pc);

    t_write_str("  #");
    t_write_u64((uint64_t)index);
    t_write_str(" ");
    t_write_str(fn != NULL ? fn->name : "<unknown>");
    t_write_str(" (");
    if (loc != NULL) {
        t_write_str(loc->file);
        t_write_str(":");
        t_write_u64(loc->line);
    } else {
        t_write_str("??:0");
    }
    t_write_str(")\n");
}

/* ------------------------------------------------------------------ */
/* handler                                                             */

static void shiro_trace_handler(int sig, siginfo_t *info, void *uctx) {
    ucontext_t *uc = (ucontext_t *)uctx;
    uintptr_t rip = (uintptr_t)uc->uc_mcontext.gregs[REG_RIP];
    uintptr_t rbp = (uintptr_t)uc->uc_mcontext.gregs[REG_RBP];
    int have_tables = __shiro_loc_table != NULL && __shiro_fn_table != NULL &&
                      __shiro_text_end != NULL;
    int frame;

    t_write_str("shiro crash trace: ");
    t_write_str(signal_name(sig));
    t_write_str(" (");
    t_write_u64((uint64_t)sig);
    t_write_str("), faulting instruction ");
    t_write_hex(rip);
    if (sig == SIGSEGV && info != NULL) {
        t_write_str(", fault address ");
        t_write_hex((uint64_t)(uintptr_t)info->si_addr);
    }
    t_write_str("\n");

    if (have_tables) {
        /* Frame 0 is the faulting instruction itself. */
        print_frame(0, (const void *)rip);

        /* Walk the rbp chain: [rbp] is the caller's rbp, [rbp + 8] is
         * the return address, which lands inside the calling
         * instruction's code and so resolves to the call site.
         * Best effort: stop on a null, non-increasing, or runaway
         * chain (e.g. once it leaves Shiro-compiled code). */
        frame = 1;
        while (rbp != 0 && frame <= 32) {
            uintptr_t ret = *(const uintptr_t *)(rbp + 8);
            uintptr_t next_rbp = *(const uintptr_t *)rbp;
            if (ret == 0) {
                break;
            }
            print_frame(frame, (const void *)ret);
            frame++;
            if (find_fn((const void *)ret) == NULL) {
                break; /* left Shiro-compiled code (libc, _start, ...) */
            }
            if (next_rbp <= rbp) {
                break;
            }
            rbp = next_rbp;
        }
    } else {
        t_write_str("  (no crash-trace tables; recompile with "
                    "--crash-trace)\n");
    }

    /* Die as if this runtime had never been linked: restore the
     * default disposition and re-raise. */
    signal(sig, SIG_DFL);
    raise(sig);
    _exit(128 + sig);
}

/* Alternate signal stack: without it, a stack-overflow SIGSEGV cannot
 * run the handler at all (there is no room left on the user stack).
 * A static buffer keeps the constructor malloc-free. */
static unsigned char shiro_alt_stack[64 * 1024]
    __attribute__((aligned(16)));

__attribute__((constructor)) static void shiro_trace_install(void) {
    stack_t ss;
    struct sigaction sa;

    ss.ss_sp = shiro_alt_stack;
    ss.ss_size = sizeof(shiro_alt_stack);
    ss.ss_flags = 0;
    sigaltstack(&ss, NULL);

    sa.sa_sigaction = shiro_trace_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_SIGINFO | SA_ONSTACK;
    sigaction(SIGSEGV, &sa, NULL);
    sigaction(SIGABRT, &sa, NULL);
    sigaction(SIGILL, &sa, NULL);
    sigaction(SIGFPE, &sa, NULL);
}
