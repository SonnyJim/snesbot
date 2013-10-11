
//TODO 
//Recorded filesize is smaller than memory contents?
//Losing sync after a couple of minutes?
// Fix argv/argc, use getopt.h?
//And for that matter, use errno.h
//Improve general program flow
//*** glibc detected *** ./snesbot: double free or corruption (!prev): 0x01907178 ***
#include <wiringPi.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <linux/input.h>
#include <linux/joystick.h>
#include <fcntl.h>
#include <string.h>
#include "snesbot.h"
#include <sys/time.h>

//Command line variables
int keyboard_input = 0;
int joystick_input = 0;
int record_input = 0;
int playback_input = 0;
int verbose = 0;
int debug_playback = 0;
int wait_for_latch = 0;

struct timeval start_time, end_time;

char *filename = "snesbot.rec";

//Number of SNES latches read by GPIO latch pin
unsigned long int latch_counter = 0;

//Handle when we get more than one event on the same latch
unsigned long int old_latch = 0;

int running = 0;

//Latency measurement
int total_latency = 0;

FILE *js_dev;
FILE *kb_dev;

//Pointers to input/output buffers
void *input_ptr;
void *output_ptr;

//Joystick evdev structure
static struct js_event ev;
static struct input_event kb_ev;

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

//Button mapping for USB PS1 controller, X/Y axis is handled differently
const int psx_mapping[14] = { X_Pin, A_Pin, B_Pin, Y_Pin, TLeft_Pin, TRight_Pin, TLeft_Pin, TRight_Pin, Select_Pin, Start_Pin };

const char button_names[14][7] = { "X", "A", "B", "Y", "Exit", "TRight", "TLeft", "TRight", "Select", "Start" };

