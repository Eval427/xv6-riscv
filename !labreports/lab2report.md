# Lab 1 Report - 10/3/26
### CSEN 383
### Zachary Common and Calianna Collins
### https://github.com/Eval427/xv6-riscv
#### https://youtu.be/-iB0XIucjic

## Modified files
```m
\ = Modified
+ = Added
- = Removed

\ Makefile

\ kernel/defs.h
\ kernel/proc.c
\ kernel/proc.h
\ kernel/syscall.c
\ kernel/syscall.h
\ kernel/sysproc.c

+ user/lab2test.c
+ user/sched_tickets.c
\ user/user.h
\ user/usys.pl
```

## Part 1
Part 1 includes the addition of two new syscalls: `sched_statistics(void)` and `sched_tickets(int)`

### `struct proc` Changes
The first necessary change to the codebase is the introduction of the necessary parameters to allow for lottery and stride scheduling. For lottery scheduling, each process needs to store its `tickets`. For stride scheduling, each process needs a `pass` and `stride`. Additionally, `ticks` was included in order to track the number of times a process is chosen in any scheduler.

```c
// Per-process state
struct proc {
  struct spinlock lock;

  // p->lock must be held when using these:
  enum procstate state;        // Process state
  void *chan;                  // If non-zero, sleeping on chan
  int killed;                  // If non-zero, have been killed
  int xstate;                  // Exit status to be returned to parent's wait
  int pid;                     // Process ID

  // wait_lock must be held when using this:
  struct proc *parent;         // Parent process

  // these are private to the process, so p->lock need not be held.
  uint64 kstack;               // Virtual address of kernel stack
  uint64 sz;                   // Size of process memory (bytes)
  pagetable_t pagetable;       // User page table
  struct trapframe *trapframe; // data page for trampoline.S
  struct context context;      // swtch() here to run process
  struct file *ofile[NOFILE];  // Open files
  struct inode *cwd;           // Current directory
  char name[16];               // Process name (debugging)
  int syscall_count;           // Total number of system calls that the current process has made
  int tickets; /*<--*/         // Number of tickets for the lottery scheduling process
  int ticks;   /*<--*/         // Number of times this process was called (chosen) by scheduler
  int pass;    /*<--*/
  int stride;  /*<--*/
};
```