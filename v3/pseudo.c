#include <linux/cdev.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/fs.h>

struct device *pdev;
struct class *pclass;
dev_t pdevid;
struct cdev cdev;
int ndevices = 1;

int pseudo_open(struct inode *inode, struct file *file)
{
  printk("in open method \n");
  return 0;
}

int pseudo_close(struct inode *inode, struct file *file)
{
  printk("in release method \n");
  return 0;
}

ssize_t pseudo_read(struct file *file, char __user *ubuf, size_t size, loff_t *off)
{
  printk("in read  method \n");
  return 0;
}

ssize_t pseudo_write(struct file *file, const char __user *ubuf, size_t size, loff_t *off)
{
  printk("in write  method \n");
  return 0;
}

struct file_operations fops = {
    .open = pseudo_open,
    .release = pseudo_close,
    .write = pseudo_write,
    .read = pseudo_read
};

static int __init pseudo_init(void)
{
  int ret, i = 0;
  pclass = class_create(THIS_MODULE, "pseudo_class");
  ret = alloc_chrdev_region(&pdevid, 0, ndevices, "pseudo_sample");
  if (ret)
  {
    printk("Pseudo: failed to register driver \n");
    return -EINVAL;
  }
  cdev_init(&cdev, &fops);
  kobject_set_name(&cdev.kobj, "pdevice %d", i);
  ret = cdev_add(&cdev, pdevid, 1);
  pdev = device_create(pclass, NULL, pdevid, NULL, "psample %d", i);
  printk("successfully registered, major=%d ,minor=%d \n ", MAJOR(pdevid), MINOR(pdevid));
  printk("Pseudo driver sample ...welcome \n");
  return 0;
}

static void __exit pseudo_exit(void)
{
  cdev_del(&cdev);
  unregister_chrdev_region(pdevid, ndevices);
  device_destroy(pclass, pdevid);
  class_destroy(pclass);
  printk("psudo driver sample...bye \n");
}

module_init(pseudo_init);
module_exit(pseudo_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Pranitha");
MODULE_DESCRIPTION("Step3");
