/*
 * snesbot.c:
 *    A Raspberry Pi based SNES/NES Bot
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

/*TODO: 
Add in a header, with what the last frame is
Print the estimated playback time at file start

Hook into the following lines from the SNES that we have available
Reset (MITM SuperCIC reset line?  Might not work with SA-1)
50/60 Hertz from the PPUs
SNES 5V line	
Reprogram the SuperCIC to talk to the Pi

Hardware:
Hook the pi output into the SNES audio output
Get the Pi to drive the power LED
Battery circuit/powerdown script for Pi
SMD LEDs to show button pushes


Pie in the sky:
Convert to single mcu rather than 4021's
Program ASCII Turbo boxes
*/

#include "snesbot.h"
//Magic number for SNESBot recorded files
long filemagic = FILEMAGIC;

unsigned short int inputs[16] = {
	SNES_BIT16, SNES_BIT15, SNES_BIT14, SNES_R, SNES_L, SNES_X, SNES_A, SNES_RIGHT, 
	SNES_LEFT, SNES_DOWN, SNES_UP, SNES_START, SNES_SELECT, SNES_Y, SNES_B};

void print_buttons (unsigned short int p1, unsigned short int p2)
{
	printf("Player 1: %#010x %lu\n", p1, latch_counter);
	if ((p1 & SNES_UP)     != 0) printf ("|  UP  " ) ; else printf (BLANK) ;
	if ((p1 & SNES_DOWN)   != 0) printf ("| DOWN " ) ; else printf (BLANK) ;
	if ((p1 & SNES_LEFT)   != 0) printf ("| LEFT " ) ; else printf (BLANK) ;
	if ((p1 & SNES_RIGHT)  != 0) printf ("|RIGHT " ) ; else printf (BLANK) ;
	if ((p1 & SNES_SELECT) != 0) printf ("|SELECT" ) ; else printf (BLANK) ;
	if ((p1 & SNES_START)  != 0) printf ("|START " ) ; else printf (BLANK) ;
	if ((p1 & SNES_A)      != 0) printf ("|  A   " ) ; else printf (BLANK) ;
	if ((p1 & SNES_B)      != 0) printf ("|  B   " ) ; else printf (BLANK) ;
	if ((p1 & SNES_X)      != 0) printf ("|  X   " ) ; else printf (BLANK) ;
	if ((p1 & SNES_Y)      != 0) printf ("|  Y   " ) ; else printf (BLANK) ;
	if ((p1 & SNES_L)      != 0) printf ("|  L   " ) ; else printf (BLANK) ;
	if ((p1 & SNES_R)      != 0) printf ("|  R   " ) ; else printf (BLANK) ;
	printf("\n");
	printf("Player 2: %#010x\n", p2);
	if ((p2 & SNES_UP)     != 0) printf ("|  UP  " ) ; else printf (BLANK) ;
	if ((p2 & SNES_DOWN)   != 0) printf ("| DOWN " ) ; else printf (BLANK) ;
	if ((p2 & SNES_LEFT)   != 0) printf ("| LEFT " ) ; else printf (BLANK) ;
	if ((p2 & SNES_RIGHT)  != 0) printf ("|RIGHT " ) ; else printf (BLANK) ;
	if ((p2 & SNES_SELECT) != 0) printf ("|SELECT" ) ; else printf (BLANK) ;
	if ((p2 & SNES_START)  != 0) printf ("|START " ) ; else printf (BLANK) ;
	if ((p2 & SNES_A)      != 0) printf ("|  A   " ) ; else printf (BLANK) ;
	if ((p2 & SNES_B)      != 0) printf ("|  B   " ) ; else printf (BLANK) ;
	if ((p2 & SNES_X)      != 0) printf ("|  X   " ) ; else printf (BLANK) ;
	if ((p2 & SNES_Y)      != 0) printf ("|  Y   " ) ; else printf (BLANK) ;
	if ((p2 & SNES_L)      != 0) printf ("|  L   " ) ; else printf (BLANK) ;
	if ((p2 & SNES_R)      != 0) printf ("|  R   " ) ; else printf (BLANK) ;
	printf ("|\n") ;
}

void wait_for_first_latch (void)
{
  printf("Waiting for first latch\n");
  //time_start (); 
  while (digitalRead (PIN_LIN) == 0 && botcfg.state != (STATE_EXITING))delay(1);
  //time_stop ();
  printf("Running\n");
}

void clear_all_buttons (void)
{ 
  int i;
  for (i = 0 ; i < 16 ; ++i)
  {
      p1.input = 0x0000;
      //Set all pins to output and default high
      pinMode (PIN_BASE + i, OUTPUT) ;
      pullUpDnControl (PIN_BASE + i, PUD_DOWN);
      digitalWrite (PIN_BASE + i, HIGH) ;
  }
}

int snes_is_on (void)
{
  return digitalRead (PIN_SNES_VCC);
}

