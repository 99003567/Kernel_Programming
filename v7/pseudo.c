
#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/kfifo.h>
#include <linux/list.h>

#include <asm/ioctl.h>

struct pschar_stat
{
    int len;
    int avail;
};

#define IOC_MAGIC                   'p'
#define PSCHAR_IOCTL_LEN            _IO(IOC_MAGIC, 1)
#define PSCHAR_IOCTL_AVAIL          _IO(IOC_MAGIC, 2)
#define PSCHAR_IOCTL_RESET          _IO(IOC_MAGIC, 3)
#define PSCHAR_IOCTL_PSTAT          _IOR(IOC_MAGIC, 4, struct pschar_stat)

dev_t pdevid;
int ndevices = 1;
struct class *pclass;

#define MAX_SIZE    (512)

struct pschar_databuf
{
    int rd_offset;
    int wr_offset;
    int buflen;
    struct kfifo ps_fifo;
};

struct pschar_dev
{
    dev_t devid;
    struct cdev cdev;
    struct device *pdevice;
    struct pschar_databuf *pdatabuf;
	struct list_head lentry;
};

LIST_HEAD(pschar_dev_list);

static long pschar_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    int ret = 0;

    struct pschar_dev *pps_dev = (struct pschar_dev *)file->private_data;
    struct pschar_databuf *pdbuf = pps_dev->pdatabuf;

    pr_info("ioctl: 0x%08x", cmd);

    switch (cmd)
    {
    case PSCHAR_IOCTL_LEN:
        pr_info("kfifo length is %d",
                kfifo_len(&pdbuf->ps_fifo));
        break;

    case PSCHAR_IOCTL_AVAIL:
        pr_info("kfifo avail is %d",
                kfifo_avail(&pdbuf->ps_fifo));
        break;

    case PSCHAR_IOCTL_RESET:
        pr_info("kfifo reset");
        kfifo_reset(&pdbuf->ps_fifo);
        break;

    case PSCHAR_IOCTL_PSTAT:
        pr_info("kfifo stat");
        {
            struct pschar_stat psstat;
            psstat.avail = kfifo_avail(&pdbuf->ps_fifo);
            psstat.len = kfifo_len(&pdbuf->ps_fifo);
            ret = copy_to_user((char __user *)arg, &psstat, sizeof(struct pschar_stat));
            if (ret)
            {
                pr_err("error in copy_to_user\n");
                return -EFAULT;
            }
        }
        break;

    default:
        ret = -EINVAL;
        break;
    }

    return ret;
}

static int pschar_open(struct inode *inode, struct file *file)
{
    struct pschar_dev *ppsdev;

    pr_info("open\n");
    /*
     * contanier *c = container_of(ptr, type, member)
     * given the `ptr` to a `member` of a container of type `type`,
     * retrieve the pointer to the container `c` which contains the
     * pointed to `member` at `ptr`.
     */
    ppsdev = container_of(inode->i_cdev, struct pschar_dev, cdev);
    file->private_data = ppsdev;
    return 0;
}

static int pschar_release(struct inode *inode, struct file *file)
{
    pr_info("release\n");
    // file->private_data = NULL;
    return 0;
}

static ssize_t pschar_read(struct file *file, char __user *ubuf, size_t size, loff_t *off)
{
    int ret, rcount;
    char* tbuf;

    struct pschar_dev *pps_dev = (struct pschar_dev *)file->private_data;
    struct pschar_databuf *pdbuf = pps_dev->pdatabuf;

    pr_info("read\n");

    if (kfifo_is_empty(&pdbuf->ps_fifo))
    {
        printk("buffer is empty\n");
        return 0;
    }
    rcount = size;
    if (rcount > kfifo_len(&pdbuf->ps_fifo))
        rcount = kfifo_len(&pdbuf->ps_fifo);

    tbuf = kmalloc(rcount, GFP_KERNEL);
    rcount = kfifo_out(&pdbuf->ps_fifo, tbuf, rcount);
    ret = copy_to_user((void *)ubuf, (void *)tbuf, rcount);
    if (ret)
    {
        printk("copy to user failed\n");
        return -EFAULT;
    }
    kfree(tbuf);
    return rcount;
}

