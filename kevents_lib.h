#ifndef KEVENTS_LIB_H
#define KEVENTS_LIB_H

#include <stdio.h>

typedef struct {
    int file;
} kevents_ctx;

typedef enum {
    NOERR,
    FILEERR,
    MSGTOOLARGE,
    NULLCTX
} ke_err_e;

int ke_init(kevents_ctx *ctx);
int ke_poll_event(kevents_ctx *ctx, int id);
int ke_send_event(kevents_ctx *ctx, int id, char *msg, int len);
int ke_get_event(kevents_ctx *ctx, int id, char *msg, int len);
int ke_close(kevents_ctx *ctx);

#endif
