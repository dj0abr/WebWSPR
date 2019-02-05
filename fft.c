#include <fftw3.h>
#include <math.h>
#include "debug.h"
#include "soundcard.h"
#include "wsprtk.h"
#include "waterfall.h"
#include "config.h"

fftw_complex *din48 = NULL;					// input data for  fft, output data from ifft
fftw_complex *cpout48 = NULL;				// ouput data from fft, input data to ifft, 24000 values = 1 value/Hz
fftw_plan plan = NULL;
int fftcnt;
int capture_rate = CAPTURE_RATE;

void init_fft()
{
	char fn[300];

	deb_printf("FFT","init fft");

	sprintf(fn, "%s/capture_fft_%d", wavdir, capture_rate);	// wisdom file for each capture rate

	if (fftw_import_wisdom_from_filename(fn) == 0)
	{
		deb_printf("FFT","creating fft wisdom file");
	}

	din48 = (fftw_complex *)fftw_malloc(sizeof(fftw_complex) * capture_rate);
	cpout48 = (fftw_complex *)fftw_malloc(sizeof(fftw_complex) * capture_rate);

	plan = fftw_plan_dft_1d(capture_rate, din48, cpout48, FFTW_FORWARD, FFTW_MEASURE);
	
	if (fftw_export_wisdom_to_filename(fn) == 0)
	{
		deb_printf("FFT","export fft wisdom error");
	}
}

void exit_fft()
{
	deb_printf("FFT","clean up fft");
	if(plan) fftw_destroy_plan(plan);
	if(din48) fftw_free(din48);
	if(cpout48) fftw_free(cpout48);

	plan = NULL;
	din48 = NULL;
	cpout48 = NULL;
}

// samples: Anzahl für 1s also capture_rate
// und zwar Stereo Samples
void doFFT(short *samples,int samplecnt, int chans)
{
int i;

    if(chans == 2)
    {
        for (i = 0; i < capture_rate; i++)
        {
            // fill samples into the fft input buffer
            // weil Stereo, nehme nur einen Kanal
            din48[i][0] = samples[i*2];
            din48[i][1] = samples[i*2+1];
        }
    }
    else
    {
        for (i = 0; i < capture_rate; i++)
        {
            // fill samples into the fft input buffer
            din48[i][0] = samples[i];
            din48[i][1] = samples[i+1];
        }
    }
    
	// calculate spectrum of a 1s long stream
	fftw_execute(plan);
	fftcnt = capture_rate / 2 + 1; // positive freq, the rest to capture_rate are the neg freq in reverse order
	
	// generate FFT data
    float real, imag, mag;
    unsigned int data[fftcnt];

	// erzeuge Datensatz
	for (int i = 0; i < fftcnt; i++)
	{
		// calculate absolute value
		real = cpout48[i][0];
		imag = cpout48[i][1];
		mag = sqrt((real * real) + (imag * imag));

		// scaling not required since max possible value fits in int
		// the max possible value is 32768 * 24000 (input size * N)

		data[i] = (unsigned int)(mag);
	}
	
	// draw waterfall
	makeWF(data, fftcnt, samplecnt);
}

