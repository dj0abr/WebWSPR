/*
 * WSPR Toolkit for Linux
 * ======================
 * by DJ0ABR 
 * 
 * spectrum.c 
 * 
 * creates a jpg graphic containing the spectrum
 * only called by the waterfall in the main windows index.html
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
#include "soundcard.h"

#define FFTSAMPLES (CAPTURE_RATE/2+1)

double samples_sum[FFTSAMPLES+2];
double samples_mid[FFTSAMPLES+2];
int samplenum = 0;
int spec_width;
int spec_height;

void drawSpec_Init(int w, int h)
{

    samplenum = 0;
    for(int i=0; i<FFTSAMPLES; i++)
        samples_sum[i] = 0;

    spec_width = w;
    spec_height = h;
}

void drawSpec(gdImagePtr dst, double *d, int fstart, int fend, int samplecnt)
{
int i;
int realh = spec_height - 2;

    if(fend > FFTSAMPLES)
    {
        printf("fend > FFTSAMPLES !");
        return;
    }

    gdImageFilledRectangle(dst,0,0,spec_width,spec_height,253);
    
    // copy new samples into mid buffer
    for(i=fstart; i<fend; i++)
    {
        samples_sum[i] += fabs(d[i]);
    }
    samplenum++;
    
    // mid value
    for(i=fstart; i<fend; i++)
        samples_mid[i] = samples_sum[i] / samplenum;

    // calculate max value
    short max = 0, min=9999;
    for(i=fstart; i<fend; i++)
    {
        if(samples_mid[i] > max) max = samples_mid[i];
        if(samples_mid[i] < min) min = samples_mid[i];
    }

    // normalize to the spec display heigth
    for(i=fstart; i<fend; i++)
    {
        samples_mid[i] = ((samples_mid[i]-min) * realh) / (max-min);    // mid values
    }
    
    // fill array of mid points
    gdPoint pnt[FFTSAMPLES+4];
    int idx = 0;
    pnt[idx].x = spec_width;
    pnt[idx].y = realh;
    idx++;
    pnt[idx].x = 0;
    pnt[idx].y = realh;
    idx++;
    for(i=fstart; i<fend; i++)
    {
        // normalize X axis
        pnt[idx].x = ((i-fstart)*spec_width)/(fend-fstart);
        pnt[idx].y = (int)(samples_mid[i]);
        if(pnt[idx].y >= spec_height) pnt[idx].y = spec_height;
        idx++;
    }
    pnt[idx].x = spec_width;
    pnt[idx].y = realh;

    // draw the spectrum, filled range
    gdImageFilledPolygon(dst,pnt,idx,0);
    
    // draw the real line 
    gdImageOpenPolygon(dst,pnt+2,idx-3,2);
    
    // draw grid
    for(i=fstart; i<fend; i++)
    {
        int x1 = ((i-fstart)*spec_width)/(fend-fstart);
        int x2 = ((i-fstart)*spec_width)/(fend-fstart);
        // draw grid
        if(i == 1400 || i == 1600)
        {
			gdImageLine(dst, x1, 0, x1, realh, 1);
            gdImageLine(dst, x2, 0, x2, realh, 1);
        }
		// WSPR mid band marker
		else if (i == 1500)
			gdImageLine(dst, x1, 0, x1, realh, 1);
		// 50 Hz marker
		else if(i == 1450 || i == 1550)
        {
            for(int j=0; j<realh; j+=5)
            {
                if(j>=realh) break;
                gdImageLine(dst, x1, j, x1, j+1, 1);
            }
        }
    }
    
    for(i=0; i<spec_width; i+=5)
    {
        if(i>=spec_width) break;
        
        gdImageLine(dst, i, spec_height/3, i+1, spec_height/3, 1);
        gdImageLine(dst, i, (spec_height*2)/3, i+1, (spec_height*2)/3, 1);
    }
}
