#include <fcntl.h>
#include <sys/ioctl.h>
#include <stdint.h>

#include "kevents_lib.h"
#include "kevents_usr.h"

int ke_init(kevents_ctx *ctx)
{
    if (!ctx)
        return NULLCTX;

    ctx->file = open("/dev/kevents", O_RDWR);

    if (ctx->file < 0) 
        return FILEERR;
    
    return 0;
}

int ke_poll_event(kevents_ctx *ctx, int id)
{
    if (!ctx)
        return NULLCTX;

    struct kevents_req_poll req = {
        .id = id,
    };

    return ioctl(ctx->file, KEVENTS_POLL, &req);
}

int ke_send_event(kevents_ctx *ctx, int id, char *msg, int len)
{
    if (!ctx)
        return NULLCTX;

    if (len > MSG_MAX)
        return MSGTOOLARGE;

    int err = 0;
    struct kevents_req_send req = {
        .id  = id,
        .msg = (uint64_t)msg,
        .len = len
    };

    return ioctl(ctx->file, KEVENTS_SEND, &req);
}

int ke_get_event(kevents_ctx *ctx, int id, char *msg, int len)
{
    if (!ctx)
        return NULLCTX;

    if (len > MSG_MAX)
        return MSGTOOLARGE;

    struct kevents_req_get req = {
        .id      = id,
        .msg     = (uint64_t) msg,
        .len     = len,
        .out_len = 0
    };

    return ioctl(ctx->file, KEVENTS_GET, &req);
}
