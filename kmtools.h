char *cleanString(char *str, int cleanspace);
char* strupr(char* s);
int getFileSize(char *fn);
int checkTime(int min, int sec);
char *bandFromQrg(double f);
int getIntervalSecond();
char *strnstr(const char *s, const char *find, size_t slen);

// Stucture that holds band and qrg relations
// these are just default values
// which are overwritten at program start with
// the values from .../phpdir/wsprconfig.js
#define NUMOFBANDS 17
typedef struct _BANDLIST_ {
    char band[10];
    double qrg;
} BANDLIST;

extern BANDLIST myband[NUMOFBANDS];
