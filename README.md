COP4600 Spring 2017

Programming Assignment 3 

Assignment Group 18


Character-mode Linux device driver as a kernel module: 
Stores bytes to a buffer and removes them from the buffer as they are read out in FIFO order. 

## Instructions:
	>> make				# execute the Makefile 
	>> sudo insmod testdev.ko	# load the module into the kernel 
	>> sudo insmod testdev2.ko  # load the second module into the kernel
	>> dmesg  			# to confirm that the device was added successfully 
	>> sudo ./test 			# use the provided userspace test program to test the kernel module (if desired)  
	>> sudo rmmod testdev2 		# remove the module when finished
	>> sudo rmmod testdev
