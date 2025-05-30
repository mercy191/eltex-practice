#include <linux/cdev.h> 
#include <linux/delay.h> 
#include <linux/device.h> 
#include <linux/fs.h> 
#include <linux/init.h> 
#include <linux/irq.h> 
#include <linux/kernel.h> 
#include <linux/module.h> 
#include <linux/poll.h> 
#include <asm/uaccess.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Nikita \"mercy191\" Ivanov");
MODULE_DESCRIPTION("Char device driver");

#define SUCCESS     0
#define DEVICE_NAME "chardev" 
#define BUF_LEN     100
#define TEXT_LEN    50

static int device_open(struct inode *, struct file *);
static int device_release(struct inode *, struct file *);
static ssize_t device_read(struct file *, char *, size_t, loff_t *);
static ssize_t device_write(struct file *, const char *, size_t, loff_t *);

static int              major;             
static int              is_device_open = 0;   
static char             msg[BUF_LEN];
static char             text[TEXT_LEN] = "";
static struct class*    cls; 

static struct file_operations chardev_fops = { 
    read: device_read, 
    write: device_write, 
    open: device_open, 
    release: device_release
};


static int device_open(struct inode *inode, struct file *file){ 
    static int counter = 0; 
 
    if (is_device_open) {
        return -EBUSY; 
    }

    is_device_open++;
    sprintf(msg, "Counter of opening: %d. Text: %s\n", counter++, text); 
    try_module_get(THIS_MODULE); 
    return SUCCESS; 
} 
 
static int device_release(struct inode *inode, struct file *file){ 
    is_device_open--;

    module_put(THIS_MODULE); 
    return SUCCESS; 
} 

static ssize_t device_read(struct file *filp, char __user *buffer, size_t length, loff_t *offset) { 
    int bytes_read = 0; 
    const char *msg_ptr = msg; 
    if (!*(msg_ptr + *offset)) { 
        *offset = 0; 
        return 0; 
    } 

    msg_ptr += *offset; 
    while (length && *msg_ptr) { 
        put_user(*(msg_ptr++), buffer++); 
        length--; 
        bytes_read++; 
    } 
    *offset += bytes_read; 

    return bytes_read; 
} 
 
static ssize_t device_write(struct file *filp, const char __user *buffer, size_t length, loff_t *offset) { 
    int bytes_write = 0; 
    char *text_ptr = text; 

    length = min(length, (size_t)(TEXT_LEN -1));
    memset(text, 0, TEXT_LEN);

    while (length) { 
        get_user(*(text_ptr++), buffer++);
        length--; 
        bytes_write++;
    } 

    text[bytes_write] = '\0';
    
    return bytes_write; 
} 

static int __init chardev_init(void) { 
    major = register_chrdev(0, DEVICE_NAME, &chardev_fops); 
 
    if (major < 0) { 
        printk(KERN_ALERT "Registering char device failed with %d\n", major); 
        return major; 
    } 
    printk(KERN_INFO "I was assigned major number %d.\n", major); 
 
    cls = class_create(DEVICE_NAME); 
    device_create(cls, NULL, MKDEV(major, 0), NULL, DEVICE_NAME); 
    printk(KERN_INFO "Device created on /dev/%s\n", DEVICE_NAME); 

    return SUCCESS; 
} 
 
static void __exit chardev_exit(void){ 
    device_destroy(cls, MKDEV(major, 0)); 
    class_destroy(cls); 
    unregister_chrdev(major, DEVICE_NAME); 

    printk(KERN_INFO "Device removed from /dev/%s\n", DEVICE_NAME); 
} 

module_init(chardev_init); 
module_exit(chardev_exit); 
