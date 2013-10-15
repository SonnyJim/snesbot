/*
##############################################################################
#	SNESBot - Pi controlled SNES Bot
#	https://github.com/sonnyjim/snesbot/
#
#	Copyright (c) 2013 Ewan Meadows
##############################################################################
# This file is part of SNESBot:
#
#    SNESBot is free software: you can redistribute it and/or modify
#    it under the terms of the GNU Lesser General Public License as published by
#    the Free Software Foundation, either version 3 of the License, or
#    (at your option) any later version.
#
#    SNESBot is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU Lesser General Public License for more details.
#
#    You should have received a copy of the GNU Lesser General Public License
#    along with SNESBot.  If not, see <http://www.gnu.org/licenses/>.
##############################################################################
*/

#include "snesbot.h"


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
	pinMode (Latch_Pin, INPUT);
	pullUpDnControl (Latch_Pin, PUD_UP);
	
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

void Latch_Pin_interrupt (void)
{
	if (interrupts_enabled)
		latched++;
}

void setup_interrupts (void)
{
	wiringPiISR (Latch_Pin, INT_EDGE_RISING, &Latch_Pin_interrupt);
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
		calc = latched / 100;
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
	while (digitalRead (Latch_Pin) == 0)
		delayMicroseconds (10);
	//printf ("Got latch! Waiting 10 seconds\n\n\n");
	//Wait for game to settle
	//sleep (10);
	read_interrupts ();
	return 0;
}
