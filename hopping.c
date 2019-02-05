/*
 * WSPR Toolkit for Linux
 * ======================
 * by DJ0ABR 
 * 
 * hopping.c
 * 
 * reads TX and hopping settings from the file phpdir/txsettings.txt
 * Builds a list of RX and TX times for 24h
 * this list is then used for RXTX switching andfrequency hopping
 * 
 * */


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <alsa/asoundlib.h>
#include <sndfile.h>
#include "hopping.h"
#include "wsprtk.h"
#include "config.h"
#include "debug.h"
#include "kmtools.h"
#include "status.h"
#include "cat.h"

void buildDefaultMap();
void setMap();
double getNextTxQrg();
double getNextRxQrg();
void printMap();
int getCurrentIntervalNr();
int getNextIntervalNr();

// this array holds a map for all intervals within 24h
// it is created by the php server for the tx settings
// if empty, use a default mapping
TXMAP map[INTVPERDAY];

double trx_frequency=0;// WSPR frequency in MHz

// read the txsettings.txt file into the map
void readTXhopping()
{
char sline[50];
int valid = 0;
int txnextband = -1;
int nextintervalnr;
int txsettings_txt_is_available = 0;
int first = 1;

    deb_printf("TXCONFIG","rebuild TX/RX and hopping intervals");
    
    // calculate the coming interval number
    nextintervalnr = getNextIntervalNr();
    
    // copy all lines from txsettings.txt to txsettings.new and make required modifications
    char fnn[256];
    snprintf(fnn,sizeof(fnn),"%s/%s/txsettings.new",htmldir,phpdir);
    FILE *fw = fopen(fnn,"w");
    if(fw)
    {
        char fn[256];
        snprintf(fn,sizeof(fn),"%s/%s/txsettings.txt",htmldir,phpdir);
        FILE *fr = fopen(fn,"r");
        if(fr)
        {
            txsettings_txt_is_available = 1;
            int line = 0;
            int mapidx = 0;
            while(fgets(sline,49,fr) != NULL)
            {
                // line 73 has the tx next band
                if(line == 73) 
                {
                    if(strlen(sline)>=1)
                    {
                        int bi = atoi(sline);
                        if(bi>=0 && bi <=16)
                        {
                            txnextband = bi;
                            // remove the txnext marker
                            strcpy(sline,"99\n");
                            //printf("next intv:%d band:%d\n",nextintervalnr,txnextband);
                        }
                    }
                }
                // the map begins at line 74 and has 720 lines, one for each interval
                if(line >= 74 && mapidx < 720)
                {
                    char *p = strchr(sline,' ');
                    if(p == NULL) 
                    {
                        deb_printf("TXCONFIG","format-1 error, line: %d: %s",line,sline);
                        break;
                    }
                    int band = atoi(p+1);
                    if(band >= 10000) band /= 10000;
                    if((sline[0] == 'R' || sline[0] == 'T') && band>=0 && band <= 16)
                    {
                        map[mapidx].qrg = myband[band].qrg;
                        
                        if(sline[0] == 'R') map[mapidx].txrx = 0;
                        else map[mapidx].txrx = 1;
                    }
                    else
                    {
                        deb_printf("TXCONFIG","format-2 error, line: %d: %s",line,sline);
                        break;  // format error
                    }
                    
                    if(txnextband != -1 && mapidx == nextintervalnr)
                    {
                        // enter the txnext interval into the map
                        map[mapidx].txrx = 1;
                        map[mapidx].qrg = myband[txnextband].qrg;
                        
                        deb_printf("TXCONFIG","tx-next Band: %f at %d. Line:%s\n",map[mapidx].qrg,mapidx,sline);
                    }
                    
                    //printf("%d: %d %f\n",mapidx,map[mapidx].txrx,map[mapidx].qrg);
                    mapidx++;
                }

                if(mapidx == 719) valid = 1;
                line++;
                fputs(sline,fw);
            }
            fclose(fr);
        }
        else
        {
            deb_printf("TXCONFIG","cannot open the file: %s",fn);
        }
        fclose(fw);
        
        // copy the new file over to txsetting.txt file
        // do this ONLY if txsettings.txt already exists
        // it must be created by php to get the apache user:group
        // since the tinyCore cp command cannot preserve attributes correctly
        // we use cat to copy the file which does not modify the file owner
        if(txsettings_txt_is_available == 1)
        {
            char s[256];
            snprintf(s,sizeof(s),"cat %s > %s",fnn,fn);
            system(s);
        }
    }
    else
    {
        deb_printf("TXCONFIG","cannot open the file: %s for writing",fnn);
    }
 
    if(!valid)
    {
        deb_printf("TXCONFIG","use default settings");
        buildDefaultMap();
        //printMap();
    }
    
    if(first)
    {
        // set TRX qrg at program start
        setNextIntervalFrequency();
        deb_printf("TXCONFIG","initial set TRX qrg to %f",trx_frequency);
        first = 0;
    }
}


