/*
 * piSnes.c:
 *	Driver for the SNES Joystick controller on the Raspberry Pi
 *	Heavily based on code from WiringPi
 *	Copyright (c) 2012 Gordon Henderson,
 *	Copyright (c) 2016 Ewan Meadows
 ***********************************************************************
 * This file was part of wiringPi:
 *	https://projects.drogon.net/raspberry-pi/wiringpi/
 *
 *    wiringPi is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU Lesser General Public License as
 *    published by the Free Software Foundation, either version 3 of the
 *    License, or (at your option) any later version.
 *
 *    wiringPi is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU Lesser General Public License for more details.
 *
 *    You should have received a copy of the GNU Lesser General Public
 *    License along with wiringPi.
 *    If not, see <http://www.gnu.org/licenses/>.
 ***********************************************************************
 */

#include <wiringPi.h>
#include "snes.h"
#include "joystick.h"

#define	MAX_SNES_JOYSTICKS	8

//Set this to as quick as possible, we don't want to hang around waiting to read in the joysticks
#define	PULSE_TIME	5

// Data to store the pins for each controller

struct snesPinsStruct
{
  unsigned int cPin, dPin, lPin ;
} ;

static struct snesPinsStruct snesPins [MAX_SNES_JOYSTICKS] ;

static int joysticks = 0 ;


/*
 * detectSnesJoystick:
 *      A SNES joystick should always return bits 14, 15 and 16 high
 *********************************************************************************
 */
int detectSnesJoystick (int joystick)
{
  int i;
  struct snesPinsStruct *pins = &snesPins [joystick] ;
  
  if (joystick > MAX_SNES_JOYSTICKS || joystick == -1)
  {
    return 1;
  }
  //Latch the controller
  digitalWrite (pins->lPin, LOW)  ; delayMicroseconds (PULSE_TIME) ;

  //Now get the next 15 bits with the clock
  for (i = 0 ; i < 15 ; ++i)
  {
    digitalWrite (pins->cPin, HIGH) ; delayMicroseconds (PULSE_TIME) ;
    digitalWrite (pins->cPin, LOW)  ; delayMicroseconds (PULSE_TIME) ;
    if (i >= 13)
    {
      if (digitalRead (pins->dPin) != 1)
      {
        digitalWrite (pins->lPin, HIGH) ; delayMicroseconds (PULSE_TIME) ;
        return 1;
      }
    }
  }
  //Set the latch high again
  digitalWrite (pins->lPin, HIGH) ; delayMicroseconds (PULSE_TIME) ;
  return 0;
};

/*
 * setupNesJoystick:
 *	Create a new SNES joystick interface, program the pins, etc.
 *********************************************************************************
 */

int setupSnesJoystick (int dPin, int cPin, int lPin)
{
  if (joysticks == MAX_SNES_JOYSTICKS)
    return -1 ;

  snesPins [joysticks].dPin = dPin ;
  snesPins [joysticks].cPin = cPin ;
  snesPins [joysticks].lPin = lPin ;

  digitalWrite (lPin, LOW) ;
  digitalWrite (cPin, LOW) ;

  pinMode (lPin, OUTPUT) ;
  pinMode (cPin, OUTPUT) ;
  pinMode (dPin, INPUT) ;

  return joysticks++ ;
}


/*
 * readSnesJoystick:
 *	Do a single scan of the NES Joystick.
 *********************************************************************************
 */

unsigned short int readSnesJoystick (int joystick)
{
  unsigned short int value = 0 ;
  int  i ;

  struct snesPinsStruct *pins = &snesPins [joystick] ;
 
// Toggle Latch - which presents the first bit

  digitalWrite (pins->lPin, LOW)  ; delayMicroseconds (PULSE_TIME) ;

// Read first bit

  value = digitalRead (pins->dPin) ;

// Now get the next 15 bits with the clock

  for (i = 0 ; i < 15 ; ++i)
  {
    digitalWrite (pins->cPin, HIGH) ; delayMicroseconds (PULSE_TIME) ;
    digitalWrite (pins->cPin, LOW)  ; delayMicroseconds (PULSE_TIME) ;
    value = (value << 1) | digitalRead (pins->dPin) ;
  }
  digitalWrite (pins->lPin, HIGH) ; delayMicroseconds (PULSE_TIME) ;

  return value ^ 0xFFFF;
}

int joystick_setup (void)
{
  p1.pisnes_num = setupSnesJoystick (PIN_P1DAT, PIN_P1CLK, PIN_P1LAT);
  if (p1.pisnes_num == -1)
  {
    fprintf (stdout, "Error in setupSnesJoystick\n");
    return 1;
  }
  
  if (detectSnesJoystick (p1.pisnes_num) != 0)
  {
    fprintf (stdout, "Didn't detect a SNES joystick in port 1\n");
    return 1;
  }

  if (p1.pisnes_num == -1)
  {
    fprintf (stdout, "Unable to setup input\n") ;
    return 1 ;
  }

  p1.input = 0x0000;
  p1.input_old = 0x0000;
  return 0;
}

void check_player_inputs (void)
{
  if ((p1.input & SNES_L) && (p1.input & SNES_R))
    fprintf (stdout, "LR PRESSED\n\n\n\n");
}

void read_player_inputs (void)
{
    p1.input = readSnesJoystick (p1.pisnes_num) ;
    //check_player_inputs();
}


unsigned short int readUSBJoystick (void)
{
    size_t js_read;

    js_read = fread (&p1.ev, 1, sizeof(struct js_event), p1.fp);
    if (js_read < sizeof(struct js_event))
    {
        fprintf (stderr, "Didn't read back a whole js_event?\n");
    }
}

FILE* open_joystick_dev (char* device)
{
    FILE* js_dev;

	js_dev = fopen (device, "r");

	if (js_dev == NULL)
		return -1;
	else
		return js_dev;
}

int setupUSBJoystick (void)
{
    p1.fp = open_joystick_dev ("/dev/input/js0");

    if (p1.fp == -1)
    {
        fprintf (stderr, "Error reading USB input device\n");
    }
    
}

unsigned short int process_ev_buttons (struct js_event)
{

}

//Coverts the USB joystick ev data in a short int
unsigned short int process_ev (struct js_event ev)
{
    unsigned short int out = 0;
	// Axis/Type 2 == dpad
	if (ev.type == JS_EVENT_AXIS)
	{
		switch (ev.number)
		{
			// X axis
			case p1.mapping.x_axis:
				if (ev.value > 0)
				{
                    out |= SNES_RIGHT;
				}
				else if (ev.value < 0)
				{
                    out |= SNES_LEFT;
				}
				break;
			//Y Axis
			case p1.mapping.y_axis:
				if (ev.value > 0)
				{
                    out |= SNES_UP;
				}
				else if (ev.value < 0)
				{
                    out |= SNES_DOWN;
				}
				break;
			default:
				break;
			}
	}
	// Axis/Type 1 == Buttons
	if (ev.type == JS_EVENT_BUTTON)
	{
		// 1 = ON/GPIO LOW
		if (ev.value == 1)
		{
			digitalWrite (psx_mapping[ev.number], LOW);
		}
		// 0 = OFF/GPIO HIGH
		else if (ev.value == 0)
		{
			digitalWrite (psx_mapping[ev.number], HIGH);
		}
	}
    return out;
}


