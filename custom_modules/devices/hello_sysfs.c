/* 
 * hello-sysfs.c sysfs example 
 */ 
#include <linux/fs.h> 
#include <linux/init.h> 
#include <linux/kobject.h> 
#include <linux/module.h> 
#include <linux/string.h> 
#include <linux/sysfs.h> 

static int myVar = 0;
static struct kobject *mymodule; 

/* see "kobj_attribute" here: https://git.kernel.org/pub/scm/linux/kernel/git/stable/linux.git/tree/include/linux/kobject.h */
static ssize_t myVar_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf ) {
    return sprintf(buf, "%d\n", myVar);
}

/* see "kobj_attribute" here: https://git.kernel.org/pub/scm/linux/kernel/git/stable/linux.git/tree/include/linux/kobject.h */
static ssize_t myVar_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count) {
    sscanf(buf, "%d\n", &myVar);
    return count;
}


static struct kobj_attribute myVar_attribute = 
    __ATTR(myVar, 0660, myVar_show, myVar_store);


static int __init mymodule_init(void) {
    int error = 0;

    /* create a kobject */
    mymodule = kobject_create_and_add("hello-sysfs", kernel_kobj); 
    if (!mymodule) {
        return -ENOMEM;
    }

    /* register our attribute to be a sysfs file */
    error = sysfs_create_file(mymodule, &myVar_attribute.attr);
    if (error) {
        pr_info("Failed to create \"myVar\" file in /sys/kernel/hello-sysfs...");
    }
    return error;
}

static void __exit mymodule_exit(void) 

{ 
    pr_info("hello-sysfs: Exit success\n"); 
    kobject_put(mymodule); 
} 

module_init(mymodule_init); 
module_exit(mymodule_exit); 

MODULE_LICENSE("GPL");
