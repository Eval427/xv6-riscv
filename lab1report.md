# Lab 1 Report - 10/3/26
### CSEN 383
### Zachary Common and Calianna Collins
### https://github.com/Eval427/xv6-riscv

## Modified files
```m
\ = Modified
+ = Added
- = Removed

\ Makefile

\ kernel/defs.h
\ kernel/kalloc.c
\ kernel/proc.c
\ kernel/proc.h
\ kernel/syscall.c
\ kernel/syscall.h
\ kernel/sysproc.c

+ user/lab1test.c
+ user/procinfo.c
+ user/sysinfo.c
\ user/user.h
\ user/usys.pl
+ user/test.c
```

## Part 0
This part of the lab is the introduction on how to implement system calls into the xv6 OS. Since the instructions for this step are provided in the slides, we will show only our output

```sh
xv6 kernel is booting

init: starting sh
$ test
Say hello to kernel 0
Hello from the kernel space 0
$ test 10
Say hello to kernel 10
Hello from the kernel space 10
```

## Part 1
The goal of part 1 was to provide the user with information about the system. Given an input of 0, 1, or 2, the total number of active processes, total system calls, or free memory pages in the system would be displayed. When the user inputs `sysinfo <0,1,2>`, the requested data is printed to the terminal and returned as an `int`.

### Dataflow
1. Every system call is executed on the user side initially. Therefore, on the user side, it is first necessary to define the name of the system call and the function declaration to determine the number of user arguments that must be given to the call.
    ```c
    // user/user.h:25

    int sysinfo(int); // sysinfo
    ```
    and
    ```c
    // user/usys.pl:40

    entry("sysinfo") # sysinfo syscall for user
    ```
    gets compiled into assembly in `usys.S` as
    ```S
    .global sysinfo
    sysinfo:
    li a7, SYS_sysinfo
    ecall
    ret
    ```

1. Once the system call is executed, it looks for its respective `main()` function. This is determined in `Makefile`:
    ```makefile
    # Makefile:99-143

    # Links the call to its respective .o file by name
    _%: %.o $(ULIB)

    UPROGS=\
        $U/_cat\
        ...
        $U/_test\
        $U/_sysinfo\
        ...
    ```
    and, for this part, implemented in `user/sysinfo.c`:
    ```c
    // user/sysinfo.c

    #include "kernel/types.h"
    #include "kernel/stat.h"
    #include "user/user.h"

    int main(int argc, char *argv[]) {
        int n=0;
        if (argc >= 2) n = atoi(argv[1]);
        else {
            printf("Usage: sysinfo <0/1/2>\n"); // active processes/num syscalls/free mem pages
            return -1;
        }
        
        return sysinfo(n);
    }
    ```

1. As soon as `sysinfo(n)` is called, the system call that was triggered and any parameters are stored in registers. The rest of the execution is then completed by the kernel (until the return value is passed back to the user on `user/sysinfo.c:13`). The table of entries in `syscall.h` allows the kernel and user space to reference the same syscall via a constant number.
    ```c
    // kernel/syscall.h:24

    #define SYS_sysinfo 23 // sysinfo
    ```

    These numbers are then mapped to functions in `syscall.c`:
    ```c
    // kernel/syscall.c:112-142
    extern uint64 sys_sysinfo(void);

    ...

    static uint64 (*syscalls[])(void) = {
    ...
    [SYS_sysinfo]  sys_sysinfo,  // sysinfo: syscall entry
    ...
    };
    ```

    And defined in `sysproc.c`:
    ```c
    // kernel/sysproc.c:103-114

    // sysinfo syscall definition
    uint64 sys_sysinfo(void) {
    int param;
    argint(0, &param);

    if (param >= 0 && param < 3) {
        print_sysinfo(param);
        return 0;
    }

    return -1;
    }
    ```

1. The function `print_sysinfo(int)` is implemented in `proc.c` not out of necessesity, but rather ease of implementation. Most information required by this system call resides in `proc.c` and changes that were required to fetch additional information were also made in the same place. Once executed, the return value of the function is routed back to `user/sysinfo.c:13` and, thusly, the user
    ```c
    // kernel/proc.c:703-733

    // sysinfo: print system info
    int print_sysinfo(int n) {
        switch (n) {
            case 0:
            struct proc *p;
            int active_procs = 0;
            for (p = proc; p < &proc[NPROC]; p++) {
                acquire(&p->lock);
                if (p->state > 1) { // > 1 is not USED/UNUSED
                active_procs++;
                }
                release(&p->lock);
            }
            printf("%d active system processes\n", active_procs);
            return active_procs;
            
            case 1:
            int total_syscalls = get_total_syscalls();
            printf("%d syscalls since boot\n", total_syscalls);
            return total_syscalls;
            
            case 2:
            int free_pages = freepages();
            printf("%d available pages\n", free_pages);
            return free_pages;
            
            default:
            printf("How did this even happen???\n");
            return -1;
        }
    }
    ```

### Changes
1. In order to get the number of free pages in the system, a new function `freepages(void)` was implemented in `kernel/kalloc.c`. To fetch the total number of available pages, it starts from the last used page in the kernel memory and walks through the linked list of free pages until it reaches the end
    ```c
    // kernel/kalloc.c:84-95

    // Fetch the number of free pages in the kernel
    int
    freepages(void)
    {
        int count = 0;
        struct run *r;

        // kmem.freelist represents the first available page in the list of pages in kernel memory
        for (r = kmem.freelist; r != 0; r = r->next) count++;

        return count;
    }
    ```
1. To track the total number of system calls executed since boot, we used an integer counter in `syscall.c` alongside a getter function:
    ```c
    // kernel/syscall.c:10-15

    // Record total number of system calls
    int total_syscalls = 0;

    int get_total_syscalls() {
    return total_syscalls;
    }
    ```
    and incremented the counter every time a system call was executed in `syscall(void)`:
    ```c
    // kernel.syscall.c:144-162

    void
    syscall(void)
    {
    int num;
    struct proc *p = myproc();

    num = p->trapframe->a7;
    if(num > 0 && num < NELEM(syscalls) && syscalls[num]) {
        // Use num to lookup the system call function for num, call it,
        // and store its return value in p->trapframe->a0
        p->trapframe->a0 = syscalls[num]();
        total_syscalls++; // <---
        p->syscall_count++;
    } else {
        printf("%d %s: unknown sys call %d\n",
                p->pid, p->name, num);
        p->trapframe->a0 = -1;
    }
    }
    ```

### Output
```sh
$ sysinfo 0
3 active system processes
$ sysinfo 1
128 syscalls since boot
$ sysinfo 2
32532 available pages
```