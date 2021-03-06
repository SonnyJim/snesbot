/*
 * joystick.c:
 *    SNES GPIO/USB Joystick reading and configuration
 *
 * Copyright (c) 2016 Ewan Meadows
 ***********************************************************************
 *
 *    This is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU Lesser General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    This is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU Lesser General Public License for more details.
 *
 *    You should have received a copy of the GNU Lesser General Public License
 *    along with this.  If not, see <http://www.gnu.org/licenses/>.
 ***********************************************************************
 */


#include "snesbot.h"



//Top 4 bits should always be high if a stick is plugged in
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

int setupSnesJoystick (int dPin, int cPin, int lPin)
{
  if (gpio_joysticks == MAX_SNES_JOYSTICKS)
    return -1 ;

  snesPins [gpio_joysticks].dPin = dPin ;
  snesPins [gpio_joysticks].cPin = cPin ;
  snesPins [gpio_joysticks].lPin = lPin ;

  digitalWrite (lPin, LOW) ;
  digitalWrite (cPin, LOW) ;

  pinMode (lPin, OUTPUT) ;
  pinMode (cPin, OUTPUT) ;
  pinMode (dPin, INPUT) ;

  return gpio_joysticks++ ;
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
}

static int setup_player_gpio (struct player_t* player)
{
  //Setup the GPIO pins and register the joystick
  if (player->num == 1)
    player->joygpio = setupSnesJoystick (PIN_P1DAT, PIN_P1CLK, PIN_P1LAT);
  else
    player->joygpio = setupSnesJoystick (PIN_P2DAT, PIN_P2CLK, PIN_P2LAT);

  if (player->joygpio == -1)
  {
    fprintf (stderr, "Error in setupSnesJoystick\n");
    return 1;
  }
  
  //Check to see if there's a joystick plugged in
  if (detectSnesJoystick (player->joygpio) != 0)
  {
    fprintf (stderr, "Didn't detect a SNES joystick in port 1\n");
    return 1;
  }

  if (player->joygpio == -1)
  {
    fprintf (stderr, "Unable to setup input\n") ;
    return 1;
  }
  
  //Everything checks out OK
  player->input = 0x0000;
  player->input_old = 0x0000;
  return 0;
}

static int setup_player_usb (struct player_t* player)
{
  if (player->num == 1)
  {
    if (openUSBJoystick (player, "/dev/input/js0") != 0)
    {
      fprintf (stderr, "Error setting up USB joystick for player 1\n");
      return 1;
    }
  }
  else if (player->num == 2)
  {
    if (openUSBJoystick (player, "/dev/input/js1") != 0)
    {
      fprintf (stderr, "Error setting up USB joystick for player 2\n");
      return 1;
    }
  }
 
  player->input = 0x0000;
  player->input_old = 0x0000;
  return 0;
}

int setup_player (struct player_t* player)
{
  switch (player->joytype)
  {
    case JOY_GPIO:
      return setup_player_gpio (player);
      break;
    case JOY_USB:
      return setup_player_usb (player);
      break;
    case JOY_NONE:
      fprintf (stdout, "Warning: Not setting up a joystick for player %i\n", player->num);
      return 0;
      break;
    default:
      return 1;
      break;
  }
}

int read_joystick_cfg (void)
{
  FILE* cfgfile;
  cfgfile = fopen ("./snes.cfg", "rb");

  if (cfgfile == NULL)
  {
    fprintf (stderr, "Error opening joystick cfg file for reading: %s\n", strerror(errno));
    return 1;
  }
  
  fread (&p1.joytype, sizeof(p1.joytype), 1, cfgfile);
  fread (&p1.mapping, sizeof(p1.mapping), 1, cfgfile);
  fread (&p2.joytype, sizeof(p1.joytype), 1, cfgfile);
  fread (&p2.mapping, sizeof(p2.mapping), 1, cfgfile);
  fclose (cfgfile);
  return 0;
}

int save_joystick_cfg (void)
{
  FILE* cfgfile;
  cfgfile = fopen ("./snes.cfg", "wb");

  if (cfgfile == NULL)
  {
    fprintf (stderr, "Error opening file for writing: %s\n", strerror(errno));
    return 1;
  }
  
  fwrite (&p1.joytype, sizeof(p1.joytype), 1, cfgfile);
  fwrite (&p1.mapping, sizeof(p1.mapping), 1, cfgfile);
  fwrite (&p2.joytype, sizeof(p1.joytype), 1, cfgfile);
  fwrite (&p2.mapping, sizeof(p2.mapping), 1, cfgfile);
  fclose (cfgfile);
  return 0;
}

int joystick_setup (void)
{
  int ret = 0;
  gpio_joysticks = 0 ;
  p1.num = 1;
  p2.num = 2;
  
  if (read_joystick_cfg () != 0)
    cfg_joysticks();

  ret = setup_player (&p1);
  //configure_player_buttons (&p1);
  return ret;
}

