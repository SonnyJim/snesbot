
#include <wiringPi.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

//Use these later on
#define SNES_RIGHT       0x01
#define SNES_LEFT        0x02
#define SNES_DOWN        0x04
#define SNES_UP          0x08
#define SNES_START       0x10
#define SNES_SELECT      0x20
#define SNES_B           0x40
#define SNES_A           0x80

//Using wiringPi numbering
#define clockPin 9
#define latchPin 7
#define dataPin 12
//RetroPie GPIO adapter button 
#define RtPie_Button 0

unsigned int buttons;

void sig_handler (int signo)
{
	if (signo == SIGINT)
	{
		printf ("Received SIGINT\n");
		//shutdown_gpio ();
		digitalWrite (dataPin, 1);
		exit (signo);
	}
}

int init_gpio (void)
{
	if (wiringPiSetup () == -1)
		return 1;
	//Set up pins
	pinMode (clockPin, INPUT);
	pinMode (latchPin, INPUT);
	pinMode (RtPie_Button, INPUT);
	
	pinMode (dataPin, OUTPUT);
	digitalWrite (dataPin, 1);
	return 0;
}

void snesbot (void)
{
	// Set priority
	piHiPri (10);
	int i;
	for (;;)
	{
		digitalWrite (dataPin, 0);
		//Wait for latch pulse, should be every 16.67ms, 12us long
		while (digitalRead (latchPin) == 0);
			
		for (i = 0; i < 16; i++)
		{
			//
			digitalWrite (dataPin, 1);
			//Wait for falling edge of Clock
			while (digitalRead (clockPin) == 1);
			
			//delayMicroseconds (6);
			
			//Clock out data
			//Start button
			if (i == 3)
				digitalWrite (dataPin, 0);
			//Last 4 bits should always be high
			else if (i > 11)
				digitalWrite (dataPin, 1);
			delayMicroseconds (6);
		}
		signal (SIGINT, sig_handler);

	}


}

int main (int argc, char *argv[])
{
	int wait_for_rtpie_button = 0;
	int show_usage = 0;
	
	printf("SNESBot\n");
	while ((argc > 1) && (argv[1][0] == '-' ))
	{
		switch (argv[1][1])
			{
				case 'b':
				wait_for_rtpie_button = 1;
				break;

				case 'h':
				show_usage = 1;
				break;
			}

		++argv;
		--argc;
	}
	
	if (show_usage)
	{
		printf("Options:\n");
		printf("-b	Wait for button press on Retropie adapter\n");
		printf("-h	show this help\n");
		return 0;
	}
	
	printf("Initialising GPIO\n");
	if (init_gpio ())
	{
		printf ("Error initilising GPIO\n");
		return 1;
	}


	if (wait_for_rtpie_button)
	{
		printf("Waiting for RetroPie adapter button\n");
		while (digitalRead (RtPie_Button) == 0)
			delay (200);
	}
	
	printf("Entering main loop\n");
	//Main loop
	snesbot ();
	return 0;
}


