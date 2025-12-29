#ifndef KEVENTS_H
#define KEVENTS_H
#include <linux/types.h>
#include <asm/types.h>
#include <linux/mutex.h>
#include "kevents_usr.h"


struct ke_event {
    u32 id;
    char msg[MSG_MAX];
    u32   len;
    struct ke_event *next; 
    struct ke_event *prev;
};

struct ke_broker {
    struct mutex lock;
    u32 count;
    struct ke_event *head;
    struct ke_event *tail;
};


void init_broker(struct ke_broker *b);
int push_event_to_broker(struct ke_broker *b, u32 id, char *msg, u64 len);
int pop_event_to_broker(struct ke_broker *b, struct ke_event *e,  u32 id);
int poll_event_broker(struct ke_broker *b, u32 id);

struct ke_event *create_event(u32 id, char *msg, u64 len);

#endif
