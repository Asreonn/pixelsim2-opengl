#include "core/thread_pool.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static void* worker_thread(void* arg) {
    ThreadPool* pool = (ThreadPool*)arg;
    
    while (1) {
        pthread_mutex_lock(&pool->queue_mutex);
        
        // Wait for work
        while (pool->queue_count == 0 && !pool->shutdown) {
            pthread_cond_wait(&pool->work_available, &pool->queue_mutex);
        }
        
        if (pool->shutdown && pool->queue_count == 0) {
            pthread_mutex_unlock(&pool->queue_mutex);
            break;
        }
        
        // Get work item
        WorkItem item = pool->queue[pool->queue_head];
        pool->queue_head = (pool->queue_head + 1) % WORK_QUEUE_SIZE;
        pool->queue_count--;
        pool->active_workers++;
        
        pthread_mutex_unlock(&pool->queue_mutex);
        
        // Execute work
        if (item.func) {
            item.func(item.arg);
        }
        
        // Mark work as done
        pthread_mutex_lock(&pool->queue_mutex);
        pool->active_workers--;
        pool->pending_work--;
        
        if (pool->pending_work == 0) {
            pthread_cond_broadcast(&pool->work_done);
        }
        pthread_mutex_unlock(&pool->queue_mutex);
    }
    
    return NULL;
}

bool thread_pool_init(ThreadPool* pool, i32 num_threads) {
    if (!pool || num_threads <= 0) return false;
    
    if (num_threads > MAX_WORKER_THREADS) {
        num_threads = MAX_WORKER_THREADS;
    }
    
    pool->queue_head = 0;
    pool->queue_tail = 0;
    pool->queue_count = 0;
    pool->active_workers = 0;
    pool->pending_work = 0;
    pool->num_threads = num_threads;
    pool->shutdown = false;
    
    if (pthread_mutex_init(&pool->queue_mutex, NULL) != 0) {
        return false;
    }
    
    if (pthread_cond_init(&pool->work_available, NULL) != 0) {
        pthread_mutex_destroy(&pool->queue_mutex);
        return false;
    }
    
    if (pthread_cond_init(&pool->work_done, NULL) != 0) {
        pthread_cond_destroy(&pool->work_available);
        pthread_mutex_destroy(&pool->queue_mutex);
        return false;
    }
    
    // Create worker threads
    for (i32 i = 0; i < num_threads; i++) {
        if (pthread_create(&pool->workers[i], NULL, worker_thread, pool) != 0) {
            // Cleanup on failure
            pool->shutdown = true;
            pthread_cond_broadcast(&pool->work_available);
            for (i32 j = 0; j < i; j++) {
                pthread_join(pool->workers[j], NULL);
            }
            pthread_cond_destroy(&pool->work_done);
            pthread_cond_destroy(&pool->work_available);
            pthread_mutex_destroy(&pool->queue_mutex);
            return false;
        }
    }
    
    printf("Thread pool initialized with %d workers\n", num_threads);
    return true;
}

void thread_pool_shutdown(ThreadPool* pool) {
    if (!pool) return;
    
    pthread_mutex_lock(&pool->queue_mutex);
    pool->shutdown = true;
    pthread_cond_broadcast(&pool->work_available);
    pthread_mutex_unlock(&pool->queue_mutex);
    
    for (i32 i = 0; i < pool->num_threads; i++) {
        pthread_join(pool->workers[i], NULL);
    }
    
    pthread_cond_destroy(&pool->work_done);
    pthread_cond_destroy(&pool->work_available);
    pthread_mutex_destroy(&pool->queue_mutex);
    
    printf("Thread pool shutdown complete\n");
}

void thread_pool_submit(ThreadPool* pool, WorkFunc func, void* arg) {
    if (!pool || !func) return;
    
    pthread_mutex_lock(&pool->queue_mutex);
    
    // Wait if queue is full
    while (pool->queue_count >= WORK_QUEUE_SIZE && !pool->shutdown) {
        pthread_mutex_unlock(&pool->queue_mutex);
        // Busy wait briefly then retry
        usleep(100);
        pthread_mutex_lock(&pool->queue_mutex);
    }
    
    if (pool->shutdown) {
        pthread_mutex_unlock(&pool->queue_mutex);
        return;
    }
    
    // Add work item
    pool->queue[pool->queue_tail].func = func;
    pool->queue[pool->queue_tail].arg = arg;
    pool->queue_tail = (pool->queue_tail + 1) % WORK_QUEUE_SIZE;
    pool->queue_count++;
    pool->pending_work++;
    
    pthread_cond_signal(&pool->work_available);
    pthread_mutex_unlock(&pool->queue_mutex);
}

void thread_pool_wait(ThreadPool* pool) {
    if (!pool) return;
    
    pthread_mutex_lock(&pool->queue_mutex);
    while (pool->pending_work > 0) {
        pthread_cond_wait(&pool->work_done, &pool->queue_mutex);
    }
    pthread_mutex_unlock(&pool->queue_mutex);
}

i32 thread_pool_get_num_cores(void) {
    long cores = sysconf(_SC_NPROCESSORS_ONLN);
    if (cores < 1) cores = 1;
    if (cores > MAX_WORKER_THREADS) cores = MAX_WORKER_THREADS;
    return (i32)cores;
}
