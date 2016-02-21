/*
 * snes.c:
 *	Test program for an old SNES controller connected to the Pi.
 *
 * Copyright (c) 2012-2013 Gordon Henderson. <projects@drogon.net>
 ***********************************************************************
 * This file is part of wiringPi:
 *	https://projects.drogon.net/raspberry-pi/wiringpi/
 *
 *    wiringPi is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU Lesser General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    wiringPi is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU Lesser General Public License for more details.
 *
 *    You should have received a copy of the GNU Lesser General Public License
 *    along with wiringPi.  If not, see <http://www.gnu.org/licenses/>.
 ***********************************************************************
 */

/*TODO: 
Check to see if a joystick is there by reading in the high bits - DONE
  
Hook into the following lines from the SNES that we have available
Reset (MITM SuperCIC reset line?  Might not work with SA-1)
50/60 Hertz from the PPUs
SNES 5V line	
Reprogram the SuperCIC to talk to the Pi

Hardware:
Hook the pi output into the SNES audio output
Get the Pi to drive the power LED
Battery circuit/powerdown script for Pi

Pie in the sky:
Convert to single mcu rather than 4021's
Program ASCII Turbo boxes
*/

#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <sys/time.h>

#include <wiringPi.h>
#include <mcp23017.h>
#include "piSnes.h"

#define PIN_LIN 15 //wiring Pi 15 reads the latch from the SNES
#define PIN_P1CLK 7 //Which GPIO are used for reading in the P1 SNES joystick
#define PIN_P1LAT 0
#define PIN_P1DAT 2

#define PIN_BRD_OK 3 //Loopback from pin 1/3.3v to see if board is connected
#define PIN_SNES_VCC 25
#define	BLANK	"|      "
#define PIN_BASE 120
//Set record buffer to 16MB
#define RECBUFSIZE	1024 * 1024 * 16 
//Magic file number
#define FILEMAGIC	0xBEC16260
#define CHUNK_SIZE (sizeof(unsigned long int) + sizeof(unsigned short int))
//Pointers to input/output buffers
char *input_ptr;
char *ptr;
//Magic number for SNESBot recorded files
long filemagic = FILEMAGIC;
//Default filename
char *filename = "snes.rec";


unsigned short int inputs[16] = {
	SNES_BIT16, SNES_BIT15, SNES_BIT14, SNES_R, SNES_L, SNES_X, SNES_A, SNES_RIGHT, 
	SNES_LEFT, SNES_DOWN, SNES_UP, SNES_START, SNES_SELECT, SNES_Y, SNES_B};


typedef enum {STATE_INIT, STATE_RUNNING, STATE_RECORDING, STATE_PLAYBACK, STATE_EXITING} states_t;

states_t state;

unsigned long int latch_counter = 0;
void latch_interrupt (void);
int write_mem_into_file (void);
int read_file_into_mem (void);
void playback_read_next ();
void record_player_inputs ();
void read_player_inputs ();

struct joy_t {
  int pisnes_num;
  unsigned short int input;
  unsigned short int input_old;
};

struct joy_t p1;
struct joy_t p2;

struct record_t {
  void *ptr;
  int filesize;
  long int filepos;
};

struct playback_t {
  void *ptr;
  long int filepos;
  long int filesize;
  unsigned long int next_latch;
  unsigned short int next_input;
};

struct record_t record;
struct playback_t playback;
/*
static void print_buttons (unsigned short int p1, unsigned short int p2)
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
*/

void wait_for_first_latch (void)
{
  printf("Waiting for first latch\n");
  while (digitalRead (PIN_LIN) == 0 && state != (STATE_EXITING));
  printf("Running\n");
}

void clear_all_buttons (void)
{ 
  int i;
  for (i = 0 ; i < 16 ; ++i)
  {
      //Set all pins to output and default high
      pinMode (PIN_BASE + i, OUTPUT) ;
      pullUpDnControl (PIN_BASE + i, PUD_UP);
      digitalWrite (PIN_BASE + i, HIGH) ;
  }
}

int snes_is_on (void)
{
  return digitalRead (PIN_SNES_VCC);
}

void signal_handler (int signal)
{
	printf ("\n\nSIGINT detected\n");
        state = STATE_EXITING;
}

