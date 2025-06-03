#include <linux/module.h>
#include <net/sock.h>
#include <linux/netlink.h>
#include <linux/skbuff.h>
#include <net/net_namespace.h>
#include "define.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Nikita \"mercy191\" Ivanov");
MODULE_DESCRIPTION("Netlink");

/* Netlink receive and send function */
static void hello_nl_recv_msg(struct sk_buff *skb);

struct sock *nl_sk = NULL;
struct netlink_kernel_cfg cfg = {
    groups:  1,
    input: hello_nl_recv_msg,
 };

static void hello_nl_recv_msg(struct sk_buff *skb) {

    struct nlmsghdr *nlh;
    int pid;
    struct sk_buff *skb_out;
    int msg_size;
    char *msg = "I see you user -_-";
    int res;

    printk(KERN_INFO "Entering: %s\n", __FUNCTION__);

    msg_size = strlen(msg);

    nlh = (struct nlmsghdr *)skb->data;
    pid = nlh->nlmsg_pid; 
    printk(KERN_INFO "Netlink received msg payload from pid %d: %s\n", pid, (char *)nlmsg_data(nlh));

    
    if (!(skb_out = nlmsg_new(msg_size, 0))) {
        printk(KERN_ERR "Failed to allocate new skb\n");
        return;
    }

    nlh = nlmsg_put(skb_out, 0, 0, NLMSG_DONE, msg_size, 0);
    NETLINK_CB(skb_out).dst_group = 0; 
    strncpy(nlmsg_data(nlh), msg, msg_size);

    if ((res = nlmsg_unicast(nl_sk, skb_out, pid)) < 0) {
        printk(KERN_INFO "Error while sending bak to user\n");
    }
}

static int __init hello_init(void) {
    printk(KERN_INFO "Initialize netlink module\n");

    if (!(nl_sk = netlink_kernel_create(&init_net, NETLINK_USER, &cfg))) {
        printk(KERN_ALERT "Error creating socket.\n");
        return -EINVAL;
    }

    return 0;
}

static void __exit hello_exit(void) {
    printk(KERN_INFO "Exiting netlink module\n");
    netlink_kernel_release(nl_sk);
}

module_init(hello_init);
module_exit(hello_exit);

