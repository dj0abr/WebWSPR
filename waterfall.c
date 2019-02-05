/*
 * WSPR Toolkit for Linux
 * ======================
 * by DJ0ABR 
 * 
 * waterfall.c 
 * 
 * creates a jpg graphic containing the waterfall
 * 
 * */

#include <gd.h>
#include "gdfontt.h"
#include "gdfonts.h"
#include "gdfontmb.h"
#include "gdfontl.h"
#include "gdfontg.h"
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <sys/time.h>
#include "debug.h"
#include "wsprtk.h"
#include "waterfall.h"
#include "color.h"
#include "config.h"
#include "hopping.h"
#include "spectrum.h"

void makeWSPR_WF_pic(double *d, int fstart, int fend, int samplecnt);
void createWFemptyImage();
void drawWFimage(gdImagePtr im, double *d, int fstart, int fend, int samplecnt);
void writeImage(gdImagePtr im);
void drawFFTline(gdImagePtr dst, double *d, int fstart, int fend);
void scaleSamples(unsigned int *psamples, double *pfsamples, int anz);
void makeWFbig(unsigned int *psamples, int anz, int samplecnt);

char wfdir[256]={"/tmp"};
char wffilename[256]={"wf.bmp"};
int wf_width;
int wf_height;

int wf_toprow;
int wf_copyheight;


#define SAMPLEANZ   24001

// parameters: psamples ... 32 bit values, anz ... number of values
// bild the waterfall for the main window (600x600px)
void makeWF(unsigned int *psamples, int anz, int samplecnt)
{
double pfsamples[SAMPLEANZ];

    if(anz > SAMPLEANZ)
    {
        deb_printf("WF","SAMPLEANZ too low: %d , anz: %d",SAMPLEANZ,anz);
        return;
    }
    
    // define the reqired picture
    wf_width = 600;     // width of the waterfall
    wf_height = 600;    // height of the waterfall
    wf_toprow = 75;     // first top row of the waterwall (higher rows are for the spectrum)
    strcpy(wffilename,"wf.bmp");    // filename of the resulting bitmap
    
    wf_copyheight = wf_height-wf_toprow;
    // scale FTT values
    scaleSamples(psamples, pfsamples, anz);
    
    // the frequency range of the array is 0 to 24000 Hz
    // for the main WSPR waterfall we need 1350 to 1650 Hz (a range of 300 Hz)
    makeWSPR_WF_pic(pfsamples, qrg_WF_left, qrg_WF_right, samplecnt);
}

void scaleSamples(unsigned int *psamples, double *pfsamples, int anz)
{
    // scale FTT values
    /*  Skalierung der FFT Werte
        gemessen: eine Änderung um 6 dBm am Antenneneingang führt zur Verdoppelung (Halbierung) der Spektrumnadel
        um die Nadel in Gleichklang zum dBm Wert zu bekommen, muss man sie mit dem Log zu Basis 1,122 nehmen (mit Excel ausprobiert)
    */
    for(int i=0; i<anz; i++)
    {
        double dval = psamples[i];
        if (dval == 0) dval = 1;    // sonst kann der log crashen
        dval = (float)(log(dval) / log(1.122));    // diese Formel entspricht dem Log zur Basis 1,122

        /*  der geringste Pegel im Spektrum kann 1 sein, der obige Log ergibt: 0
            der höchste 32768*24000 = 786.432.000, der obige Log ergibt: 178
            der darstellbare Dynamikbereich ist also 178 dBm
            schreibe das ins Array specvals
        */
        dval = dval - 180;  // drehe, weil wir mit negativen dBm Werten arbeiten

        // an dieser Stelle hat man mit "dval" einen Wert der sehr genau dem dBm Wert am Antenneneingang folgt
        // korrigiere mit dem Kalibrationswert
        dval += refminus80dBm;

        pfsamples[i] = dval;
    }
}

void makeWSPR_WF_pic(double *d, int fstart, int fend, int samplecnt)
{
    // bitmap's filename
    char fn[256];
    snprintf(fn,sizeof(fn)-1,"%s/tmp_%s",wfdir,wffilename);
    
    // read the bitmap
    gdImagePtr im = gdImageCreateFromFile(fn);
    if(im)
    {
        drawWFimage(im, d, fstart, fend, samplecnt);
        gdImageDestroy(im);
    }
    else
    {
        // image file does not exist, create an empty image
        deb_printf("WF","Create new WF image. %s not found",fn);
        createWFemptyImage();
    }
}

void writeImage(gdImagePtr im)
{
char fn[256];

    snprintf(fn,sizeof(fn)-1,"%s/tmp_%s",wfdir,wffilename);
    gdImageFile(im, fn);
    
    snprintf(fn,sizeof(fn)-1,"cp %s/tmp_%s %s/%s",wfdir,wffilename,htmldir,wffilename);
    system(fn);
}

