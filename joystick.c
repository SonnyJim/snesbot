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


static void readSnesJoystick (struct player_t* player)
{
  unsigned short int value = 0 ;
  int  i ;

  struct snesPinsStruct *pins = &snesPins [player->joygpio] ;
 
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

  player->input = value ^ 0xFFFF;
  //return value ^ 0xFFFF;
}

int setup_player (struct player_t* player)
{
  if (player->joytype == JOY_GPIO)
  {
    if (player->num == 1)
      player->joygpio = setupSnesJoystick (PIN_P1DAT, PIN_P1CLK, PIN_P1LAT);
    else
      player->joygpio = setupSnesJoystick (PIN_P2DAT, PIN_P2CLK, PIN_P2LAT);

    if (player->joygpio == -1)
    {
      fprintf (stderr, "Error in setupSnesJoystick\n");
      return 1;
    }

  
    if (detectSnesJoystick (player->joygpio) != 0)
    {
      fprintf (stderr, "Didn't detect a SNES joystick in port 1\n");
      return 1;
    }

    if (player->joygpio == -1)
    {
      fprintf (stderr, "Unable to setup input\n") ;
      return 1 ;
    }
  }
  else if (player->joytype == JOY_USB)
  {
    if (player->num == 1)
    {
      if (setupUSBJoystick (player, "/dev/input/js0") != 0)
      {
        fprintf (stderr, "Error setting up USB joystick for player 1\n");
        return 1;
      }
    }
    else if (player->num == 2)
    {
      if (setupUSBJoystick (player, "/dev/input/js1") != 0)
      {
        fprintf (stderr, "Error setting up USB joystick for player 2\n");
        return 1;
      }
    }
  }

  player->input = 0x0000;
  player->input_old = 0x0000;
  return 0;

}

int joystick_setup (void)
{
  int ret = 0;
  p1.num = 1;
  p2.num = 2;
  ret = setup_player (&p1);
  return ret;
}

void check_player_inputs (void)
{
  if ((p1.input & SNES_L) && (p1.input & SNES_R))
    fprintf (stdout, "LR PRESSED\n\n\n\n");
}

static void read_input (struct player_t* player)
{
  switch (player->joytype)
  {
    case JOY_USB:
      readUSBJoystick(player);
      break;
    case JOY_GPIO:
      readSnesJoystick(player);
      break;
    default:
    case JOY_NONE:
      break;
  }
}

void read_player_inputs (void)
{
    read_input (&p1);
    //p1.input = readSnesJoystick (p1.pisnes_num) ;
    //check_player_inputs();
}

//Coverts the USB joystick ev data in a short int
void process_ev (struct js_event ev, struct player_t* player)
{
  fprintf (stdout, "T%d N%d V%d\n", ev.type, ev.number, ev.value);
  //if (ev.type != JS_EVENT_BUTTON || ev.type != JS_EVENT_AXIS)
  //  return;
  if (ev.type == JS_EVENT_AXIS)
  {
    if (ev.number == player->mapping.x_axis)
    {
      if (ev.value > 0)
        player->input |= SNES_RIGHT;
      else if (ev.value < 0)
        player->input |= SNES_LEFT;
      else
      {
        player->input &= ~SNES_RIGHT;
        player->input &= ~SNES_LEFT;
      }
    }
    else if (ev.number == player->mapping.y_axis)
    {
      if (ev.value < 0)
        player->input |= SNES_UP;
      else if (ev.value > 0)
        player->input |= SNES_DOWN;
      else
      {
        player->input &= ~SNES_UP;
        player->input &= ~SNES_DOWN;
      }
    }
  }
  else if (ev.type == JS_EVENT_BUTTON)
  {
    if (ev.number == player->mapping.b)
    {
      if (ev.value)
        player->input |= SNES_B;
      else
        player->input &= ~SNES_B;
    }
    else if (ev.number == player->mapping.y)
    {
      if (ev.value)
        player->input |= SNES_Y;
      else
        player->input &= ~SNES_Y;
    }
    else if (ev.number == player->mapping.select)
    {
      if (ev.value)
        player->input |= SNES_SELECT;
      else
        player->input &= ~SNES_SELECT;
    }
    else if (ev.number == player->mapping.start)
    {
      if (ev.value)
        player->input |= SNES_START;
      else
        player->input &= ~SNES_START;
    }
    else if (ev.number == player->mapping.a)
    {
      if (ev.value)
        player->input |= SNES_A;
      else
        player->input &= ~SNES_A;
    }
    else if (ev.number == player->mapping.x)
    {
      if (ev.value)
        player->input |= SNES_X;
      else
        player->input &= ~SNES_X;
    }
    else if (ev.number == player->mapping.l)
    {
      if (ev.value)
        player->input |= SNES_L;
      else
        player->input &= ~SNES_L;
    }
    else if (ev.number == player->mapping.r)
    {
      if (ev.value)
        player->input |= SNES_R;
      else
        player->input &= ~SNES_R;
    }
  }
  fprintf (stdout, "%#10x out\n", player->input);
}

int readUSBJoystick (struct player_t* player)
{
  struct js_event ev;
  int count = 1;
  //while (count != 0 )
  //{
    count = fread (&ev, sizeof(ev), 1, player->fp);
    process_ev (ev, player);
  //}
    /* EAGAIN is returned when the queue is empty */
  if (errno != EAGAIN && errno != 0)
  {
    fprintf (stdout, "\nerrno %i\n", errno);
    fprintf (stderr, "Error reading joystick %s\n", strerror(errno));
    return 1;      
  }
  
  return 0;
}

int setupUSBJoystick (struct player_t* player, char* device)
{
  player->fp = fopen (device, "rb");

  if (player->fp == NULL)
  {
    fprintf (stderr, "Error opening %s: %s\n", device, strerror(errno));
    return 1;
  }
  return 0;
}

/*
int setupUSBJoystick (struct player_t player, char* device)
{
    char num_buttons_char;
    char num_axis_char;
    int num_buttons;
    int num_axis;

    char name[128];
    
    player.fd = open (device, O_RDONLY | O_NONBLOCK);
    if (!player.fd)
    {
        fprintf (stderr, "Error reading USB input device\n");
        return 1;
    }
    
    ioctl (player.fd, JSIOCGBUTTONS, &num_buttons_char);
    num_buttons = num_buttons_char;

    ioctl (player.fd, JSIOCGAXES, &num_axis_char);
    num_axis = num_axis_char;
    
    if (ioctl(player.fd, JSIOCGNAME(sizeof(name)), name) < 0)
      strncpy(name, "Unknown", sizeof(name));
    printf("Joystick name: %s, %i buttons, %i axis\n", name, num_buttons, num_axis);
    
    if (num_buttons < 8)
    {
      fprintf (stderr, "Error, joystick doesn't have enough buttons to be a SNES joystick!\n");
      return 1;
    }
    return 0;
}
*/
