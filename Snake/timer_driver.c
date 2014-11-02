/*
 * timer_driver.c
 *
 * HWThread Adapter component to interface QUANT HW threads to Linux.
 *
 * Author: Aws Ismail, Andrei-Liviu Simion, Eric Matthews, Kevan Thompson
 *
 * Revision History:
 *	Feb 18 2010 - Initial version.
 *	July 22 2011 - Modified driver to support PetaLinux v2.1
 *	Nov 5, 2012 - Used to create Timer Driver	
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/slab.h>
#include <linux/fs.h>

#include <asm/io.h>
#include <asm/uaccess.h>

#include <linux/miscdevice.h>
#include <linux/list.h>
#include <linux/wait.h>

#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/of_platform.h>

#include <linux/interrupt.h>
#include <asm/irq.h>
#include "timer_ioctl.h"


// Module information.
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("timer_driver");
#define DRIVER_NAME "timer_driver"
#define DRIVER_NAME_ID(id) DRIVER_NAME#id
#define NUM_SLAVE_REGS 10

// Version information.
int timer_major = MISC_MAJOR;
module_param(timer_major, int, 0);
static int dev_id = 0;

struct timer_local *instance_local = 0;
struct fasync_struct *async_queue; //async_queue


//-------------------------------------------------------------------
// Prototypes 
//-------------------------------------------------------------------

// File operations.
static int	timer_open(struct inode *inode, struct file *file);
static int	timer_release(struct inode *inode, struct file *file);
static ssize_t	timer_mmap(struct file *file, struct vm_area_struct *vma);
static long	timer_ioctl(struct file *file, unsigned int cmd, unsigned long arg);

// Driver functions.
static int	__devinit timer_probe(struct platform_device *ofdev, const struct of_device_id *match);
static int	__devexit timer_remove(struct platform_device *ofdev);
static int	__init timer_init(void);
static void	__exit timer_exit(void);

// Miscellaneous functions.
//static struct	timer_local* get_instance(unsigned int minor, int set_busyflag, int busy_flag);

// Structures.
struct of_platform_driver	timer_driver;
static struct	file_operations		timer_fops;
struct timer_local;
static struct	of_device_id		__devinitdata timer_match[];
//timer Interrupt
//timer ISR
static irqreturn_t timer_interrupt(int irq, void *dev_id, struct pt_regs *regs);

//Asynchronous Notification
static int timer_fasync(int fd, struct file *filp, int mode);


/* Structure. ************************************************************************************/

struct of_platform_driver timer_driver = {
	.driver = {
		.name  =	DRIVER_NAME,
		.owner = THIS_MODULE,
		.of_match_table =	timer_match,
	},
	.probe  =	timer_probe,
	.remove = __devexit_p(timer_remove),
};

static struct file_operations timer_fops = {
	.open    = timer_open,
	.release = timer_release,
	.fasync  = timer_fasync, //Whats this for? for Asynchronous
	.mmap    = timer_mmap,
	.unlocked_ioctl = timer_ioctl,
};

struct timer_local {
	unsigned long base_phys;	// IP core base address.
	unsigned int base_addr;	   // Virtual address for the reg file IO MEM resource.
	unsigned long remap_size;
	struct miscdevice	*miscdev;

};
//The compatible option here must be the same as the compatible option in the 
// dts file. 
// /software/petalinux-dist/linux-2.6.x/arch/microblaze/boot/dts/Digilent-ensc351_project.dts
static struct of_device_id __devinitdata timer_match[] = {
	{ .compatible = "timer_test", }, //xlnx,xps-gpio-2.00.a, xlnx,xps-gpio-1.00.a generic-uio
	{ /* end of list */ },
};
MODULE_DEVICE_TABLE(of, timer_match);


/* File operations. ******************************************************************************/

static int timer_open(struct inode *inode, struct file *file) {
	while(try_module_get(THIS_MODULE) == 0) {};
	return 0;
}

static int timer_release(struct inode *inode, struct file *file) {
	module_put(THIS_MODULE);
	//What does this function do?? asynchrnorous notification
	timer_fasync(-1, file, 0);
	return 0;
}

