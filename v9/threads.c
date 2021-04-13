#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/delay.h>

static struct task_struct *task1;
static struct task_struct *task2;

static void noinline task_dummy(void)
{
   printk("This is a Dummy Function");
}   

static int noinline my_thread_one(void *pargs)
{
 int k=0;
 while(!kthread_should_stop())
 {
    printk("Thread A--%d\n",k++);
    task_dummy();
    msleep(1000); //ssleep, usleep
 }
 do_exit(0);
 return 0;
}

static int noinline my_thread_two(void *pargs)
{
 int k=0;
 while(!kthread_should_stop())
 {
    printk("Thread B--%d\n",k++);
    msleep(1000); //ssleep, usleep
 }
 do_exit(0);
 return 0;
}

static int __init pseudo_init(void)
{
  task1=kthread_run(my_thread_one, NULL, "thread_A");
  task2=kthread_run(my_thread_two, NULL, "thread_B");
  return 0;
}

static void __exit pseudo_exit(void) 
{
  if(task1)
    kthread_stop(task1);
  if(task2)
    kthread_stop(task2);
}
module_init(pseudo_init);
module_exit(pseudo_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Pranitha");
MODULE_DESCRIPTION("Threads");
