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
//Pointers to input/output buffers
char *input_ptr;
char *ptr;
//Magic number for SNESBot recorded files
extern long filemagic;
//Default filename
char *filename;


extern unsigned short int inputs[16];

typedef enum {STATE_INIT, STATE_RUNNING, STATE_RECORDING, STATE_PLAYBACK, STATE_EXITING} states_t;

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
void read_player_inputs ();
void signal_handler (int signal);
void wait_for_snes_powerup (void);
void time_start (void);
void time_stop (void);
//cfg.c

int setup ();
int port_setup (void);
int joystick_setup (void);
void check_player_inputs (void);
void read_player_inputs (void);
int read_options (int argc, char **argv);

int is_a_pi (void);

struct conf_t {
  int snesgpio_num; //How many SNES controllers we have plugged in
  char* outfile;
  char* infile;
  states_t state;
};

struct joy_t {
  int pisnes_num;
  unsigned short int input;
  unsigned short int input_old;
};

struct joy_t p1;
struct joy_t p2;
struct conf_t botcfg;

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
};

struct timeval start_time, stop_time;
struct record_t record;
struct playback_t playback;