//TODO Probe the i2c line to make sure the mcp23017 is there
//Setup the mcp23017 and Pi GPIO
int port_setup (void)
{
  fprintf (stdout, "Setting up GPIO\n");
  pinMode (PIN_BRD_OK, INPUT);
  pullUpDnControl (PIN_BRD_OK, PUD_DOWN);
  pinMode (PIN_SNES_VCC, INPUT);
  pullUpDnControl (PIN_SNES_VCC, PUD_DOWN);
  
    
  if (digitalRead (PIN_BRD_OK) != 1)
  {
    fprintf (stdout, "Warning:  Board not detected or 3.3v missing?\n");
    return 1;
  }
  //Setup the latch input from the SNES
  pinMode (PIN_LIN, INPUT);
  pullUpDnControl (PIN_LIN, PUD_UP);
  wiringPiISR (PIN_LIN, INT_EDGE_RISING, &latch_interrupt);
  
  fprintf (stdout, "Setting up mcp23017\n");
  mcp23017Setup (PIN_BASE, 0x20);
  clear_all_buttons ();
  
  return 0;
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

void latch_interrupt (void)
{
  if ((state == STATE_PLAYBACK) && (playback.next_latch == latch_counter))
  {
    //We are due to load up the next set of inputs
    set_inputs(PIN_BASE, p1.input);
    playback_read_next ();
  } 
  else if (state == STATE_RECORDING && p1.input != p1.input_old)
  {
    p1.input_old = p1.input;
    record_player_inputs ();
    set_inputs(PIN_BASE, p1.input);
  }
  else if (state == STATE_RUNNING && p1.input != p1.input_old)
  {
    p1.input_old = p1.input;
    set_inputs(PIN_BASE, p1.input);
  }

  latch_counter++;
}

int joystick_setup ()
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

int setup ()
{
  if (wiringPiSetup () == -1)
  {
    fprintf (stdout, "oops: %s\n", strerror (errno)) ;
    return 1 ;
  }
	
  if (port_setup () != 0 || joystick_setup () != 0)
  {
    return 1;
  }

 signal(SIGINT, signal_handler);
  return 0;
}

void check_player_inputs ()
{
  if ((p1.input & SNES_L) && (p1.input & SNES_R))
    fprintf (stdout, "LR PRESSED\n\n\n\n");
}

void read_player_inputs ()
{
    p1.input = readSnesJoystick (p1.pisnes_num) ;
    //check_player_inputs();
}

int read_options (int argc, char **argv)
{
  int c;
  while ((c = getopt (argc, argv, "rp")) != -1)
  {
      switch (c)
      {
        case 'r':
          if (state == STATE_PLAYBACK)
          {
            fprintf (stderr, "Error, both playback and record specified\n");
            return 1;
          }
          state = STATE_RECORDING;
          break;
        case 'p':
          if (state == STATE_RECORDING)
          {
            fprintf (stderr, "Error, both playback and record specified\n");
            return 1;
          }
          state = STATE_PLAYBACK;
          break;
      }
  }
  return 0;
}

//Reads the next set of inputs ready to be pressed
void playback_read_next ()
{
  memcpy (&playback.next_latch, playback.ptr + (playback.filepos * CHUNK_SIZE), sizeof(latch_counter));
  memcpy (&p1.input, playback.ptr + sizeof(latch_counter) + (playback.filepos * CHUNK_SIZE), sizeof(p1.input));
  playback.filepos++;
  if ((playback.filepos * CHUNK_SIZE) > playback.filesize)
  {
    fprintf (stdout, "Playback finished\n");
    state = STATE_EXITING;
  }
  //fprintf (stdout, "%i, %x, %i\n", latch_counter, p1.input, playback.next_latch);
}

int playback_start ()
{
  if (read_file_into_mem () != 0)
  {
    fprintf (stderr, "Error reading input file into memory\n");
    return 1;
  }
  fprintf (stdout, "Filesize: %li\n", playback.filesize);
  //Load up the initial inputs
  playback_read_next ();

  return 0;
}

int record_start ()
{
  //malloc some memory for the output ptr
  record.ptr = malloc (RECBUFSIZE);
  if (record.ptr == NULL)
  {
    printf("Unable to allocate %i bytes for record buffer\n", RECBUFSIZE);
    state = STATE_EXITING;
    return 1;
  }
  else
    printf("%i bytes allocated for record buffer\n", RECBUFSIZE);
  record.filepos = 0;
  state = STATE_RECORDING;
  return 0;
}

void record_player_inputs ()
{
  //fprintf (stdout, "size %i ptr: %#10x\n", record.filepos * CHUNK_SIZE, record.ptr + (record.filepos * CHUNK_SIZE));
  memcpy (record.ptr + record.filepos * CHUNK_SIZE, &latch_counter, sizeof(latch_counter));
  memcpy (record.ptr + sizeof(latch_counter) + (record.filepos * CHUNK_SIZE), &p1.input, sizeof(p1.input));
  record.filepos++;
}

void record_save ()
{
  if (record.filepos > 0)
  {
    write_mem_into_file ();
  }
}

void main_loop ()
{ 
  if (state == STATE_RECORDING)
  {
    fprintf (stdout, "RECORDING\n");
    if (record_start () != 0)
    {
      fprintf (stderr, "Error setting up record buffer\n");
      state = STATE_EXITING;
    }
    if (snes_is_on ())
    {
      fprintf (stdout, "SNES power detected, please power cycle the SNES\n");
      while (snes_is_on ()) delayMicroseconds (100);
      fprintf (stdout, "SNES Powered off\n");
    }
    fprintf (stdout, "Waiting for SNES power on\n");
    while (!snes_is_on ()) delayMicroseconds (100);
  }
  else if (state == STATE_PLAYBACK)
  {
    fprintf (stdout, "PLAYBACK\n");
    if (playback_start () != 0)
    {
      fprintf (stderr, "Error setting up playback buffer\n");
      state = STATE_EXITING;
    }
  }
  clear_all_buttons (); 
  
  if (state == STATE_PLAYBACK || state == STATE_RECORDING)
    wait_for_first_latch ();
  
  while (state != STATE_EXITING)
  {
    if (state != STATE_PLAYBACK)
      read_player_inputs();
    if (!snes_is_on())
    {
      fprintf (stdout, "SNES poweroff detected\n");
      state = STATE_EXITING;
    }
  }
  //Always attempt a save before exiting
  record_save ();
}

int main (int argc, char **argv)
{
  state = STATE_INIT;
  if (setup () != 0)
  {
    fprintf (stdout, "Error setting up\n");
    return 1;
  }
  
  if (read_options (argc, argv) != 0)
  {
    fprintf (stderr, "Error reading options\n");
    return 1;
  }
  
  if (state == STATE_INIT)
  {
    //User didn't chose either record or replay, falling back to passthrough
    state = STATE_RUNNING;
  }

  main_loop ();
 
  fprintf (stdout, "Exiting...\n");
  return 0 ;
}

int write_mem_into_file (void)
{
	//Open output file
	FILE *output_file = fopen (filename, "wb");
	
	if (output_file == NULL)
	{
		printf("Problem opening %s for writing\n", filename);
		return 1;
	}
	
	//Write magic number to file header
//	fwrite (&filemagic, 1, sizeof(filemagic), output_file);

	//Calculate filesize
	record.filesize = CHUNK_SIZE * record.filepos;
	printf ("Recorded %i bytes of input\n", record.filesize);
	
	//Store to output file
	long result = fwrite (record.ptr, 1, record.filesize, output_file);
	printf ("Wrote %lu bytes to %s\n", result, filename);
	fclose(output_file);
	return 0;
}

int read_file_into_mem (void)
{
	//Open the file
	FILE *input_file = fopen (filename, "rb");
	
	if (input_file == NULL)
	{
		printf ("Could not open %s for reading\n", filename);
		return 1;
	}
        /*        
        long filemagic_check;
        fread (&filemagic_check, 1, sizeof(filemagic), input_file);
        if (filemagic_check != filemagic)
        {
          printf ("Incompatible file type!\n");
          return 1;
        }
        else 
          printf ("SNESBot filetype detected\n");
        */
	//Seek to the end
	fseek (input_file, 0, SEEK_END);
	//Find out how many bytes it is
	playback.filesize = ftell(input_file);
	

	//Rewind it back to the start ready for reading into memory
	rewind (input_file);	
	
        /*
	//Strip off file magic number
        filesize = filesize - sizeof(filemagic);
        //Seek past the magic number
        fseek (input_file, sizeof(filemagic), 0);
        */

	//Allocate some memory
	playback.ptr = malloc (playback.filesize);
	if (playback.ptr == NULL)
	{
		printf("malloc of %lu bytes failed\n", playback.filesize);
		return 1;
	}
	else
		printf("malloc of %lu bytes succeeded\n", playback.filesize);
	
	//Read the input file into allocated memory
	fread (playback.ptr, 1, playback.filesize, input_file);
        playback.filepos = 0;	
	//Close the input file, no longer needed
	fclose (input_file);
	return 0;
}
