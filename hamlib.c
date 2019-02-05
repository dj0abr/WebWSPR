/*
 * WSPR Toolkit for Linux
 * ======================
 * by DJ0ABR 
 * 
 * hamlib.c
 * 
 * all hamlib related jobs
 * initialized from wsprtk.c and called during runtime from cat.c
*/

#include <string.h>
#include <pthread.h>
#include <alsa/asoundlib.h>
#include <dirent.h>
#include <time.h>
#include <sys/time.h>
#include "debug.h"
#include "wsprtk.h"
#include "cat.h"
#include "config.h"

void setRig(char *s);
int hamlibfound = 0;

// generate transceiver list and copy it into the phpdir folder
void hamlib_maketable()
{
char s[256];

    snprintf(s,sizeof(s),"rigctl -l 2> /dev/nul");
    FILE *fp = popen(s,"r");
    if(fp)
    {
        while (fgets(s, sizeof(s)-1, fp) != NULL) 
        {
            //printf("%s",s);
            // check response
            hamlibfound = 1;
        }
        pclose(fp);
    }
    if(!hamlibfound)
    {
        deb_printf("HAMLIB", "ERROR: cannot start rigctl, is HAMLIB installed ?");
        return;
    }
    
    // generate TRX list, split and sort it to get a readable list
    snprintf(s,sizeof(s),"rigctl -l | tr -s \" \" |  cut -d \' \' -f 2,3,4  | sort -k2 > %s/%s/trx.lst",htmldir,phpdir);
    system(s);
}

// rigctl -m 114 -r /dev/ttyUSB0 -s 4800 -P RIG
// for PTT add:  T 1 ot T 0

void hamlib_ptt(int onoff)
{
    if(onoff)
        setRig("T 1");
    else
        setRig("T 0");
}

// trx_frequency is in MHz
void hamlib_setQRG(double trx_frequency)
{
    trx_frequency *= 1000000;
    char s[256];
    snprintf(s,sizeof(s),"F %.0f T 0",trx_frequency);
    setRig(s);
}

void setRig(char *s)
{
char txt[256];

    if(!hamlibfound) 
    {
        deb_printf("HAMLIB","hamlib not installed");
        return;
    }
    
    snprintf(txt,sizeof(txt),"rigctl -m %d -r %s -s %d -P RIG %s",hamlib_trxnr,device,4800/*hamlib_baudrate*/,s);
    deb_printf("HAMLIB","%s\n",txt);
    system(txt);
    usleep(250000);
}