static ssize_t timer_mmap(struct file *file, struct vm_area_struct *vma) {
	if (remap_pfn_range(	vma, vma->vm_start, vma->vm_pgoff, vma->vm_end - vma->vm_start, vma->vm_page_prot)) {
		printk(KERN_ALERT "***mmap failed...\n");
		return -EAGAIN;
	}

	return 0;
}

//This is the ioctl function
//We can use this function to read from/write to the device
static long timer_ioctl(struct file *file, unsigned int cmd, unsigned long arg) {

		
	struct timer_ioctl_data ioctl_data;

	if (copy_from_user(&ioctl_data, (void *)arg, sizeof(struct timer_ioctl_data))) {
		//Note: This is a kernal level file, so we use printk instead of printf
		printk(KERN_ALERT "***Unsuccessful transfer of ioctl argument...\n");
		return -EFAULT;
	}

	switch (cmd) {
		//TIMER_READ_REG comes from the file timer_ioctl.h
	case TIMER_READ_REG: 
		//We can use the iowrite32 function to write to the hardware
		//we can also use ioread32 to read from the hardware
		ioctl_data.data = ioread32(instance_local-> base_addr+ioctl_data.offset );
		if (copy_to_user((struct timer_ioctl_data *)arg, &ioctl_data, sizeof(struct timer_ioctl_data))) {
			printk(KERN_ALERT "***Read using ioctl unsuccessful...\n");
			return -EFAULT;
		}
		break;
   	case TIMER_WRITE_REG:
	
	iowrite32(ioctl_data.data, instance_local-> base_addr+ioctl_data.offset );
	break;

	default:
		printk(KERN_ALERT "***Invalid ioctl command...\n");
		return -EINVAL;
	}

	return 0;
	//Write to the base register assum that the value is stored at *ptr
	
	
}


/* Driver functions. *****************************************************************************/

