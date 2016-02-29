extern struct playback_t pb;
extern struct playback_t macro;

int read_macro_into_mem (char* filename, struct playback_t* pb);
void macro_start (struct playback_t* macro);
