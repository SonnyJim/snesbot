
#include <wiringPi.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

//                      WiringPi        P1 Pin
#define B_Pin            8              // 3
#define Y_Pin            9              // 5
#define Select_Pin       7              // 7
#define Start_Pin        15             // 8
#define Up_Pin           16             // 10
#define Down_Pin         0              // 11
#define Left_Pin         1              // 12
#define Right_Pin        2              // 13
#define A_Pin            3              // 15
#define X_Pin            4              // 16
#define TLeft_Pin        5              // 18
#define TRight_Pin       12             // 19

#define latchPin 13

float latched = 0;
int interrupts_enabled = 0;

void clear_buttons (void)
{
        //Set all buttons to off ie HIGH
        digitalWrite (B_Pin, HIGH);
        digitalWrite (Y_Pin, HIGH);
        digitalWrite (Select_Pin, HIGH);
        digitalWrite (Start_Pin, HIGH);
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
	pinMode (latchPin, INPUT);
	
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

void latchPin_interrupt (void)
{
	if (interrupts_enabled)
		latched++;
}

void setup_interrupts (void)
{
	wiringPiISR (latchPin, INT_EDGE_RISING, &latchPin_interrupt);
}
void read_interrupts (void)
{
	float calc;
	int pass = 1;
	
	interrupts_enabled = 0;
	setup_interrupts ();
	
	// Run tests 5 times
	while (pass <= 5)
	{
		printf ("=================================\n");
		printf ("        Pass %i of 5\n", pass);
		printf ("=================================\n");
		printf ("Reading interrupts for 10 seconds\n\n");
		latched = 0;
		interrupts_enabled = 1;
		delay (10000);	
		interrupts_enabled = 0;
	
		//Calculate and print results
		printf ("Total Latches: %f\n", latched);
		calc = 10000 / latched;
		printf ("Latches/sec: %f\n", calc);
		pass++;
	}
}
int main (void)
{
	// Set priority
	piHiPri (100); sleep (1);
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
