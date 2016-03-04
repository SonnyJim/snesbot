/*
 * record.c:
 *    Various recording and playback routines
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

//Currently only used for macro playback
void pb_read_next (struct playback_t* pb)
{
  memcpy (&pb->next_latch, pb->ptr + (pb->filepos * CHUNK_SIZE), sizeof(latch_counter));
  memcpy (&p1.input, pb->ptr + sizeof(latch_counter) + (pb->filepos * CHUNK_SIZE), sizeof(p1.input));
  pb->filepos++;
  if ((pb->filepos * CHUNK_SIZE) > pb->filesize)
  {
    pb->filepos = 0;
    clear_all_buttons ();
    read_joystick_inputs();
    botcfg.state = STATE_RUNNING;
  }
  pb->next_latch += pb->start_latch;
}

//Reads the next set of inputs ready to be pressed
void playback_read_next ()
{
  memcpy (&playback.next_latch, playback.ptr + (playback.filepos * CHUNK_SIZE), sizeof(latch_counter));
  memcpy (&p1.input, playback.ptr + sizeof(latch_counter) + (playback.filepos * CHUNK_SIZE), sizeof(p1.input));
  playback.filepos++;
  //fprintf (stdout, "Next latch:%i\n", playback.next_latch);
  if ((playback.filepos * CHUNK_SIZE) > playback.filesize)
  {
    fprintf (stdout, "Playback finished\n");
    clear_all_buttons ();
    read_joystick_inputs();
    botcfg.state = STATE_RUNNING;
  }
  //fprintf (stdout, "%i, %x, %i\n", latch_counter, p1.input, playback.next_latch);
}

void dump_latches (void)
{
  while (botcfg.state != STATE_RUNNING)
  {
    fprintf (stdout, "Next latch: %lu\n", playback.next_latch);
    print_buttons (p1.input, p1.input);
    playback_read_next ();
  }
}

int playback_start ()
{
  if (read_file_into_mem () != 0)
  {
    fprintf (stderr, "Error reading input file into memory\n");
    return 1;
  }
  //Load up the initial inputs
  playback_read_next ();
//  dump_latches ();
  return 0;
}

int record_start ()
{
  //malloc some memory for the output ptr
  record.ptr = malloc (RECBUFSIZE);
  if (record.ptr == NULL)
  {
    printf("Unable to allocate %i bytes for record buffer\n", RECBUFSIZE);
    botcfg.state = STATE_EXITING;
    return 1;
  }
  else
    printf("%i bytes allocated for record buffer\n", RECBUFSIZE);
  record.filepos = 0;
  botcfg.state = STATE_RECORDING;
  return 0;
}

void record_player_inputs ()
{
  //fprintf (stdout, "size %i ptr: %#10x\n", record.filepos * CHUNK_SIZE, record.ptr + (record.filepos * CHUNK_SIZE));
  memcpy (record.ptr + record.filepos * CHUNK_SIZE, &latch_counter, sizeof(latch_counter));
  memcpy (record.ptr + sizeof(latch_counter) + (record.filepos * CHUNK_SIZE), &p1.input, sizeof(p1.input));
  record.filepos++;
}

void record_save (void)
{
  if (record.filepos > 0)
  {
    write_mem_into_file ();
  }
}

int write_mem_into_file (void)
{
	//Open output file
	FILE *output_file = fopen (botcfg.outfile, "wb");
	
	if (output_file == NULL)
	{
		printf("Problem opening %s for writing\n", botcfg.outfile);
		return 1;
	}
	
	//Write magic number to file header
//	fwrite (&filemagic, 1, sizeof(filemagic), output_file);

	//Calculate filesize
	record.filesize = CHUNK_SIZE * record.filepos;
	printf ("Recorded %i bytes of input\n", record.filesize);
	
	//Store to output file
	long result = fwrite (record.ptr, 1, record.filesize, output_file);
	printf ("Wrote %lu bytes to %s\n", result, botcfg.outfile);
        if (result != record.filesize)
          fprintf (stderr, "Error:  Didn't write all the bytes?\n");
	fclose(output_file);
	return 0;
}

int read_file_into_mem (void)
{
	//Open the file
	FILE *input_file = fopen (botcfg.infile, "rb");
	
	if (input_file == NULL)
	{
		printf ("Could not open %s for reading\n", botcfg.infile);
		return 1;
	}
        /*        
        long filemagic_check;
        fread (&filemagic_check, 1, sizeof(filemagic), input_file);
        if (filemagic_check != filemagic)
        {
          printf ("Incompatible file type!\n");
          return 1;
        }
        else 
          printf ("SNESBot filetype detected\n");
        */
	//Seek to the end
	fseek (input_file, 0, SEEK_END);
	//Find out how many bytes it is
	playback.filesize = ftell(input_file);
	

	//Rewind it back to the start ready for reading into memory
	rewind (input_file);	
	
        /*
	//Strip off file magic number
        filesize = filesize - sizeof(filemagic);
        //Seek past the magic number
        fseek (input_file, sizeof(filemagic), 0);
        */

	//Allocate some memory
	playback.ptr = malloc (playback.filesize);
	if (playback.ptr == NULL)
	{
		printf("malloc of %lu bytes failed\n", playback.filesize);
		return 1;
	}
	else
		printf("malloc of %lu bytes succeeded\n", playback.filesize);
	
	//Read the input file into allocated memory
	fread (playback.ptr, 1, playback.filesize, input_file);
        playback.filepos = 0;	
	//Close the input file, no longer needed
	fclose (input_file);
	return 0;
}
