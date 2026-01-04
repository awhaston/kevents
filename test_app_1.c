#include <unistd.h>
#include <stdint.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>

#include <sys/ioctl.h>
#include "kevents_lib.h"
#include "kevents_usr.h"

#define MY_ID    1
#define OTHER_ID 2

int main(void)
{
    kevents_ctx ctx;
    int err;
    int messages = 0;
    int id = 1;

    err = ke_init(&ctx);

    if (err) {
        printf("Could not init kevents");
        return 1;
    }

    char *msg = "Hello world\n";
    char buff[1040] = { 0 };

    // for (int i = 0; i < 100; ++i) {
    //     snprintf(buff, strlen(msg), "%s %d\n", msg, i);
    //
    //     err = ke_send_event(&ctx, OTHER_ID, buff, sizeof(buff));
    //
    //     if (err)
    //         return 1;
    // }
    //
    // ioctl(ctx.file, KEVENTS_DEBUG, NULL);

    snprintf(buff, sizeof(buff), "%s\n", msg);

    err = ke_send_event(&ctx, OTHER_ID, buff, sizeof(buff));

    if (err)
        return 1;



    while (1) {
        if (ke_poll_event(&ctx, MY_ID)) {
            err = ke_get_event(&ctx, MY_ID, buff, sizeof(buff));
            
            if (err)
                return 1;

            printf("App 1: Got message %s\n", buff);
            messages++;

            snprintf(buff, sizeof(buff) - 1, "This is message from app 1! %d \n", messages);
            
            err = ke_send_event(&ctx, OTHER_ID, buff, strlen(buff));
            if (err)
                return 1;
        }

        usleep(100);
    }


    
    return 0;
}