// fill the map with default values
// default: qrg=7.0386 MHz, only RX
void buildDefaultMap()
{
    for(int i=0; i<INTVPERDAY; i++)
    {
        map[i].txrx = 0;
        map[i].qrg = 7.0386;
    }
}

// update status.txt values
void updateNextFreq()
{
int foundtx = 0;

    int actual_int = getCurrentIntervalNr();
    int next_int = getNextIntervalNr();

    // next TX interval, go through all intervals a day
    int next_tx_int = actual_int;
    for(int i=0; i<((60*24)/2); i++)
    {
        if(map[next_tx_int].txrx == 1)
        {
            // found it
            foundtx = 1;
            break;
        }
        next_tx_int++;
        next_tx_int %= ((60*24)/2);
    }

    // store it in the file status.txt
    fsave_status(RXTXFREQ,map[actual_int].qrg*1000000);
    fsave_status(FREQNEXTINTV,map[next_int].qrg*1000000);
    
    if(foundtx == 1)
    {
        fsave_status(NEXTTXTIME,next_tx_int*2);
        fsave_status(NEXTTXFREQU,map[next_tx_int].qrg*1000000);
    }
    else
    {
        fsave_status(NEXTTXTIME,-1);    // no TX
        fsave_status(NEXTTXFREQU,-1);
    }
}

int getCurrentIntervalNr()
{
    // get number of current interval
    struct timeval  tv;
	gettimeofday(&tv, NULL);
    // convert to GMT 
    struct tm *tm = gmtime(&tv.tv_sec);
    // get minute of the day
    int min = tm->tm_hour*60 + tm->tm_min;
    min /= 2;
//printf("current:%d\n",min);    
    // actual and next interval
    return min;
}

int getNextIntervalNr()
{
    int i = getCurrentIntervalNr();
    i++;
    i %= 720;
//printf("next:%d\n",i);        
    return i;
}

// check if the current interval is tx or rx
int checkForTX()
{
    if(txmode == 0) return 0; 
    
    int intv = getCurrentIntervalNr();
    return map[intv].txrx;
}

// check if the next interval is tx or rx
int checkForTX_nextIntv()
{
    if(txmode == 0) return 0; 
    
    int intv = getNextIntervalNr();
    return map[intv].txrx;
}

void setNextIntervalFrequency()
{
    int intv = getNextIntervalNr();
    trx_frequency = map[intv].qrg;
    // wait until the last CAT command is processed
    int to=0;
    while(ser_command) 
    {
        usleep(100000);
        if(!running) break;
        if(++to >= 10) break;
    }
    //printf("ser_command = 4\n");
    ser_command = 4;
}

// show map for debugging purposes only
void printMap()
{
    return;
    
    for(int i=0; i<INTVPERDAY; i++)
    {
        //if(map[i].txrx)
        printf("intv:%02d:%02d txrx:%d qrg:%f\n",(i*2)/60,(i*2)-((i*2)/60)*60,map[i].txrx,map[i].qrg);
    }
}
