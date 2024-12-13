#include <linux/module.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/moduleparam.h>
#include <linux/kfifo.h>
#include "ioctl.h"

#define MAX 32

//info of all devices
typedef struct pchardev {
    struct kfifo mybuf;
    dev_t devno;
    struct cdev pchar_cdev;
    int id;
}pchardev_t;

static int DEVCNT = 4;
module_param_named(devcnt, DEVCNT, int, 0444);

//array of devices
static pchardev_t *devices;

//other global variables
static dev_t devno;
static int major;
static struct class *pclass;

int pchar_open(struct inode *pinode, struct file *pfile){
    pchardev_t *dev = container_of(pinode->i_cdev, pchardev_t, pchar_cdev);
    pfile->private_data = dev;
    pr_info("%s: pchar_open() called for pchar%d.\n", THIS_MODULE->name, dev->id);
    return 0;
}

int pchar_close(struct inode *pinode, struct file *pfile){
    pchardev_t *dev = (pchardev_t *)pfile->private_data;
    pr_info("%s: pchar_close() called for pchar%d.\n", THIS_MODULE->name, dev->id);
    return 0;
}

static ssize_t pchar_write(struct file *pfile, const char __user *ubuf, size_t bufsize, loff_t *pf_pos){
    pchardev_t *dev = (pchardev_t *)pfile->private_data;
    pr_info("%s: pchar_write() called.\n", THIS_MODULE->name);
    int ret, nbytes;
    pr_info("%s: pchar_write() called for pchar%d.\n", THIS_MODULE->name, dev->id);
    ret = kfifo_from_user(&dev->mybuf, ubuf, bufsize, &nbytes);
    if(ret < 0){
        pr_err("%s: kfifo_from_user() failed for pchar%d.\n", THIS_MODULE->name, dev->id);
        return ret;
    }
    printk(KERN_INFO "%s: pchar_write() written %d bytes in pchar%d.\n", THIS_MODULE->name, nbytes, dev->id);
    
    return nbytes;
}

static ssize_t pchar_read(struct file *pfile, char __user *ubuf, size_t bufsize, loff_t *pf_pos){
    pchardev_t *dev = (pchardev_t *)pfile->private_data;
    int ret, nbytes;
    pr_info("%s: pchar_read() called for pchar%d.\n", THIS_MODULE->name, dev->id);
    ret = kfifo_to_user(&dev->mybuf, ubuf, bufsize, &nbytes);
    if(ret < 0){
        pr_err("%s: kfifo_to_user() failed for pchar%d.\n", THIS_MODULE->name, dev->id);
        return ret;
    }

    printk(KERN_INFO "%s: pchar_read() read %d bytes if pchar%d.\n", THIS_MODULE->name, nbytes, dev->id);
    
    return nbytes   ;
    
}

loff_t pchar_lseek(struct file *pfile, loff_t offset, int origin){
    loff_t newpos = 0;
    pr_info("%s: pchar_llseek() called.\n", THIS_MODULE->name);
    switch(origin){
        case SEEK_SET:
            newpos = 0 + offset;
            break;
        case SEEK_END:
            newpos = MAX + offset;
            break;
        case SEEK_CUR:
            newpos = pfile->f_pos + offset;
            break;
    };
    if(newpos < 0)
        newpos = 0;
    if(newpos > MAX)
        newpos = MAX;
    pfile->f_pos = newpos;
    pr_info("%s: pchar_llseek() newpos = %lld.\n", THIS_MODULE->name, newpos);
    return newpos;
}

static long pchar_ioctl(struct file *pfile, unsigned int cmd, unsigned long param){
	devinfo_t info;
    pchardev_t *dev = (pchardev_t *)pfile->private_data;

	int ret;
	switch (cmd){
		case FIFO_CLEAR:
			kfifo_reset(&dev->mybuf);
			pr_info("%s: pchar_ioctl() dev buffer is cleared.\n", THIS_MODULE->name);
			return 0;
		case FIFO_GETINFO:	
			info.size = kfifo_size(&dev->mybuf);
			info.len = kfifo_len(&dev->mybuf);    
			info.avail = kfifo_avail(&dev->mybuf);
			ret = copy_to_user((void *)param, &info, sizeof(info));
			if(ret<0){
				pr_err("%s: copy_to_user() failed in pchar_ioctl().\n", THIS_MODULE->name);
				return ret;
			}
			pr_info("%s: pchar_ioctl() read dev buffer info.\n", THIS_MODULE->name);
			return 0;
		case FIFO_RESIZE:
            // allocate temp array to store fifo contents - kmalloc() (of length same as fifo).
			char *temp;
			int nbytes;
			int len = kfifo_len(&dev->mybuf);
			temp =  kmalloc(len, GFP_KERNEL);
			if(IS_ERR(temp)){
				pr_err("%s: kmalloc() failed in pchar_ioctl().\n", THIS_MODULE->name);
				return -1;
			}
		    // copy fifo contents into that temp array - kfifo_out()
			nbytes = kfifo_out(&dev->mybuf, temp, len);
            // release kfifo memory - kfifo_free()
			kfifo_free(&dev->mybuf);
            // allocate new memory for the fifo of size "param" (3rd arg) - kfifo_alloc()
            ret = kfifo_alloc(&dev->mybuf,param, GFP_KERNEL);
			if(ret != 0){
				pr_err("%s: kfifo_alloc() failed in pchar_ioctl().\n", THIS_MODULE->name);
				kfree(temp);
				return ret;				
			}
			// copy contents from temp memory into kfifo - kfifo_in()
            kfifo_in(&dev->mybuf, temp, len);
			// release temp array - kfree()
			kfree(temp);
            return 0;	
		
	}
	pr_err("%s: ioclt() invalid commad!\n", THIS_MODULE->name);
	return -EINVAL;
}

