/*
 * WSPR Toolkit for Linux
 * ======================
 * by DJ0ABR 
 * 
 * dds.c 
 * 
 * functions to control the U02 DDS Synthesizer
 * 
 * */

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>
#include <math.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include "debug.h"
#include "wsprtk.h"
#include "config.h"
#include "hopping.h"
#include "soundcard.h"
#include "cat.h"
#include "dds.h"
#include "crc.h"
#include "kmtools.h"

/* Data Format:
 * Byte 0: 0x4e
 * Byte 1: 0x7a
 * Byte 2: 0xe7
 * Byte 3: 0x45
 * Byte 4: length of the following Control Code String
 * Byte 5... : Control Code String
 * Control Code String: 1st Byte: Control Code, followed by parameter
 * Byte n-4 ... CRC32 [LSB]
 * Byte n-3 ... CRC32
 * Byte n-2 ... CRC32
 * Byte n-1 ... CRC32 [MSB]
 * 
 * Control Codes:
 * 'a' ... switch to WSPR mode
 * then the DDS goes into RX mode send sets the frequency to the rx-band
 * 
 * 'b'+band ... start one WSPR transmission with the next interval
 * were band is one of the following:
 * bandname = { "2200", "630", "160", "80", "60", "40", "30", "20", "17", "15", "12", "10", "6", "4", "2", "70", "23" }
 * 
 * 'I1'+rxband ... set DDS to RX mode and set the RX band
 * 
 * '1'+callsign ... set callsign
 * '2'+locator ... set locator
 * 
 * 'B'+txoffset ... txoffset: 16bit int between 1400 and 1600 [MSB first]
 * 
 * 'K'+zf ... zf: 32bit in Hz, bit 32 must be set to 1 [MSB first]
 * 
 * '3'+power ... set output power, power: string number in dBm i.e. "37"
 * 
 * 'R'+time ... set clock, time: 3 bytes: hh,mm,ss
 * */

void readDDSmessage(int reti);
void writeDDS_string(uint8_t command, char *txt);
void writeDDS_16bit(uint8_t command, uint16_t data);
void writeDDS_32bit(uint8_t command, uint32_t data);
void writeDDS(uint8_t *pdata, int len);
char *writeDDS_time();

int dds_update_power = 0;   // set to 1 by config to update power immediately
int dds_update_cal = 0;

// init the DDS
void init_dds()
{
char t[20];
char *tim;

    tim = writeDDS_time();                // sebd PC time to DDS
    writeDDS_string('1', callsign); // set callsign
    
    strcpy(t,qthloc);
    t[4]=0;
    writeDDS_string('2', t);        // set qthloc
    
    dds_updatePower();
    
    writeDDS_16bit('B', (uint16_t)txoffset);    // set TX offset 
    dds_updateCalib();
    writeDDS_32bit('K', dds_if);    // RX IF frequency
    writeDDS_string('O', NULL);     // switch DDS off

    deb_printf("U02DDS","init DDS: %s %s %s %d",tim,callsign,t,txoffset);

    sleep(1);
}

// dds_txpwr in in % 0-1000 (calc to 1-16383)
void dds_updatePower()
{
    int outlev = (dds_txpwr * 16382) / 1000 + 1;
    deb_printf("U02DDS","set power: %d\n",outlev);
    writeDDS_16bit('L', outlev);      // set output level (1..16383)
    
    dds_update_power = 0;
 }
 
 void dds_updateCalib()
 {
    deb_printf("U02DDS","set cal:%d\n",dds_cal);
    writeDDS_32bit('7', dds_cal);         // set calibration
    
    dds_update_cal = 0;
 }

char *writeDDS_time()
{
static char t[100];

   	struct timeval  tv;
	struct tm      *tm;
	
	gettimeofday(&tv, NULL);
	tm = gmtime(&tv.tv_sec);

    uint8_t tx[4];
    tx[0] = 'R';
    tx[1] = tm->tm_hour;
    tx[2] = tm->tm_min;
    tx[3] = tm->tm_sec;
    
    writeDDS(tx,4);
    
    sprintf(t,"%02d:%02d:%02d",tm->tm_hour,tm->tm_min,tm->tm_sec);
    return t;
}

// trx_frequency is in MHz
// will be called before any (RX+TX) interval
void dds_setQRG(double trx_frequency)
{
char t[100];

    writeDDS_string('a', NULL);
    
    char *band = bandFromQrg(trx_frequency);
    if(band)
    {
        // is the next interval TX or RX ?
        if(checkForTX_nextIntv())
        {
            deb_printf("U02DDS","Set TX: band %s found for:%f\n",band, trx_frequency);
            writeDDS_string('b',band);
        }
        else
        {
            deb_printf("U02DDS","Set RX: band %s found for:%f\n",band, trx_frequency);
            sprintf(t,"1%s",band);
            writeDDS_string('I',t);     // SET rx BAND
            writeDDS_string('a', NULL); // init mode, this disables the TX
        }
    }
    else
        deb_printf("U02DDS","DDS: band not found for:%f\n",trx_frequency);
}
    
void writeDDS_string(uint8_t command, char *txt)
{
    uint8_t tx[100];
    int idx = 0;
    tx[idx++] = command;
    
    if(txt != NULL)
    {
        int len = strlen(txt);
        if(len > 90) 
        {
            printf("DDS wrong string length: %s !!!!\n",txt);
            return;
        }
        for(int i=0; i<len; i++)
            tx[idx++] = txt[i];
        tx[idx++] = '\n';
    }
    
    writeDDS(tx,idx);
}

void writeDDS_16bit(uint8_t command, uint16_t data)
{
    uint8_t tx[3];
    tx[0] = command;
    tx[1] = data >> 8;
    tx[2] = data;
    
    writeDDS(tx,3);
}

void writeDDS_32bit(uint8_t command, uint32_t data)
{
    uint8_t tx[5];
    tx[0] = command;
    tx[1] = data >> 24;
    tx[2] = data >> 16;
    tx[3] = data >> 8;
    tx[4] = data;
    
    writeDDS(tx,5);
}

void writeDDS(uint8_t *pdata, int len)
{
    uint8_t txdata[100];
    int idx = 0;
    txdata[idx++] = 0x4e;
    txdata[idx++] = 0x7a;
    txdata[idx++] = 0xe7;
    txdata[idx++] = 0x45;
    txdata[idx++] = len;
    for(int i=0; i<len; i++)
        txdata[idx++] = pdata[i];
    
    uint32_t crc32 = crc32_messagecalc(txdata+5,len);
    
    txdata[idx++] = crc32;
    txdata[idx++] = crc32 >> 8;
    txdata[idx++] = crc32 >> 16;
    txdata[idx++] = crc32 >> 24;
    
    /*
    for(int i=0; i<idx; i++)
    {
        printf("%02X ",txdata[i]);
    }
    printf("\n");
    */
    
    write_port(txdata,idx);
    usleep(100000); 
}

// receive one byte from DDS
void readDDSmessage(int reti)
{
}
