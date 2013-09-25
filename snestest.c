
#include <wiringPi.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

//Using wiringPi numbering
#define clockPin 9
#define latchPin 7
#define dataPin 12

float clocked = 0;
float latched = 0;
int interrupts_enabled = 0;

int init_gpio (void)
{
	if (wiringPiSetup () == -1)
		return 1;
	//Set up pins
	pinMode (clockPin, INPUT);
	pinMode (latchPin, INPUT);
	
	pinMode (dataPin, OUTPUT);
	digitalWrite (dataPin, 0);
	return 0;
}

void latchPin_interrupt (void)
{
	if (interrupts_enabled)
		latched++;
}



void clockPin_interrupt (void)
{
	if (interrupts_enabled)
		clocked++;
}

void setup_interrupts (void)
{
	wiringPiISR (latchPin, INT_EDGE_RISING, &latchPin_interrupt);
	wiringPiISR (clockPin, INT_EDGE_RISING, &clockPin_interrupt);
}
void read_interrupts (void)
{
	float calc;
	int i;
	int pass = 1;
	
	interrupts_enabled = 0;
	setup_interrupts ();
	
	// Run tests 5 times
	while (pass <= 5)
	{
		printf ("=================================\n");
		printf ("        Pass %i of 5\n", pass);
		printf ("=================================\n");
		for (i = 0;i < 2; i++)
		{
			digitalWrite (dataPin, i);
			
			if (i)
				printf ("Data line is HIGH\n");
			else
				printf ("Data line is LOW\n");
			
			printf ("Reading interrupts for 10 seconds\n\n");
			latched = 0;
			clocked = 0;
			interrupts_enabled = 1;
			delay (10000);	
			interrupts_enabled = 0;
	
			//Calculate and print results
			printf ("Total Latches: %f\n", latched);
			printf ("Total Clocks: %f\n", clocked);
			calc = 10000 / latched;
			printf ("Latches/sec: %f\n", calc);
			calc = 10000 / clocked;
			printf ("Clocks/sec: %f\n", calc);
			calc = clocked / latched;
			printf ("Clocks/latch: %f\n\n\n", calc);
		}
		pass++;
	}
}
int main (void)
{
	// Set priority
	piHiPri (10); sleep (1);
	printf("=============================\n");	
	printf(" SNES Controller Timing Test\n");
	printf("=============================\n");	
	printf("Initialising GPIO\n");
	if (init_gpio ())
	{
		printf ("Error initilising GPIO\n");
		return 1;
	}
	
	//Wait for first latch
	printf ("Waiting for first latch\n");
	while (digitalRead (latchPin) == 0)
		delayMicroseconds (10);
	printf ("Got latch! Waiting 10 seconds\n\n\n");
	//Wait for game to settle
	sleep (10);
	read_interrupts ();
	return 0;
}