void createWFemptyImage()
{
    gdImagePtr im = gdImageCreate(wf_width,wf_height);      // create Image
    gdImageColorAllocate(im, 0, 0, 0);                      // create BG color (black)
    writeImage(im);
    gdImageDestroy(im);
}

int timepos[4] = {24,42,0,50};
int qrgpos[4] = {34,75,0,96};

#define MAXMIDVALLEN 21
double midsamples[MAXMIDVALLEN][SAMPLEANZ];     // to calculate the mid value of lines
double midsample[SAMPLEANZ];                    // result

void drawWFimage(gdImagePtr im, double *d, int fstart, int fend, int samplecnt)
{
    // shift image down one row
    // by copying the image to another image
    gdImagePtr dst = gdImageCreate(wf_width,wf_height);
    allocatePalette(dst);
    
    if((samplecnt % secondsPerLine) == 0)
    {
        // draw top row into the new image
        gdImageCopy(dst,im,0,wf_toprow+1,0,wf_toprow,wf_width,wf_copyheight-1);
        //drawFFTline(dst,d,fstart,fend);
        drawFFTline(dst,midsample,fstart,fend);
    }
    else
    {
        // copy the image as it is, no change
        gdImageCopy(dst,im,0,wf_toprow,0,wf_toprow,wf_width,wf_copyheight);
    }

    // store pixels for mid value calculation
    // d contains the SAMPLEANZ FFT samples
    // write it to the mid buffer
    // samplecnt is a counter for each 1s SAMPLEANZ
    // WFmidnum is the number of mids from the config, check and correct user input
    if(WFmidnum >= (MAXMIDVALLEN-1)) WFmidnum = MAXMIDVALLEN-1; 
    if(WFmidnum < 1) WFmidnum = 1;
    
    int bufpos = samplecnt % WFmidnum;  // index to the mid buffer
    
    // store the actual sample in the mid buffer
    for(int i=0; i<SAMPLEANZ; i++)
    {
        midsamples[bufpos][i] = d[i];
    }
    
    // calculate the mid value
    for(int i=0; i<SAMPLEANZ; i++)
    {
        midsample[i] = 0;
        for(int j=0; j<WFmidnum; j++)
        {
            midsample[i] += midsamples[j][i];
        }
        midsample[i] /= WFmidnum;
    }

	if(samplecnt == 0)
	{
		// new interval, draw white hor line
		gdImageLine(dst, 0, wf_toprow, wf_width-1, wf_toprow, 253);
        drawSpec_Init(wf_width,wf_toprow-1);
	}
    else if (samplecnt == timepos[secondsPerLine-1])
	{
		// draw time into the bitmap
		char tim[100];
        struct timeval  tv;
        struct tm      *tm;
        gettimeofday(&tv, NULL);
        tm = gmtime(&tv.tv_sec);
        if((tm->tm_min&1)==0)
        {
            strftime(tim,99,"%Y.%m.%d %H.%M",tm);
            gdImageString (dst,gdFontGetSmall(),2,wf_toprow,(unsigned char *)tim,253);
        }
	}
    else if (samplecnt == qrgpos[secondsPerLine-1])
	{
		// draw frequency into the bitmap
		char t[100];
		snprintf(t,sizeof(t)-1,"%.6f MHz",trx_frequency);
		gdImageString (dst,gdFontGetSmall(),2,wf_toprow,(unsigned char *)t,253);
	}
	
	// draw spectrum
	drawSpec(dst, d, fstart, fend, samplecnt);
    
    // write to file
    writeImage(dst);
        
    gdImageDestroy(dst);
}

void drawFFTline(gdImagePtr dst, double *d, int fstart, int fend)
{
static int phase = 0;

    calcColorParms(fstart,fend,d);
    
    for(int i=fstart; i<fend; i++)
    {
        int xs = ((i-fstart)*wf_width)/(fend-fstart);
        int xe = (((i+1)-fstart)*wf_width)/(fend-fstart);
        
		// WSPR edges, draw white solid vertical line
		if(i == 1400 || i == 1600)
			gdImageLine(dst, xs, wf_toprow, xe, wf_toprow, 253);
		// WSPR mid band marker
		else if (i == 1500 && (phase%2)==0)
			gdImageLine(dst, xs, wf_toprow, xe, wf_toprow, 253);
		// 50 Hz marker
		else if((i == 1450 || i == 1550) && (phase%3)==0)
			gdImageLine(dst, xs, wf_toprow, xe, wf_toprow, 253);
		// normal FFT line
		else
            gdImageLine(dst, xs, wf_toprow, xe, wf_toprow, getPixelColor(d[i]));
    }

	phase++;

    //printf("\n");
}

// copy WF picture into tmp folder at program start
void restoreWF()
{
    char fn[256];
    snprintf(fn,sizeof(fn)-1,"cp %s/%s %s/tmp_%s",htmldir,wffilename,wfdir,wffilename);
    system(fn);
}
