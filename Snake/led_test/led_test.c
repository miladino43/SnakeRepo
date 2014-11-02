/*
Filename: 		led_test.c
Author: 		Kevan Thompson
Date Created: 	August 28, 2012
Date Update:	August 30, 2012
 */
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include "../../user-modules/led_driver/led_ioctl.h"


struct led_ioctl_data data;
unsigned long value;
int fd;

int main(void)
{
	
	
	volatile unsigned *ptr;
	char val [5];
	val[5] = '\0'; 
	int oflags;
	data.offset = 0;

	printf("Starting LED Test!\n"); 
	fd = open("/dev/led_driver",O_RDWR); 
	//Test to make sure the file opened correctly
	if(fd != -1){
	
		//Set  GIOP as Output
		//0's = Output, 1's = Input
		value = 0;
		data.offset = 4;
		data.data = value;
		ioctl(fd, LED_WRITE_REG, &data);
		//Turn on all the LEDS
		//1 = on, 0 = off
		data.offset = 0;
		//data.data = 0xFF;

		int i;
		for (i=1; i<256; i++)
		{
			data.data = i;
			ioctl(fd, LED_WRITE_REG, &data);
			sleep(1);
			
			
		}			


		
	}else{
		printf("Failed to open driver... :( \n");
	}
	
	printf("Ending LED Test!\n"); 
	return 0;
}


