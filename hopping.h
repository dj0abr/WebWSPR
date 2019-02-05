
void readTXhopping();
int checkForTX();
void setNextIntervalFrequency();
void updateNextFreq();
int checkForTX_nextIntv();

// hold information for one interval
typedef struct _TXMAP_
{
    char txrx;      // 0=rx, 1=tx
    double qrg;     // frequency in MHz
} TXMAP;

#define INTVPERDAY  (24*60/2)

extern TXMAP map[INTVPERDAY];
extern double trx_frequency;
