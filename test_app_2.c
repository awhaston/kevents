#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>

#include <sys/ioctl.h>

#include "kevents_lib.h"

#include "kevents_lib.h"

#define MY_ID    2
#define OTHER_ID 1

int main(void)
{
    kevents_ctx ctx;
    int err;
    int id = 1;
    int messages = 0;
    err = ke_init(&ctx);

    if (err) {
        printf("Could not init kevents");
        return 1;
    }


    char buff[1040] = { 0 };

    while (1) {
        if (ke_poll_event(&ctx, MY_ID)) {
            err = ke_get_event(&ctx, MY_ID, buff, sizeof(buff));
            
            if (err) {
                printf("Unable to get event\n");
                return 1;
            }

            printf("App 2: Got message %s\n", buff);
            messages++;

            snprintf(buff, sizeof(buff) - 1, "This is message from app 2! %d \n", messages);
            
            err = ke_send_event(&ctx, OTHER_ID, buff, strlen(buff));
            if (err) {
                printf("Unable to send event\n");
                return 1;
            }

            sleep(1);
        }

        sleep(1);
    }
    
    return 0;
}
