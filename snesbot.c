
#include <wiringPi.h>
#include <stdio.h>

#define SNES_RIGHT       0x01
#define SNES_LEFT        0x02
#define SNES_DOWN        0x04
#define SNES_UP          0x08
#define SNES_START       0x10
#define SNES_SELECT      0x20
#define SNES_B           0x40
#define SNES_A           0x80

#define clockPin 9
#define latchPin 7
#define dataPin 12

unsigned int buttons;

int init_gpio (void)
{
	if (wiringPiSetup () == -1)
		return 1;
	//Set up pins
	pinMode (clockPin, INPUT);
	pinMode (latchPin, INPUT);
	pinMode (dataPin, OUTPUT);
	return 0;
}

int main (void)
{
	int i;
	int latched = 0;
	int clocked = 1;

	printf("SNESBot\n");
	printf("Initialising GPIO\n");
	if (init_gpio ())
	{
		printf ("Error initilising GPIO\n");
		return 1;
	}

	printf("Entering main loop\n");
	//Main loop
	for (;;)
	{
		//Wait for latch, should be every 16.67ms
//		printf("Waiting for latch\n");
		while (latched == 0)
		{
			latched = digitalRead (latchPin);
			delayMicroseconds (12);
		}

		//Start clocking 16 bits of data after falling edge of latch
		while (i++ < 16)
		{
			//Wait for falling edge of Clock
			while (clocked == 1)
				clocked = digitalRead (clockPin);
			//Clock out data
			if (i == 4)
				digitalWrite (dataPin, 1);

		}
		i = 0;
		latched = 0;
		clocked = 1;
		//delayMicroseconds (6);
	}
	return 0;
}


