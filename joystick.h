#define	MAX_SNES_JOYSTICKS	8
#define	PULSE_TIME	5

#include <linux/input.h>
#include <linux/joystick.h>

extern struct player_t player;

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


struct snesPinsStruct
{
  unsigned int cPin, dPin, lPin ;
};

void process_ev (struct js_event ev, struct player_t* player);
struct snesPinsStruct snesPins [MAX_SNES_JOYSTICKS] ;

int gpio_joysticks;

void configure_player_buttons (struct player_t* player);
int read_joystick_cfg (void);
int save_joystick_cfg (void);
void check_joystick_inputs (void);
void read_joystick_inputs (void);
int joystick_setup (void);
int setupUSBJoystick (struct player_t* player, char* device);
int readUSBJoystick (struct player_t* player);
