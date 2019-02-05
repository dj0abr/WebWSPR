/*
 * WSPR Toolkit for Linux
 * ======================
 * by DJ0ABR 
 * 
 * status.c ... write a file with status information into the HTML directory
 * it is read by the GUI to display these information
 * 
 * 
 * */

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <stdlib.h>
#include <stdint.h>
#include <wchar.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <sys/file.h>
#include <pthread.h>
#include "wsprtk.h"
#include "debug.h"
#include "soundcard.h"
#include "fft.h"
#include "config.h"
#include "coder.h"
#include "civ.h"
#include "hopping.h"
#include "status.h"


// write status file for GUI
// file: htmldir/status.txt
// format:  section:value\n
int status_value[MAXSTATUSLINES];

pthread_mutex_t crit_sec;
pthread_mutex_t waitmutex;
#define LOCK	pthread_mutex_lock(&crit_sec)
#define UNLOCK	pthread_mutex_unlock(&crit_sec)


void fsave_status(int id, int value)
{
static char fn[256];
static int f=1;

    if(f)
    {
        // write status file
        snprintf(fn,sizeof(fn)-1,"%s/%s/status.txt",htmldir,phpdir);
        memset(status_value,0,sizeof(status_value));
        f=0;
    }

     LOCK;

    //insert new value
    status_value[id] = value;
    
    FILE *fw = fopen(fn,"w");
    if(fw)
    {
        for(int i=0; i<MAXSTATUSLINES; i++)
        {
            fprintf(fw,"%d\n",status_value[i]);
        }
        fclose(fw);
    }
    else
        printf("cannot open status file:%s. Error:%s\n",fn,strerror(errno));
    
    UNLOCK;
}
