#include<linux/module.h>
#include<linux/fs.h>
#include<linux/kernel.h>
#include<linux/init.h>
#include<net/sock.h>
#include<linux/pid.h>
#include<linux/list.h>
#include<linux/netlink.h>
#include<linux/types.h>
#include<linux/sched.h>
#include<linux/skbuff.h>
#include"ksimple_pstree.h"
#include<linux/string.h>
#define NETLINK_USER 22
#define USER_MSG    (NETLINK_USER + 1)
#define USER_PORT   50

MODULE_LICENSE("GPL");

typedef struct Parent_process {
    char process_name[50];
    int pid;

} P;
static struct sock *netlinkfd = NULL;
static int send_msg_to_user(int8_t *pbuf, uint16_t len)
{
    struct sk_buff *nl_skb;
    struct nlmsghdr *nlh;
    int ret;

    nl_skb = nlmsg_new(len, GFP_ATOMIC);
    if(!nl_skb) {
        printk("netlink_alloc_skb error\n");
        return -1;
    }
    nlh = nlmsg_put(nl_skb, 0, 0, USER_MSG, len, 0);
    if(nlh == NULL) {
        printk("nlmsg_put() error\n");
        nlmsg_free(nl_skb);
        return -1;
    }

    memcpy(nlmsg_data(nlh), pbuf, len);
    ret = netlink_unicast(netlinkfd, nl_skb, USER_PORT, MSG_DONTWAIT);
    return ret;
}
static void str_ini(char TEST[],int len)
{
    int i;
    for(i=0; TEST[i]!='\0'; i++)
        TEST[i]='\0';

    return;

}
static void recv_cb(struct sk_buff *skb)
{
    struct nlmsghdr *nlh = NULL;
    void *data = NULL;
    char *recv_data;
    char MODE;
    char pid_string[50]= {'\0'};
    int i;
    int j;
    int k;
    int pid;
    int converted;

    char D[50]= {'\0'};
    char TEST[200]= {'\0'};
    char TEST_child[100]= {'\0'};

    struct task_struct *temp=NULL;
    struct task_struct *temp_child=NULL;
    struct task_struct* task_to_find=NULL;
    struct list_head *head=NULL;
    struct list_head *child_head=NULL;
    //P parent_process[50];
    int u=0;
    printk("skb->len:%u\n", skb->len);
    if(skb->len >= nlmsg_total_size(0)) {
        nlh= nlmsg_hdr(skb);
        data = NLMSG_DATA(nlh);
        if(data) {
            printk("kernel receive data: %s\n", (int8_t *)data);
            recv_data = (char *)data;//convert data to String
            MODE = recv_data[0]; //mode selection
            for(i=0; recv_data[i+2]!='\0'; i++)
                pid_string[i]=recv_data[i+2];



            converted=sscanf(pid_string,"%d",&pid); //turn string to int


            //**********************************************

            task_to_find = pid_task(find_get_pid(pid),PIDTYPE_PID);



            if(MODE == 'c') {
                sprintf(TEST,"%s(%d)",task_to_find->comm,task_to_find->pid); //set data
                send_msg_to_user(TEST, strlen(TEST)); //send data & length
                str_ini(TEST,strlen(TEST)); //initialize TEST
                list_for_each(head,&task_to_find->children) {
                    temp=list_entry(head,struct task_struct,sibling);

                    sprintf(TEST,"    %s(%d)",temp->comm,temp->pid);

                    list_for_each(child_head,&temp->children) { //child of temp
                        temp_child=list_entry(child_head,struct task_struct,sibling);

                        sprintf(TEST_child,"        %s(%d)",temp_child->comm,temp_child->pid);
                        send_msg_to_user(TEST_child,strlen(TEST_child));
                        str_ini(TEST_child,strlen(TEST_child));
                    }
                    send_msg_to_user(TEST, strlen(TEST));
                    str_ini(TEST,strlen(TEST));
                }



            } else if(MODE == 's') {
                sprintf(TEST,"%s(%d)",task_to_find->comm,task_to_find->pid); //set data
                send_msg_to_user(TEST, strlen(TEST)); //send data & length
                str_ini(TEST,strlen(TEST)); //initialize TEST
                list_for_each(head,&task_to_find->sibling) {
                    temp=list_entry(head,struct task_struct,sibling);


                    if(strcmp(current->comm, "simple-pstree")==0 && u==0) {
                        sprintf(TEST,"%s(%d)",current->comm,current->pid);
                        send_msg_to_user(TEST, strlen(TEST));
                        str_ini(TEST,strlen(TEST));
                        u++;
                    } else if(strcmp(current->comm, "simple-pstree")==0) {
                        str_ini(TEST,strlen(TEST));

                    } else {
                        sprintf(TEST,"%s(%d)",current->comm,current->pid);
                        send_msg_to_user(TEST, strlen(TEST));
                        str_ini(TEST,strlen(TEST));

                    }

                }

            } else if(MODE == 'p') {
                sprintf(TEST,"%s(%d)",task_to_find->comm,task_to_find->pid); //set data
                send_msg_to_user(TEST, strlen(TEST)); //send data & length
                str_ini(TEST,strlen(TEST)); //initialize TEST
                temp = vmalloc(sizeof(struct task_struct*));
                temp = task_to_find->parent;
                while(temp->pid!=0) {
                    sprintf(TEST,"%s(%d)",temp->comm,temp->pid);
                    send_msg_to_user(TEST, strlen(TEST));
                    str_ini(TEST,strlen(TEST));
                    temp=temp->parent;
                }


            }


        } else {
            printk("Fail to recieve data!!\n");
        }
    }
}
struct netlink_kernel_cfg cfg = { .input = recv_cb, };
static int __init test_netlink_init(void)
{
    printk("init netlink_demo!\n");
    netlinkfd = netlink_kernel_create(&init_net, USER_MSG, &cfg);
    if(!netlinkfd) {
        printk(KERN_ERR "can not create a netlink socket!\n");
        return -1;
    }
    printk("netlink demo init ok!\n");
    return 0;
}
static void __exit test_netlink_exit(void)
{
    sock_release(netlinkfd->sk_socket);
    printk(KERN_DEBUG "netlink exit\n!");
}
module_init(test_netlink_init);
module_exit(test_netlink_exit);