//INITIALIZATION FUNCTION
static int __devinit timer_probe(struct platform_device *ofdev, const struct of_device_id *match) {
	int irq=0;
	//struct timer_local *instance_local = 0;
	struct miscdevice *miscdev = 0;
	struct resource *r_regs;
	int res = 0;
	const char* dev_name = DRIVER_NAME_ID(0);

	printk(KERN_ALERT "\tProbing %s...\n", match->compatible);
	printk(KERN_ALERT "\t--------------------------------------------------------------------------------\n");

	// The device instance isn't loaded.
	if (!ofdev) {
		printk(KERN_ALERT "\t***The device isn't properly loaded, cannot probe...\n");
		res = -EINVAL;
		goto error0;
	}
	
	// Get iospace for the device's register file.
	r_regs = platform_get_resource(ofdev, IORESOURCE_MEM, 0);
	if (!r_regs || ((r_regs->end - r_regs->start + 1) < (NUM_SLAVE_REGS*4))) {
		printk(KERN_ALERT "\t***Couldn't get registers resource...\n");
		res = -EFAULT;
		goto error0;
	}
	printk(KERN_ALERT "\tGetting iospace for registers resource of the device succeeded (size: %d)...\n",
						(r_regs->end - r_regs->start + 1));

	// Allocate memory for the instance.
	instance_local = (struct timer_local *)kmalloc(sizeof(struct timer_local), GFP_KERNEL);
	if (!instance_local) {
		printk(KERN_ALERT "\t***Device allocation failed...\n");
		res = -ENOMEM;
		goto error1;
	}
	memset(instance_local, 0, sizeof(struct timer_local));
	printk(KERN_ALERT "\tDevice allocation succeeded...\n");

	// Lock iospace into kernel memory for device's register file.
	instance_local->remap_size = r_regs->end - r_regs->start + 1;
	if ( (instance_local->remap_size != (r_regs->end - r_regs->start + 1)) ||
			(!request_mem_region(r_regs->start, instance_local->remap_size, DRIVER_NAME)) ) {
		printk(KERN_ALERT "\t***Locking memory region at 0x%08lX failed...\n",(unsigned long)r_regs->start);
		res = -EBUSY;
		goto error2;
	}
	printk(KERN_ALERT "\tLocking memory region 0x%08lX to 0x%08lX was successful...\n",
						(unsigned long)r_regs->start, (unsigned long) r_regs->end);

	// Get the virtual base address for the device.
	//instance_local->base_addr = (unsigned int)ioremap((unsigned long)r_regs->start, PAGE_SIZE);
	instance_local->base_addr = (unsigned int)ioremap((unsigned long)r_regs->start, instance_local->remap_size);
	if (!instance_local->base_addr) {
		printk(KERN_ALERT "\t***Remapping memory at 0x%08lX failed...\n", (unsigned long)r_regs->start);
		res = -EFAULT;
		goto error3;
	}
	printk(KERN_ALERT "\tRemapping memory at 0x%08lX to 0x%08lX succeeded...\n",
						(unsigned long)r_regs->start, (unsigned long)r_regs->end);

	// Fill in the device instance structure.
	instance_local->base_phys = r_regs->start;

	printk(KERN_ALERT "\tFilling in the device instance structure was successful...\n");



		// Allocate memory for a misc device.
		miscdev = (struct miscdevice *)kzalloc(sizeof(struct miscdevice), GFP_KERNEL);
		if (!miscdev) {
			printk(KERN_ALERT "\t\t***Couldn't allocate device private record...\n");
			res  = -ENOMEM;
			goto error3;
		}
		printk(KERN_ALERT "\t\tAllocated device private record...\n");

		// Fill in misc device info.
		miscdev->minor = TIMER_MINOR; //HINT LOOK IN .h  FILE
		miscdev->name = dev_name;
		miscdev->nodename = dev_name;
		miscdev->fops = &timer_fops;

		// Register the device as misc device.
		res = misc_register(miscdev);
		if (res != 0) {
			printk(KERN_ALERT "\t\t***Could not register miscdev...\n");
			goto error3;
		}

		// Complete registration.
		printk(KERN_ALERT "\t\tCompleting registration...\n");
		instance_local->miscdev = miscdev;

		printk(KERN_ALERT "\tRegistration successful...\n");

	// Device registration information.
///*
	printk(KERN_ALERT  "Requesting IRQ\n");
	int result;
	//What is this function doing? // asking for irq from the system
	result = request_irq(irq, (void*)timer_interrupt, 0, "timer", miscdev);

	if (result) {
	     printk(KERN_ALERT "Timer: can't get assigned irq %i\n",irq); //TIMER_INTERRUPT_NUM
	     return -1;
	}else{
		printk(KERN_ALERT "Timer: Assigned irq number %i\n",irq); //TIMER_INTERRUPT_NUM
	}	


	// Device registration information.

	printk(KERN_ALERT "----------------------------------------------------------------------------------------\n");
	printk(KERN_ALERT "Name:\t\t%s\n", miscdev->name);
	printk(KERN_ALERT "Major:\t\t%d\n", MISC_MAJOR);
	printk(KERN_ALERT "Minor:\t\t%d\n", miscdev->minor);
	printk(KERN_ALERT "Physical base address (of IP core):\n\t\t\t0x%08lX\n",
					(unsigned long) instance_local->base_phys);
	printk(KERN_ALERT "Size of physical memory: \n\t\t\t%luK\n",
					((unsigned long) instance_local->remap_size)/1024);
	printk(KERN_ALERT "Virtual base address (in kernel memory):\n\t\t\t0x%08lX\n",
					(unsigned long) instance_local->base_addr);
	printk(KERN_ALERT "----------------------------------------------------------------------------------------\n");

	printk(KERN_ALERT "-->Finished probing device...\n");

	
	return 0;

error3: // Undo 'ioremap()'.
	printk(KERN_ALERT "Unmapping memory...\n");
	iounmap((void *)(instance_local->base_addr));
error2: // Undo 'request_mem_region'.
	printk(KERN_ALERT "Releasing iospace for registers resource...\n");
	release_mem_region(r_regs->start, instance_local->remap_size);
error1: // Undo 'k*alloc()'.
	printk(KERN_ALERT "Freeing allocated memory...\n");
	kfree(instance_local);
error0: // Exit probe for current device.
	printk(KERN_ALERT "***Probing failed...\n");
	return res;

}

