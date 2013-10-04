#include <wiringPi.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <linux/input.h>
#include <linux/joystick.h>
#include <fcntl.h>
#include <string.h>

// 			WiringPi	P1 Pin
#define B_Pin		 8 		// 3
#define Y_Pin		 9 		// 5
#define Select_Pin	 7 		// 7
#define Start_Pin	 15 		// 8
#define Up_Pin		 16 		// 10
#define Down_Pin	 0 		// 11
#define Left_Pin	 1 		// 12
#define Right_Pin	 2 		// 13
#define A_Pin		 3 		// 15
#define X_Pin		 4 		// 16
#define TLeft_Pin	 5 		// 18
#define TRight_Pin	 12 		// 19

#define Latch_Pin	 13 		// 21

#define RECBUFSIZE	1024 * 16 //Set record buffer to 16MB

int keyboard_input = 0;
int joystick_input = 0;
int record_input = 0;
int playback_input = 0;
int verbose = 0;
int debug_playback = 0;
char *filename = "snesbot.rec";
long filesize = 0;

//Number of SNES latches read by GPIO latch pin
int latch_counter = 0;

//Handle when we get more than one event on the same latch
int old_latch = 0;

int running = 0;

//Latency measurement
int total_latency = 0;

//Crusty code, used by keyboard only
int in_file;

FILE *js_dev;

//Pointers to input/output buffers
void *input_ptr;
void *output_ptr;

//Joystick evdev structure
static struct js_event ev;

// Current playback latch
static int playback_latch = 0;

//Whereabouts in the playback/record file we are
int filepos = 0;

void latch_interrupt (void)
{
	latch_counter++;
}

void setup_interrupts (void)
{
	wiringPiISR (Latch_Pin, INT_EDGE_RISING, &latch_interrupt);
}

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

