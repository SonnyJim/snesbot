#include <wiringPi.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <linux/input.h>
#include <linux/joystick.h>
#include <sys/types.h>
#include <sys/stat.h>
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
#define Right_Pin	 2 		// 13 Had a weird problem
#define A_Pin		 3 		// 15
#define X_Pin		 4 		// 16
#define TLeft_Pin	 5 		// 18
#define TRight_Pin	 12 		// 19

#define Latch_Pin	 13 		// 21

int keyboard_input = 0;
int joystick_input = 0;
int record_input = 0;
int playback_input = 0;
int verbose = 0;
int debug_playback = 0;
char *filename = "snesbot.rec";
long filesize = 0;

//Number of SNES latchs read by GPIO latch pin
int latch_counter = 0;
int running = 0;

//Latency measurements
int total_latency = 0;
int drift = 0;

int in_file;
//FILE *in_file;
int out_file;
//FILE *out_file;

void *input_ptr;

struct js_event ev;


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

inline void write_joystick_gpio (void)
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
		// 1 = ON/LOW, 0 = OFF/HIGH
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
					delayMicroseconds (16);
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

void handle_exit (void)
{
	if (record_input)
	{
		printf ("Writing to file %s\n", filename);
	//	fclose(out_file);
		close(out_file);
	}
	else if (playback_input)
	{
		printf ("Finished playback\n");
		if (verbose)
			printf ("Total latency %i\n", total_latency);
		//fclose(in_file);
		free (input_ptr);
		//close(in_file);
	}
}

long int read_input_file_size (void)
{
	FILE *input_file = fopen (filename, "rb");
	fseek (input_file, 0, SEEK_END);
	filesize = ftell(input_file);
	fclose (input_file);
	return filesize;
}

void read_file_into_mem (void)
{
	FILE *input_file = fopen (filename, "rb");
	filesize = read_input_file_size ();
	
	input_ptr = malloc (filesize);
	if (input_ptr == NULL)
		printf("malloc of %lu bytes failed\n", filesize);
	else
		printf("malloc of %lu bytes succeeded\n", filesize);
	fread (input_ptr, 1, filesize, input_file);
	fclose (input_file);
}

void read_joystick (void)
{
	// Current playback latch
	int playback_latch = 0;

	//Whereabout in the playback file we are
	int filepos = 0;
	
	if (playback_input)
		read_file_into_mem ();
	else	
	{
		in_file = open("/dev/input/js0", O_RDONLY);
		if (in_file == -1)
			printf ("Couldn't open /dev/input/js0\n");
			return;
	}
	if (record_input)
	{
		int i;
		out_file = open(filename, O_WRONLY | O_CREAT, 0664);
		//out_file = fopen(filename, "w");
		//Junk the first 18 reads from event queue?
		for (i = 0; i < 18; i++)
			//fread (&ev, 1, sizeof(struct js_event), in_file);
			read (in_file, &ev, sizeof(struct js_event));
	}
	
	//Start the SNES latch counter
	setup_interrupts ();
	
	running = 1;	
	while (running)
	{
		if (playback_input)
		{
			//Copy evdev state and latch into vars
			memcpy (&ev, input_ptr + (filepos * (sizeof(struct js_event) + sizeof(int))), sizeof(struct js_event));
			memcpy (&playback_latch, input_ptr + sizeof(struct js_event) +(filepos * (sizeof(struct js_event) + sizeof(int))), sizeof(int));
			
			//check to see if we are at the end
			if (filepos * (sizeof(struct js_event) + sizeof(int)) >= filesize)	
				running = 0;
			
			filepos++;
			if (verbose)
			{
				printf ("Waiting for latch %i\n", playback_latch);
				printf("Current SNES latch %i\n", latch_counter);
				total_latency += (latch_counter - playback_latch);
				drift = latch_counter - playback_latch;
			}
			//Wait for the correct SNES latch
			while (((latch_counter)< playback_latch) && !debug_playback)
				delayMicroseconds (1);
		}
		else 
		// Read inputs into ev struct
		read (in_file, &ev, sizeof(struct js_event));
		//fread (&ev, 1, sizeof(struct js_event), input_ptr);
		
		if (record_input)
		{
			//Write input to file
			write (out_file, &ev, sizeof(struct js_event));
			//fwrite (&ev, 1, sizeof(struct js_event), out_file);
			//Write current latch to file
			write (out_file, &latch_counter, sizeof(int));
			//fwrite (&latch_counter, 1, sizeof(int), out_file);
			if (verbose)
			{
				printf("Latch counter %i\n", latch_counter);
			}
		}
		if (verbose && playback_input)
		{
			printf("axis %d, button %d, value %d\n", ev.type, ev.number,ev.value);
			printf("Wanted latch %i, got latch %i\n", playback_latch, latch_counter);
			printf ("Drift %i\n", drift);
		}
		write_joystick_gpio ();
	}
	handle_exit ();
}

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
	clear_buttons ();
	
	
	printf("Go go SNESBot\n");
	if (keyboard_input)
	{
		printf("Reading input from keyboard\n");
		read_keyboard ();
	}
	else if (joystick_input)
	{
		if (record_input)
			printf ("Recording input to %s\n", filename);
		else if (playback_input)
			printf ("Playing back input from %s\n", filename);
		else
			printf("Reading input from joystick\n");
		read_joystick ();
	}

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
	printf(" -d	debug playback (doesn't wait for SNES latch)\n");
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
		piHiPri (10); sleep (1);
	}
	
	if (joystick_input && keyboard_input)
	{
		print_usage ();
		printf ("Choose only joystick OR keyboard input, not both\n");
		return 1;
	}
	else if (!joystick_input && !keyboard_input)
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
	
	if (record_input)
		printf("Record mode\n");
	else if (playback_input)
		printf("Playback mode\n");
	
	printf("Initialising GPIO\n");
	if (init_gpio ())
	{
		printf ("Error initilising GPIO\n");
		return 1;
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
