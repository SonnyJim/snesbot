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


/*
TODO 
Use errno.h
Improve general program flow
Bug with Total latches
*/

#include "snesbot.h"

//Command line variables
int keyboard_input = 0;
int joystick_input = 0;
int record_input = 0;
int playback_input = 0;
int verbose = 0;
int debug_playback = 0;
//Default is always wait for a latch before starting
int wait_for_latch = 1;
int lsnes_input_file = 0;
int record_after_playback = 0;

//Number of latches to wait or skip before playback
int wait_latches = 0;
int start_pos = 0;

struct timeval start_time, end_time;

//Default filename
char *filename = "snesbot.rec";

//Number of SNES latches read by GPIO latch pin
unsigned long int latch_counter = 0;

//Handle when we get more than one event on the same latch
unsigned long int old_latch = 0;

int running = 0;
int playback_finished = 0;

FILE *js_dev;
FILE *kb_dev;

//Pointers to input/output buffers
void *input_ptr;
void *output_ptr;

//Joystick evdev structure
static struct js_event ev;
static struct input_event kb_ev;

//lsnes polldump.lua input format is 
// 0000 RLXA rldu SsYB
unsigned short int lsnes_buttons = 0;

// Current and next playback latch
static unsigned long int playback_latch = 0;
static unsigned long int next_playback_latch = 0;

//Whereabouts in the playback/record file we are
unsigned long int filepos = 0;
// Where the end of the playback file is
unsigned long int filepos_end = 0;
// How big (in bytes) the playback/record file is
unsigned long int filesize = 0;

//TODO Make this a bit more generic for other controller types, 
// At the moment it's heavily based around the PS1 controller
//SNES Controller layout
const int buttons[NUMBUTTONS] = { X_Pin, A_Pin, B_Pin, Y_Pin, TLeft_Pin, TRight_Pin, Select_Pin, Start_Pin, Up_Pin, Down_Pin, Left_Pin, Right_Pin };

const int lsnes_input_map[NUMBUTTONS] = { B_Pin, Y_Pin, Select_Pin, Start_Pin, Up_Pin, Down_Pin, Left_Pin, Right_Pin, A_Pin, X_Pin, TLeft_Pin, TRight_Pin};

const char lsnes_button_names[14][7] = {"B", "Y", "Select", "Start", "Up", "Down", "Left", "Right", "A", "X", "TLeft", "TRight"};

//Button mapping for USB PS1 controller, X/Y axis is handled differently
const int psx_mapping[14] = { X_Pin, A_Pin, B_Pin, Y_Pin, TLeft_Pin, TRight_Pin, TLeft_Pin, TRight_Pin, Select_Pin, Start_Pin };

const char button_names[14][7] = { "X", "A", "B", "Y", "TLeft", "TRight", "TLeft", "TRight", "Select", "Start" };

//Magic number for SNESBot recorded files
long filemagic = FILEMAGIC;

void signal_handler (int signal)
{
	printf ("\nSIGINT detected, exiting\n");
	running = 0;
}

void wait_for_first_latch (void)
{
	if (wait_for_latch && !debug_playback)
	{
		printf("Waiting for first latch\n");
		while (digitalRead (Latch_Pin) == 0 && running);
			//delayMicroseconds (1);
	}
}

void clear_buttons (void)
{
	int i;
	for (i = 0; i < NUMBUTTONS; i++)
	{
		//Set all buttons to off ie HIGH
		digitalWrite (buttons[i], HIGH);
	}
}

int init_gpio (void)
{
	int i;

	if (wiringPiSetup () == -1)
		return 1;
	
	//Set up pins
	pinMode (Latch_Pin, INPUT);
	//Set Pull down resistor
	pullUpDnControl (Latch_Pin, PUD_OFF);
	
	for (i = 0; i < NUMBUTTONS; i++)
	{
		pinMode (buttons[i], OUTPUT);
		pullUpDnControl (buttons[i], PUD_OFF);
	}

	clear_buttons ();
	return 0;
}