void print_joystick_input (void)
{
	if (old_latch != playback_latch)
		printf ("\n%i ", playback_latch);
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
					printf ("Centre X");
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
					printf ("Centre Y");
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
			switch (ev.number)
			{
				case 8:
					printf ("Select");
					break;
				case 9:
					printf ("Start");
					break;
				case 2:
					printf("B");
					break;
				case 1:
					printf ("A");
					break;
				case 3:
					printf ("Y");
					break;
				case 0:
					printf ("X");
					break;
				case 6:
					printf ("TL");
					break;
				case 7:
					printf ("TR");
					break;
				case 4: 
				case 5:
					printf ("Exit");
					running = 0;
					break;
				default:
					break;
			}
			printf (" pressed ");
		}
		// 0 = OFF/GPIO HIGH
		if (ev.value == 0)
		{
			switch (ev.number)
			{
				case 8:
					printf ("Select");
					break;
				case 9:
					printf ("Start");
					break;
				case 2:
					printf ("B");
					break;
				case 1:
					printf ("A");
					break;
				case 3:
					printf ("Y");
					break;
				case 0:
					printf ("X");
					break;
				case 4:
				case 6:
					printf ("TL");
					break;
				case 5:
				case 7:
					printf ("TR");
					break;
				default:
					break;
			}
			printf (" released");
		}
	}
	old_latch = playback_latch;
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
			switch (ev.number)
			{
				case 8:
					digitalWrite (Select_Pin, LOW);
					break;
				case 9:
					digitalWrite (Start_Pin, LOW);
					break;
				case 2:
					digitalWrite (B_Pin, LOW);
					break;
				case 1:
					digitalWrite (A_Pin, LOW);
					break;
				case 3:
					digitalWrite (Y_Pin, LOW);
					break;
				case 0:
					digitalWrite (X_Pin, LOW);
					break;
				case 6:
					digitalWrite (TLeft_Pin, LOW);
					break;
				case 7:
					digitalWrite (TRight_Pin, LOW);
					break;
				case 4: 
				case 5:
					running = 0;
					break;
				default:
					break;
			}
		}
		// 0 = OFF/GPIO HIGH
		if (ev.value == 0)
		{
			switch (ev.number)
			{
				case 8:
					digitalWrite (Select_Pin, HIGH);
					break;
				case 9:
					digitalWrite (Start_Pin, HIGH);
					break;
				case 2:
					digitalWrite (B_Pin, HIGH);
					break;
				case 1:
					digitalWrite (A_Pin, HIGH);
					break;
				case 3:
					digitalWrite (Y_Pin, HIGH);
					break;
				case 0:
					digitalWrite (X_Pin, HIGH);
					break;
				case 4:
				case 6:
					digitalWrite (TLeft_Pin, HIGH);
					break;
				case 5:
				case 7:
					digitalWrite (TRight_Pin, HIGH);
					break;
				default:
					break;
			}
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
	filesize = (sizeof(struct js_event) + sizeof(int)) * filepos;
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


void handle_exit (void)
{
	if (!debug_playback)
	{
		printf ("Clearing buttons\n");
		clear_buttons ();
	}

	if (record_input)
	{
		printf ("Finished recording\n");
		printf ("Writing memory contents to file %s\n", filename);
		if (write_mem_into_file () == 1)
			printf("Problem writing memory contents to file\n");
		//Free memory used by output file
		free (output_ptr);
	}
	else if (playback_input)
	{
		printf ("Finished playback\n");
		if (verbose)
			printf("Total latency %i\n", total_latency);
		//Free memory used by input file
		free (input_ptr);
	}

	if (joystick_input)
	{
		fclose (js_dev);
	}

}

void playback_joystick_inputs (void)
{
	//Copy the input file into memory
	if (read_file_into_mem () == 1)
	{
		printf("Problem copying input file to memory\n");
		return;
	}

	filepos = 0;
	int drift = 0;
	// Precalculate end of file position
	int filepos_end = filesize / (sizeof(struct js_event) + sizeof(int));
	//printf ("filepos_end %i\n", filepos_end);
	
	//Start latch interrupt counter
	setup_interrupts ();
	while (1)
	{
		//Copy evdev and latch state into vars
		memcpy (&ev, input_ptr + (filepos * (sizeof(struct js_event) + sizeof(int))), sizeof(struct js_event));
		memcpy (&playback_latch, input_ptr + (sizeof(struct js_event) + (filepos * (sizeof(struct js_event) + sizeof(int)))), sizeof(int));
		
		//printf ("Waiting for latch %i\n", playback_latch);
		//Wait for the correct SNES latch
		while ((latch_counter < playback_latch) &&
				(old_latch != playback_latch))
				usleep (1);
		old_latch = playback_latch;
		if (verbose)
		{
			total_latency += latch_counter - playback_latch;
			//printf ("Drift %i\n", drift);
			//printf("axis %d, button %d, value %d, latch %i\n", ev.type, ev.number,ev.value, playback_latch);
		}
		//Write the vars to GPIO
		write_joystick_gpio ();
		//check to see if we are at the end of the input file
		if (++filepos == filepos_end)	
			break;
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
	
	int filepos_end = filesize / (sizeof(struct js_event) + sizeof(int));
	//printf ("filepos_end %i\n", filepos_end);
	filepos = 0;
	while (1)
	{
		//Copy evdev state and latch into vars
		memcpy (&ev, input_ptr + (filepos * (sizeof(struct js_event) + sizeof(int))), sizeof(struct js_event));
		memcpy (&playback_latch, input_ptr + sizeof(struct js_event) +(filepos * (sizeof(struct js_event) + sizeof(int))), sizeof(int));
		
		//Print the values
		print_joystick_input ();
	//	printf("axis %d, button %d, value %d, latch %i\n", ev.type, ev.number,ev.value, playback_latch);
	//	printf ("filepos %i\n", filepos);
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
	
	//Start latch interrupt counter
	setup_interrupts ();
	while (running)
	{
		// Read joystick inputs into ev struct
		fread (&ev, 1, sizeof(struct js_event), js_dev);
		
		//Copy ev struct and playback latch into record buffer
		memcpy (output_ptr + (filepos * (sizeof(struct js_event) + sizeof(int))), &ev, sizeof(struct js_event));
		memcpy (output_ptr + sizeof(struct js_event) + (filepos * (sizeof(struct js_event) + sizeof(int))), &latch_counter, sizeof(int));
		
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
		//Write the joystick vars to GPIO
		write_joystick_gpio ();
	}
}


//TODO Crusty code below
void read_keyboard (void)
{
	int in_file;
	struct input_event ev[2];

	in_file = open("/dev/input/event0", O_RDONLY);
	setup_interrupts ();

	for (;;)
	{
		read (in_file, &ev, sizeof(struct input_event) * 2);
		if (verbose)
			printf("type %d, code %d, value %d\n", ev[1].type, ev[1].code,ev[1].value);
		
		if (ev[1].value == 1)
		{
			switch (ev[1].code)
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
		
		if (ev[1].value == 0)
		{
			switch (ev[1].code)
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

				default:
					break;
			}
		}
	}
}


int init_gpio (void)
{
	if (wiringPiSetup () == -1)
		return 1;
	
	//Set up pins
	pinMode (Latch_Pin, INPUT);
	
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

void snesbot (void)
{
	printf("Go go SNESBot\n");
	if (keyboard_input)
	{
		printf("Reading input from keyboard\n");
		read_keyboard ();
	}
	else if (joystick_input)
	{
		if (record_input)
		{
			printf ("Recording input to %s\n", filename);
			record_joystick_inputs ();
		}
		else if (playback_input)
		{
			printf ("Playing back input from %s\n", filename);
			playback_joystick_inputs ();
		}
		else
		{
			printf("Reading input from joystick\n");
			live_joystick_input ();
		}
	}
	else if (debug_playback)
	{
		debug_playback_input ();
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
	int wait_for_latch = 0;

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
		piHiPri (40); sleep (1);
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

	if (debug_playback)
	{
		record_input = 0;
		playback_input = 0;
		joystick_input = 0;
		keyboard_input = 0;
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

	if (wait_for_latch && !debug_playback)
	{
		printf("Waiting for first latch\n");
		while (digitalRead (Latch_Pin) == 0);
	}
	//Main loop
	snesbot ();
	return 0;
}
