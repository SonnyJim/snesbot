
#include <wiringPi.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

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

// 	WiringPi	 	P1 Pin
#define B_Pin		 8 	// 3
#define Y_Pin		 9 	// 5
#define Select_Pin	 7 	// 7
#define Start_Pin	 15 	// 8
#define Up_Pin		 16 	// 10
#define Down_Pin	 0 	// 11
#define Left_Pin	 1 	// 12
#define Right_Pin	 2 	// 13
#define A_Pin		 3 	// 15
#define X_Pin		 4 	// 16
#define TLeft_Pin	 5 	// 18
#define TRight_Pin	 12 	// 19

#define Latch_Pin	 13 	// 21

int enable_interrupts = 0;


void sig_handler (int signo)
{
	if (signo == SIGINT)
	{
		printf ("Received SIGINT\n");
		//shutdown_gpio ();
		exit (signo);
	}
}

void clear_buttons (void)
{
	//Set all buttons to off ie HIGH
	digitalWrite (B_Pin, HIGH);
	digitalWrite (Y_Pin, HIGH);
	digitalWrite (Select_Pin, HIGH);
	digitalWrite (Start_Pin, LOW);
	digitalWrite (Up_Pin, HIGH);
	digitalWrite (Down_Pin, HIGH);
	digitalWrite (Left_Pin, HIGH);
	digitalWrite (Right_Pin, HIGH);
	digitalWrite (A_Pin, HIGH);
	digitalWrite (X_Pin, HIGH);
	digitalWrite (TLeft_Pin, HIGH);
	digitalWrite (TRight_Pin, HIGH);
}

int init_gpio (void)
{
	if (wiringPiSetup () == -1)
		return 1;
	
	//Set up pins
	pinMode (Latch_Pin, INPUT);
	
	pinMode (B_Pin, OUTPUT);
	pinMode (Y_Pin, OUTPUT);
	pinMode (Select_Pin, OUTPUT);
	pinMode (Start_Pin, OUTPUT);
	pinMode (Up_Pin, OUTPUT);
	pinMode (Down_Pin, OUTPUT);
	pinMode (Left_Pin, OUTPUT);
	pinMode (Right_Pin, OUTPUT);
	pinMode (A_Pin, OUTPUT);
	pinMode (X_Pin, OUTPUT);
	pinMode (TLeft_Pin, OUTPUT);
	pinMode (TRight_Pin, OUTPUT);
	
	clear_buttons ();
	return 0;
}



void load_sr (void)
{
	clear_buttons ();
}

void latch_interrupt (void)
{
	if (enable_interrupts)
	{
		// 16 x 12us (192us) for SNES to read SR's
		delayMicroseconds (206);
		// Load up the SR's with data
		load_sr ();
	}
}

void setup_interrupts (void)
{
	wiringPiISR (Latch_Pin, INT_EDGE_RISING, &latch_interrupt);
}

void snesbot (void)
{
	setup_interrupts ();
	enable_interrupts = 1;
	for (;;)
	{
		signal (SIGINT, sig_handler);
	}
}

int main (int argc, char *argv[])
{
	// Set priority
	piHiPri (10); sleep (1);
	int show_usage = 0;
	
	printf("SNESBot v3\n");
	while ((argc > 1) && (argv[1][0] == '-' ))
	{
		switch (argv[1][1])
			{
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
		printf("-h	show this help\n");
		return 0;
	}
	
	printf("Initialising GPIO\n");
	if (init_gpio ())
	{
		printf ("Error initilising GPIO\n");
		return 1;
	}
	printf("Entering main loop\n");
	//Main loop
	snesbot ();
	return 0;
}
