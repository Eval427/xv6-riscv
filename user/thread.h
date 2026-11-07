struct lock_t {
  uint locked;
};

int thread_create(void *(start_routine)(void*), void *arg);
void lock_init(struct lock* lock);
void lock_acquire(struct lock* lock);
void lock_release(struct lock* lock);
