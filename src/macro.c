/*
 * macro.c:
 *    Macros for joystick events
 *
 * Copyright (c) 2016 Ewan Meadows
 ***********************************************************************
 *
 *    This is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU Lesser General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    This is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU Lesser General Public License for more details.
 *
 *    You should have received a copy of the GNU Lesser General Public License
 *    along with this.  If not, see <http://www.gnu.org/licenses/>.
 ***********************************************************************
 */


#include "snesbot.h"

void macro_start (struct playback_t* macro)
{
  macro->filepos = 0;

  botcfg.state = STATE_MACRO;
  //Load up the initial inputs
  macro->start_latch = latch_counter;
  pb_read_next (macro);
}

int read_macro_into_mem (char* filename, struct playback_t* pb)
{
  FILE *input_file = fopen (filename, "rb");
  if (input_file == NULL)
  {
    fprintf (stderr, "Couldn't open %s, %s\n", filename, strerror(errno));
    return 1;
  }
  
  fseek (input_file, 0, SEEK_END);
  //Find out how many bytes it is
  pb->filesize = ftell(input_file);
  //Rewind it back to the start ready for reading into memory
  rewind (input_file);	
  //Allocate some memory
  pb->ptr = malloc (pb->filesize);
  if (pb->ptr == NULL)
  {
    fprintf(stderr, "malloc of %lu bytes failed %s\n", pb->filesize, strerror(errno));
    return 1;
  }
  else
    fprintf(stdout, "malloc of %lu bytes succeeded for %s\n", pb->filesize, filename);
  //Read the input file into allocated memory
  size_t count;
  count = fread (pb->ptr, 1, pb->filesize, input_file);
  if (count != pb->filesize)
  {
    fprintf (stderr, "Error opening file: %s\n", strerror(errno));
    return 1;
  }

  pb->filepos = 0;	
  //Close the input file, no longer needed
  fclose (input_file);
  return 0;
}


