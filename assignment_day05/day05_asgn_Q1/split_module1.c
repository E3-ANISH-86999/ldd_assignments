#include <linux/init.h>
#include <linux/module.h>

static int vdg_init(void){
    printk(KERN_INFO "This is the module with two files...\n");
    return 0;
}


module_init(vdg_init);

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("AnishMhaske");
MODULE_DESCRIPTION("Two file kernel module");