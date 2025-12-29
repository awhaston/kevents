#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include "kevents_usr.h"

int main(void)
{
    int dev = 0;
    dev = open("/dev/kevents", O_RDWR);

    if (dev < 0) {
        printf("Unable to open dev file\n");
        return 1;
    }

    char message[] = "Hello!";

    struct kevents_req_send req =  {
        .id = 1,
        .len = sizeof(message),
        .message = "Hello!"
    };

    struct kevents_resp resp = { 0 };
    struct kevents_req_get get_req= {
       .id = 1,
       .resp = &resp
    };

    ioctl(dev, KEVENTS_SEND, &req);

    ioctl(dev, KEVENTS_GET, &get_req);

    printf("%s", resp.msg);

    
    return 0;
}
