#include <linux/init.h>
#include <linux/module.h>

static int vdg_init(void){
    printk(KERN_INFO "Hello world...\n");
    return 0;
}


static void vdg_exit(void){
    printk(KERN_INFO "Exiting from module...\n");
}
module_inti(vdg_init);
module_exit(vdg_exit);


MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("AnishMhaske");
MOUDLE_DESCRIPTION("VDG: First linux kernel module");