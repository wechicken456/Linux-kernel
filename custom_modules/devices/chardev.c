/* 

 * chardev.c: Creates a read-only char device that says how many times 

 * you have read from the dev file 

 */ 
 
#include <linux/atomic.h> 
#include <linux/cdev.h> 
#include <linux/delay.h> 
#include <linux/device.h> 
#include <linux/fs.h> 
#include <linux/init.h> 
#include <linux/kernel.h> /* for sprintf() */ 
#include <linux/module.h> 
#include <linux/printk.h> 
#include <linux/types.h> 
#include <linux/uaccess.h> /* for get_user and put_user */ 
#include <linux/version.h> 

#include <asm/errno.h> 

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Tin");

static int device_open(struct inode *, struct file *);
static int device_release(struct inode *, struct file *);
static ssize_t device_read(struct file *, char __user *, size_t, loff_t *);
static ssize_t device_write(struct file *, const char __user *, size_t, loff_t *);

#define SUCCESS 0 
#define DEVICE_NAME "chardev" /* Dev name as it appears in /proc/devices   */ 
#define BUF_LEN 80 /* Max length of the message from the device */ 


/* Global variables are declared as static, so are global within the file. */ 
 
static int major; /* major number assigned to our device driver */ 

enum {
	CDEV_NOT_USED = 0,
	CDEV_EXCLUSIVE_OPEN = 1,
};	

/* Is device open? Used to prevent multiple access to device */ 
static atomic_t already_open = ATOMIC_INIT(CDEV_NOT_USED);

static char msg[BUF_LEN + 1]; /* The msg the device will give when asked */ 

static struct class *cls;

static struct file_operations chardev_fops = {
	.read = device_read,
	.write = device_write,
	.open = device_open,
	.release = device_release,
};

static int __init chardev_init(void) {
	// return the allocated major number specified by the 1st argument
	// passing 0 as argument to return a DYNAMICALLY allocated major number
	major = register_chrdev(0, DEVICE_NAME, &chardev_fops);
	
	if (major < 0) {
		pr_alert("Registering chardev failed with %d", major);
		return major;
	}
	pr_info("I was assigned major number %d.\n", major);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 4, 0)
	cls = class_create(DEVICE_NAME);
#else
	cls = class_create(THIS_MODULE, DEVICE_NAME);
#endif
	
	device_create(cls, NULL, MKDEV(major, 0), NULL, DEVICE_NAME);
	
	pr_info("Device created on /dev/%s\n", DEVICE_NAME);
	
	return 0;
}

static void __init chardev_exit(void) {
	device_destroy(cls, MKDEV(major, 0));
	class_destroy(cls);

	/* Unregister the device */
	unregister_chrdev(major, DEVICE_NAME);
}


/* Methods */ 

/* Called when a process tries to open the device file, like 
 * "sudo cat /dev/chardev" 
 */ 
static int device_open(struct inode *inode, struct file *file) {
	static int counter = 0;
	
	/* performs an atomic compare exchange operation on the atomic value at "mem", with the given old and new values
		1. Compares “old” with the value currently at “mem”.
		2. If they are equal, “new” is written to “mem”.
		3. Regardless, the current value at “mem” is returned.
	 */
	if (atomic_cmpxchg(&already_open, CDEV_NOT_USED, CDEV_EXCLUSIVE_OPEN)) {
		return -EBUSY;
	}

	sprintf(msg, "I already said \"Hello World!\n\" %d times...", counter++);
	
	/* Increment the reference count of this module 
	 * https://www.kernel.org/doc/html/next/driver-api/basics.html
	 */
	try_module_get(THIS_MODULE);

	return SUCCESS;
}


/* Called when a process closes the device file. */
static int device_release(struct inode *inode, struct file *file) {
	/* We're now ready for the next caller */
	atomic_set(&already_open, CDEV_NOT_USED);

	/* Decrement the usage count, or else once you opened the file, you will
     * never get rid of the module.
     */
    module_put(THIS_MODULE);
	
	return SUCCESS;
}	


static ssize_t device_read(struct file *filp, /* see include/linux/fs.h   */ 
                           char __user *buffer, /* USER buffer to fill with data */ 
                           size_t length, /* length of the buffer     */ 
                           loff_t *offset) {	/* current position in the file */ 
	/* Number of bytes actually written to the buffer */ 

    int bytes_read = 0; 
    const char *msg_ptr = msg; 


    if (!*(msg_ptr + *offset)) { /* we are at the end of message */ 
        *offset = 0; /* reset the offset */ 
        return 0; /* signify end of file */ 
    }
	
	msg_ptr += *offset;
	while (length && *msg_ptr) {
		/* The buffer is in the user data segment, not the kernel 
		 * segment so "*" assignment won't work.  We have to use 
		 * "put_user" which copies data from the kernel data segment to 
		 * the user data segment. 
		 */ 
		put_user(*(msg_ptr++), buffer++);
		length--;
		bytes_read++;
	}

	*offset += bytes_read;	
	return bytes_read;
}


/* Called when a process writes to the device e.g. echo "hi" > /dev/chardev */
static ssize_t  device_write(struct file *filp, const char __user *buff, 
							size_t length, loff_t *offset) {
	pr_alert("Sorry, write is not implemented.\n");
	return SUCCESS;
	//return -EINVAL;
}

module_init(chardev_init);
module_exit(chardev_exit);
