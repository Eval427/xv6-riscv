# Lab 3 Report - 11/12/26
### CSEN 383
### Zachary Common and Calianna Collins
### https://github.com/Eval427/xv6-riscv

#### https://youtu.be/iQllyCDZj8Y
## Modified Files
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
\ kernel/trampoline.S
\ kernel/trap.c
+ kernel/thread.c
+ kernel/thread.h

+ user/lab3test.c
+ user/thread.c
+ user/thread.h
\ user/user.h
\ user/usys.pl
```
## Part 1
In part 1, we added one new syscall, `clone(void*)`.
### Process Changes
In order for threads to be unique processes, we had to again edit each process in order to contain a thread id value, labeled `tid`. Processes also need to store the number of threads they currently own, since they have an upper limit of 20.
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
  int tid; /*<--*/             // Thread ID
  int threads; /*<--*/         // Number of threads in a process

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
  int tickets;                 // Number of tickets for the lottery scheduling process
  int ticks;                   // Number of times this process was called (chosen) by scheduler
  int pass;
  int stride;
};
```
### Threads vs Children
To create `clone`, we copied and modified fork so that it used the parents address space instead of a new one.
```c
int clone(void *stack) {
  struct proc *thread;
  struct proc *parent = myproc();
  int tid;
  int pid;
  int i;

  if (stack == 0) return -1;

  if (parent->tid != 0) return -1; // Threads cannot create threads

  if ((thread = allocthread(parent)) == 0) {
    return -1;
  }

  tid = thread->tid;
  thread->pagetable = parent->pagetable;
  thread->sz = parent->sz;

  if(mappages(thread->pagetable,
              TRAPFRAME - PGSIZE * tid,
              PGSIZE,
              (uint64)thread->trapframe,
              PTE_R | PTE_W) < 0){
    thread->pagetable = 0;    // Prevent freeproc() from freeing parent table
    freeproc(thread);
    release(&thread->lock);
    return -1;
  }

  *(thread->trapframe) = *(parent->trapframe);

  thread->trapframe->a0 = 0;
  thread->trapframe->sp = (uint64)stack;

    // increment reference counts on open file descriptors.
  for(i = 0; i < NOFILE; i++)
    if(parent->ofile[i])
      thread->ofile[i] = filedup(parent->ofile[i]);
  thread->cwd = idup(parent->cwd);

  safestrcpy(thread->name, parent->name, sizeof(thread->name));

  pid = thread->pid;

  release(&thread->lock);

  acquire(&wait_lock);
  thread->parent = parent;
  release(&wait_lock);

  acquire(&thread->lock);
  thread->state = RUNNABLE;
  release(&thread->lock);

  return pid;
}
```
Each thread also needed a new way to allocate it, so we created a new struct allocthread, which takes in a parent proc struct and assigns the threads characteristics based on this parent struct.
```c
static struct proc* allocthread(struct proc *parent)
{
  struct proc *p;
  uint64 K = 10000;

  for(p = proc; p < &proc[NPROC]; p++) {
    acquire(&p->lock);
    if(p->state == UNUSED) {
      goto found;
    } else {
      release(&p->lock);
    }
  }
  return 0;

found:
  if (parent->threads >= 20) {
    release(&p->lock);
    return 0;
  }

  p->pid = allocpid();
  p->tid = ++parent->threads;
  p->state = USED;
  p->tickets = 100; // Need at least one ticket to run at all - Give 100 for faster testing
  p->syscall_count = 0;
  p->stride = K / p->tickets;
  p->pass = p->stride;
  p->ticks = 0;

  p->pagetable = 0;
  p->sz = 0;

  p->trapframe = (struct trapframe *) kalloc();
  if (p->trapframe == 0) {
    freeproc(p);
    release(&p->lock);
    return 0;
  }

  memset(&p->context, 0, sizeof(p->context));
  p->context.ra = (uint64)forkret;
  p->context.sp = p->kstack + PGSIZE;

  return p;
}
```
This struct also calls `freeproc(struct proc)`, which we edited so that threads are freed differently from other processes. Notably, they have to keep the trapframe of the parent process intact, instead of freeing it back to the kernel.
```c
static void
freeproc(struct proc *p)
{
  if (p->tid > 0) {
    uvmunmap(p->pagetable, TRAPFRAME - PGSIZE * p->tid, 1, 0);
  }

  if(p->trapframe)
    kfree((void*)p->trapframe);
  p->trapframe = 0;
  if(p->tid == 0 && p->pagetable)
    proc_freepagetable(p->pagetable, p->sz);
  p->pagetable = 0;
  p->sz = 0;
  p->pid = 0;
  p->parent = 0;
  p->name[0] = 0;
  p->chan = 0;
  p->killed = 0;
  p->xstate = 0;
  p->state = UNUSED;
}
```
This allowed us to avoid making any changes to `wait()`, since all the changes to wait are actually relevant to processes freeing.