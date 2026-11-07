#include "user/thread.h"
#include "user/user.h"
#include "kernel/proc.h"

int thread_create(void *(start_routine)(void*), void *arg) {
  int pid, tid;

  struct proc p = myproc();

  pid = clone(p->kstack);
  if (pid < 0){
    printf("thread_create failed");
    exit(1);
  }
  else if (pid == 0){
    start_routine(arg); // child executes specified function
  }
}

void lock_init(struct lock_t* lock) {
  lock->locked = 0;
}

void lock_acquire(struct lock_t* lock) {
  while (__sync_lock_test_and_set(&lock->locked, 1) != 0)
    ;

  __sync_synchronize();
}

void lock_release(struct lock_t* lock) {
  __sync_synchronize();

  __sync_lock_release(&lock->locked);
}
