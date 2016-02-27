#include "snes.h"

int main (int argc, char** argv)
{
  if (setupUSBJoystick (&p1, "/dev/input/js0") != 0)
  {
    fprintf (stderr, "Error setting up USB joystick\n");
    return 1;
  }
  
  p1.mapping.x_axis = 0;
  p1.mapping.y_axis = 1;
  p1.mapping.b = 0;
  p1.mapping.y = 1;
  p1.mapping.select = 2;
  p1.mapping.start = 3;
  p1.mapping.a = 4;
  p1.mapping.x = 5;
  p1.mapping.l = 6;
  p1.mapping.r = 7;
  while (1)
  {
  if (readUSBJoystick (&p1) != 0)
  {
    return 1;
  }
  }
  return 0;
}
