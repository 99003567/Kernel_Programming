#include <linux/cdev.h>
#include <linux/kfifo.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
dev_t pdevid;
unsigned char *pbuffer;
int rd_offset=0;
int wr_offset=0;
int buflen=0;
#define MAX_SIZE 1024
int ndevices=1;
struct cdev cdev;
struct device *pdev; //global
struct class *pclass; //global

int pseudo_open(struct inode* inode , struct file* file)
{
	printk("Pseudo--open method\n");
	return 0;
}
int pseudo_close(struct inode* inode , struct file* file)
{
	printk("Pseudo--release method\n");
	return 0;
}
ssize_t pseudo_read(struct file * file, char __user * buf , size_t size, loff_t * off)
{
	printk("Pseudo--read method\n");
	return 0;
}
ssize_t pseudo_write(struct file * file, const char __user * buf , size_t size, loff_t * off)
{
	printk("Pseudo--write method\n");
	return -ENOSPC;
}

struct file_operations fops = {
.open = pseudo_open,
.release = pseudo_close,
.write = pseudo_write,
.read = pseudo_read
};

static int __init pseudo_init(void)
{
	int ret,i=0;
	pclass = class_create(THIS_MODULE, "pseudo_class");
	pbuffer = kmalloc(MAX_SIZE, GFP_KERNEL);
	ret=alloc_chrdev_region(&pdevid, 0, ndevices, "pseudo_sample");
	if(ret)
	{
	  printk("Pseudo: Failed to register driver\n");
	  return -EINVAL;
	}
	cdev_init(&cdev, &fops);
	kobject_set_name(&cdev.kobj,"pdevice%d",i);
	ret = cdev_add(&cdev, pdevid, 1);
	pdev = device_create(pclass, NULL, pdevid, NULL, "psample%d",i);
	printk("Successfully registered,major=%d,minor=%d\n",MAJOR(pdevid), MINOR(pdevid));
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
	kfree(pbuffer);
}
module_init(pseudo_init);
module_exit(pseudo_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Pranitha");
MODULE_DESCRIPTION("Buffer as pseudo device without kfifo");
