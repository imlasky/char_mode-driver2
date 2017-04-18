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

#define DEVICE_NAME "testdev2"
#define CLASS_NAME "chardev2"
#define BUFF_SIZE 1024 

//License and author info
MODULE_LICENSE("GPL");
MODULE_AUTHOR("OS Group 18");
MODULE_DESCRIPTION("A simple character-mode Driver.");

static int majorNumber;
extern int messageSize;
extern char message[];
static struct class * testdev2Class = NULL;
static struct device* testdev2Device = NULL;
extern struct mutex global_mutex;
//prototypes for file ops
static int dev_open(struct inode *inodep, struct file *filep);
static int dev_release(struct inode* inodep, struct file* filep);
static ssize_t dev_read(struct file* filep, char* buffer, size_t len, loff_t* offset);

static struct file_operations fops = 
{
	.open = dev_open,
	.read = dev_read,
	.release = dev_release,
};


//Initialization: register a Major number, save it for later mknod
static int __init testdev2_init(void) {
	//Request Major device number
	majorNumber = register_chrdev(0, DEVICE_NAME, &fops);
	
	if (majorNumber < 0) {
		printk(KERN_ALERT "testdev2: Failed to register a major version number.\n");
		return majorNumber;
	}
	printk(KERN_INFO "testdev2: Registered major version number %d.\n", majorNumber);

	testdev2Class = class_create(THIS_MODULE, CLASS_NAME);
	if (IS_ERR(testdev2Class)){
		unregister_chrdev(majorNumber,DEVICE_NAME);
		printk(KERN_ALERT "Failed to register device class\n");
		return PTR_ERR(testdev2Class);
	}
	printk(KERN_INFO "testdev2: Device class registered correctly\n");

	testdev2Device = device_create(testdev2Class, NULL, MKDEV(majorNumber,0), NULL, DEVICE_NAME);
	if (IS_ERR(testdev2Device)){
		class_destroy(testdev2Class);
		unregister_chrdev(majorNumber, DEVICE_NAME);
		printk(KERN_ALERT "testdev2: Failed to create the device\n");
		return PTR_ERR(testdev2Device);
	}
	printk(KERN_INFO "testdev2: Device class created correctly\n");
	strcpy(message, "");
	return 0;
}


//Exit: unregister the Major number
static void __exit testdev2_exit(void) {
	device_destroy(testdev2Class, MKDEV(majorNumber, 0));
	printk(KERN_INFO "testdev2: Destroyed device\n");
	class_unregister(testdev2Class);
	printk(KERN_INFO "testdev2: Unregistered class\n");
	class_destroy(testdev2Class);
	printk(KERN_INFO "testdev2: Destroyed class\n");
	unregister_chrdev(majorNumber, DEVICE_NAME);
	printk(KERN_INFO "testdev2: Unregistered major version number %d.\n", majorNumber);
}


//Open: open the device
static int dev_open(struct inode* inodep, struct file* filep) {
	printk(KERN_INFO "testdev2: Device opened.\n");
	return 0;
}

//Release: close the device
static int dev_release(struct inode* inodep, struct file* filep) {
	printk(KERN_INFO "testdev2: Device closed.\n");
	return 0;
}


//Read: read information from the device
static ssize_t dev_read(struct file* filep, char* buffer, size_t len, loff_t* offset) {
	int err = 0;
	static char messageTemp [BUFF_SIZE] = {0};
	size_t i;
	if(!mutex_trylock(&global_mutex)){
		printk(KERN_ALERT "testdev2: Device in use by another process");
		return -EBUSY;
	}

	if (len >= messageSize) {
		if (messageSize == 0 ) {
			err = copy_to_user(buffer,"",1);
		} else {
			err = copy_to_user(buffer,message,messageSize);
		}
		if (err == 0) {
			printk(KERN_INFO "testdev2: Sent %d characters to the user\n",messageSize);
			printk(KERN_INFO "testdev2: %s\n",message);
			strcpy(message,"");
			messageSize = 0;
			mutex_unlock(&global_mutex);
			return 0;
		} else {
			printk(KERN_INFO "testdev2: Failed to send %d characters to the user.\n",messageSize);
			mutex_unlock(&global_mutex);
			return -EFAULT;
		}
	} else {
		err = copy_to_user(buffer,message,len);
		if(err == 0) {
			printk(KERN_INFO "testdev2: Sent %d characters to the user\n",len);
			messageSize -= len;
			for (i = len; i < BUFF_SIZE; i++) {
				messageTemp[i-len] = message[i];
			}
			strcpy(message,messageTemp);
			strcpy(messageTemp,"");
			mutex_unlock(&global_mutex);
			return 0;
		} else {
			printk(KERN_INFO "testdev2: Failed to send %d characters to the user.\n", len);
			mutex_unlock(&global_mutex);
			return -EFAULT;
		}
	}
	mutex_unlock(&global_mutex);
}

module_init(testdev2_init);
module_exit(testdev2_exit);
