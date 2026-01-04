#include <asm-generic/errno-base.h>
#include <asm-generic/errno.h>
#include <linux/string.h>
#include <linux/uaccess.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/uaccess.h>
#include <linux/device.h>
#include <linux/kdev_t.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/mutex.h>

#include "kevents_kern.h"
#include "kevents_usr.h"

#define CLASS_NAME "kevents_class"
#define DEVICE_NAME "kevents"

#define LOG_INFO(msg) pr_info("kevents: %s", msg)
#define LOG_ERR(msg) pr_err("kevents: %s", msg)

static struct ke_broker *broker;


void init_broker(struct ke_broker *b)
{
    mutex_init(&b->lock);
    b->head  = NULL;
    b->count = 0;
}

struct ke_event *create_event(u32 id, char *msg, u64 len)
{
    struct ke_event *e = kmalloc(sizeof(struct ke_event), GFP_KERNEL);
    if (IS_ERR(e)) {
        LOG_ERR("unable to allocate new event\n");
        return e;
    }

    e->next = NULL;
    e->prev = NULL;
    e->id = id;
    e->msg = msg;
    e->len = len;
    
    return e;
}

int push_event_to_broker(struct ke_broker *b, u32 id, char *msg, u64 len)
{
    struct ke_event *e = create_event(id, msg, len);
    if (IS_ERR(e))
        return -EFAULT;

    if (b->head == NULL) {
        b->head = e;
        b->tail = e;
    
        return 0;
    } 

    struct ke_event *t = b->tail;

    t->next = e;
    e->prev = t;
    b->tail = e;
    return 0;
}

int push_event_to_front(struct ke_broker *b, struct ke_event *event)
{
    if (!b || !event)
        return -EINVAL;

    if (!b->head) {
        b->head = event;
        b->tail = event;
        event->next = NULL;
        event->prev = NULL;
        return 0;
    }

    event->next = b->head;
    event->prev = NULL;
    b->head->prev = event;
    b->head = event;
    return 0;
}

struct ke_event *pop_event_to_broker(struct ke_broker *b, u32 id)
{
    struct ke_event *c_event = b->head;

    while (c_event != NULL) {
        if (c_event->id == id) {
            break;
        }

        c_event = c_event->next;
    }

    if (c_event == NULL)
        return NULL;

    if (c_event->prev)
        c_event->prev->next = c_event->next;
    else
        b->head = c_event->next;

    if (c_event->next)
        c_event->next->prev = c_event->prev;
    else
        b->tail = c_event->prev;       

    return c_event;
}

int poll_event_broker(struct ke_broker *b, u32 id)
{
    struct ke_event *e = b->head;
    while (e != NULL) {
        if (e->id == id) {
            return 1;
        }
        e = e->next;
    }
    return 0; 
}

void free_event(struct ke_event *event)
{
   if (event->msg) 
       kfree(event->msg);
   if (event)
       kfree(event);
}

void free_events(struct ke_broker *b)
{
    struct ke_event *e = b->head;
    struct ke_event *n = NULL;
    while (e != NULL) {
        n = e->next;
        free_event(e);
        e = n;
    }
}

