/*
 =====================================================================================
 Driver Name : SYNCH_SEMAPHORE
 Author      : CDAC
 Version     :
 License	 : GPL
 Description : Sample driver code demonstrating the concept of semaphores.
 	 	 	   The driver loads by creating a character driver followed by its
 	 	 	   associated node. Then it initialises a semaphore variable to an
 	 	 	   unlocked state.
 	 	 	   Now the driver is allowed to interact with the user space.
 	 	 	   The driver created here is a basic character driver which only
 	 	 	   supports read operation. Assuming that from the read function,
 	 	 	   the value is read from the hardware, which is actually a variable
 	 	 	   getting incremented every time. A sleep is also introduced in
 	 	 	   the critical section, thus creating an illusion that the driver is
 	 	 	   taking that much amount of time to read the value from the hardware.
 	 	 	   The whole critical region is protected by a semaphore, thus allowing
 	 	 	   only one process to interact with it at a particular instant.
 	 	 	   The user application program(also supplied with this code) is designed
 	 	 	   in such a way that it calls read() on this device twice, in parallel.
 	 	 	   As such the second read has to be blocked by the semaphore
 	 	 	   until the first read() finishes, that is, not until a duration of
 	 	 	   'DELAY' defined here.
 	 	 	   Finally, before the module is unloaded, the device nodes are cleared
 	 	 	   and the character driver is unregistered.
 =====================================================================================
 */
#include"synch_semaphore.h"

#define LOCK 0
#define UNLOCK 1
#define NMINORS 1

/* Assume that this is the delay to get the data from the hardware */
#define DELAY 5000

MODULE_LICENSE("GPL");
MODULE_AUTHOR("CDAC");

/* Declare a variable of type 'struct semaphore' to perform the operations */
struct semaphore sample_sem;

/* The variable decelerations pertaining to the char driver */
int major;
int minor = 0;
dev_t deviceno;
struct cdev synch_semaphore_cdev;
struct class *synch_semaphore_class;
struct device *synch_semaphore_device;

/* Character driver's read function : Called when read() is called on the device node
 * The function returns the number of bytes actually written
 */
ssize_t synch_semaphore_read(struct file *filp, char __user *Ubuff, size_t count, loff_t *offp)
{
	static unsigned int fake_val = 10;

	printk(" In the read function \n");

	/* Acquire the semaphore */
	if(down_interruptible(&sample_sem))
		return -ERESTARTSYS;
	/* Critical section starts here */

	/* Let the current thread goto sleep, thus creating an illusion of reading
	 * the value from the hardware
	 */
	msleep(DELAY);

	/* Let the value received from the device is an increment of the previous */
	fake_val++;

	/* Send the data to the user */
	copy_to_user(Ubuff, &fake_val, sizeof(fake_val));

	/* Release the semaphore */
	up(&sample_sem);
	/* Critical section ends here */

	return sizeof(fake_val);
}

/* Define the structure corresponding to the file operations */
struct file_operations synch_semaphore_fops= {
	.owner = THIS_MODULE,
	.read=synch_semaphore_read
};

/* Initialisation fucntion */
static int __init synch_semaphore_init(void)
{
	PINFO("In init() function\n");

	/* Create a character driver */
	alloc_chrdev_region(&deviceno , minor, NMINORS , DRIVER_NAME);
	major = MAJOR(deviceno);

	cdev_init(&synch_semaphore_cdev, &synch_semaphore_fops);
	cdev_add(&synch_semaphore_cdev , deviceno , 1);

	/* Create a node in /dev by the name "sample_cdev" */
	synch_semaphore_class = class_create(THIS_MODULE , DRIVER_NAME);
	synch_semaphore_device = device_create(synch_semaphore_class , NULL , deviceno , NULL , "sample_cdev");

	/* Initialise the semaphore */
	sema_init(&sample_sem ,UNLOCK);

	return 0;
}

static void __exit synch_semaphore_exit(void)
{
	PINFO("In exit function\n");

	/* Delete the device node created */
	device_destroy(synch_semaphore_class , deviceno);
	class_destroy(synch_semaphore_class);

	/* Unregister the character driver */
	cdev_del(&synch_semaphore_cdev);
	unregister_chrdev_region(deviceno ,NMINORS);
}

module_init( synch_semaphore_init)
module_exit( synch_semaphore_exit)
