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

static im_subscription_manager_t g_sub_manager;

im_result_t im_event_init() {
    printf("  Event System: Initializing...\n");
    g_sub_manager.count = 0;
    pthread_mutex_init(&g_sub_manager.mutex, NULL);
    return IM_OK;
}

im_result_t im_event_shutdown() {
    printf("  Event System: Shutting down...\n");
    pthread_mutex_destroy(&g_sub_manager.mutex);
    return IM_OK;
}

im_result_t im_event_emit(im_event_t event) {
    pthread_mutex_lock(&g_sub_manager.mutex);
    
    // In a real system, we might queue this and dispatch in a separate thread
    // For now, synchronous dispatch to all subscribers (careful with deadlocks!)
    for (int i = 0; i < g_sub_manager.count; i++) {
        if (g_sub_manager.subscribers[i].type == event.type || g_sub_manager.subscribers[i].type == IM_EVENT_NONE) {
            // Synchronous call for now. 
            // TODO: Move to async task if specified in the event/subscriber
            g_sub_manager.subscribers[i].callback(&event, g_sub_manager.subscribers[i].user_data);
        }
    }
    
    if (event.payload && event.payload_size > 0) {
        free(event.payload);
    }
    
    pthread_mutex_unlock(&g_sub_manager.mutex);
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
            // Remove by swapping with last
            g_sub_manager.subscribers[i] = g_sub_manager.subscribers[g_sub_manager.count - 1];
            g_sub_manager.count--;
            pthread_mutex_unlock(&g_sub_manager.mutex);
            return IM_OK;
        }
    }
    
    pthread_mutex_unlock(&g_sub_manager.mutex);
    return IM_ERR_NOT_FOUND;
}