static int __devexit timer_remove(struct platform_device *ofdev) {

	//struct timer_local *instance_local = 0;
	int res = 0;

//	printk(KERN_ALERT "\tRemoving %s (id #%d)...\n", ofdev->dev.of_node->name, dev_id-1);
//	printk(KERN_ALERT "\t--------------------------------------------------------------------------------\n");

	// The device instance isn't loaded - can't use 'rmmod' on it.
	if (!ofdev) {
		printk(KERN_ALERT "\t\t***The device instance isn't loaded properly...\n");
		res = -EINVAL;
		goto error0;
	}
	

	// Device instance information.
/*	printk(KERN_ALERT "----------------------------------------------------------------------------------------\n");
	printk(KERN_ALERT "Name:\t\t%s\n", instance_local->miscdev->name);
	printk(KERN_ALERT "Major:\t\t%d\n", MISC_MAJOR);
	printk(KERN_ALERT "Minor:\t\t%d\n", instance_local->miscdev->minor);
	printk(KERN_ALERT "Device ID:\t%d\n", instance_local->device_id);
	printk(KERN_ALERT "In use?:\t%d\n", instance_local->is_inuse);
	printk(KERN_ALERT "----------------------------------------------------------------------------------------\n");
*/

//		printk(KERN_ALERT "\tUnregistering the device...\n");

		// Unregister the misc device.
		res = misc_deregister(instance_local->miscdev);
		if (res != 0) {
			printk(KERN_ALERT "\t\t***Could not unregister miscdev...\n");
			goto error0;
		}

		// Complete unregistration.
//		printk(KERN_ALERT "\t\tCompleting unregistration...\n");


//		printk(KERN_ALERT "\tUnregistration successful...\n");


	// Undo probe's 'ioremap()', 'request_mem_region()', and 'k*alloc()'.
//	printk(KERN_ALERT "\tUndoing probe's memory management...\n");
//	printk(KERN_ALERT "\t\tUnmapping memory...\n");
	iounmap((void *)(instance_local->base_addr));
//	printk(KERN_ALERT "\t\tReleasing iospace for registers resource...\n");
	release_mem_region(instance_local->base_phys, instance_local->remap_size);
//	printk(KERN_ALERT "\t\tFreeing allocated memory...\n");
	kfree(instance_local);

//	printk(KERN_ALERT "-->Finished removing device...\n");
	return 0;

error0:	printk(KERN_ALERT "***Removing instance failed...\n");
	return res;

}

static int __init timer_init(void) {

	int res;

//	printk(KERN_ALERT "Loading module... **********************************************************************\n");

	res = of_register_platform_driver(&timer_driver);

	if (res != 0) printk(KERN_ALERT
		"***Loading failed... *******************************************************************\n\n");
//	else printk(KERN_ALERT
//		"Loading was successful... **************************************************************\n\n");

	return res;
}

static void __exit timer_exit(void) {

//	printk(KERN_ALERT "Unloading module... ********************************************************************\n");

	of_unregister_platform_driver(&timer_driver);

//	printk(KERN_ALERT "Unloading module finished... ***********************************************************\n");

}

module_init(timer_init);
module_exit(timer_exit);

//Timer ISR 
static irqreturn_t timer_interrupt(int irq, void *dev_id, struct pt_regs *regs)
{
	//When is this function called?
	
	//DEBUG_PRINT(KERN_ALERT "timer_interrupt()\n");
	
	//Do any other stuff you need to here

	// Clears interrupt flag	
	struct timer_ioctl_data ioctl_data;
	//ioctl_data.data = ioread32(instance_local-> base_addr); 
	//iowrite32(ioctl_data.data, instance_local-> base_addr);
	
	//Send message to any connected user apps//when data arrives the statement must be executed to signal asychrnous readers
	if (async_queue)
    		kill_fasync(&async_queue, SIGUSR1, POLL_IN); //used to signal the interest processes when data arrives.
	return IRQ_HANDLED;  
}

static int timer_fasync(int fd, struct file *filp, int mode)
{
    return fasync_helper(fd, filp, mode, &async_queue);
}

