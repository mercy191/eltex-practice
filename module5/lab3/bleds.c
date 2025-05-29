#include <linux/module.h>
#include <linux/configfs.h>
#include <linux/init.h>
#include <linux/tty.h>         
#include <linux/kd.h>           
#include <linux/vt.h>
#include <linux/console_struct.h>  
#include <linux/vt_kern.h>
#include <linux/timer.h>
#include <linux/printk.h> 
#include <linux/kobject.h> 
#include <linux/sysfs.h> 
#include <linux/fs.h>
#include <linux/string.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Nikita \"mercy191\" Ivanov");
MODULE_DESCRIPTION("Blinds on keyborad");

// For sysfs
#define PERMS 0660

// For blinding lighs
#define BLINK_DELAY   100
#define ALL_LEDS_ON   0x07
#define RESTORE_LEDS  0xFF

// For sysfs
static struct kobject *sfs_kobject; 
static int fds; 

// For blinding lighs
struct timer_list timer;
struct tty_driver* driver;
static int _kbledstatus = 0;
static int light_mode = 0;

// For sysfs
static ssize_t sfs_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf);
static ssize_t sfs_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count);
static struct kobj_attribute sfs_attribute = __ATTR(fds, PERMS, sfs_show, sfs_store);

// For blinding lighs
/*
 * Function timer_func blinks the keyboard LEDs periodically by invoking
 * command KDSETLED of ioctl() on the keyboard driver. To learn more on virtual
 * terminal ioctl operations, please see file:
 *     /usr/src/linux/drivers/char/vt_ioctl.c, function vt_ioctl().
 *
 * The argument to KDSETLED is alternatively set to 7 (thus causing the led
 * mode to be set to LED_SHOW_IOCTL, and all the leds are lit) and to 0xFF
 * (any value above 7 switches back the led mode to LED_SHOW_FLAGS, thus
 * the LEDs reflect the actual keyboard status). To learn more on this,
 * please see file:
 *     /usr/src/linux/drivers/char/keyboard.c, function setledstate().
 *
 */
static void timer_func(struct timer_list *ptr);
static void start_bleds(void);
static void stop_bleds(void);
static int __init bleds_init(void);
static void __exit bleds_exit(void);


static ssize_t sfs_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf) {
    return sprintf(buf, "%d\n", fds);
}

static ssize_t sfs_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count) {
    int value;
    if (sscanf(buf, "%d", &value) != 1) {
        return -EINVAL;
    }

    fds = value;
    light_mode = value;

    if (value == 0) {
        stop_bleds();
    }
    else if (value >= 1 && value <= 7) {
        start_bleds();
    } 
    else {
        return -EINVAL;
    }

    return count;
}

static void timer_func(struct timer_list *ptr) {
        
    int *pstatus = &_kbledstatus;
    if (*pstatus == light_mode) {
        *pstatus = RESTORE_LEDS;
    }
    else {
        *pstatus = light_mode;
    }

    (driver->ops->ioctl) (vc_cons[fg_console].d->port.tty, KDSETLED, *pstatus);
    mod_timer(&timer, jiffies + BLINK_DELAY);
}

static void start_bleds(void)
{
    int i;
    printk(KERN_INFO "kbleds: loading\n");
    printk(KERN_INFO "kbleds: fgconsole is %x\n", fg_console);
    for (i = 0; i < MAX_NR_CONSOLES; i++) {
        if (!vc_cons[i].d) {
            break;
        }
        printk(KERN_INFO "poet_atkm: console[%i/%i] #%i, tty %lx\n", i,
               MAX_NR_CONSOLES, vc_cons[i].d->vc_num,
               (unsigned long)vc_cons[i].d->port.tty);
    }
    printk(KERN_INFO "kbleds: finished scanning consoles\n");

    printk(KERN_INFO "kbleds: starting blinking\n");
    driver = vc_cons[fg_console].d->port.tty->driver;
    mod_timer(&timer, jiffies + BLINK_DELAY);
}

static void stop_bleds(void) {  
    del_timer_sync(&timer);
    (driver->ops->ioctl) (vc_cons[fg_console].d->port.tty, KDSETLED, RESTORE_LEDS);
    printk(KERN_INFO "kbleds: stopped blinking\n");
}

static int __init bleds_init(void)
{
    int error = 0;
 
    printk(KERN_INFO "Module initialized successfully \n");
    if(!(sfs_kobject = kobject_create_and_add("bleds", kernel_kobj))) {
        return -ENOMEM;
    }
 
    if ((error = sysfs_create_file(sfs_kobject, &sfs_attribute.attr))) {
        printk(KERN_WARNING "Failed to create the foo file in /sys/kernel/bleds \n");
    }

    timer_setup(&timer, timer_func, 0);
  
    return error;
}
 
static void __exit bleds_exit(void)
{
    kobject_put(sfs_kobject);
    del_timer_sync(&timer);
    printk (KERN_INFO "Module cleanup successfully \n");
}

module_init(bleds_init);
module_exit(bleds_exit);