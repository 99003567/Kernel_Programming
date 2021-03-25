#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/kfifo.h>
#include <linux/slab.h>
#include <linux/uaccess.h>

unsigned char *pbuffer;
#define MAX_SIZE 1024

struct kfifo kfifo;
int rcount, wcount, ret;

struct device *pdev;
struct class *pclass;
dev_t pdevid;
struct cdev cdev;
int ndevices = 1;

int pseudo_open(struct inode *inode, struct file *file) {
  printk("Pseudo--Open method\n");
  return 0;
}

int pseudo_close(struct inode *inode, struct file *file) {
  printk("Pseudo--Release method\n");
  return 0;
}

ssize_t pseudo_read(struct file *file, char __user *ubuf, size_t size,loff_t *off) 
{
  char *tbuf;
  printk("Pseudo--Read method\n");
  if(kfifo_is_empty(&kfifo)) {
    printk("buffer is empty\n");
    return 0;
  }
  rcount = size;
  if(rcount > kfifo_len(&kfifo))
    rcount = kfifo_len(&kfifo);
  tbuf = kmalloc(rcount, GFP_KERNEL);
  ret=kfifo_out(&kfifo, tbuf, rcount);
  ret=copy_to_user(ubuf, tbuf, rcount);
  //error handling
  kfree(tbuf);
  return rcount;
  
}

ssize_t pseudo_write(struct file *file, const char __user* ubuf, size_t size,loff_t *off)
{
  char* tbuf;
  if(kfifo_is_full(&kfifo))
  {
    printk("buffer is full\n");
    return -ENOSPC;
  }

  wcount = size;
  if(wcount > kfifo_avail(&kfifo))
  wcount = kfifo_avail(&kfifo);
  tbuf=kmalloc(wcount, GFP_KERNEL);
  ret=copy_from_user(tbuf, ubuf, wcount);
  //error handling if copy_form_user
  kfifo_in(&kfifo, tbuf, wcount);
  kfree(tbuf);
  return wcount;
  
}

struct file_operations fops = {.open = pseudo_open,
                               .release = pseudo_close,
                               .write = pseudo_write,
                               .read = pseudo_read
};

static int __init pseudo_init(void)
{
  int i = 0;
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

  pbuffer=kmalloc(MAX_SIZE, GFP_KERNEL);
  kfifo_init(&kfifo, pbuffer, MAX_SIZE);
  //kfifo_alloc(&kfifo, MAX_SIZE, GFP_KERNEL);

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

  kfifo_free(&kfifo);
}

module_init(pseudo_init);
module_exit(pseudo_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Pranitha");
MODULE_DESCRIPTION("kfifo usage in Pseudo Driver");
