#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/kfifo.h>
//#include <linux/wait.h>
#include <asm/ioctl.h>

#define IOC_MAGIC 'p'
#define MY_IOCTL_LEN _IO(IOC_MAGIC, 1)
#define MY_IOCTL_AVAIL _IO(IOC_MAGIC, 2)
#define MY_IOCTL_RESET _IO(IOC_MAGIC, 3)

//struct device *pdev;
struct class *pclass;
unsigned char *pbuffer;

#define MAX_SIZE 1024

//struct kfifo kfifo;
dev_t pdevid;
//struct cdev cdev;

int tbuf;
int ndevices=1;
int ret;

typedef struct priv_obj {
	struct cdev cdev;
	struct kfifo kfifo;
	struct device* pdev;
}PRIV_OBJ;

PRIV_OBJ* pobj;

int pseudo_open(struct inode* inode, struct file* file)
{
	printk("Pseudo--open method\n");
	return 0;
}

int pseudo_close(struct inode* inode, struct file* file)
{
	printk("Pseudo--release method\n");
	return 0;
}

ssize_t pseudo_read(struct file* file, char __user * ubuf, size_t size, loff_t * off)
{
	//printk("Pseudo--read method\n");
	//return 0;
	int rcount;
	char* tbuf;
	printk("in read  method \n");
	if(kfifo_is_empty(&pobj->kfifo)) 
	{
		printk("buffer is empty\n");
		return 0;
	}
//min
	rcount = size;
	if(rcount > kfifo_len(&pobj->kfifo))
	rcount = kfifo_len(&pobj->kfifo);
	tbuf = kmalloc(rcount, GFP_KERNEL);
	ret=kfifo_out(&pobj->kfifo, tbuf, rcount);
	ret=copy_to_user(ubuf, tbuf,rcount);
//error handling
	return rcount;
	kfree(tbuf);
}

ssize_t pseudo_write(struct file * file, const char __user * ubuf , size_t size, loff_t * off)
{
	//printk("Pseudo--write method\n");
	//return -ENOSPC;
	int wcount;
	char* tbuf;
	printk("in write  method \n");
	if(kfifo_is_full(&pobj->kfifo))
	{
		printk("buffer is full\n");
		return -ENOSPC;
	}
	wcount = size;
	if(wcount > kfifo_avail(&pobj->kfifo))
	wcount = kfifo_avail(&pobj->kfifo);
	tbuf=kmalloc(wcount, GFP_KERNEL);
	ret=copy_from_user(tbuf, ubuf, wcount);
//error handling if copy_form_user
	kfifo_in(&pobj->kfifo, tbuf, wcount);
	return wcount;
	kfree(tbuf);
	return 0;
}

static long pseudo_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	printk("Pseudo--ioctl method\n");
	switch(cmd) {
		case MY_IOCTL_LEN :
			printk("ioctl--kfifo length is %d\n", kfifo_len(&pobj->kfifo));
			return(kfifo_len(&pobj->kfifo));
			break;
		case MY_IOCTL_AVAIL :
			printk("ioctl--kfifo avail is %d\n", kfifo_avail(&pobj->kfifo));
			return(kfifo_avail(&pobj->kfifo));
			break;
		case MY_IOCTL_RESET :
			printk("ioctl--kfifo got reset\n");
			kfifo_reset(&pobj->kfifo);
			return 0;
			break;
	}
	return 0;
}

struct file_operations fops = {
	.open = pseudo_open,
	.release = pseudo_close,
	.write = pseudo_write,
	.read = pseudo_read,
	.unlocked_ioctl = pseudo_ioctl
};

static int __init pseudo_init(void)
{
	int ret,i=0;
	pclass = class_create(THIS_MODULE, "pseudo_class");
	ret=alloc_chrdev_region(&pdevid, 0, ndevices, "pseudo_sample");
	pobj = kmalloc(sizeof(PRIV_OBJ), GFP_KERNEL);
	pbuffer=kmalloc(MAX_SIZE, GFP_KERNEL);
	kfifo_init(&pobj->kfifo, pbuffer,MAX_SIZE);
	if(ret)
	{
		printk("Pseudo: failed to register driver \n");
		return  -EINVAL;
	}

	cdev_init(&pobj->cdev, &fops);
	kobject_set_name(&pobj->cdev.kobj,"pdevice %d",i);
	ret = cdev_add(&pobj->cdev, pdevid,1);
	pobj->pdev = device_create(pclass,NULL,pdevid,NULL, "psample %d",i);
	printk("successfully registered, major=%d ,minor=%d \n ",MAJOR(pdevid), MINOR(pdevid));
	printk("Pseudo driver sample ...welcome \n");
	return 0;

}

static void __exit pseudo_exit(void)
{
	cdev_del(&pobj->cdev);
	unregister_chrdev_region(pdevid, ndevices);
	device_destroy(pclass, pdevid);
	class_destroy(pclass);
	printk("Pseudo driver sample...bye \n");
	kfifo_free(&pobj->kfifo);
	kfree(pobj);
}



module_init(pseudo_init);
module_exit(pseudo_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Pranitha");
MODULE_DESCRIPTION("Private Object in Pseudo Driver");
