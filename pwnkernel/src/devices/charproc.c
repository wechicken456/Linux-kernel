/*
 * character device using procfs
 *
 */

#include<linux/kernel.h>
#include<linux/module.h>
#include<linux/proc_fs.h>
#include<linux/uaccess.h>   /* for copy_to_user */
#include<linux/version.h>
#include<linux/minmax.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Tin");

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 6, 0)
#define HAVE_PROC_OPS
#endif

#define PROCFS_NAME "charproc"
#define PROCFS_MAX_SIZE 2048UL

/* This structure holds information about our /proc file*/
static struct proc_dir_entry *our_proc_file;
/* The kernel buffer used to store characters for this module */ 
static char procfs_buffer[PROCFS_MAX_SIZE]; 
/* The current size of the buffer (how many bytes the user has )*/ 
static unsigned long procfs_buffer_size = 0; 

// only allow one read after each write - doesn't matter if the read didn't drain `procfs_buffer`. 
// return the number of btes read
static ssize_t procfs_read(struct file *filp, char __user *buffer, size_t buffer_length, loff_t *offset) {

    // if offset is not 0, then we have read before. Since we only allow 1 read, return error.
    if (*offset || procfs_buffer_size == 0) {
        pr_info("procfs_read: END\n");
        *offset = 0;
        return 0;
    }    

    procfs_buffer_size = min(procfs_buffer_size, buffer_length);
    if (copy_to_user(buffer, procfs_buffer, procfs_buffer_size)) {
        return -EFAULT;
    }

    *offset += procfs_buffer_size;
    pr_debug("procfs_read: read %lu bytes\n", procfs_buffer_size);
    pr_info("procfile read from: %s\n", filp->f_path.dentry->d_name.name); 
    return procfs_buffer_size;
}


static ssize_t procfs_write(struct file *filp, const char __user *buffer, size_t len, loff_t *off) {

    procfs_buffer_size = min(PROCFS_MAX_SIZE, len);

    // copy `procfs_buffer_size` bytes from user buffer `buffer` to our kernel buffer
    if (copy_from_user(procfs_buffer, buffer, procfs_buffer_size)) {
        return -EFAULT;
    }
    *off += procfs_buffer_size;

    pr_debug("procfs_write: wrote %lu bytes\n", procfs_buffer_size);
    return procfs_buffer_size;
}

static int procfs_open(struct inode *inode, struct file *file) {
    try_module_get(THIS_MODULE);
    return 0;
}

static int procfs_close(struct inode *inode, struct file *file) {
    module_put(THIS_MODULE);
    return 0;
}


#ifdef HAVE_PROC_OPS
static struct proc_ops file_ops_4_our_proc_file = {
    .proc_read = procfs_read,
    .proc_write = procfs_write,
    .proc_open = procfs_open,
    .proc_release = procfs_close,
};
#else
static struct file_operations file_ops_4_our_proc_file = {
    .read = procfs_read,
    .write = procfs_write,
    .open = procfs_open,
    .release = procfs_close,
};
#endif 

static int __init procfs_init(void) {
    our_proc_file = proc_create(PROCFS_NAME, 0644, NULL, &file_ops_4_our_proc_file);
    
    if (our_proc_file == NULL) {
        pr_debug("Error: Could not initialize /proc/%s\n", PROCFS_NAME);
        return -ENOMEM;
    }

    proc_set_size(our_proc_file, 80);
    proc_set_user(our_proc_file, GLOBAL_ROOT_UID, GLOBAL_ROOT_GID); /* Our proc file is owned by root */

    pr_debug("/proc/%s created!\n", PROCFS_NAME);
    return 0;
}

static void __exit procfs_exit(void) {
    remove_proc_entry(PROCFS_NAME, NULL);
    pr_debug("/proc/%s removed\n", PROCFS_NAME);
}

module_init(procfs_init);
module_exit(procfs_exit);






