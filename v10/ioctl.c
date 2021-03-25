#include <linux/fs.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/kfifo.h>
#include <linux/slab.h>
#include <linux/uaccess.h>

#define IOC_MAGIC	'p'
#define MY_IOCTL_LEN	_IO(IOC_MAGIC,1)
#define MY_IOCTL_AVAIL	_IO(IOC_MAGIC,2)
#define MY_IOCTL_RESET	_IO(IOC_MAGIC,3)

struct cdev cdev;
struct kfifo kfifo;
dev_t pdevid;
int ndevices=1;
struct device *pdev; 
struct class *pclass;
unsigned char *pbuffer;
#define MAX_SIZE 1024
int ret;
char* tbuf;
/*
struct __kfifo {

unsigned int in;
unsigned int out;
unsigned int mask;
unsigned int esize;
void         *data;

};
*/

int pseudo_open(struct inode* inode , struct file* file)
{
 printk("Pseudo--open method\n");
 return 0;
}

int pseudo_close(struct inode* inode , struct file* file)
{
 printk("Pseudo --release method\n");
 return 0;
}
ssize_t pseudo_read(struct file* file, char __user *ubuf , size_t size, loff_t * off)
{
 int rcount;
 printk("Pseudo--read method\n");
 

 if(kfifo_is_empty(&kfifo))
 {
  printk("buffer is empty\n");
  return 0;
 }
 
 rcount = size;
 if(rcount > kfifo_len(&kfifo))
   rcount = kfifo_len(&kfifo);

 tbuf = kmalloc(rcount,GFP_KERNEL);
 kfifo_out(&kfifo,tbuf,rcount);
 
 ret=copy_to_user(tbuf,ubuf,rcount);

 kfree(tbuf);
 return rcount;
}

ssize_t pseudo_write(struct file* file,const  char __user *ubuf , size_t size, loff_t * off) 
{
 int wcount;
 printk("Pseudo--write method\n");

 if(kfifo_is_full(&kfifo))
 {
  printk("buffer is full\n");
  return -ENOSPC;
 }
 wcount = size;
 if(wcount > kfifo_avail(&kfifo))
  wcount = kfifo_avail(&kfifo);

 tbuf=kmalloc(wcount, GFP_KERNEL);
 
 ret=copy_from_user(tbuf,ubuf,wcount);
 kfifo_in(&kfifo,tbuf,wcount);
 kfree(tbuf);

 return wcount;

}

static long pseudo_ioctl(struct file *file,unsigned int cmd, unsigned long arg)
{
 int ret;
 printk("Pseudo--ioctl method\n");
  switch (cmd) {
  case MY_IOCTL_LEN :
       printk("ioctl--kfifo length is %d\n", kfifo_len(&kfifo));
       break;
  case MY_IOCTL_AVAIL:
       printk("ioctl--kfifo avail is %d\n", kfifo_avail(&kfifo));
       break;
  case MY_IOCTL_RESET:
       printk("ioctl--kfifo got reset\n");
       kfifo_reset(&kfifo);
       break;
  }
 return 0;
}

struct file_operations fops ={
 .open     = pseudo_open,
 .release  = pseudo_close,
 .write    = pseudo_write,
 .read     = pseudo_read,
 .unlocked_ioctl  = pseudo_ioctl
};


static int __init pseudo_init(void)
{
 int i=0;
  ret=alloc_chrdev_region(&pdevid, 0, ndevices, "pseudo_sample");
  if(ret) 
  {
   printk("Pseudo: Failed to register driver\n");
   return -EINVAL;
  }
 cdev_init(&cdev,&fops);
 kobject_set_name(&cdev.kobj ,"pdevice%d",i);
 ret = cdev_add(&cdev,pdevid,1);

 pclass = class_create(THIS_MODULE, "pseudo_class");
 pdev = device_create(pclass, NULL, pdevid, NULL, "psample%d",i);

 pbuffer=kmalloc(MAX_SIZE, GFP_KERNEL);
 kfifo_init(&kfifo,pbuffer,MAX_SIZE);

 printk("Successfully registered,major=%d,minor=%d\n",MAJOR(pdevid),MINOR(pdevid));
 printk("Pseudo Driver Sample..welcome\n");
 return 0;
}
static void __exit pseudo_exit(void)
{

cdev_del(&cdev);
unregister_chrdev_region(pdevid, ndevices);
printk("Pseudo Driver Sample..Bye\n");
device_destroy(pclass, pdevid);
class_destroy(pclass);
kfifo_free(&kfifo);

}

module_init(pseudo_init);
module_exit(pseudo_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Pranitha");
MODULE_DESCRIPTION("A Hello, World Module");
