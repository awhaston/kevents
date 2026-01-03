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
    char message2[] = "Hello from message 2!";
    char out_msg[sizeof(message2)];

    struct kevents_req_send req =  {
        .id = 1,
        .len = sizeof(message),
        .msg = (uint64_t)&message
    };
    struct kevents_req_send req2 =  {
        .id = 2,
        .len = sizeof(message2),
        .msg = (uint64_t)&message2
    };

    struct kevents_req_get get_req= {
        .id = 2,
        .len = sizeof(out_msg),
        .out_len = 0,
        .msg = (uint64_t)&out_msg
    };

    ioctl(dev, KEVENTS_SEND, &req);
    ioctl(dev, KEVENTS_SEND, &req2);

    ioctl(dev, KEVENTS_GET, &get_req);
    printf("%s\n", (char *)get_req.msg);
    get_req.id = 1;
    ioctl(dev, KEVENTS_GET, &get_req);
    printf("%s\n", (char *)get_req.msg);


    
    return 0;
}
