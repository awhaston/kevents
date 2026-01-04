# KEvents Kernel Module
KEvents is a simple Linux kernel ioctl device for passing asynchronous messages between processes through the kernel.
It provides a small event broker inside the kernel where messages are tagged by an ID and retrieved later by any process that asks for that ID.
This repository contains:
- The kernel module
- A small userspace library
- 2 test applications 

## How it works
Userspace sends an event into the kernel along with an integer ID.
The kernel stores that event in a linked list protected by a mutex.
Other processes can query the kernel to see if an event with a given ID exists.
If it does, the message can be retrieved and removed from the broker.
Events are:
- FIFO within the same ID
- Removed once successfully retrieved
- Held in kernel memory until consumed
The kernel enforces mutual exclusion so only one process manipulates the event broker at a time.

## Library
A small C library wraps the ioctl interface and exposes a simple API.
Typical usage:
```C
// Initialize the context
kevents_ctx ctx;
int id = 1;
int err = ke_init(&ctx);
if (err) {
    // Handle error
}

char *msg = "Hello world\n";

// Send message to kernel
err = ke_send_event(&ctx, id, msg, strlen(msg));
if (err) {
    // Handle error
}

// Poll for message
if (ke_poll_event(&ctx, id)) {
    char buff[50];

    // Get message from kernel
    err = ke_get_event(&ctx, id, buff, sizeof(buff));
    if (err) {
        // Handle error
    }

    // Print message
    printf("%s", buff);
}
```

The library handles:
- Opening and closing the device
- Issuing the correct ioctl calls
- Passing data between userspace to the kernel

## Design notes
Messages are copied into kernel memory when sent and copied back out when retrieved.
There is no persistence. Events exist only in RAM.
The API is intentionally minimal and synchronous.

## Limitations
This project is not designed for:
- High throughput
- Large messages
- Untrusted input
- Security-sensitive environments
There is no authentication, rate limiting, or isolation between clients. Any process that can open the device can send and receive events.
