#include <asm-generic/errno-base.h>
#include <asm-generic/errno.h>
#include <linux/string.h>
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
    memcpy(e->msg, msg, len);
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

int pop_event_to_broker(struct ke_broker *b, struct ke_event *e, u32 id)
{
    struct ke_event *c_event = b->head;

    while (c_event != NULL) {
        if (c_event->id == id) {
            break;
        }

        c_event = c_event->next;
    }

    if (c_event == NULL)
        return -2;

    e->id = c_event->id;
    e->len = c_event->len;
    memcpy(e->msg, c_event->msg, e->len);
    

    struct ke_event *next = c_event->next;
    struct ke_event *prev = c_event->prev;

    if (c_event == b->head)
        b->head = NULL;

    // If we don't have a prev it should null the tail
    if (c_event == b->tail)
        b->tail = prev;

    // Link the next and prev together
    if (next != NULL) {
        next->prev = prev;
    }

    if (prev != NULL) {
        prev->next = next;
    }

    // No memory leaks here or double frees
    if(c_event)
        kfree(c_event);

    return 0;
}

static long int kevents_ioctl(struct file *file,
                          unsigned int cmd,
                          unsigned long arg)
{


    LOG_INFO("Message from ioctl");
    
    switch (cmd) {
    case KEVENTS:
        LOG_INFO("Cmd 1\n");
        break;
    case KEVENTS_SEND: {
        LOG_INFO("Send request\n");
        struct kevents_req_send req = { 0 };
        int err = copy_from_user(&req, (void *)arg, sizeof(req));
        
        if (err){
            pr_alert("error from copying %d", err);
            return -EFAULT;
        }

        pr_info("Struct given %d, %llu, %s", req.id, req.len, req.message);

        for (int i = 0; i < req.len; ++i) {
            printk("%c\n", req.message[i]);
        }

        mutex_lock(&broker->lock);
        err = push_event_to_broker(broker, req.id, req.message, req.len);
        mutex_unlock(&broker->lock);
        if (err != 0) {
            LOG_ERR("unable to push event to queue\n");
            return -ENOTTY;
        }

        break;
    }
    case KEVENTS_GET: {
        LOG_INFO("Get request\n");
        struct kevents_req_get req = { 0 };
        struct ke_event event = { 0 };
        int err = copy_from_user(&req, (void *)arg, sizeof(req));

        if (err){
            pr_alert("error from copying %d", err);
            return -EFAULT;
        }

        mutex_lock(&broker->lock);
        err = pop_event_to_broker(broker, &event, req.id);
        mutex_unlock(&broker->lock);
        if (err != 0) {
            pr_alert("unable to pop event to queue %d\n", err);
            return -ENOTTY;
        }

        req.resp->len = event.len;
        if (copy_to_user(req.resp->msg, event.msg, event.len)) {
            return -ENOTTY;
        }

        break;
    }
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
        return -EFAULT;
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

