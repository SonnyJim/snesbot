/*
 * piSnes.c:
 *	Driver for the SNES Joystick controller on the Raspberry Pi
 *	Copyright (c) 2012 Gordon Henderson,
 ***********************************************************************
 * This file is part of wiringPi:
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

#include "piSnes.h"

#define	MAX_SNES_JOYSTICKS	8

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
