#ifndef PIXELSIM_THREAD_POOL_H
#define PIXELSIM_THREAD_POOL_H

#include "core/types.h"
#include <pthread.h>

#define MAX_WORKER_THREADS 16
#define WORK_QUEUE_SIZE 256

typedef void (*WorkFunc)(void* arg);

typedef struct WorkItem {
    WorkFunc func;
    void* arg;
} WorkItem;

typedef struct ThreadPool {
    pthread_t workers[MAX_WORKER_THREADS];
    pthread_mutex_t queue_mutex;
    pthread_cond_t work_available;
    pthread_cond_t work_done;
    
    WorkItem queue[WORK_QUEUE_SIZE];
    i32 queue_head;
    i32 queue_tail;
    i32 queue_count;
    
    i32 active_workers;
    i32 pending_work;
    i32 num_threads;
    bool shutdown;
} ThreadPool;

// Lifecycle
bool thread_pool_init(ThreadPool* pool, i32 num_threads);
void thread_pool_shutdown(ThreadPool* pool);

// Work submission
void thread_pool_submit(ThreadPool* pool, WorkFunc func, void* arg);
void thread_pool_wait(ThreadPool* pool);

// Utility
i32 thread_pool_get_num_cores(void);

#endif
