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

#include "subs.h"
#include "joystick.h"

#define PIN_LIN 15 //wiring Pi 15 reads the latch from the SNES
#define PIN_P1CLK 7 //Which GPIO are used for reading in the P1 SNES joystick
#define PIN_P1LAT 0
#define PIN_P1DAT 2
#define PIN_P2CLK 4 //Which GPIO are used for reading in the P1 SNES joystick
#define PIN_P2LAT 5
#define PIN_P2DAT 12
#define PIN_BRD_OK 3 //Loopback from pin 1/3.3v to see if board is connected
#define PIN_SNES_VCC 25


#define	BLANK	"|      "

//Set the pin base for the MCP23017
#define PIN_BASE 120

#define	SNES_B		0x8000
#define	SNES_Y		0x4000
#define	SNES_SELECT	0x2000
#define	SNES_START	0x1000
#define	SNES_UP		0x0800
#define	SNES_DOWN	0x0400
#define	SNES_LEFT	0x0200
#define	SNES_RIGHT	0x0100
#define	SNES_A		0x0080
#define	SNES_X		0x0040
#define	SNES_L		0x0020
#define	SNES_R		0x0010
#define	SNES_BIT14	0x0008
#define	SNES_BIT15	0x0004
#define	SNES_BIT16	0x0002

//Set record buffer to 16MB
#define RECBUFSIZE	1024 * 1024 * 16 
//Magic file number
#define FILEMAGIC	0xBEC16260
#define CHUNK_SIZE (sizeof(unsigned long int) + sizeof(unsigned short int))

//Magic number for SNESBot recorded files
extern long filemagic;
//Default filename
char *filename;

typedef enum {STATE_INIT, STATE_RUNNING, STATE_MACRO, STATE_RECORDING, STATE_PLAYBACK, STATE_EXITING} states_t;
states_t state;

unsigned long int latch_counter;
void latch_interrupt (void);
int write_mem_into_file (void);
int read_file_into_mem (void);
void clear_all_buttons (void);
extern int record_start (void);
extern int playback_start (void);
extern void playback_read_next ();
extern void record_save (void);

void record_player_inputs ();
void signal_handler (int signal);
void wait_for_snes_powerup (void);
void time_start (void);
void time_stop (void);
//cfg.c

int setup ();
int port_setup (void);
int read_options (int argc, char **argv);
int interrupt_enable (void);

int is_a_pi (void);

typedef enum {JOY_NONE, JOY_GPIO, JOY_USB} joytype_t;



//Use this to store button mapping config for USB-> SNES
struct joymap_t {
    int x_axis;
    int y_axis;
    int b;
    int y;
    int select;
    int start;
    int a;
    int x;
    int l;
    int r;
    int macro; //Button for macro shifting
};

struct conf_t {
  int snesgpio_num; //How many SNES controllers we have plugged in
  int wait_for_power;
  signed int skew;
  char* outfile;
  char* infile;
  char* subfile;
  states_t state;
} botcfg;

struct player_t {
  int num; //Which player we are
  int macro; //Which macro we are due to play
  joytype_t joytype;
  int joygpio; //GPIO port, 0 = p1, 1 = p2
  unsigned short int input;
  unsigned short int input_old;
  FILE* fp; //fd for /dev/input/jsX
  struct joymap_t mapping;
};

struct player_t p1; 
struct player_t p2;


void print_buttons (unsigned short int p1, unsigned short int p2);

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
  unsigned long int start_latch;
};

struct timeval start_time, stop_time;
struct record_t record;
struct playback_t playback;
struct playback_t macro1;


void pb_read_next (struct playback_t* pb);
int read_macro_into_mem (char* filename, struct playback_t* pb);
void macro_start (struct playback_t* macro);
int remove_pid (void);
int check_pid (void);
