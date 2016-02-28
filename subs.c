#include "snes.h"

void sub_read_next (void)
{
  if (!subs.running)
  {
    fprintf (stderr, "Error, attempted to read subtitles when we are finished?\n");
    return;
  }

  memcpy (&subs.start_latch, subs.ptr + subs.filepos, sizeof(subs.start_latch));
  subs.filepos += sizeof(subs.start_latch);

  memcpy (&subs.end_latch, subs.ptr + subs.filepos, sizeof(subs.end_latch));
  subs.filepos += sizeof(subs.end_latch);

  memcpy (&subs.len, subs.ptr + subs.filepos, sizeof(subs.len));
  subs.filepos += sizeof(subs.len);
  
  if (subs.len > 0 && subs.len < TEXT_BUFF_SIZE)
  {
    strncpy (subs.text, subs.ptr + subs.filepos, subs.len);
    subs.text[subs.len] = '\0';
    subs.filepos += subs.len;
  }
  else
  {
    subs.running = 0;
    fprintf (stdout, "Subtitles finished\n");
    return;
  }
}

int ends_with(const char* name, const char* extension, size_t length)
{
  const char* ldot = strrchr(name, '.');
  if (ldot != NULL)
  {
    if (length == 0)
      length = strlen(extension);
    return strncmp(ldot + 1, extension, length) == 0;
  }
  return 0;
}

int read_sub_file_into_mem (char* filename)
{
  char sub_filename[TEXT_BUFF_SIZE];
 
  //Look for an accompying subtitle file
  if (ends_with (filename, "rec", 3))
  {
    strncpy (sub_filename, filename, strlen(filename) - 4);
    strcat (sub_filename, ".sub");
  }
  else
    strcpy (sub_filename, filename);

  FILE *input_file = fopen (sub_filename, "rb");

  if (input_file == NULL)
  {
    fprintf (stderr, "Couldn't open %s, %s\n", sub_filename, strerror(errno));
    return 1;
  }
  
  fseek (input_file, 0, SEEK_END);
  //Find out how many bytes it is
  subs.filesize = ftell(input_file);
  //Rewind it back to the start ready for reading into memory
  rewind (input_file);	
  //Allocate some memory
  subs.ptr = malloc (subs.filesize);
  if (subs.ptr == NULL)
  {
    fprintf(stderr, "malloc of %i bytes failed %s\n", subs.filesize, strerror(errno));
    return 1;
  }
  else
    fprintf(stdout, "malloc of %i bytes succeeded for %s\n", subs.filesize, sub_filename);
  //Read the input file into allocated memory
  size_t count;
  count = fread (subs.ptr, 1, subs.filesize, input_file);
  if (count != subs.filesize)
  {
    fprintf (stderr, "Error opening file: %s\n", strerror(errno));
    return 1;
  }

  subs.filepos = 0;
  subs.running = 1;
  sub_read_next ();
  //Close the input file, no longer needed
  fclose (input_file);
  return 0;
}
