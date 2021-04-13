#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/mutex.h>
#include <linux/kthread.h>

int count=100;
module_param(count,int,S_IRUGO);

static DEFINE_MUTEX(m1);

static struct task_struct *task1;
static struct task_struct *task2;

static int thread_one(void *pargs)
{
    int i;
    mutex_lock(&m1);
    for(i=1;i<=count;i++)
    {
        printk("Thread A--%d\n",i);
    }
    mutex_unlock(&m1);
	return 0;
}

static int thread_two(void *pargs)
{
    int i;
    mutex_lock(&m1);
    for(i=1;i<=count;i++)
    {
        printk("Thread B--%d\n",i);
    }
    mutex_unlock(&m1);
	return 0;
}

static int __init mutexdemo_init(void) 
{        
    mutex_init(&m1);
    task1=kthread_run(thread_one, NULL, "thread_A");
    task2=kthread_run(thread_two, NULL, "thread_B");
    printk("Mutex Demo..welcome\n");
    return 0;
}
static void __exit mutexdemo_exit(void) 
{       
    if(task1)
        kthread_stop(task1);
    if(task2)
        kthread_stop(task2);
    printk("Mutex Demo,Leaving the world\n");
}

module_init(mutexdemo_init);
module_exit(mutexdemo_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Pranitha");
MODULE_DESCRIPTION("Mutex Example Module");
