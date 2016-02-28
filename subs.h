#define TEXT_BUFF_SIZE 2048

struct sub_t {
  void *ptr; //pointer to where our sub file is
  int filepos;
  int filesize;
  int running;
  unsigned long int start_latch;
  unsigned long int end_latch;
  unsigned long int len;
  char text[TEXT_BUFF_SIZE];
} subs;

int read_sub_file_into_mem (char* filename);
void sub_read_next (void);
