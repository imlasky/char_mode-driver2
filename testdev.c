/*
 
COP 4600 - Operating Systems
Programming Assignment 2

A simple kernel module: a charater-mode device driver
 */
 
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/fs.h>		//needed for character driver init
#include <linux/init.h>
#include <linux/uaccess.h>	//needed for copy_to_user
#include <linux/mutex.h>

#define DEVICE_NAME "testdev"
#define CLASS_NAME "chardev"
#define BUFF_SIZE 1024 

//License and author info
MODULE_LICENSE("GPL");
MODULE_AUTHOR("OS Group 18");
MODULE_DESCRIPTION("A simple character-mode Driver.");

static int majorNumber;
static char message[BUFF_SIZE] = {0};
static int messageSize = 0;
static struct class * testdevClass = NULL;
static struct device* testdevDevice = NULL;
static DEFINE_MUTEX(global_mutex);

//prototypes for file ops
static int dev_open(struct inode *inodep, struct file *filep);
static int dev_release(struct inode* inodep, struct file* filep);
static ssize_t dev_write(struct file* filep, const char* buffer, size_t len, loff_t* offset);

static struct file_operations fops = 
{
	.open = dev_open,
	.write = dev_write,
	.release = dev_release,
};


//Initialization: register a Major number, save it for later mknod
static int __init testdev_init(void) {
	//Request Major device number
	mutex_init(&global_mutex);
	majorNumber = register_chrdev(0, DEVICE_NAME, &fops);
	
	if (majorNumber < 0) {
		printk(KERN_ALERT "testdev: Failed to register a major version number.\n");
		return majorNumber;
	}
	printk(KERN_INFO "testdev: Registered major version number %d.\n", majorNumber);

	testdevClass = class_create(THIS_MODULE, CLASS_NAME);
	if (IS_ERR(testdevClass)){
		unregister_chrdev(majorNumber,DEVICE_NAME);
		printk(KERN_ALERT "Failed to register device class\n");
		return PTR_ERR(testdevClass);
	}
	printk(KERN_INFO "testdev: Device class registered correctly\n");

	testdevDevice = device_create(testdevClass, NULL, MKDEV(majorNumber,0), NULL, DEVICE_NAME);
	if (IS_ERR(testdevDevice)){
		class_destroy(testdevClass);
		unregister_chrdev(majorNumber, DEVICE_NAME);
		printk(KERN_ALERT "testdev: Failed to create the device\n");
		return PTR_ERR(testdevDevice);
	}
	printk(KERN_INFO "testdev: Device class created correctly\n");
	strcpy(message, "");
	return 0;
}


//Exit: unregister the Major number
static void __exit testdev_exit(void) {
	mutex_destroy(&global_mutex);
	device_destroy(testdevClass, MKDEV(majorNumber, 0));
	printk(KERN_INFO "testdev: Destroyed device\n");
	class_unregister(testdevClass);
	printk(KERN_INFO "testdev: Unregistered class\n");
	class_destroy(testdevClass);
	printk(KERN_INFO "testdev: Destroyed class\n");
	unregister_chrdev(majorNumber, DEVICE_NAME);
	printk(KERN_INFO "testdev: Unregistered major version number %d.\n", majorNumber);
}


//Open: open the device
static int dev_open(struct inode* inodep, struct file* filep) {
	printk(KERN_INFO "testdev: Device opened.\n");
	return 0;
}

//Release: close the device
static int dev_release(struct inode* inodep, struct file* filep) {
	printk(KERN_INFO "testdev: Device closed.\n");
	return 0;
}

//Write: write information to the device
static ssize_t dev_write(struct file* filep, const char* buffer, size_t len, loff_t* offset) {
	size_t space = BUFF_SIZE - messageSize;
	mutex_lock(&global_mutex);
	if (len > BUFF_SIZE)
		len = BUFF_SIZE;
	if (space > 0) {
		if (space > len) {
			strcat(message,buffer);
			messageSize = strlen(message);
			printk(KERN_INFO "testdev: Received %zu characters from the user\n", len);
			mutex_unlock(&global_mutex);
			return len;
		} else {
			if (space == len) {
				strcpy(message,buffer);
				messageSize = BUFF_SIZE;
				printk(KERN_INFO "testdev: Buffer full. Only wrote %zu characters from the user.\n",len);
				mutex_unlock(&global_mutex);
				return len;
			}
			strncat(message,buffer,len-space);
			messageSize = strlen(message);
			printk(KERN_INFO "testdev: Buffer full. Only wrote %zu characters from the user.\n", len-space);
			mutex_unlock(&global_mutex);
			return len-space;
		}
	} else {
		printk(KERN_INFO "testdev: Cannot receive characters from the user. Buffer full.\n");
		mutex_unlock(&global_mutex);
		return -1;
	}
	mutex_unlock(&global_mutex);
}
EXPORT_SYMBOL(global_mutex);
EXPORT_SYMBOL(message);
EXPORT_SYMBOL(messageSize);
module_init(testdev_init);
module_exit(testdev_exit);
