/*
 * hwthread_ioctl.h
 *
 * IOCTL constants for the HWThread device driver.
 *
 * Author: Aws Ismail and Andrei-Liviu Simion
 *
 * Revision History:
 *	Feb 18 2011 - Initial version.
 *	July 22 2011 - Added extra IOCTL definitions (4-14).
 */

#ifndef _TIMER_IOCTL_H
#define _TIMER_IOCTL_H

#include <linux/ioctl.h>
#include <linux/types.h>

//These should be unique for each of your Drivers
#define TIMER_IOCTL_BASE 'T'
#define TIMER_MINOR 60 

struct timer_ioctl_data {
	__u32 offset;
	__u32 data;
};

// READ/WRITE ops
#define TIMER_READ_REG  _IOR(TIMER_IOCTL_BASE, 0, struct timer_ioctl_data)
#define TIMER_WRITE_REG _IOW(TIMER_IOCTL_BASE, 1, struct timer_ioctl_data)

#endif