void print_joystick_input (void)
{
	if (old_latch != playback_latch)
		printf ("\n%lu ", playback_latch);
	else
		printf (" + ");
	
	// Axis/Type 2 == dpad
	if (ev.type == 2)
	{
		switch (ev.number)
		{
			// X axis
			case 0:
				if (ev.value > 0)
				{
					printf ("Right");
				}
				else if (ev.value < 0)
				{
					printf ("Left");
				}
				else
				{
					printf ("X Centred");
				}
				break;
			//Y Axis
			case 1:
				if (ev.value > 0)
				{
					printf ("Down");
				}
				else if (ev.value < 0)
				{
					printf ("Up");
				}
				else
				{
					printf ("Y Centred");
				}
				break;
			default:
				break;
			}
	}
	// Axis/Type 1 == Buttons
	if (ev.type == 1)
	{
		// 1 = ON/GPIO LOW
		if (ev.value == 1)
		{
			printf ("%s pressed ", button_names[ev.number]);
		}
		// 0 = OFF/GPIO HIGH
		else if (ev.value == 0)
		{
			printf ("%s released ", button_names[ev.number]);
		}
	}
	old_latch = playback_latch;
}

void write_keyboard_gpio (void)
{
	if (kb_ev.value == 1)
	{
		switch (kb_ev.code)
		{
			case KEY_UP:
				digitalWrite (Up_Pin, LOW);
				break;
			case KEY_DOWN:
				digitalWrite (Down_Pin, LOW);
				break;
			case KEY_LEFT:
				digitalWrite (Left_Pin, LOW);
				break;
			case KEY_RIGHT:
				digitalWrite (Right_Pin, LOW);
				break;
			case KEY_ENTER:
				digitalWrite (Start_Pin, LOW);
				break;
			case KEY_A:
				digitalWrite (B_Pin, LOW);
				break;
			case KEY_S:
				digitalWrite (A_Pin, LOW);
				break;
			case KEY_Q:
				digitalWrite (Y_Pin, LOW);
				break;
			case KEY_W:
				digitalWrite (X_Pin, LOW);
				break;
			case KEY_RIGHTSHIFT:
				digitalWrite (Select_Pin, LOW);
				break;
			case KEY_1:
				digitalWrite (TLeft_Pin, LOW);
				break;
			case KEY_2:
				digitalWrite (TRight_Pin, LOW);
				break;
			default:
				break;
		}
	}
		
	if (kb_ev.value == 0)
	{
		switch (kb_ev.code)
		{
			case KEY_UP:
				digitalWrite (Up_Pin, HIGH);
				break;
			case KEY_DOWN:
				digitalWrite (Down_Pin, HIGH);
				break;
			case KEY_LEFT:
				digitalWrite (Left_Pin, HIGH);
				break;
			case KEY_RIGHT:
				digitalWrite (Right_Pin, HIGH);
				break;
			case KEY_ENTER:
				digitalWrite (Start_Pin, HIGH);
				break;
			case KEY_A:
				digitalWrite (B_Pin, HIGH);
				break;
			case KEY_S:
				digitalWrite (A_Pin, HIGH);
				break;
			case KEY_Q:
				digitalWrite (Y_Pin, HIGH);
				break;
			case KEY_W:
				digitalWrite (X_Pin, HIGH);
				break;
			case KEY_RIGHTSHIFT:
				digitalWrite (Select_Pin, HIGH);
				break;
			case KEY_1:
				digitalWrite (TLeft_Pin, HIGH);
				break;
			case KEY_2:
				digitalWrite (TRight_Pin, HIGH);
				break;
			case KEY_ESC:
				running = 0;
				break;
			default:
				break;
		}
	}
}

void write_joystick_gpio (void)
{
	// Axis/Type 2 == dpad
	if (ev.type == 2)
	{
		switch (ev.number)
		{
			// X axis
			case 0:
				if (ev.value > 0)
				{
					digitalWrite (Right_Pin, LOW);
					digitalWrite (Left_Pin, HIGH);
				}
				else if (ev.value < 0)
				{
					digitalWrite (Left_Pin, LOW);
					digitalWrite (Right_Pin, HIGH);
				}
				else
				{
					digitalWrite (Right_Pin, HIGH);
					digitalWrite (Left_Pin, HIGH);
				}
				break;
			//Y Axis
			case 1:
				if (ev.value > 0)
				{
					digitalWrite (Down_Pin, LOW);
					digitalWrite (Up_Pin, HIGH);
				}
				else if (ev.value < 0)
				{
					digitalWrite (Up_Pin, LOW);
					digitalWrite (Down_Pin, HIGH);
				}
				else
				{
					digitalWrite (Up_Pin, HIGH);
					digitalWrite (Down_Pin, HIGH);
				}
				break;
			default:
				break;
			}
	}
	// Axis/Type 1 == Buttons
	if (ev.type == 1)
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
}