static long int kevents_ioctl(struct file *file,
                          unsigned int cmd,
                          unsigned long arg)
{


    LOG_INFO("Message from ioctl");
    
    switch (cmd) {
    case KEVENTS_POLL: {
        LOG_INFO("Poll request\n");
        struct kevents_req_poll req = { 0 };

        if (copy_from_user(&req, (void *) arg, sizeof(req))) {
            LOG_ERR("unable to copy user struct\n");
            return -EFAULT;
        }
        
        mutex_lock(&broker->lock);
        pr_info("kevents: got req id %d\n", req.id);
        int ret = poll_event_broker(broker, req.id);
        mutex_unlock(&broker->lock);

        return ret;
    } break;
    case KEVENTS_SEND: {
        LOG_INFO("Send request\n");
        struct kevents_req_send req = { 0 };
        int err = copy_from_user(&req, (void *)arg, sizeof(req));
        
        if (err){
            pr_alert("error from copying %d", err);
            return -EFAULT;
        }

        if (req.len > MSG_MAX) {
            return -EINVAL;
        }

        char *msg = memdup_array_user((const void *)req.msg, req.len, sizeof(char));

        if (IS_ERR(msg)) {
            return -ENOMEM;
        }

        mutex_lock(&broker->lock);
        pr_info("kevents: got req id %d\n", req.id);
        err = push_event_to_broker(broker, req.id, msg, req.len);
        mutex_unlock(&broker->lock);
        if (err != 0) {
            LOG_ERR("unable to push event to queue\n");
            return -EFAULT;
        }

        LOG_INFO("Successfully send message\n");
    } break;
    case KEVENTS_GET: {
        LOG_INFO("Get request\n");
        struct kevents_req_get req = { 0 };
        struct ke_event *event = NULL;
        int err = copy_from_user(&req, (void *)arg, sizeof(req));

        if (err){
            pr_alert("kevents: error from copying %d", err);
            return -EFAULT;
        }

        if (req.len > MSG_MAX) {
            LOG_ERR("error message too big");
            return -EINVAL;
        }

        pr_info("kevents: got req id %d\n", req.id);

        mutex_lock(&broker->lock);
        event = pop_event_to_broker(broker, req.id);
        if (event == NULL) {
            mutex_unlock(&broker->lock);
            return 0;
        }

        if (event->len > req.len) {
            LOG_ERR("message len too short\n");
            push_event_to_front(broker, event);
            mutex_unlock(&broker->lock);
            return -EINVAL;
        }

        mutex_unlock(&broker->lock); // We do not need the mutex from here

        req.out_len = event->len;
        if (copy_to_user((void *)req.msg, event->msg, event->len)) {
            free_event(event);
            return -EFAULT;
        }

        if (copy_to_user((void *)arg, &req, sizeof(req))) {
            free_event(event);
            return -EFAULT;
        }

        free_event(event);

    } break;
#if DEBUG
    case KEVENTS_DEBUG: {
        struct ke_event *e = broker->head;
        while (e != NULL) {
            pr_info("Message with id %d and message %s\n", e->id, e->msg);
            e = e->next;
        }
    } break;
#endif
    default:
        return -ENOTTY;
    }

    return 0;
}

static struct class  *class = NULL;
static struct device *dev   = NULL;
static dev_t          devno = 0; 
static struct cdev    cdev;

static const struct file_operations kevents_fops = {
    .owner          = THIS_MODULE,
    .unlocked_ioctl = kevents_ioctl
};

static int __init kevents_init(void)
{
    broker = kmalloc(sizeof(struct ke_broker), GFP_KERNEL);
    if (IS_ERR(broker)) {
        LOG_ERR("unable to allocate memory for broker\n");
        return -ENOMEM;
    }

    init_broker(broker);

    // Dynamically allocate a major number
    if (alloc_chrdev_region(&devno, 0, 1, DEVICE_NAME) < 0) {
        pr_err("unable to create character device\n");
        return devno;
    }

    class = class_create(CLASS_NAME);
    if (IS_ERR(class)) {
        LOG_ERR("unable to create class\n");
        unregister_chrdev_region(devno, 1);
        return -EFAULT;
    }

    dev = device_create(class, NULL, devno, NULL, DEVICE_NAME);

    if (IS_ERR(dev)) {
        LOG_ERR("unable to create device\n");
        class_destroy(class);
        unregister_chrdev_region(devno, 1);
        return -EFAULT;
    }

    cdev_init(&cdev, &kevents_fops);
    if (cdev_add(&cdev, devno, 1)) {
        LOG_ERR("unable to create cdev\n");
        device_destroy(class, MKDEV(devno, 1));
        class_destroy(class);
        unregister_chrdev_region(devno, 1);
        return -EFAULT;
    }

    LOG_INFO("loaded module\n");
    return 0;
}

static void __exit kevents_exit(void)
{
    LOG_INFO("removing kevents driver\n");
    free_events(broker);
    if (broker)
        kfree(broker);

    cdev_del(&cdev);
    device_destroy(class, devno);
    class_destroy(class);
    unregister_chrdev_region(devno, 1);
    LOG_INFO("driver removed successfully\n");
}



module_init(kevents_init);
module_exit(kevents_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Wyatt Haston");
MODULE_DESCRIPTION("Kernel based message system");
MODULE_VERSION("0.1");