static struct file_operations pchar_fops = {
    .owner = THIS_MODULE,
    .open = pchar_open,
    .release = pchar_close,
    .read = pchar_read,
    .write = pchar_write,
    .llseek = pchar_lseek,
    .unlocked_ioctl = pchar_ioctl
};


static int __init pchar_init(void){
    int ret, i;
    struct device *pdevice;
    pr_info("%s: pchar_init() called.\n", THIS_MODULE->name);

    //allocate array device private structure
    devices = kmalloc(DEVCNT * sizeof(pchardev_t), GFP_KERNEL);
    if(IS_ERR(devices)){
        pr_err("%s: kmalloc() failed!\n", THIS_MODULE->name);
        ret = -1;
        goto KMALLOC_FAILED;
    }

    // allocate device number
    
    ret = alloc_chrdev_region(&devno, 0, DEVCNT, "pchar");
    if(ret != 0){
        pr_err("%s: alloc_chrdev_region() failed.\n", THIS_MODULE->name);
        goto ALLOC_CHRDEV_REGION_FAILED;
    }
    major = MAJOR(devno);
    pr_info("%s: alloc_chrdev_region() allocated device number: major = %d.\n", THIS_MODULE->name, major);
    // create device class
    pclass = class_create("pchar_class");
    if(IS_ERR(pclass)){
        pr_err("%s: class_create() failed.\n", THIS_MODULE->name);
        ret = -1;
        goto CLASS_CREATE_FAILED;
    }
    pr_info("%s: class_create() created pchar device.\n", THIS_MODULE->name);

	// create device file
    for(i = 0; i< DEVCNT; i++){
        devno = MKDEV(major, i);
        pdevice = device_create(pclass, NULL, devno, NULL, "pchar%d", i);
        if(IS_ERR(pdevice)){
            pr_err("%s: device_create() failed for pchar%d.\n", THIS_MODULE->name, i);
            ret = -1;
            goto DEVICE_CREATE_FAILED;
        }
        pr_info("%s: device_create() created pchar%d device.\n", THIS_MODULE->name, i);
    }
    // initialize cdev object and add it in kernel
    
    for(i=0; i < DEVCNT; i++){
        devno = MKDEV(major, i);
        devices[i].pchar_cdev.owner = THIS_MODULE;
        cdev_init(&devices[i].pchar_cdev, &pchar_fops);
        ret = cdev_add(&devices[i].pchar_cdev, devno, 1);
        if(ret != 0){
            pr_err("%s: cdev_add() failed for pchar%d.\n", THIS_MODULE->name, i);
            goto CDEV_ADD_FAILED;
        }
        pr_info("%s: cdev_add() added cdev into kernel for pchar%d.\n", THIS_MODULE->name, i);
    }

    //initialize device info
    for(i=0; i < DEVCNT; i++){
        devices[i].id = i;
        devices[i].devno = MKDEV(major, i);
        ret = kfifo_alloc(&devices[i].mybuf, MAX, GFP_KERNEL);
        if(ret != 0){
            pr_err("%s: kfifo_alloc() failed for pchar%d.\n", THIS_MODULE->name, i);
            goto KFIFO_ALLOC_FAILED;
        }
        pr_info("%s: kfifo_alloc() allocated fifo for pchar%d\n", THIS_MODULE->name, i);
    }
    return 0;


KFIFO_ALLOC_FAILED:
    for(i = i-1; i>=0; i--){
        kfifo_free(&devices[i].mybuf);
    }
    i = DEVCNT;
CDEV_ADD_FAILED:
    for(i = i-1; i>=0; i--){
        cdev_del(&devices[i].pchar_cdev);
    }
    i = DEVCNT;
DEVICE_CREATE_FAILED:
    for(i = i-1; i>=0; i--){
        devno = MKDEV(major, i);
        device_destroy(pclass, devno);
    }
    class_destroy(pclass);
CLASS_CREATE_FAILED:
    unregister_chrdev_region(devno, DEVCNT);

ALLOC_CHRDEV_REGION_FAILED:
    kfree(devices);

KMALLOC_FAILED:
    return ret;
}


static void __exit pchar_exit(void){
    int i;
    pr_info("%s: pchar_exit() called.\n", THIS_MODULE->name);
    // deinitialize device info
    for(i=0; i<DEVCNT; i++){
        kfifo_free(&devices[i].mybuf);
        pr_info("%s: Kfifo_free() released fifo for pchar%d\n", THIS_MODULE->name, i);
    }
    
    // remove cdev object from kernel
    for(i=0; i<DEVCNT; i++){
        cdev_del(&devices[i].pchar_cdev);
        pr_info("%s: cdev_del() removed device from kernel for pchar%d\n.\n", THIS_MODULE->name, i);
    }
    // destroy device file
    for(i=0; i<DEVCNT; i++){
        device_destroy(pclass, devices[i].devno);
        pr_info("%s: device_destroy() destroyed device file pchar%d.\n", THIS_MODULE->name, i);
    }
    // destroy device class
	class_destroy(pclass);
    pr_info("%s: class_destroy() destroyed devices class.\n", THIS_MODULE->name);
    // release device number
    unregister_chrdev_region(devno, DEVCNT);
    pr_info("%s: unalloc_chrdev_region() released device numbers: major = %d.\n", THIS_MODULE->name, major);
}

module_init(pchar_init);
module_exit(pchar_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("pchar 2nd attempt");
MODULE_AUTHOR("Anish Mhaske <anishmhaske02@gmail.com");
