#ifndef KEVENTS_USR_H
#define KEVENTS_USR_H

#include <asm/types.h>

#define MSG_MAX 1440

struct kevents_req_send {
    __u32 id;
    __u64 msg;
    __u64 len;
};

struct kevents_req_get {
    __u32 id;
    __u64 msg;
    __u64 len;
    __u64 out_len;
};

struct kevents_req_poll {
    __u32 id;
};

#define MAGIC           'M'
#define KEVENTS_POLL    _IOWR(MAGIC, 1, struct kevents_req_poll)
#define KEVENTS_GET     _IOWR(MAGIC, 2, struct kevents_req_get)
#define KEVENTS_SEND    _IOWR(MAGIC, 3, struct kevents_req_send)
#if DEBUG
#define KEVENTS_DEBUG   _IO(MAGIC, 4)
#endif

#endif
