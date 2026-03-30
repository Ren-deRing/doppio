#pragma once

/* POSIX SIGNALS */
#define SIGHUP       1  /* Hangup */
#define SIGINT       2  /* Interrupt */
#define SIGQUIT      3  /* Quit */
#define SIGILL       4  /* Illegal instruction */
#define SIGTRAP      5  /* Trace trap */
#define SIGABRT      6  /* Abort signal from system call */
#define SIGBUS       7  /* Bus Error */
#define SIGFPE       8  /* Erroneous arithmetic operation */
#define SIGKILL      9  /* YOU WERE MURDERED BY THE KERNEL! (or other process) */
#define SIGUSR1     10  /* User-defined Signal #1 */
#define SIGSEGV     11  /* Segmentation fault */
#define SIGUSR2     12  /* User-defined Signal #2 */
#define SIGPIPE     13  /* Accessed a broken pipe */
#define SIGALRM     14  /* wake up alarm (some kind of nightmare...) */
#define SIGTERM     15  /* User killed you */

#define SIGCHLD     17  /* Child status report */
#define SIGCONT     18  /* Resume a stopped process */
#define SIGSTOP     19  /* Pause */
#define SIGTSTP     20  /* Terminal stop request */

#define SIGWINCH    28  /* Window resize */
#define SIGIO       29  /* I/O now possible */
#define SIGPWR      30  /* Power failure */
#define SIGSYS      31  /* Bad system call */
#define NSIG        32  /* Signal Count */