void check_joystick_inputs (void)
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

static int wait_for_js_axis (struct player_t* player)
{
  struct js_event ev;
  int waiting = 1;
  fprintf (stdout, "Move axis\n");
  while (waiting)
  {
    fread (&ev, sizeof(ev), 1, player->fp);
    if ((ev.type == JS_EVENT_AXIS) && (ev.value != 0))
    {
      fprintf (stdout, "1T%d N%d V%d\n", ev.type, ev.number, ev.value);
      waiting = 0;
    }
  }
  return ev.number; 
}

static int wait_for_js_button (struct player_t* player)
{
  struct js_event ev;
  int waiting = 1;
  fprintf (stdout, "Press button\n");
  while (waiting)
  {
    fread (&ev, sizeof(ev), 1, player->fp);
    if ((ev.type == JS_EVENT_BUTTON) && (ev.value > 0))
    {
      fprintf (stdout, "1T%d N%d V%d\n", ev.type, ev.number, ev.value);
      waiting = 0;
    }
  }
  return ev.number; 
}

static void cfg_player_buttons (struct player_t* player)
{
  int i;
  fprintf (stdout, "Configuring buttons for player %i\n", player->num);
  fprintf (stdout, "Select joypad type:\n (0=None, 1=GPIO, 2=USB)\n");
  scanf("%d",&i);
  player->joytype = i;
  if (player->joytype == JOY_NONE || player->joytype == JOY_GPIO)
    return;

  if (player->joytype > JOY_USB)
  {
    player->joytype = JOY_USB;
  }
  

  fprintf (stdout, "X axis\n");
  player->mapping.x_axis = wait_for_js_axis (player);
  fprintf (stdout, "Y axis\n");
  player->mapping.y_axis = wait_for_js_axis (player);
  
  fprintf (stdout, "B\n");
  player->mapping.b = wait_for_js_button (player);
  fprintf (stdout, "A\n");
  player->mapping.a = wait_for_js_button (player);
  fprintf (stdout, "Y\n");
  player->mapping.y = wait_for_js_button (player);
  fprintf (stdout, "X\n");
  player->mapping.x = wait_for_js_button (player);
  fprintf (stdout, "L\n");
  player->mapping.l = wait_for_js_button (player);
  fprintf (stdout, "R\n");
  player->mapping.r = wait_for_js_button (player);
  fprintf (stdout, "Select\n");
  player->mapping.select = wait_for_js_button (player);
  fprintf (stdout, "Start\n");
  player->mapping.start = wait_for_js_button (player);
  fprintf (stdout, "Macro\n");
  player->mapping.macro = wait_for_js_button (player);
  fprintf (stdout,"Finished configuring buttons for player %i\n", player->num);

}

void cfg_joysticks (void)
{
  if (openUSBJoystick (&p1, "/dev/input/js0") == 0)
      cfg_player_buttons (&p1);
  
  if (openUSBJoystick (&p2, "/dev/input/js1") == 0)
      cfg_player_buttons (&p2);
  
  save_joystick_cfg ();
}

void read_joystick_inputs (void)
{
    read_input (&p1);
    //check_player_inputs();
}

//Coverts the USB joystick event data in a short int
void process_ev (struct js_event ev, struct player_t* player)
{
  if (ev.type == JS_EVENT_AXIS)
  {
    if (ev.number == player->mapping.x_axis)
    {
      if (ev.value > (0 + DEADZONE))
        player->input |= SNES_RIGHT;
      else if (ev.value < (0 - DEADZONE))
        player->input |= SNES_LEFT;
      else
      {
        player->input &= ~SNES_RIGHT;
        player->input &= ~SNES_LEFT;
      }
    }
    else if (ev.number == player->mapping.y_axis)
    {
      if (ev.value < (0 - DEADZONE))
        player->input |= SNES_UP;
      else if (ev.value > (0 + DEADZONE))
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
    else if (ev.number == player->mapping.macro)
    {
      if (ev.value && macro1.filesize)
        macro_start (&macro1);
    }
  }
}

int readUSBJoystick (struct player_t* player)
{
  struct js_event ev;
  
  fread (&ev, sizeof(ev), 1, player->fp);
  process_ev (ev, player);
    /* EAGAIN is returned when the queue is empty */
  if (errno != EAGAIN && errno != 0)
  {
    return 1;      
  }
  
  return 0;
}

int openUSBJoystick (struct player_t* player, char* device)
{
  if (player->fp != NULL)
  {
    fprintf (stderr, "Error: joystick device already opened?\n");
    return 1;
  }

  player->fp = fopen (device, "rb");

  if (player->fp == NULL)
  {
    fprintf (stderr, "Error opening %s: %s\n", device, strerror(errno));
    return 1;
  }
  return 0;
}

