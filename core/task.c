#include "task.h"
#include "event.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdatomic.h>

#define MAX_WORKERS 4
#define MAX_TASKS 1024

typedef struct {
    im_task_t* tasks[MAX_TASKS];
    int count;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
} im_task_queue_t;

static im_task_queue_t g_task_queue;
static pthread_t g_workers[MAX_WORKERS];
static _Atomic bool g_running = false;
static _Atomic uint64_t g_next_task_id = 1;

static void* worker_proc(void* arg) {
    (void)arg;
    while (g_running) {
        im_task_t* task = NULL;

        pthread_mutex_lock(&g_task_queue.mutex);
        while (g_running && g_task_queue.count == 0) {
            pthread_cond_wait(&g_task_queue.cond, &g_task_queue.mutex);
        }

        if (!g_running) {
            pthread_mutex_unlock(&g_task_queue.mutex);
            break;
        }

        int best_idx = -1;
        im_priority_t highest_prio = -1;

        for (int i = 0; i < g_task_queue.count; i++) {
            if ((int)g_task_queue.tasks[i]->priority > (int)highest_prio) {
                highest_prio = g_task_queue.tasks[i]->priority;
                best_idx = i;
            }
        }

        if (best_idx != -1) {
            task = g_task_queue.tasks[best_idx];
            g_task_queue.tasks[best_idx] = g_task_queue.tasks[g_task_queue.count - 1];
            g_task_queue.count--;
        }

        pthread_mutex_unlock(&g_task_queue.mutex);

        if (task) {
            task->state = IM_TASK_STATE_RUNNING;
            task->result = task->func(task->input_data, &task->cancelled);
            task->state = task->cancelled ? IM_TASK_STATE_CANCELLED : IM_TASK_STATE_COMPLETED;
            
            im_event_t ev = {0};
            ev.type = IM_EVENT_TASK_COMPLETED;
            ev.payload = malloc(sizeof(uint64_t));
            *(uint64_t*)ev.payload = task->id;
            ev.payload_size = sizeof(uint64_t);
            im_event_emit(ev);
        }
    }
    return NULL;
}

im_result_t im_task_init() {

    
    g_task_queue.count = 0;
    pthread_mutex_init(&g_task_queue.mutex, NULL);
    pthread_cond_init(&g_task_queue.cond, NULL);
    
    g_running = true;
    for (int i = 0; i < MAX_WORKERS; i++) {
        pthread_create(&g_workers[i], NULL, worker_proc, NULL);
    }
    
    return IM_OK;
}

im_result_t im_task_shutdown() {

    
    g_running = false;
    pthread_mutex_lock(&g_task_queue.mutex);
    pthread_cond_broadcast(&g_task_queue.cond);
    pthread_mutex_unlock(&g_task_queue.mutex);
    
    for (int i = 0; i < MAX_WORKERS; i++) {
        pthread_join(g_workers[i], NULL);
    }
    
    pthread_mutex_destroy(&g_task_queue.mutex);
    pthread_cond_destroy(&g_task_queue.cond);
    
    return IM_OK;
}

im_result_t im_task_submit(im_task_t* task) {
    if (!task) return IM_ERR_INVALID_ARG;
    
    pthread_mutex_lock(&g_task_queue.mutex);
    if (g_task_queue.count >= MAX_TASKS) {
        pthread_mutex_unlock(&g_task_queue.mutex);
        return IM_ERR;
    }
    
    task->id = atomic_fetch_add(&g_next_task_id, 1);
    task->state = IM_TASK_STATE_QUEUED;
    task->cancelled = false;
    
    g_task_queue.tasks[g_task_queue.count++] = task;
    
    pthread_cond_signal(&g_task_queue.cond);
    pthread_mutex_unlock(&g_task_queue.mutex);
    
    return IM_OK;
}

im_result_t im_task_cancel(uint64_t task_id) {
    pthread_mutex_lock(&g_task_queue.mutex);
    for (int i = 0; i < g_task_queue.count; i++) {
        if (g_task_queue.tasks[i]->id == task_id) {
            g_task_queue.tasks[i]->cancelled = true;
            pthread_mutex_unlock(&g_task_queue.mutex);
            return IM_OK;
        }
    }
    pthread_mutex_unlock(&g_task_queue.mutex);
    return IM_ERR_NOT_FOUND;
}

im_task_state_t im_task_get_status(uint64_t task_id) {
    (void)task_id;
    return IM_TASK_STATE_RUNNING; 
}
