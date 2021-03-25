#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/kfifo.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/uaccess.h>

struct device *pdev;
struct class *pclass;
unsigned char *pbuffer;
#define MAX_SIZE 1024
struct kfifo kfifo;
dev_t pdevid;
struct cdev cdev;
// char* tbuf,ubuf;

int ndevices = 1;
unsigned char *pbuffer;
int rd_offset=0;
int wr_offset=0;
int buflen=0;
int ret,rcount,wcount;

int pseudo_open(struct inode *inode, struct file *file) {
  printk("in open method \n");
  return 0;
}
int pseudo_close(struct inode *inode, struct file *file) {
  printk("in release method \n");
  return 0;
}
ssize_t pseudo_read(struct file *file, char __user *ubuf, size_t size,
                    loff_t *off) {
  if (buflen == 0)
  // wr_offset-rd_offset==0
  {
    printk("buffer is empty\n");
    return 0;
  }
  rcount = size;
  if (rcount > buflen)
    rcount = buflen;
  // min of buflen, size
  ret = copy_to_user(ubuf, pbuffer + rd_offset, rcount);
  if (ret) {
    printk("copy to user failed\n");
    return -EFAULT;
  }
  rd_offset += rcount;
  buflen -= rcount;
}
ssize_t pseudo_write(struct file *file, const char __user *ubuf, size_t size,
                     loff_t *off) {
  if (wr_offset >= MAX_SIZE) {
    printk("buffer is full\n");
    return -ENOSPC;
  }
  wcount = size;
  if (wcount > MAX_SIZE - wr_offset)
    wcount = MAX_SIZE - wr_offset;
  // min
  ret = copy_from_user(ubuf, pbuffer + wr_offset, wcount);
  if (ret) {
    printk("copy from user failed\n");
    return -EFAULT;
  }
  wr_offset += wcount;
  buflen += wcount;
}

struct file_operations fops = {
    .open = pseudo_open,
    .release = pseudo_close,
    .write = pseudo_write,
    .read = pseudo_read,
};

static int __init pseudo_init(void)

{

  int ret, i = 0;
  pclass = class_create(THIS_MODULE, "pseudo_class");
  ret = alloc_chrdev_region(&pdevid, 0, ndevices, "pseudo_sample");
  pbuffer = kmalloc(MAX_SIZE, GFP_KERNEL);
  kfifo_init(&kfifo, pbuffer, MAX_SIZE);
  if (ret)

  {
    printk("Pseudo: failed to register driver \n");
    return -EINVAL;
  }

  cdev_init(&cdev, &fops);
  kobject_set_name(&cdev.kobj, "pdevice %d", i);

  ret = cdev_add(&cdev, pdevid, 1);
  pdev = device_create(pclass, NULL, pdevid, NULL, "psample %d", i);

  printk("successfully registered, major=%d ,minor=%d \n ", MAJOR(pdevid),
         MINOR(pdevid));
  printk("Pseudo driver sample ...welcome \n");

  return 0;
}

static void __exit pseudo_exit(void) {

  cdev_del(&cdev);
  unregister_chrdev_region(pdevid, ndevices);
  device_destroy(pclass, pdevid);
  class_destroy(pclass);
  printk("psudo driver sample...bye \n");
  kfifo_free(&kfifo);
  kfree(pbuffer);
}
module_init(pseudo_init);
module_exit(pseudo_exit);
MODULE_LICENSE("GPL");
