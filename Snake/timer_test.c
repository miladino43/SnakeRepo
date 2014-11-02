/*
*Filename: 		timer_test.c
*Date Update:		November 5, 2012
*/

#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <linux/fs.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include "../../user-modules/timer_driver/timer_ioctl.h"

static void timer_sleep( int sleeptime );

volatile struct timer_ioctl_data TCSR0;	// Control regs
//struct timer_ioctl_data TCSR1;	// Control regs
volatile struct timer_ioctl_data TLR0;	// Load reg
volatile struct timer_ioctl_data TCR0;	// Counter reg

int fd;
int interval=1;
volatile int timer_flag;


//TIMER ISR
void timer_intr(void){
	//PUT ISR CODE HERE
	int mask;
	
	ioctl(fd, TIMER_READ_REG, &TCSR0);	
	mask = TCSR0.data & 0x100;
	printf("Start of interrupt: TCSR0 = %x, %x \n", TCSR0.data, TCSR0.offset);

	if (mask == 0x100){
	TCSR0.data = TCSR0.data | 0x100;
	ioctl(fd, TIMER_WRITE_REG, &TCSR0);
				
	printf("INTERRUPT\n");
	
	
	ioctl(fd, TIMER_READ_REG, &TCSR0);
	printf("End of interrupt: TCSR0 = %x \n", TCSR0.data);
			
	fflush(stdout);	 
	timer_flag = 1;
	}

}

/*
int main(void)
{

	int oflags;
	int i = 0;

	// Offsets from Register Address Map in Data Sheet
	TCSR0.offset=0x00;
	TLR0.offset=0x04;
	TCR0.offset=0x08;

	fd = open("/dev/timer_driver",O_RDWR); 
	printf("we are alive\n");
	fflush(stdout);	

	//Test to make sure the file opened correctly
	if(fd != -1)
	{
	
		//Set up Asynchronous Notification
		signal(SIGIO,timer_intr); //calls the timer_intr
		fcntl(fd, F_SETOWN, getpid()); //set process as the owner of the file
		oflags = fcntl(fd, F_GETFL);
		fcntl(fd, F_SETFL,oflags|FASYNC); //set the FASYNC flag in the device to enable async notification		
		
		// Set load reg with interval=3 seconds 
		printf("Setting load register...\n");
		fflush(stdout);	
		TLR0.data=interval*75000000-2; // 75MHz
		ioctl(fd,TIMER_WRITE_REG,&TLR0); 
		printf("TLR0 = %x \n", TLR0.data);
		
		// Load the value in TLR0 to timer
		printf("load the value to timer\n");
		fflush(stdout);	
		TCSR0.data=0x20;
		ioctl(fd,TIMER_WRITE_REG,&TCSR0);
		printf("TCSR0 = %x \n", TCSR0.data);
		
		// Enable timer
		printf("enable\n");
		fflush(stdout);	

		// Enable timer0,enable timer to give interrupt, auto-reload, enable external generate signal, and set to count down
		TCSR0.data=0xD6;
		ioctl(fd,TIMER_WRITE_REG,&TCSR0);
		printf("TCSR0 = %x \n", TCSR0.data);

		for (i=0; i<10; i++)
		{
			sleep(1);
			timer_flag = 1;
//			printf("outside interrupt flag = %d \n", timer_flag);			
			ioctl(fd, TIMER_READ_REG, &TCR0);
			printf("Time = %d \n", TCR0.data/75000000);
			ioctl(fd, TIMER_READ_REG, &TCSR0);
			printf("Outside interrupt: TCSR0 = %x \n", TCSR0.data);
		
		}

	}TCSR0.data
		

	else
	{
		printf("Failed to open driver... :( \n");
	}
		
	return 0;
}
*/

int main(void)
{

	int oflags;
	int i = 0;

	// Offsets from Register Address Map in Data Sheet
	TCSR0.offset=0x00;
	TLR0.offset=0x04;
	TCR0.offset=0x08;


	fd = open("/dev/timer_driver",O_RDWR); 

	//Test to make sure the file opened correctly
	if(fd != -1){
		//Set up Asynchronous Notification
		signal(SIGIO,timer_intr); //calls the timer_intr
		fcntl(fd, F_SETOWN, getpid()); //set process as the owner of the file
		oflags = fcntl(fd, F_GETFL);
		fcntl(fd, F_SETFL,oflags|FASYNC); //set the FASYNC flag in the device to enable async notification		
	}
	else
		printf("Failed to open timer driver... :( \n");

	for (i = 0; i < 10; i++){
		printf("in loop number %d\n",i);
		timer_sleep(1);
		printf("out of loop %d\n", i);
	}
	
	return 0;
}


static void timer_sleep( int sleeptime ){
	

	timer_flag = 0;

//	timer_fd = open("/dev/timer_driver",O_RDWR); 

	//Test to make sure the file opened correctly
	//if(timer_fd != -1)
	//{
		
		// Set load reg with interval=3 seconds 
		TLR0.data=sleeptime*75000000-2; // 75MHz
		ioctl(fd,TIMER_WRITE_REG,&TLR0); 
		
		// Load the value in TLR0 to timer
		TCSR0.data=0x20;
		ioctl(fd,TIMER_WRITE_REG,&TCSR0);

		// Enable timer0,enable timer to give interrupt, auto-reload, enable external generate signal, and set to count down
		TCSR0.data=0xC6;
		ioctl(fd,TIMER_WRITE_REG,&TCSR0);
//	}
//	else
//	{
//		fprintf(gamelog,"Failed to open timer driver... :( \n");
//	}

	//fprintf(gamelog,"Sleeping for 1 second...\n");

	while (timer_flag == 0 ) {}
}


