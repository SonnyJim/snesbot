#include "snes.h"
//Reads the next set of inputs ready to be pressed
void playback_read_next ()
{
  memcpy (&playback.next_latch, playback.ptr + (playback.filepos * CHUNK_SIZE), sizeof(latch_counter));
  memcpy (&p1.input, playback.ptr + sizeof(latch_counter) + (playback.filepos * CHUNK_SIZE), sizeof(p1.input));
  playback.filepos++;
  if ((playback.filepos * CHUNK_SIZE) > playback.filesize)
  {
    fprintf (stdout, "Playback finished\n");
    clear_all_buttons ();
    state = STATE_RUNNING;
  }
  //fprintf (stdout, "%i, %x, %i\n", latch_counter, p1.input, playback.next_latch);
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

  return 0;
}

int record_start ()
{
  //malloc some memory for the output ptr
  record.ptr = malloc (RECBUFSIZE);
  if (record.ptr == NULL)
  {
    printf("Unable to allocate %i bytes for record buffer\n", RECBUFSIZE);
    state = STATE_EXITING;
    return 1;
  }
  else
    printf("%i bytes allocated for record buffer\n", RECBUFSIZE);
  record.filepos = 0;
  state = STATE_RECORDING;
  return 0;
}

void record_player_inputs ()
{
  //fprintf (stdout, "size %i ptr: %#10x\n", record.filepos * CHUNK_SIZE, record.ptr + (record.filepos * CHUNK_SIZE));
  memcpy (record.ptr + record.filepos * CHUNK_SIZE, &latch_counter, sizeof(latch_counter));
  memcpy (record.ptr + sizeof(latch_counter) + (record.filepos * CHUNK_SIZE), &p1.input, sizeof(p1.input));
  record.filepos++;
}

void record_save ()
{
  if (record.filepos > 0)
  {
    write_mem_into_file ();
  }
}


