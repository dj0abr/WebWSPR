int soundcard_init();
int scan_sndcards();
int init_soundcard();
int init_soundcard_playback();
void exit_soundcard();
int wait_even_minute();
int recordWSPRinterval();
void soundcard_exit();
int record1s(int maxseconds, int phase);
int saveWAV(char *filename, short *data, int anz, int rate);
int restart_soundcard();

#define MAXSECONDS 111      // space for max 2 minutes recorded samples
#define CAPTURE_RATE 48000  // capture rate of the sound card, 48000 is ok for WSPR, higher rates make no improvement since WSPR works with 12000 internally
#define WSPR_RATE 12000

extern struct timeval  tv_start;