static ssize_t pschar_write(struct file *file, const char __user *ubuf, size_t size, loff_t *off)
{
    int ret, wcount;
    char * tbuf;

    struct pschar_dev *pps_dev = (struct pschar_dev *)file->private_data;
    struct pschar_databuf *pdbuf = pps_dev->pdatabuf;

    pr_info("write\n");
    if (kfifo_is_full(&pdbuf->ps_fifo))
    {
        printk("buffer is full\n");
        return -ENOSPC;
    }
    wcount = size;
    if (wcount > kfifo_avail(&pdbuf->ps_fifo))
        wcount = kfifo_avail(&pdbuf->ps_fifo);

    tbuf = kmalloc(wcount, GFP_KERNEL);

    ret = copy_from_user((void *)tbuf, (const void *)ubuf, wcount);
    if (ret)
    {
        printk("copy from user failed\n");
        return -EFAULT;
    }
    kfifo_in(&pdbuf->ps_fifo, tbuf, wcount);
    kfree(tbuf);
    return wcount;
}

struct file_operations fops = {
    .owner = THIS_MODULE,
    .unlocked_ioctl = pschar_ioctl,
    .open = pschar_open,
    .release = pschar_release,
    .write = pschar_write,
    .read = pschar_read
};

static int __init pschar_init(void)
{
    int ret;
    int i = 0;
    struct pschar_dev *pps_dev;

    pr_notice("Driver Version = v5-kfifo-ioctl\n");

    ret = alloc_chrdev_region(&pdevid, 0, ndevices, "pschar");
    if (ret)
    {
        pr_err("Failed to register driver\n");
        return -EINVAL;
    }

    pclass = class_create(THIS_MODULE, "pchar_class");

    for (i = 0; i < ndevices; i++)
    {
        pps_dev = kmalloc(sizeof(struct pschar_dev), GFP_KERNEL);
        pps_dev->devid = pdevid + i;
        // Setup data buffer
        pps_dev->pdatabuf = kmalloc(sizeof(struct pschar_databuf), GFP_KERNEL);
        pps_dev->pdatabuf->buflen = 0;
        pps_dev->pdatabuf->rd_offset = 0;
        pps_dev->pdatabuf->wr_offset = 0;
        ret = kfifo_alloc(&pps_dev->pdatabuf->ps_fifo, MAX_SIZE, GFP_KERNEL);
        // Setup Character device
        cdev_init(&pps_dev->cdev, &fops);
        pps_dev->cdev.owner = THIS_MODULE;
        kobject_set_name(&pps_dev->cdev.kobj, "pschar%d", i);
        ret = cdev_add(&pps_dev->cdev, pps_dev->devid, 1);
        // Setup  device file
        pps_dev->pdevice = device_create(pclass, NULL, pps_dev->devid, NULL, pps_dev->cdev.kobj.name, i);

        list_add_tail(&pps_dev->lentry, &pschar_dev_list);
        pr_info("Registered major=%d, minor=%d @ /dev/%s\n",
                MAJOR(pps_dev->devid),
                MINOR(pps_dev->devid),
                pps_dev->cdev.kobj.name);
    }

    return 0;
}

static void __exit pschar_exit(void)
{
    struct list_head *pcur, *pbak;
    struct pschar_dev *pps_dev;
    list_for_each_safe(pcur, pbak, &pschar_dev_list)
    {
        pps_dev = list_entry(pcur, struct pschar_dev, lentry);
        pr_info("Unregistered major=%d, minor=%d @ /dev/%s\n",
                MAJOR(pps_dev->devid),
                MINOR(pps_dev->devid),
                pps_dev->cdev.kobj.name);
        device_destroy(pclass, pps_dev->devid);
        cdev_del(&pps_dev->cdev);
        kfifo_free(&pps_dev->pdatabuf->ps_fifo);
        kfree(pps_dev->pdatabuf);
        kfree(pps_dev);
    }
    class_destroy(pclass);
    unregister_chrdev_region(pdevid, ndevices);
}

module_init(pschar_init);
module_exit(pschar_exit);
module_param(ndevices, int, 0); // Perms are 0. Can't modify in sysfs once loaded
MODULE_LICENSE("GPL");
MODULE_AUTHOR("satyasindhu");
MODULE_DESCRIPTION("Simple Hello Module");
