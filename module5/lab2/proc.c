#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/sched.h>
#include <linux/uaccess.h>
#include <linux/slab.h>

#define MSG_SIZE 10

static int len, temp;
static char *msg;

MODULE_LICENSE("CustomLicence");
MODULE_AUTHOR("Nikita \"mercy191\" Ivanov");
MODULE_DESCRIPTION("Proc read/write module");

/* Copy into userspace */
ssize_t read_proc(struct file *filp, char *buf, size_t count, loff_t *offp );

/* Copy from userspace */
ssize_t write_proc(struct file *filp, const char *buf, size_t count, loff_t *offp);

/* Create module entry */
void create_proc_entry(void);
 

ssize_t read_proc(struct file *filp, char *buf, size_t count, loff_t *offp ) {
    if(count > temp) {
        count = temp;
    }
    temp = temp - count;
    if (copy_to_user(buf, msg, count)) {
        printk(KERN_WARNING "Failde to copy data into userspace");
    }
    if(count == 0)
        temp = len;
    return count;
}
 
ssize_t write_proc(struct file *filp, const char *buf, size_t count, loff_t *offp) {
    if (copy_from_user(msg, buf, count)) {
        printk(KERN_WARNING "Failde to copy data from userspace");
    }
    len = count;
    temp = len;
    return count;
}

static const struct proc_ops proc_fops = {
    proc_read: read_proc,
    proc_write: write_proc,
};


void create_proc_entry(void) {
    proc_create("proc", 0, NULL, &proc_fops);
    msg = kmalloc(MSG_SIZE*sizeof(char), GFP_KERNEL);
}


static int __init proc_init(void) {
    printk(KERN_INFO "The module PROC has been successfully initialized!\n");
    create_proc_entry();
    return 0;
}


static void __exit proc_cleanup(void) {
    remove_proc_entry("proc", NULL);
    kfree(msg);
    printk(KERN_INFO "The module PROC has been successfully cleared!\n");
}

module_init(proc_init);
module_exit(proc_cleanup);