//Writes to the mcp23017 based on a 16bit short
static void set_inputs (int pin_base, unsigned short int input)
{
	int i;
	for (i = 0; i < 15; i++)
	{
		if (((input + i) & inputs[i]) != 0) 
		{
			digitalWrite (pin_base + i, LOW);
		}
		else
			digitalWrite (pin_base + i, HIGH);
	}
}

void time_start (void)
{
  gettimeofday(&start_time, NULL);
}

void time_stop (void)
{
  struct timeval stop_time;
  long ms;
  gettimeofday(&stop_time, NULL);
  ms = (stop_time.tv_usec - start_time.tv_usec);
  printf ("Elapsed time = %li ms\n", ms);
}


inline void latch_interrupt (void)
{
  delay(2);
  //fprintf (stdout, "%lu|", latch_counter);
  if ((botcfg.state == STATE_MACRO) && (macro1.next_latch == latch_counter - 1))
  {
    set_inputs (PIN_BASE, p1.input);
    pb_read_next (&macro1);
  }
  else if ((botcfg.state == STATE_PLAYBACK) && (playback.next_latch == latch_counter)) 
  {
    //We are due to load up the next set of inputs
    set_inputs(PIN_BASE, p1.input);
    playback_read_next ();
  } 
  else if (botcfg.state == STATE_RECORDING && p1.input != p1.input_old)
  {
    p1.input_old = p1.input;
    record_player_inputs ();
    set_inputs(PIN_BASE, p1.input);
  }
  else if (botcfg.state == STATE_RUNNING && p1.input != p1.input_old)
  {
    p1.input_old = p1.input;
    set_inputs(PIN_BASE, p1.input);
  }
  latch_counter++;
  if (subs.running && subs.start_latch == latch_counter)
  {
    fprintf (stdout, "%s\n", subs.text);
    sub_read_next ();
  }
}

void wait_for_snes_powerup (void)
{
  if (!botcfg.wait_for_power)
    return;
  
  if (snes_is_on ())
  {
    fprintf (stdout, "SNES power detected, please power cycle the SNES\n");
    while (snes_is_on ()) delay(1);
    fprintf (stdout, "SNES Powered off\n");
  }
  
  fprintf (stdout, "Waiting for SNES power on\n");
  while (!snes_is_on ()) delay(1);
}

void main_loop (void)
{ 
  clear_all_buttons (); 
  
  if (botcfg.state == STATE_RECORDING)
  {
    fprintf (stdout, "RECORDING\n");
    if (record_start () != 0)
    {
      fprintf (stderr, "Error setting up record buffer\n");
      botcfg.state = STATE_EXITING;
    }
    wait_for_snes_powerup ();
  }
  else if (botcfg.state == STATE_PLAYBACK)
  {
    fprintf (stdout, "PLAYBACK\n");
    if (playback_start () != 0)
    {
      fprintf (stderr, "Error setting up playback buffer\n");
      botcfg.state = STATE_EXITING;
    }
    wait_for_snes_powerup ();
  }
  
  interrupt_enable();
 
  if (botcfg.state == STATE_PLAYBACK || botcfg.state == STATE_RECORDING)
    wait_for_first_latch ();
  
  while (botcfg.state != STATE_EXITING)
  {
    if (botcfg.state != STATE_PLAYBACK && botcfg.state != STATE_MACRO)
    {
      read_joystick_inputs();
    }

    if (!snes_is_on())
    {
      fprintf (stdout, "SNES poweroff detected\n");
      botcfg.state = STATE_EXITING;
    }
  }
  //Always attempt a save before exiting
  record_save ();
}

int drop_privs (void)
{
  char* user;
  char* group;
  
  user = getenv("SUDO_UID");
  group = getenv("SUDO_GID");

  fprintf (stdout, "Dropping privileges to %s:%s\n", user, group);
  if (setgid(atoi(group)) == -1) 
  {
    fprintf (stderr, "Error dropping group privileges\n");
    return 1;
  }
  if (setuid(atoi(user)) == -1) {
    fprintf (stderr, "Error dropping user privileges\n");
    return 1;
  }
  return 0;
}

int main (int argc, char **argv)
{
  botcfg.state = STATE_INIT;
   //piHiPri (45);
  if (setup () != 0)
  {
    fprintf (stderr, "Error setting up\n");
    return 1;
  }

  if (read_options (argc, argv) != 0)
  {
    fprintf (stderr, "Error reading options\n");
    return 1;
  }
  
  if (botcfg.state == STATE_INIT)
  {
    //User didn't chose either record or replay, falling back to passthrough
    botcfg.state = STATE_RUNNING;
  }
  
  if (joystick_setup () != 0)
  {
    fprintf (stderr, "Error setting up joysticks\n");
    return 1;
  }
  
  //drop_privs ();
  
  if (check_pid ())
  {
    fprintf (stderr, "Are we already running?\n");
    return 1;
  }
  
  main_loop ();
  clear_all_buttons (); 
  fprintf (stdout, "Exiting...\n");
  remove_pid ();
  return 0 ;
}