void wait_for_first_latch (void)
{
	if (wait_for_latch && !debug_playback)
	{
		printf("Waiting for first latch\n");
		while (digitalRead (Latch_Pin) == 0);
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
	pullUpDnControl (Latch_Pin, PUD_OFF);
	
	for (i = 0; i < NUMBUTTONS; i++)
	{
		pinMode (buttons[i], OUTPUT);
		pullUpDnControl (buttons[i], PUD_UP);
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
			//R1 is quit
			if (ev.number == 4)
				running = 0;
			else
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

int write_mem_into_file (void)
{
	//Open output file
	FILE *output_file = fopen (filename, "w");
	
	if (output_file == NULL)
	{
		printf("Problem opening %s for writing\n", filename);
		return 1;
	}
	
	//Calculate filesize
	filesize = (sizeof(struct js_event) + sizeof(latch_counter)) * filepos;
	printf ("Recorded %lu bytes\n", filesize);
	
	//Store to output file
	long result = fwrite (output_ptr, 1, filesize, output_file);
	printf ("Wrote %lu bytes to %s\n", result, filename);
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

	//Seek to the end
	fseek (input_file, 0, SEEK_END);
	//Find out how many bytes it is
	filesize = ftell(input_file);
	
	//Rewind it back to the start ready for reading into memory
	rewind (input_file);	
	
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

	if (record_input)
	{
		print_time_elapsed ();
		printf("Total latches %lu \n", latch_counter);

		printf ("Writing memory contents to file %s\n", filename);
		if (write_mem_into_file () == 1)
			printf("Problem writing memory contents to file\n");
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

void playback_interrupt (void)
{
	if (!running)
		return;
		
	latch_counter++;

	//Copy evdev and latch state into vars
	memcpy (&ev, input_ptr + (filepos * (sizeof(struct js_event) + sizeof(latch_counter))), sizeof(struct js_event));
	memcpy (&playback_latch, input_ptr + (sizeof(struct js_event) + (filepos * (sizeof(struct js_event) + sizeof(latch_counter)))), sizeof(latch_counter));


	//Wait until we are on the correct latch
	if (latch_counter >= playback_latch)
	{
		write_joystick_gpio ();
		
		if (++filepos == filepos_end)
		{
			return;
			running = 0;
		}
	
		//Copy the next playback_latch into var
		memcpy (&next_playback_latch, input_ptr + (sizeof(struct js_event) + (filepos * (sizeof(struct js_event) + sizeof(latch_counter)))), sizeof(latch_counter));
		if (latch_counter != playback_latch)
			printf("Lost sync!! expected: %lu got: %lu\n", playback_latch, latch_counter);
		//Wait for the correct latch
		//Make sure all events on that latch are done
		while (latch_counter >= next_playback_latch)
		{
			//Copy the evdev state from mem and write to GPIO
			memcpy (&ev, input_ptr + (filepos * (sizeof(struct js_event) + sizeof(latch_counter))), sizeof(struct js_event));
			

			write_joystick_gpio ();
			
			if (++filepos == filepos_end)
			{
				return;
				running = 0;
			}
			
			//Copy the next playback_latch into var
			memcpy (&next_playback_latch, input_ptr + (sizeof(struct js_event) + (filepos * (sizeof(struct js_event) + sizeof(latch_counter)))), sizeof(latch_counter));
		}
	}
}

void latch_interrupt (void)
{
	if (running)
		latch_counter++;
}

void setup_interrupts (void)
{
	//Time how long playback/record takes
	gettimeofday (&start_time, NULL);
	
	if (playback_input)
	{
		wait_for_first_latch ();
		wiringPiISR (Latch_Pin, INT_EDGE_RISING, &playback_interrupt);
	}
	else if (record_input)
	{
		wait_for_first_latch ();
		wiringPiISR (Latch_Pin, INT_EDGE_RISING, &latch_interrupt);
	}
}

// Precalculate end of file position
void calc_eof_position (void)
{
	filepos_end = filesize / (sizeof(struct js_event) + sizeof(latch_counter));
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
		filepos = 0;
		running = 1;
		
		//Start playback
		setup_interrupts ();
		
		//Wait for file to finish playback
		while (running)
			sleep (1);
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
	
	filepos = 0;
	
	while (1)
	{
		//Copy evdev state and latch into vars
		memcpy (&ev, input_ptr + (filepos * (sizeof(struct js_event) + sizeof(latch_counter))), sizeof(struct js_event));
		memcpy (&playback_latch, input_ptr + sizeof(struct js_event) +(filepos * (sizeof(struct js_event) + sizeof(latch_counter))), sizeof(latch_counter));
		
		//Print the values
		print_joystick_input ();
		
		//check to see if we are at the end of the input file
		if (++filepos == filepos_end)	
			break;
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
		memcpy (output_ptr + (filepos * (sizeof(struct js_event) + sizeof(latch_counter))), &ev, sizeof(struct js_event));
		memcpy (output_ptr + sizeof(struct js_event) + (filepos * (sizeof(struct js_event) + sizeof(latch_counter))), &latch_counter, sizeof(latch_counter));
		
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
		debug_playback_input ();
	}
	else if (playback_input)
	{
		printf ("Playing back input from %s\n", filename);
		start_playback ();
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
	printf(" -P	Use higher priority\n");
	printf(" -k	Read inputs from keyboard \n");
	printf(" -j	Read inputs from joystick \n");
	printf(" -l	Wait for latch pulse before starting\n");
	printf(" -r	Record input\n");
	printf(" -p	Playback input\n");
	printf(" -f	read from/write to filename (default: %s)\n", filename);
	printf(" -d	debug playback (doesn't write to GPIO)\n");
	printf(" -v	Verbose messages\n");
	printf(" -h	show this help\n\n");
}

int main (int argc, char *argv[])
{
	int show_usage = 0;
	int high_priority = 0;

	printf("SNESBot v3\n");
	while ((argc > 1) && (argv[1][0] == '-' ))
	{
		switch (argv[1][1])
			{

				case 'd':
					debug_playback = 1;
					playback_input = 1;
					verbose = 1;
					break;

				case 'f':
					filename = &argv[1][2];
					break;
				
				case 'j':
					joystick_input = 1;
					break;

				case 'k':
					keyboard_input = 1;
					break;

				case 'l':
					wait_for_latch = 1;
					break;

				case 'p':
					playback_input = 1;
					break;

				case 'P':
					high_priority = 1;
					break;

				case 'r':
					record_input = 1;
					break;

				case 'v':
					verbose = 1;
					break;

				default:
				case 'h':
					show_usage = 1;
					break;
			}
		++argv;
		--argc;
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
	else if (!joystick_input && !keyboard_input && !debug_playback)
	{
		print_usage ();
		printf ("Choose either joystick (-j) or keyboard (-k) input\n");
		return 1;
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

	if (record_input)
		printf("Record mode\n");
	else if (playback_input)
		printf("Playback mode\n");
	else if (debug_playback)
		printf("Debug playback mode\n");

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
