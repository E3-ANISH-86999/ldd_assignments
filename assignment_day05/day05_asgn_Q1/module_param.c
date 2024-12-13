#include <linux/module.h>
#include <linux/moduleparam.h>

static char *toWhom = "World";
static int howManyTimes = 1;

module_param(toWhom, charp, S_IRUGO);
module_param(howManyTimes, int, S_IRUGO);

static int vdg_init(void){
    int i;
    
    for(i=0; i<howManyTimes; i++){
        printk(KERN_INFO "Hello %s !!!\n", toWhom);
    }
    return 0;
}

static void vdg_exit(void){
    int i;

    for(i=0; i<howManyTimes; i++)
        printk(KERN_INFO "Goodbye %s !!!\n", toWhom);
}

module_init(vdg_init);
module_exit(vdg_exit);

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("AnishMhaske");
MODULE_DESCRIPTION("First Linux Kernel Module");

