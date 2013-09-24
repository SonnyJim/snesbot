
#include <wiringPi.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

/* SNES Controller button to clock
   Clock	Button
   0		B
   1		Y
   2		Select
   3		Start
   4		Up
   5		Down
   6		Left
   7		Right
   8		A
   9		X
   10		L
   11		R
   12		Always High
   13		Always High
   14		Always High
   15		Always High
*/

//Using wiringPi numbering
#define clockPin 9
#define latchPin 7
#define dataPin 12
//RetroPie GPIO adapter button 
#define RtPie_Button 0

int clockBit = 0;
int latched = 0;

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

void latchPin_interrupt (void)
{
}

void clockPin_interrupt (void)
{
	if (latched)
	{
		if (clockBit == 3)
			digitalWrite (dataPin, 0);
		//Last 4 bits should always be high
		else if (clockBit > 11)
			digitalWrite (dataPin, 1);

		if (++clockBit > 15)
		{
			clockBit = 0;
			latched = 0;	
		}
	}
}

void setup_interrupts (void)
{
//	wiringPiISR (latchPin, INT_EDGE_FALLING, &latchPin_interrupt);
	wiringPiISR (clockPin, INT_EDGE_RISING, &clockPin_interrupt);
}

void snesbot (void)
{
	// Set priority
	piHiPri (10);
	for (;;)
	{
		// data line should be normally low
		digitalWrite (dataPin, 0);
		
		//Wait for latch pulse, should be every 16.67ms, 12us long
		while (digitalRead (latchPin) == 0);
		
		latched = 1;	
		
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
				
				default:
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
	setup_interrupts ();
	snesbot ();
	return 0;
}
