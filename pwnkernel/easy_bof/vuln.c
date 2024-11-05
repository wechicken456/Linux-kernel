#include <linux/compiler.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/uaccess.h>


MODULE_DESCRIPTION("vuln1");
MODULE_AUTHOR("sash");
MODULE_LICENSE("GPL");

#define IOCTL_VULN1_WRITE 4141

static int vuln1_open(struct inode *inode, struct file *file)
{
	return 0;
}

static int vuln1_release(struct inode *inodep, struct file *filp)
{
	return 0;
}

static ssize_t vuln1_read(struct file *filp, /* see include/linux/fs.h   */ 
                           char __user *buffer, /* USER buffer to fill with data */ 
                           size_t length, /* length of the buffer     */ 
                           loff_t *offset) {
    char vuln_buf[32];
    
    /* The function calls _copy_to_user instead of copy_to_user in order to prevent the implemented security checks mitigate the buffer overflow. */
    int ret = _copy_to_user(buffer, vuln_buf, length);
    if (ret != 0){
        return -EFAULT;
    }
    return ret;
}
/* By declaring a function inline, you can direct GCC to make calls to that function faster. 
One way GCC can achieve this is to integrate that function's code into the code for its callers. 
This makes execution faster by eliminating the function-call overhead; i*/
static noinline int vuln1_do_breakstuff(unsigned long addr)
{
	char buffer[256];
	volatile int size = 512;
	/* The function calls _copy_from_user instead of copy_from_user in order to prevent the implemented security checks mitigate the buffer overflow. */
	return _copy_from_user(&buffer, (void __user *)addr, size);
}



static long vuln1_ioctl(struct file *fd, unsigned int cmd, unsigned long value)
{
	long to_return;

	switch (cmd) {
		case IOCTL_VULN1_WRITE:
			to_return = vuln1_do_breakstuff(value);
			break;
		default:
			to_return = -EINVAL;
			break;
	}

	return to_return;
}

static const struct file_operations vuln1_file_ops = {
	.owner		= THIS_MODULE,
	.open		= vuln1_open,
    .read       = vuln1_read,
	.unlocked_ioctl = vuln1_ioctl,
	.release	= vuln1_release,
	.llseek 	= no_llseek,
};

struct miscdevice vuln1_device = {
	.minor	= MISC_DYNAMIC_MINOR,
	.name	= "vuln",
	.fops	= &vuln1_file_ops,
	.mode	= 0666,
};

module_misc_device(vuln1_device);

