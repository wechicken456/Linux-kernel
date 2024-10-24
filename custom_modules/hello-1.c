/*
 *	Hello World 1 module - the simplest module
 */

#include<linux/module.h>	/* needed by all modules */
#include<linux/printk.h>	/* needed for pr_info() */

int init_module(void) {
	pr_info("Hello World 1. \n");

	/* A non-0 return means failed init_module; module can't be loaded */
	return 0;
}

void cleanup_module(void) {
	pr_info("Goodbye world 1.\n");
}

MODULE_LICENSE("GPL");
