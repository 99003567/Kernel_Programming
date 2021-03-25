#include <linux/fs.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>

dev_t pdevid;
int ndevices=1;
static int __init psuedo_init(void)
{
    int ret;
    ret=alloc_chrdev_region(&pdevid, 0, ndevices, "pseudo_sample");
    if(ret) 
    {
        printk("Pseudo: Failed to register driver\n");
        return -EINVAL;
    }
    printk("Successfully registered,major=%d,minor=%d\n",
    MAJOR(pdevid), MINOR(pdevid));
    printk("Pseudo Driver Sample..welcome\n");
    return 0;
}
static void __exit psuedo_exit(void) 
{
    unregister_chrdev_region(pdevid, ndevices);
    printk("Pseudo Driver Sample..Bye\n");
}

module_init(psuedo_init);
module_exit(psuedo_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Pranitha");
MODULE_DESCRIPTION("Step1");
