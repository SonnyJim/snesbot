#include "snes.h"

void print_usage (void)
{
	printf("Usage:\n");
	printf("sudo snes [options] -f [filename]\n\n");
	printf("Options:\n");
	printf(" -r       Record\n");
	printf(" -p       Playback\n");
}


int interrupt_enable (void)
{
  return wiringPiISR (PIN_LIN, INT_EDGE_FALLING, &latch_interrupt);
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
  pullUpDnControl (PIN_LIN, PUD_DOWN);
  
  fprintf (stdout, "Setting up mcp23017\n");
  mcp23017Setup (PIN_BASE, 0x20);
  clear_all_buttons ();
  
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

void set_joystick_mapping (void)
{
  p1.joytype = JOY_USB;
  p1.mapping.x_axis = 0;
  p1.mapping.y_axis = 1;
  p1.mapping.b = 2;
  p1.mapping.y = 3;
  p1.mapping.select = 8;
  p1.mapping.start = 9;
  p1.mapping.a = 1;
  p1.mapping.x = 0;
  p1.mapping.l = 4;
  p1.mapping.r = 5;
}

int read_options (int argc, char **argv)
{
  int c;
  filename = "snes.rec";
  set_joystick_mapping ();
  while ((c = getopt (argc, argv, "rpf:")) != -1)
  {
      switch (c)
      {
        case 'r':
          if (botcfg.state == STATE_PLAYBACK)
          {
            fprintf (stderr, "Error, both playback and record specified\n");
            return 1;
          }
          botcfg.state = STATE_RECORDING;
          break;
        case 'p':
          if (botcfg.state == STATE_RECORDING)
          {
            fprintf (stderr, "Error, both playback and record specified\n");
            return 1;
          }
          botcfg.state = STATE_PLAYBACK;
          break;
        case 'f':
          botcfg.outfile = optarg;
          botcfg.infile = optarg;
          break;
      }
  }

  return 0;
}

void signal_handler (int signal)
{
	printf ("\nSIGINT detected, exiting\n");
        botcfg.state = STATE_EXITING;
}