#include "snes.h"

int main (int argc, char** argv)
{
  if (setupUSBJoystick () != 0)
  {
    fprintf (stderr, "Error setting up USB joystick\n");
    return 1;
  }
  
  p1.mapping.x_axis = 0;
  p1.mapping.y_axis = 1;
  while (1)
  {
  if (readUSBJoystick () != 0)
  {
    fprintf (stderr, "Error joy rad\n");
    return 1;
  }
  }
  return 0;
}
