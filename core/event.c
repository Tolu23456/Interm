#include "event.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#define MAX_SUBSCRIBERS 256
#define MAX_PENDING_EVENTS 1024

typedef struct {
    im_event_type_t type;
    im_event_callback_t callback;
    void* user_data;
} im_subscriber_t;

typedef struct {
    im_subscriber_t subscribers[MAX_SUBSCRIBERS];
    int count;
    pthread_mutex_t mutex;
} im_subscription_manager_t;

typedef struct {
    im_event_t events[MAX_PENDING_EVENTS];
    int head;
    int tail;
    int count;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
} im_event_queue_t;

static im_subscription_manager_t g_sub_manager;
static im_event_queue_t g_event_queue;
static pthread_t g_event_thread;
static bool g_running = false;

static void* event_dispatch_proc(void* arg) {
    (void)arg;
    while (g_running) {
        im_event_t event;
        bool has_event = false;

        pthread_mutex_lock(&g_event_queue.mutex);
        while (g_running && g_event_queue.count == 0) {
            pthread_cond_wait(&g_event_queue.cond, &g_event_queue.mutex);
        }

        if (!g_running) {
            pthread_mutex_unlock(&g_event_queue.mutex);
            break;
        }

        if (g_event_queue.count > 0) {
            event = g_event_queue.events[g_event_queue.head];
            g_event_queue.head = (g_event_queue.head + 1) % MAX_PENDING_EVENTS;
            g_event_queue.count--;
            has_event = true;
        }
        pthread_mutex_unlock(&g_event_queue.mutex);

        if (has_event) {
            pthread_mutex_lock(&g_sub_manager.mutex);
            for (int i = 0; i < g_sub_manager.count; i++) {
                if (g_sub_manager.subscribers[i].type == event.type || g_sub_manager.subscribers[i].type == IM_EVENT_NONE) {
                    g_sub_manager.subscribers[i].callback(&event, g_sub_manager.subscribers[i].user_data);
                }
            }
            pthread_mutex_unlock(&g_sub_manager.mutex);

            if (event.payload && event.payload_size > 0) {
                free(event.payload);
            }
        }
    }
    return NULL;
}

im_result_t im_event_init() {
    printf("  Event System: Initializing...\n");
    g_sub_manager.count = 0;
    pthread_mutex_init(&g_sub_manager.mutex, NULL);

    g_event_queue.head = 0;
    g_event_queue.tail = 0;
    g_event_queue.count = 0;
    pthread_mutex_init(&g_event_queue.mutex, NULL);
    pthread_cond_init(&g_event_queue.cond, NULL);

    g_running = true;
    pthread_create(&g_event_thread, NULL, event_dispatch_proc, NULL);

    return IM_OK;
}

im_result_t im_event_shutdown() {
    printf("  Event System: Shutting down...\n");
    g_running = false;
    pthread_mutex_lock(&g_event_queue.mutex);
    pthread_cond_signal(&g_event_queue.cond);
    pthread_mutex_unlock(&g_event_queue.mutex);
    pthread_join(g_event_thread, NULL);

    pthread_mutex_destroy(&g_sub_manager.mutex);
    pthread_mutex_destroy(&g_event_queue.mutex);
    pthread_cond_destroy(&g_event_queue.cond);
    return IM_OK;
}

im_result_t im_event_emit(im_event_t event) {
    pthread_mutex_lock(&g_event_queue.mutex);
    if (g_event_queue.count >= MAX_PENDING_EVENTS) {
        pthread_mutex_unlock(&g_event_queue.mutex);
        return IM_ERR;
    }

    g_event_queue.events[g_event_queue.tail] = event;
    g_event_queue.tail = (g_event_queue.tail + 1) % MAX_PENDING_EVENTS;
    g_event_queue.count++;

    pthread_cond_signal(&g_event_queue.cond);
    pthread_mutex_unlock(&g_event_queue.mutex);
    return IM_OK;
}

im_result_t im_event_subscribe(im_event_type_t type, im_event_callback_t callback, void* user_data) {
    pthread_mutex_lock(&g_sub_manager.mutex);
    if (g_sub_manager.count >= MAX_SUBSCRIBERS) {
        pthread_mutex_unlock(&g_sub_manager.mutex);
        return IM_ERR;
    }
    g_sub_manager.subscribers[g_sub_manager.count++] = (im_subscriber_t){type, callback, user_data};
    pthread_mutex_unlock(&g_sub_manager.mutex);
    return IM_OK;
}

im_result_t im_event_unsubscribe(im_event_type_t type, im_event_callback_t callback) {
    pthread_mutex_lock(&g_sub_manager.mutex);
    for (int i = 0; i < g_sub_manager.count; i++) {
        if (g_sub_manager.subscribers[i].type == type && g_sub_manager.subscribers[i].callback == callback) {
            g_sub_manager.subscribers[i] = g_sub_manager.subscribers[g_sub_manager.count - 1];
            g_sub_manager.count--;
            pthread_mutex_unlock(&g_sub_manager.mutex);
            return IM_OK;
        }
    }
    pthread_mutex_unlock(&g_sub_manager.mutex);
    return IM_ERR_NOT_FOUND;
}
