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
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>
#include <signal.h>

#include <wiringPi.h>
#include <mcp23017.h>
#include "piSnes.h"

#define PIN_LIN 15 //wiring Pi 15 reads the latch from the SNES
#define PIN_P1CLK 7 //Which GPIO are used for reading in the P1 SNES joystick
#define PIN_P1LAT 0
#define PIN_P1DAT 2

#define PIN_BRD_OK 3 //Loopback from pin 1/3.3v to see if board is connected

#define	BLANK	"|      "
#define PIN_BASE 120
//Set record buffer to 16MB
#define RECBUFSIZE	1024 * 1024 * 16 
//Magic file number
#define FILEMAGIC	0xBEC16260
#define CHUNK_SIZE (sizeof(unsigned long int) + sizeof(unsigned short int))
//Pointers to input/output buffers
void *input_ptr;
void *ptr;
//Magic number for SNESBot recorded files
long filemagic = FILEMAGIC;
//Default filename
char *filename = "snes.rec";
// How big (in bytes) the playback/record file is
unsigned long int filesize = 0;


unsigned short int inputs[16] = {
	SNES_BIT16, SNES_BIT15, SNES_BIT14, SNES_R, SNES_L, SNES_X, SNES_A, SNES_RIGHT, 
	SNES_LEFT, SNES_DOWN, SNES_UP, SNES_START, SNES_SELECT, SNES_Y, SNES_B};


typedef enum {STATE_INIT, STATE_RUNNING, STATE_RECORDING, STATE_PLAYBACK, STATE_EXITING} states_t;

states_t state;

unsigned long int latch_counter = 0;
void latch_interrupt (void);

struct joy_t {
  int pisnes_num;
  unsigned short int input;
  unsigned short int input_old;
};

struct joy_t p1;
struct joy_t p2;

struct record_t {
  int *ptr;
  long int filepos;
};

struct record_t record;

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

//TODO Monitor the +5V SNES line
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
  latch_counter++;
}

//Memory routines
int malloc_record_buffer (void)
{
  //Allocate memory for recorded inputs
  ptr = malloc (RECBUFSIZE);
  if (ptr == NULL)
  {
    printf("Unable to allocate %i bytes for record buffer\n", RECBUFSIZE);
    return 1;
  }
  else
    printf("%i bytes allocated for record buffer\n", RECBUFSIZE);
  return 0;
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

  //joy2 = setupSnesJoystick ( 1, 0, 15);

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
    check_player_inputs();
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
  memcpy (record.ptr + (record.filepos * CHUNK_SIZE), &latch_counter, sizeof(latch_counter));
  memcpy (record.ptr + sizeof(latch_counter) + (record.filepos * CHUNK_SIZE), &p1.input, sizeof(p1.input));
  record.filepos++;
}

void main_loop ()
{
  //TODO While SNES is on, halt and show message to turn off
  if (state == STATE_PLAYBACK || state == STATE_RECORDING)
    wait_for_first_latch ();
  
  if (state == STATE_RECORDING)
    if (record_start () != 0)
    {
      fprintf (stderr, "Error setting up record buffer\n");
      state = STATE_EXITING;
    }


  while (state != STATE_EXITING)
  {
    read_player_inputs();
    if (p1.input != p1.input_old)
    {
      p1.input_old = p1.input;
      print_buttons (p1.input, p2.input);
      set_inputs (PIN_BASE, p1.input);
      record_player_inputs ();
    }
  }
}

int main ()
{
  state = STATE_INIT;
  if (setup () != 0)
  {
    fprintf (stdout, "Error setting up\n");
    return 1;
  }
  state = STATE_RUNNING;
  main_loop ();
 
  fprintf (stdout, "Exiting...\n");
  return 0 ;
}
/*
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
	fwrite (&filemagic, 1, sizeof(filemagic), output_file);

	//Calculate filesize
	filesize = CHUNK_SIZE * filepos;
	printf ("Recorded %lu bytes of input\n", filesize);
	
	//Store to output file
	long result = fwrite (ptr, 1, filesize, output_file);
	printf ("Wrote %lu bytes to %s\n", result + sizeof(filemagic), filename);
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
        
        long filemagic_check;
        fread (&filemagic_check, 1, sizeof(filemagic), input_file);
        if (filemagic_check != filemagic)
        {
          printf ("Incompatible file type!\n");
          return 1;
        }
        else 
          printf ("SNESBot filetype detected\n");

	//Seek to the end
	fseek (input_file, 0, SEEK_END);
	//Find out how many bytes it is
	filesize = ftell(input_file);
	

	//Rewind it back to the start ready for reading into memory
	rewind (input_file);	
	
	//Strip off file magic number
        filesize = filesize - sizeof(filemagic);
        //Seek past the magic number
        fseek (input_file, sizeof(filemagic), 0);

	//Allocate some memory
	input_ptr = malloc (filesize);
	if (input_ptr == NULL)
	{
		printf("malloc of %lu bytes failed\n", filesize);
		return 1;
	}
	else
		printf("malloc of %lu bytes succeeded\n", filesize);
	
	//Read the input file into allocated memory
	fread (input_ptr, 1, filesize, input_file);
	
	//Close the input file, no longer needed
	fclose (input_file);
	return 0;
}
*/
