/*
 *	Hello World 2 module
 *	Demonstrating the module_init() and module_exit() macros
 *	This is prefered over init_module() and cleanup_module()
 */

#include<linux/module.h>	/* needed by all modules */
#include<linux/printk.h>	/* needed for pr_info() */
#include<linux/init.h>		/* needed by the macros */
/*
int init_module(void) {
	pr_info("Hello World 1. \n");

	// A non-0 return means failed init_module; module can't be loaded 
	return 0;
}

void cleanup_module(void) {
	pr_info("Goodbye world 1.\n");
}
*/

static int hello_2_data __initdata = 2;

static int __init hello_2_init(void) {
	pr_info("Hello World %d\n", hello_2_data);
	return 0;
}

static void __exit hello_2_exit(void) {
	pr_info("Goodbye World 2\n");
}

module_init(hello_2_init)
module_exit(hello_2_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Tin");
MODULE_DESCRIPTION("A very simple loadable kernel module");

