/*
 *	Hello World 3 module
 *	Demonstrating command line argument passing to a module
 */

#include<linux/module.h>	/* needed by all modules */
#include<linux/printk.h>	/* needed for pr_info() */
#include<linux/init.h>		/* needed by the macros */

static int myint= 1337;
static long mylong = 13371337;
static int myarray[2] = {0xdead, 0xbeef};
static int arr_cnt = 0;
/* module_param(foo, int, 0000)

 * The first param is the parameter's name.

 * The second param is its data type.

 * The final argument is the permissions bits,

 * for exposing parameters in sysfs (if non-zero) at a later stage.

 */
module_param(myint, int, S_IRUGO | S_IWUSR); // U, G, O can read, User can also write == 0644 in octal
MODULE_PARM_DESC(myint, "An int"); 
module_param(mylong, long, S_IRUGO | S_IWUSR); // U, G, O can read, User can also write == 0644 in octal
MODULE_PARM_DESC(mylong, "A long"); 
/* module_param_array(name, type, num, perm); 

 * The first param is the parameter's (in this case the array's) name. 

 * The second param is the data type of the elements of the array. 

 * The third argument is a pointer to the variable that will store the number 

 * of elements of the array initialized by the user at module loading time. 

 * The fourth argument is the permission bits. 

 */ 
module_param_array(myarray, int, &arr_cnt, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(myintarray, "An array of integers"); 

// this variable lives in `.init.data` so it will be freed/discarded when initialization is done (even if it's defined as `static`).
// while the global static variables above will live.
static int hello_3_data __initdata = 3;

static int __init hello_3_init(void) {
	pr_info("Hello World %d\n", hello_3_data);
	pr_info("myint: %d\n", myint);
	pr_info("mylong: %ld\n", mylong);
	pr_info("myarray's length arr_cnt: %d\n", arr_cnt);
	pr_info("myarray[0]: %d\n", myarray[0]);
	pr_info("myarray[1]: %d\n", myarray[1]);
	pr_info("Done\n");
	return 0;
}

static void __exit hello_3_exit(void) {
	pr_info("Goodbye World 3\n");
}

module_init(hello_3_init)
module_exit(hello_3_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Tin");
MODULE_DESCRIPTION("A very simple loadable kernel module");