//Memory routines
int malloc_record_buffer (void)
{
	//Allocate memory for recorded inputs
	output_ptr = malloc (RECBUFSIZE);
	if (output_ptr == NULL)
	{
		printf("Unable to allocate %i bytes for record buffer\n", RECBUFSIZE);
		return 1;
	}
	else
		printf("%i bytes allocated for record buffer\n", RECBUFSIZE);
	return 0;
}

int append_mem_to_file (void)
{
	//Open output file
	FILE *output_file;
	long result;
	char key_buff[3];

	//Ask before appending contents
	printf("Append memory cotents to %s? y/n\n", filename);
	fgets(key_buff, sizeof(key_buff), stdin);

	if (strncmp (key_buff, "y\n", sizeof(key_buff)) != 0)
	{
		printf("Not writing memory contents to %s\n", filename);
		return 0;
	}
	//Reopen for appending
	output_file = fopen (filename, "a+b");
	if (output_file == NULL)
	{
		printf("Problem opening %s for appending\n", filename);
		return 1;
	}
	
	//Calculate how many bytes to append
	filesize = CHUNK_SIZE * filepos;
	printf ("Recorded %lu bytes of input\n", filesize);
	
	//Store to output file
	result = fwrite (output_ptr, 1, filesize, output_file);
	printf ("Appended %lu bytes to %s\n", result, filename);
	fclose(output_file);
	return 0;

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
	fwrite (&filemagic, 1, sizeof(filemagic), output_file);

	//Calculate filesize
	filesize = CHUNK_SIZE * filepos;
	printf ("Recorded %lu bytes of input\n", filesize);
	
	//Store to output file
	long result = fwrite (output_ptr, 1, filesize, output_file);
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
	
	//Try to detect magic number in header
	if (!lsnes_input_file)
	{
		long filemagic_check;
		fread (&filemagic_check, 1, sizeof(filemagic), input_file);
		if (filemagic_check != filemagic)
		{
			printf ("Incompatible file type detected!\n");
			return 1;
		}
		else 
			printf ("SNESBot filetype detected\n");
	}

	//Seek to the end
	fseek (input_file, 0, SEEK_END);
	//Find out how many bytes it is
	filesize = ftell(input_file);
	

	//Rewind it back to the start ready for reading into memory
	rewind (input_file);	
	
	//Strip off file magic number
	if (!lsnes_input_file)
	{
		filesize = filesize - sizeof(filemagic);
		//Seek past the magic number
		fseek (input_file, sizeof(filemagic), 0);
	}

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

void print_time_elapsed (void)
{
	gettimeofday (&end_time, NULL);
	int time_elapsed = (end_time.tv_sec - start_time.tv_sec);
	printf ("Elapsed time = %i seconds\n", time_elapsed);
}

void handle_exit (void)
{
	
	if (!debug_playback)
	{
		printf ("Clearing buttons\n");
		clear_buttons ();
	}

	if (record_input || record_after_playback)
	{
		print_time_elapsed ();
		printf("Total latches %lu \n", latch_counter);

		if (record_after_playback)
		{
			printf ("Adding memory contents to %s\n", filename);
			if (append_mem_to_file () == 1)
				printf("Problem adding memory contents to file\n");
		}
		else if (write_mem_into_file () == 1)
		{
			printf ("Writing memory contents to file %s\n", filename);
			printf("Problem writing memory contents to file\n");
		}
		//Free memory used by output file
		free (output_ptr);
	}
	else if (playback_input)
	{
		if (!debug_playback)
		{
			print_time_elapsed ();
			free (input_ptr);
		}
		
		printf ("\nFinished playback\n");
		//Free memory used by input file
		
		if (playback_latch > next_playback_latch)
			printf ("Total latches: %lu\n", playback_latch);
		else
			printf ("Total latches: %lu\n", next_playback_latch);
	}

	if (joystick_input)
	{
		fclose (js_dev);
	}

}

void lsnes_print_gpio (void)
{
	for (int i = 0; i < 12; i++)
	{
		printf("%i %s ", i, lsnes_button_names[i]);
	
		if ((lsnes_buttons & (1 << (i))) > 0)
			printf ("1\n");
		else
			printf ("0\n");
	}

}

void lsnes_write_gpio (void)
{
	for (int i = 0; i < 12; i++)
	{
		if ((lsnes_buttons & (1 << i)) > 0)
			digitalWrite(lsnes_input_map[i], LOW);
		else
			digitalWrite(lsnes_input_map[i], HIGH);
	}

}

void lsnes_playback_interrupt (void)
{
	if (!running)
		return;

	latch_counter++;
	
	if (wait_latches > latch_counter)
		return;
	//delayMicroseconds (192);
	// Copy button state into lsnes_buttons
	memcpy (&lsnes_buttons, input_ptr + (filepos * (sizeof(lsnes_buttons))), sizeof(lsnes_buttons));
	
	lsnes_buttons =	ntohs(lsnes_buttons);
	lsnes_write_gpio ();

	if (++filepos == filepos_end)
		running = 0;
}

// Precalculate end of file position
void calc_eof_position (void)
{
	if (lsnes_input_file)
		filepos_end = filesize / sizeof(lsnes_buttons);
	else
		filepos_end = filesize / CHUNK_SIZE;
}

void debug_lsnes_playback (void)
{
	//Copy the input file into memory
	if (read_file_into_mem () == 1)
	{
		printf("Problem copying input file to memory\n");
		return;
	}
	
	calc_eof_position ();
	filepos = start_pos;
	running = 1;

	while (running)
	{
		//Copy evdev state and latch into vars
		memcpy (&lsnes_buttons, input_ptr + (filepos * (sizeof(lsnes_buttons))), sizeof(lsnes_buttons));
		
		lsnes_buttons =	ntohs(lsnes_buttons);
		//Print the values
		if (lsnes_buttons != 0)
		{
			printf ("lsnes_buttons = %i filepos = %lu\n", lsnes_buttons, filepos);
			lsnes_print_gpio ();
		}
		//check to see if we are at the end of the input file
		if (++filepos == filepos_end)
			running = 0;
	}
}


void playback_interrupt (void)
{
	latch_counter++;
	if (!running || playback_finished)
		return;
		

	//Copy evdev and latch state into vars
	memcpy (&ev, input_ptr + (filepos * CHUNK_SIZE), sizeof(struct js_event));
	memcpy (&playback_latch, input_ptr + (sizeof(struct js_event) + (filepos * CHUNK_SIZE)), sizeof(latch_counter));


	//Wait until we are on the correct latch
	if (latch_counter == playback_latch)
	{
		write_joystick_gpio ();
		
		if (++filepos == filepos_end)
		{
			playback_finished = 1;
			running = 0;
			return;
		}
	
		//Copy the next playback_latch into var
		memcpy (&next_playback_latch, input_ptr + (sizeof(struct js_event) + (filepos * CHUNK_SIZE)), sizeof(latch_counter));
		if (latch_counter != playback_latch)
			printf("Lost sync!! expected: %lu got: %lu\n", playback_latch, latch_counter);
		//Wait for the correct latch
		//Make sure all events on that latch are done
		while (latch_counter == next_playback_latch)
		{
			//Copy the evdev state from mem and write to GPIO
			memcpy (&ev, input_ptr + (filepos * CHUNK_SIZE), sizeof(struct js_event));
			

			write_joystick_gpio ();
			
			if (++filepos == filepos_end)
			{
				running = 0;
				return;
			}
			
			//Copy the next playback_latch into var
			memcpy (&next_playback_latch, input_ptr + (sizeof(struct js_event) + (filepos * CHUNK_SIZE)), sizeof(latch_counter));
		}
	}
}

//Just used for recording at the moment
void latch_interrupt (void)
{
//	if (running)
	latch_counter++;
}

void setup_interrupts (void)
{
	//Time how long playback/record takes
	gettimeofday (&start_time, NULL);
	
	if (lsnes_input_file)
	{
		wait_for_first_latch ();
		wiringPiISR (Latch_Pin, INT_EDGE_RISING, &lsnes_playback_interrupt);
	}
	else if (playback_input && !playback_finished)
	{
		wait_for_first_latch ();
		//Start playback interrupt
		wiringPiISR (Latch_Pin, INT_EDGE_RISING, &playback_interrupt);
	}
	//Might not have to start latch_counter incrementing, 
	//as playback_interrupt is doing that for us now
	else if (record_input && !playback_finished)
	{
		wait_for_first_latch ();
		wiringPiISR (Latch_Pin, INT_EDGE_RISING, &latch_interrupt);
	}
}

void start_playback (void)
{	
	//Copy the input file into memory
	if (read_file_into_mem () == 1)
	{
		printf("Problem copying input file to memory\n");
		return;
	}
	else
	{
		// Precalculate end of file position
		calc_eof_position ();
		
		filepos = start_pos;
		running = 1;
		
		//Start playback
		setup_interrupts ();
		
		//Wait for file to finish playback
		while (running)
			delayMicroseconds (1000);
	}
}

void debug_playback_input (void)
{


	//Copy the input file into memory
	if (read_file_into_mem () == 1)
	{
		printf("Problem copying input file to memory\n");
		return;
	}
	
	calc_eof_position ();
	filepos = start_pos;
	running = 1;

	while (running)
	{
		//Copy evdev state and latch into vars
		memcpy (&ev, input_ptr + (filepos * CHUNK_SIZE), sizeof(struct js_event));
		memcpy (&playback_latch, input_ptr + sizeof(struct js_event) +(filepos * CHUNK_SIZE), sizeof(latch_counter));
		
		//Print the values
		print_joystick_input ();
		
		//check to see if we are at the end of the input file
		if (++filepos == filepos_end)	
			running = 0;
	}
}

int open_joystick_dev (void)
{
	js_dev = fopen ("/dev/input/js0", "r");
	if (js_dev == NULL)
		return 1;
	else
		return 0;
}

int open_keyboard_dev (void)
{
	kb_dev = fopen ("/dev/input/event0", "r");
	if (kb_dev == NULL)
		return 1;
	else
		return 0;
}

void record_joystick_inputs (void)
{
	int i;
	if (open_joystick_dev () == 1)
	{
		printf ("Couldn't open /dev/input/js0\n");
		return;
	}
	
	if (malloc_record_buffer () == 1)
	{
		printf("Problem allocating record buffer\n");
		return;
	}

	//Junk the first 18 reads from event queue
	//Pretty sure we don't need this as it is just the initial state
	//of the joystick, which will be set by clear_buttons anyway.
	for (i = 0; i < 18; i++)
		fread (&ev, 1, sizeof(struct js_event), js_dev);

	running = 1;
	filepos = 0;

	//Start recording latch counter
	setup_interrupts ();

	while (running)
	{
		// Read joystick inputs into ev struct
		fread (&ev, 1, sizeof(struct js_event), js_dev);
		
		//Copy ev struct and playback latch into record buffer
		memcpy (output_ptr + (filepos * CHUNK_SIZE), &ev, sizeof(struct js_event));
		memcpy (output_ptr + sizeof(struct js_event) + (filepos * CHUNK_SIZE), &latch_counter, sizeof(latch_counter));
		
		//Write the joystick vars to GPIO
		write_joystick_gpio ();
		filepos++;
	}
}

void live_joystick_input (void)
{	
	//Open joystick for reading
	if (open_joystick_dev () == 1)
	{
		printf ("Couldn't open /dev/input/js0\n");
		return;
	}
	running = 1;	
	while (running)
	{
		// Read joystick inputs into ev struct
		fread (&ev, 1, sizeof(struct js_event), js_dev);
		
		if (verbose)
			printf("axis %d, button %d, value %d, latch %lu\n", ev.type, ev.number,ev.value, playback_latch);
		
		//Write the joystick vars to GPIO
		write_joystick_gpio ();
	}
}


//TODO Crusty code below
void read_keyboard (void)
{
	if (open_keyboard_dev () == 1)
	{
		printf ("Couldn't open /dev/input/event0\n");
		return;
	}
	
	running = 1;
	setup_interrupts ();

	while (running)
	{
		//Read the events into the event struct
		fread (&kb_ev, 1, sizeof(struct input_event), kb_dev);
		
		if (verbose)
			printf("type %d, code %d, value %d\n", kb_ev.type, kb_ev.code, kb_ev.value);
		write_keyboard_gpio ();
	}
}

void snesbot (void)
{
	//Setup signal handler
	signal(SIGINT, signal_handler);
	if (keyboard_input)
	{
		printf("Reading input from keyboard\n");
		read_keyboard ();
	}
	
	if (record_input)
	{
		printf ("Recording input to %s\n", filename);
		record_joystick_inputs ();
	}
	else if (debug_playback)
	{
		if (lsnes_input_file)
			debug_lsnes_playback ();
		else
			debug_playback_input ();
	}
	else if (playback_input)
	{
		printf ("Playing back input from %s\n", filename);
		if (lsnes_input_file)
			printf ("lsnes polldump input file selected\n");
		if (wait_latches > 0)
			printf ("Waiting %i latches before starting playback\n", wait_latches);
		if (start_pos > 0)
			printf ("Starting on file position %i\n", start_pos);
		
		start_playback ();
		
		if (record_after_playback)
			record_joystick_inputs ();
	}
	else
	{
		printf("Reading input from joystick\n");
		live_joystick_input ();
	}
	handle_exit ();
	printf("Exiting\n");
}

void print_usage (void)
{
	printf("Usage:\n");
	printf("sudo snesbot [options] -f [filename]\n\n");
	printf("Options:\n");
	printf(" -c	Start recording after playback has finished\n");
	printf(" -d	debug playback (doesn't write to GPIO)\n");
	printf(" -f	read from/write to filename (default: %s)\n", filename);
	printf(" -h	show this help\n\n");
	printf(" -k	Read inputs from keyboard \n");
	printf(" -j	Read inputs from joystick \n");
	printf(" -l	Don't wait for latch pulse before starting\n");
	printf(" -r	Record input\n");
	printf(" -s X	Start on file position X (playback only)\n");
	printf(" -p	Playback input\n");
	printf(" -P	Use higher priority\n");
	printf(" -v	Verbose messages\n");
	printf(" -w X	Start playback after X latches\n");
}

int main (int argc, char **argv)
{
	int show_usage = 0;
	int high_priority = 0;

	printf("SNESBot v3\n");
	
	int c;
	while ((c = getopt (argc, argv, "cdf:jklLvhpPrs:w:")) != -1)
	{
		switch (c)
		{
			case 'c':
				playback_input = 1;
				record_after_playback = 1;
				break;
			case 'd':
				debug_playback = 1;
				playback_input = 1;
				verbose = 1;
				break;

			case 'f':
				filename = optarg;
				break;
				
			case 'j':
				joystick_input = 1;
				break;

			case 'k':
				keyboard_input = 1;
				break;

			case 'l':
				wait_for_latch = 0;
				break;
			case 'L':
				lsnes_input_file = 1;
				playback_input = 1;
				break;

			case 'p':
				//TODO Hacky hacky hacky....
			//	joystick_input = 1;
				playback_input = 1;
				break;

			case 'P':
				high_priority = 1;
				break;

			case 'r':
				record_input = 1;
				break;
			case 's':
				start_pos = atoi(optarg);
				break;

			case 'v':
				verbose = 1;
				break;

			default:
			case 'h':
				show_usage = 1;
				break;
			
			case '?':
				if (optopt == 'c')
					printf("Option %c requires an argument\n", optopt);
				break;
			case 'w':
				wait_latches = atoi (optarg);
				break;
			}
	}
		
	if (show_usage)
	{
		print_usage ();
		return 0;
	}
	
	if (high_priority)
	{	// Set priority
		printf("Setting high priority\n");
		piHiPri (45); sleep (1);
	}
	
	if (joystick_input && keyboard_input)
	{
		print_usage ();
		printf ("Choose only joystick OR keyboard input, not both\n");
		return 1;
	}
	// Default to joystick
	else if (!joystick_input && !keyboard_input && !debug_playback)
	{
		joystick_input = 1;
	}

	if (record_input && (playback_input | debug_playback))
	{
		print_usage ();
		printf ("Can't record and playback at the same time!\n");
		return 1;
	}
	
	if ((record_input | playback_input) && keyboard_input)
	{
		printf("Record/Playback from keyboard currently unsupported\n");
		return 1;
	}
	
	if ((wait_latches || start_pos > 0) && !playback_input)
	{
		printf("-w and -s options only valid during playback!\n");
		return 1;
	}


	if (record_input)
	{
		printf("Record mode\n");
		if (keyboard_input)
			printf ("Keyboard input selected\n");
		else if (joystick_input)
			printf ("Joystick input selected\n");
	}
	else if (playback_input)
		printf("Playback mode\n");
	else if (debug_playback)
		printf("Debug playback mode\n");
	else
		printf("Live Input mode\n");
	
	if (!debug_playback)
	{
		
		printf("Initialising GPIO\n");
		if (init_gpio ())
		{
			printf ("Error initilising GPIO\n");
			return 1;
		}
	}
	else
	{
		//User might set options that don't mean 
		//anything during debug playback
		joystick_input = 0;
		keyboard_input = 0;
	}
	
	
	//Main loop
	snesbot ();
	return 0;
}
