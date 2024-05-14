#include <linux/module.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/kernel.h>
#include <linux/uaccess.h>

#define DEVICE_DRIVER_NAME "driver_test"

long number = 0;
int major_number;

int test_device_driver_open(struct inode *, struct file *);
int test_device_driver_release(struct inode *, struct file *);
ssize_t test_device_driver_write(struct file *, const char *, size_t, loff_t *);
ssize_t test_device_driver_read(struct file *, char *, size_t, loff_t *);
static struct file_operations device_driver_fops =
{ .open = test_device_driver_open, .write = test_device_driver_write, .read= test_device_driver_read, .release = test_device_driver_release };


int test_device_driver_open(struct inode *minode, struct file *mfile) {
	printk("test_device_driver_open\n");
	return 0;
}

int test_device_driver_release(struct inode *minode, struct file *mfile) {
	printk("test_device_driver_release\n");
	return 0;
}


ssize_t test_device_driver_write(struct file *inode, const char *gdata, size_t length, loff_t *off_what) {
	const char *tmp = gdata;
	char kernel_buff[4];


	printk("Write\n");
	if (copy_from_user(&kernel_buff, tmp, 4)) {
		return -EFAULT;
	}
	number = simple_strtol(kernel_buff, NULL, 10);
	printk("Copy number to kernel buffer : %d \n",number);

	return 1;
}
ssize_t test_device_driver_read(struct file *inode, char *gdata, size_t length, loff_t * off_what){
	int result = 0;
	char *tmp = gdata;
	result += (number / 1000) %10;
	result += (number /100) % 10;
	result += (number /10) % 10;
	result +=(number) % 10;
	printk("Read\n");
	if (copy_to_user(tmp, &result, 1)) {
		return -EFAULT;
	}
	printk("RST :%d\n",result);
	return 1;
}

int __init device_driver_init(void)
{

	printk("device_driver_init\n");
	
	major_number = register_chrdev(0, DEVICE_DRIVER_NAME, &device_driver_fops);
	if(major_number <0) {
		printk( "error %d\n",major_number);
		return major_number;
	}

    printk( "dev_file: /dev/%s , major: %d\n", DEVICE_DRIVER_NAME, major_number);
	
	printk("init module\n");
	return 0;
}

void __exit device_driver_exit(void)
{
	printk("device_driver_exit\n");

	unregister_chrdev(major_number, DEVICE_DRIVER_NAME);
}

module_init(device_driver_init);
module_exit(device_driver_exit);

MODULE_LICENSE ("GPL");
MODULE_AUTHOR ("author");
