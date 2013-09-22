
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

//Using wiringPi numbering
#define clockPin 9
#define latchPin 7
#define dataPin 12
//RetroPie GPIO adapter button 
#define RtPie_Button 0

unsigned int buttons;

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

	printf("Waiting for RetroPie adapter button\n");
	while (digitalRead (RtPie_Button) == 0)
		delay (200);

	printf("Entering main loop\n");
	//Main loop
	for (;;)
	{
		i = 0;
		latched = 0;
		digitalWrite (dataPin, 1);
		
		//Wait for latch, should be every 16.67ms, 12us long
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
			{
				clocked = digitalRead (clockPin);
			//	delayMicroseconds (10);
			}
			delayMicroseconds (10);
			//Clock out data
			//Start button
			if (i == 4)
				digitalWrite (dataPin, 0);
			//Last 4 bits should always be high
			if (i > 12)
			{
				digitalWrite (dataPin, 1);
			}
			clocked = 1;
			//delayMicroseconds (10);
		}
		//Random wait TODO
	}
	return 0;
}


