/*
 * WSPR Toolkit for Linux
 * ======================
 * by DJ0ABR 
 * 
 * soundcard.c
 * 
 * this program searches and opens the soundcard, 
 * waits for the beginning of an even minute
 * records 51s
 * and then starts again from beginning
 * 
 * the resulting file is stored in "wavdir"
 * 
 * this task is running in a separate thread.
 * The thread can be closed by setting "running" to 0.
 * 
 * usage:
 * at program start call soundcard_init() once.
 * then just read and use the resulting wav files from "wavdir"
 * the exit set running = 0
 * 
 * */

#include <pthread.h>
#include <alsa/asoundlib.h>
#include <sndfile.h>
#include <time.h>
#include <sys/time.h>
#include "wsprtk.h"
#include "debug.h"
#include "soundcard.h"
#include "fft.h"
#include "config.h"
#include "coder.h"
#include "hopping.h"
#include "status.h"
#include "cat.h"
#include "kmtools.h"

int checkForTX();
int checkTime(int min, int sec);
int preparePlayback();
void stopPlayback();
int playback1s();
void maxval(short *pdata, int anz, short *pmean, short *ppeak);

int rec_channels = 2;           // 2=Stereo Soundcard
int playb_channels = 2;         // some sticks rec=1 and pb=2

short samples[CAPTURE_RATE * 2];            // 1 second, stereo
short samples2min[MAXSECONDS * WSPR_RATE];  // complete WSPR interval, mono 12000 samples/s
int samplecnt = 0;
struct timeval  tv_start;  // start time of current interval

void *sndproc(void *pdata);
pthread_t snd_tid; 
char wspr_cardname[80];
int wspr_cardnum = -1;
short mean,peak;

snd_pcm_t *capture_handle=NULL, *playback_handle=NULL;

// creates a thread to run all soundcard specific jobs
// call this once after program start
// returns 0=failed, 1=ok
int soundcard_init()
{
    // start a new process
    int ret = pthread_create(&snd_tid,NULL,sndproc, 0);
    if(ret)
    {
        deb_printf("SOUND","sndproc NOT started");
        return 0;
    }
    
    deb_printf("SOUND","OK: sndproc started");
    return 1;
}

// this process does all soundcard related jobs
void *sndproc(void *pdata)
{
int phase = 0;  // 0=wait, 1=record
int txrx = 0;   // 0=rx, 1=tx
int alcnt = 0;  // alive counter;
char lastsndcard[50] = {0};

    pthread_detach(pthread_self());

    deb_printf("SOUND","sndproc process started\n");
    
    while(running)
    {
        // re-read the TXRX configuration if it was changed
        // signaled by the existance of the file phpdir/updateqrgs.cmd
        char s[256];
        snprintf(s,sizeof(s),"%s/%s/updateqrgs.cmd",htmldir,phpdir);
        if(getFileSize(s) != -1)
        {
            snprintf(s,sizeof(s),"rm %s/%s/updateqrgs.cmd",htmldir,phpdir);
            system(s);
            readTXhopping();
        }
        
        // if the user selects another soundcard, reopen sound
        if(strcmp(sndcard_selection,lastsndcard))
        {
            // soundcard selection has changed
            exit_soundcard();
            capture_handle = NULL;
            playback_handle = NULL;
            strncpy(lastsndcard,sndcard_selection,sizeof(lastsndcard));
        }
        
        // wenn der handle NULL ist öffne die Soundkarte
        if(capture_handle == NULL && playback_handle == NULL)
        {
            scan_sndcards();
            int ret = restart_soundcard();
            if(ret)
            {
                phase = 0;
                txrx = 0;
                deb_printf("SOUND","soundcard: OK\n");
            }
            else
            {
                deb_printf("SOUND","error opening the soundcard, retrying ...\n");
                sleep(1);
                continue;
            }
        }
        
        if(capture_handle != NULL && playback_handle != NULL)
        {
            fsave_status(SNDALIVE,alcnt++);
        }
        
        // prüfe ob ein WSPR Intervall beginnt
        if(checkTime(0,0))
        {
            if(phase == 0) 
            {
                struct tm * now;
                gettimeofday(&tv_start, NULL);
                now = gmtime(&tv_start.tv_sec);
                samplecnt = 0;
                deb_printf("SOUND","Start new interval at: %d:%02d",now->tm_min, now->tm_sec);
                phase = 1;
            }
            
            if(checkForTX())
            {
                // TX Intervall beginnt
                wsprcoder();
                if(preparePlayback() == -1)
                {
                    deb_printf("SOUND","soundkarten Init-Playback Fehler\n");
                    exit_soundcard();
                    continue;
                }
                txrx = 1;
                ser_command = 2;    // set ptt 
            }
        }
        
        // actions at the end of an interval
        if(checkTime(1,53))
        {
            // release the PTT at the end of any interval
            ser_command = 3; // release ptt 
            // set the frequency for the NEXT interval
            readTXhopping();            // rebuild map
            setNextIntervalFrequency(); // set QRG for next interval
        }
        
        // do not transmit if the call/qthloc is invalid
        if(!strcmp(callsign,"NOCALL") || !strcmp(qthloc,"AA0AA"))
        {
            deb_printf("SOUND", "please enter your callsign and QTH locator !!!");
            txrx = 0;
        }
        
        if(txrx == 0)
        {
            // Record
            // die Soundkarte ist jetzt offen
            // Lese kontinuierlich alle samples aus, nicht benötigte Sample verwerfen, aber trotzdem auslesen
            int ret = record1s(MAXSECONDS,phase);
            if(ret == -1)
            {
                if(running)
                    deb_printf("SOUND","soundcard read error\n");
                exit_soundcard();
                continue;
            }
            if(ret == 1)
            {
                deb_printf("SOUND","WSPR Interval beendet\n");
                phase = 0;
            }
        }
        else
        {
            // playback
            // read samples and ignore, just check for errors
            // the delay of the read function gives also the required seconds-clock
            // for playback, since playback1s() is non blocking
            int ret = record1s(MAXSECONDS,0);
            if(ret == -1)
            {
                if(running)
                    deb_printf("SOUND","soundcard read error\n");
                exit_soundcard();
                continue;
            }
            
            ret = playback1s();
            if(ret == -1)
            {
                if(running)
                    deb_printf("SOUND","soundcard write error\n");
                exit_soundcard();
                continue;
            }
            if(ret == 1)
            {
                // playback finished
                stopPlayback();
                txrx = 0;
                phase = 0;
                ser_command = 3; // release ptt
            }
        }
    }
    exit_soundcard();
    deb_printf("SOUND","OK: sndproc stopped");
    
    pthread_exit(NULL);
}

// automatically scan the computer for a sound card which is usable for WSPR
// (if you have multiple soundcards then you may need to modify this)
// stores the number of the usable WSPR soundcard in wspr_cardnum, this number is then used to open the soundcard
// returns if soundcard found: 0=no, 1=yes
int scan_sndcards()
{
	int  err;
	int cardNum;
	cardNum = -1;			//ALSA starts numbering at 0 so this is intially set to -1.
	char cardname[80];
    
    if(strcmp(sndcard_selection,"auto"))
    {
        return 1;
    }
    
	deb_printf("SOUND","auto sndcard search mode selected. Seek available sound cards");

	while(1)
	{
		snd_ctl_t *cardHandle;

		// Get next sound card's card number.
		// when "cardNum" == -1, then ALSA fetches the first card
		if ((err = snd_card_next(&cardNum)) < 0)
		{
			deb_printf("SOUND","Can't get the next card number: %s", snd_strerror(err));
			break;
		}

		// no more cards? ALSA sets "cardNum" to -1 if so
		if (cardNum < 0) break;

		// open this cardNum's control interface.
		// we specify only the card number -- not any device nor sub-device
        char   str[64];
        sprintf(str, "hw:%i", cardNum);
        // the following line eventually prints "home dir not accessible", but all is working
        if ((err = snd_ctl_open(&cardHandle, str, 0)) < 0)
        {
            deb_printf("SOUND","Can't open card %i: %s", cardNum, snd_strerror(err));
            continue;
        }
		snd_ctl_card_info_t *cardInfo;	//Used to hold card information

		//We need to get a snd_ctl_card_info_t. Just alloc it on the stack
		snd_ctl_card_info_alloca(&cardInfo);
		//Tell ALSA to fill in our snd_ctl_card_info_t with info about this card
		if ((err = snd_ctl_card_info(cardHandle, cardInfo)) < 0)
			deb_printf("SOUND","Can't get info for card %i: %s", cardNum, snd_strerror(err));
		else
        {
            snprintf(cardname,79,"%s",snd_ctl_card_info_get_name(cardInfo));
			deb_printf("SOUND","sound card number %i = %s", cardNum, cardname);
            if(strstr(cardname,"USB") && !strstr(cardname,"HDMI"))
            {
                deb_printf("SOUND","usable for WSPR ? ... : yes");
                if(wspr_cardnum == -1)
                {
                    wspr_cardnum = cardNum;
                    strcpy(wspr_cardname,cardname);
                }
            }
            else
                deb_printf("SOUND","usable for WSPR ? ... : no");
        }

		// Close the card's control interface after we're done with it
		snd_ctl_close(cardHandle);
	}

	// ALSA allocates some mem to load its config file when we call some of the
	// above functions. Now that we're done getting the info, let's tell ALSA
	// to unload the info and free up that mem
    snd_config_update_free_global();
    
    if(wspr_cardnum == -1)
    {
        deb_printf("SOUND","no soundcard found");
        return 0;
    }
    
    deb_printf("SOUND","all soundcards checked, for WSPR use this card: %d %s", wspr_cardnum, wspr_cardname);
    return 1;
}

// initialize the soundcard for recording
// returns: 1=ok, 0=error
int init_soundcard()
{
	int err;
	snd_pcm_hw_params_t *hw_params;
	unsigned int rate = CAPTURE_RATE;
    char sndcard[80];
    
    if(strcmp(sndcard_selection,"auto"))
    {
        // no auto mode, use specified card
        strcpy(sndcard,wspr_cardname);
        strcpy(sndcard,sndcard_selection);
    }
    else
        snprintf(sndcard,sizeof(sndcard)-1,"hw:%i,0",wspr_cardnum);
    
    // if pulseaudio is used the uncomment these lines
	//strcpy(sndcard,"pulse");
	//strcpy(wspr_cardname,"pulse");

	deb_printf("SOUND","init soundcard: %s ... %s", wspr_cardname,sndcard);

	if ((err = snd_pcm_open(&capture_handle, sndcard, SND_PCM_STREAM_CAPTURE, 0)) < 0)
	{
		
        if(strstr(snd_strerror(err),"busy"))
            deb_printf("SOUND","this soundcard is already in use");
        else
            deb_printf("SOUND","cannot open audio device: %s (%s)", sndcard, snd_strerror(err));
		return 0;
	}

	if ((err = snd_pcm_hw_params_malloc(&hw_params)) < 0) {
		deb_printf("SOUND","cannot allocate hardware parameter structure (%s)", snd_strerror(err));
		return 0;
	}

	if ((err = snd_pcm_hw_params_any(capture_handle, hw_params)) < 0) {
		deb_printf("SOUND","cannot initialize hardware parameter structure (%s)", snd_strerror(err));
		return 0;
	}

	if ((err = snd_pcm_hw_params_set_access(capture_handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
		deb_printf("SOUND","cannot set access type (%s)", snd_strerror(err));
		return 0;
	}

	if ((err = snd_pcm_hw_params_set_format(capture_handle, hw_params, SND_PCM_FORMAT_S16_LE)) < 0) {
		deb_printf("SOUND","cannot set sample format (%s)", snd_strerror(err));
		return 0;
	}

	if ((err = snd_pcm_hw_params_set_rate_near(capture_handle, hw_params, &rate, 0)) < 0) {
		deb_printf("SOUND","cannot set sample rate (%s)", snd_strerror(err));
		return 0;
	}

	if ((err = snd_pcm_hw_params_set_channels(capture_handle, hw_params, rec_channels)) < 0) 
    {
		deb_printf("SOUND","cannot set channel count %d. (%s)", rec_channels,snd_strerror(err));
        deb_printf("SOUND","try as mono input card ...");
        rec_channels = 1;
        if ((err = snd_pcm_hw_params_set_channels(capture_handle, hw_params, rec_channels)) < 0) 
        {
            deb_printf("SOUND","cannot set channel count %d, give up. (%s)", rec_channels,snd_strerror(err));
            return 0;
        }
	}

	if ((err = snd_pcm_hw_params(capture_handle, hw_params)) < 0) {
		deb_printf("SOUND","cannot set parameters (%s)", snd_strerror(err));
		return 0;
	}

	snd_pcm_hw_params_free(hw_params);

	if ((err = snd_pcm_prepare(capture_handle)) < 0) {
		deb_printf("SOUND","cannot prepare audio interface for use (%s)", snd_strerror(err));
		return 0;
	}
	
	deb_printf("SOUND","OK: soundcard opened for recording");
    samplecnt = 0;

    return 1;
}

int init_soundcard_playback()
{
	int err;
	snd_pcm_hw_params_t *hw_params;
	unsigned int rate = CAPTURE_RATE;
    char sndcard[80];

    if(strcmp(sndcard_selection,"auto"))
    {
        // no auto mode, use specified card
        strcpy(sndcard,wspr_cardname);
        strcpy(sndcard,sndcard_selection);
    }
    else
    {
        snprintf(sndcard,sizeof(sndcard)-1,"hw:%i,0",wspr_cardnum);
    }

	deb_printf("SOUND","Initialize soundcard: %s ... %s", wspr_cardname,sndcard);

    if ((err = snd_pcm_open(&playback_handle, sndcard, SND_PCM_STREAM_PLAYBACK, 0)) < 0)
    {
        deb_printf("SOUND","please disable playback. cannot open audio device %s (%s)\n", sndcard, snd_strerror(err));
        return 0;
    }

    else if ((err = snd_pcm_hw_params_malloc(&hw_params)) < 0) {
        deb_printf("SOUND","please disable playback.cannot allocate hardware parameter structure (%s)\n", snd_strerror(err));
        return 0;
    }

    else if ((err = snd_pcm_hw_params_any(playback_handle, hw_params)) < 0) {
        deb_printf("SOUND","please disable playback. cannot initialize hardware parameter structure (%s)\n", snd_strerror(err));
        return 0;
    }

    else if ((err = snd_pcm_hw_params_set_access(playback_handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
        deb_printf("SOUND","please disable playback.cannot set access type (%s)\n", snd_strerror(err));
        return 0;
    }

    else if ((err = snd_pcm_hw_params_set_format(playback_handle, hw_params, SND_PCM_FORMAT_S16_LE)) < 0) {
        deb_printf("SOUND","please disable playback.cannot set sample format (%s)\n", snd_strerror(err));
        return 0;
    }

    else if ((err = snd_pcm_hw_params_set_rate_near(playback_handle, hw_params, &rate, 0)) < 0) {
        deb_printf("SOUND","please disable playback. cannot set sample rate (%s)\n", snd_strerror(err));
        return 0;
    }

    else if ((err = snd_pcm_hw_params_set_channels(playback_handle, hw_params, playb_channels)) < 0) {
        deb_printf("SOUND","please disable playback. cannot set channel count (%s)\n", snd_strerror(err));
        return 0;
    }

    else if ((err = snd_pcm_hw_params(playback_handle, hw_params)) < 0) {
        deb_printf("SOUND","please disable playback. cannot set parameters (%s)\n", snd_strerror(err));
        return 0;
    }
    else
    {
        snd_pcm_hw_params_free(hw_params);

        if ((err = snd_pcm_prepare(playback_handle)) < 0) {
            deb_printf("SOUND","please disable playback. cannot prepare audio interface for use (%s)", snd_strerror(err));
            return 0;
        }
    }
    deb_printf("SOUND","OK: soundcard open for playback");
    
	return 1;
}


void exit_soundcard()
{
	//deb_printf("SOUND","close soundcard");
	if(capture_handle) snd_pcm_close(capture_handle);
	if (playback_handle) snd_pcm_close(playback_handle);
	playback_handle = NULL;
	capture_handle = NULL;
    
}

int restart_soundcard()
{
	exit_soundcard();
	if(init_soundcard() == 0)
    {
        deb_printf("SOUND","Open soundcard for recording: FAILED\n");
        playback_handle = NULL;
        capture_handle = NULL;
        return 0;
    }
    
	if(init_soundcard_playback() == 0)
    {
        deb_printf("SOUND","Open soundcard for playback: FAILED\n");
        playback_handle = NULL;
        capture_handle = NULL;
        return 0;
    }
    return 1;
}

// wait for the begin of an even minute
// returns: 1=ok, 0=exit program
int wait_even_minute()
{
struct tm      *tm;

    deb_printf("SOUND","waiting for next WSPR interval");
    while(1)
    {
        struct timeval  tv;
        gettimeofday(&tv, NULL);
        tm = gmtime(&tv.tv_sec);
        
        if((tm->tm_min & 1) == 0 && tm->tm_sec == 0)  // original WSPR function
        //if((tm->tm_sec %10) == 0)   // for testing ONLY !!! starts a new interval every 10s
        {
            deb_printf("SOUND","WSPR interval started");
            return 1;
        }
        
        if(running == 0)
            return 0;
        
        usleep(100000);
    }
}

// records 1s from soundcard
// after maxseconds the record cycle is complete
//phase: 0=discard samples, 1=record samples
// ret: 0=OK, -1=sound-error, 1=all samples complete
int record1s(int maxseconds, int phase)
{
int err;

    if(capture_handle == NULL)
    {
        deb_printf("SOUND","record1s: no capture handle");
        return -1;
    }
    
    //printf( "start recording %ds",samplecnt);
    // record capture_rate samples, which is 1s
	if ((err = snd_pcm_readi(capture_handle, samples, CAPTURE_RATE)) != CAPTURE_RATE) 
    {
        if(running)
            deb_printf("SOUND","read from audio interface failed (%s)", snd_strerror(err));
        return -1;
    }
    
	// 1s is now recorded into "samples"
	maxval(samples,CAPTURE_RATE,&mean,&peak);
    fsave_status(PEAKLEVEL,(int)((double)(peak) * 100.0 / 32768.0));
	
	// save fft into a file for the waterfall
	doFFT(samples,samplecnt,rec_channels);

	if(!checkTime(1,51))
    {
        // copy this second to the previous samples
        // copy one channel only, ignore the other
        // copy every 4th sample to downsample from 48k to 12k
        if(samplecnt < maxseconds)
        {
            int id = 0;
            if(rec_channels == 2)
            {
                for(int i=0; i<(CAPTURE_RATE*2); i+=(2*4))	// (2*4) means: 2...one channel only, 4... every 4th sample
                {
                    int destidx = WSPR_RATE*samplecnt + id++;
                    if(destidx < sizeof(samples2min))
                    {
                        samples2min[destidx] = samples[i];
                    }
                }
            }
            else
            {
                for(int i=0; i<CAPTURE_RATE; i+=4)
                {
                    int destidx = WSPR_RATE*samplecnt + id++;
                    if(destidx < sizeof(samples2min))
                    {
                        samples2min[destidx] = samples[i];
                    }
                }
            }
            samplecnt++;
        }
    }
    else
    {
        if(phase == 0) return 0;  // waitung for first WSPR interval
        // stop recording at 01:51
		// Filename: YYMMDD_HHMM.wav
		struct tm * now;
		now = gmtime(&tv_start.tv_sec);
        now->tm_min = now->tm_min & ~1;

		char filename[256];
        unsigned int iqrg = (unsigned int)(trx_frequency*1000000);
		snprintf(filename,255,"%s/%02d%02d%02d_%02d%02dq%010d.wav",wavdir,now->tm_year-100,now->tm_mon+1,now->tm_mday,now->tm_hour,now->tm_min,iqrg);
		deb_printf("SOUND","all samples complete, store WAV file: %s",filename);
        
		int ret = saveWAV(filename, samples2min, WSPR_RATE*maxseconds, WSPR_RATE);
        if(ret == 0)
        {
            deb_printf("SOUND","ERROR: cannot save WAV file");
            return 1;
        }
        
        deb_printf("SOUND","max. level: %d [%d] = %d %%", peak, 32768, (int)peak * 100 / 32768);
        
        return 1;
	}

	return 0;
}

// save a WAV file
// returns: 1=ok, 0=error
// first save it to a temporary file, then copy this file to the real file
// this is required because the decoder process is constantly looking for WAV files
// and may use it before it is completely written
int saveWAV(char *filename, short *data, int anz, int rate)
{
    char fn[256];
    snprintf(fn,255,"%s/wspr_wav.tmp",wavdir);

    deb_printf("SNDFILE","save WAV file: %s",fn);
    SF_INFO rec_info;
    rec_info.samplerate = rate;
    rec_info.channels = 1;
    rec_info.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;

    // open wav file
    SNDFILE *wavfile;
    wavfile = sf_open(fn, SFM_WRITE, &rec_info);
    if (wavfile == NULL)
    {
        int err = sf_error(NULL);
        deb_printf("SNDFILE","Failed to open file: %s error=%d: %s", fn,err,sf_error_number(err));
        return 0;
    }

    int ret = sf_write_short(wavfile, data, anz);
	if (ret <= 0)
	{
		deb_printf("SNDFILE","write samples error");
        sf_close(wavfile);
        return 0;
	}

	sf_close(wavfile);
    deb_printf("SNDFILE","OK: WAV file stored");

    
    // copy temp file to real file
    char para[512];
    snprintf(para,511,"mv %s %s",fn, filename);
    system(para);
    
	return 1;
}

// calc max and mean value
void maxval(short *pdata, int anz, short *pmean, short *ppeak)
{
short p=0;
float fm = 0;

	for(int i=0; i<(anz*2); i++)
	{
		fm += pdata[i];
		if(pdata[i] > p) p=pdata[i];
	}
	*ppeak = p;
	fm /= anz;
	*pmean = (short)fm;
}   

// Playback WSPR TX File    
int txpos = 0;
short txStereosamples[CAPTURE_RATE*2];	// real samples to be sent within 1s

int prepare1sSamples()
{
int d=0;

	if(txpos >= (WSPR_RATE*MAXSECONDS))
	{
		return 0;
	}

	for(int i=0; i<WSPR_RATE; i++)
	{
		for(int j=0; j<(4*2); j++)	// 4=conversion 12k->48k und 2=mono->stereo
		{
			txStereosamples[d++] = txsamples[txpos];
		}
		txpos++;
	}

	return 1;
}

int preparePlayback()
{
int err;

	txpos = 0;			// start at the beginning of the WSPR samples in txsamples
	
	if(playback_handle == NULL) 
    {
        deb_printf("SOUND","no playback handle, soundcard not opened for playback");
        return -1;
    }
	
	deb_printf("SOUND","bereite das Senden von einem WSPR Intervall vor\n");
    // prepare output stream
	if ((err = snd_pcm_prepare(playback_handle)) < 0) 
	{
		deb_printf("SOUND","cannot prepare audio interface for use (%s)\n", snd_strerror(err));
		return -1;
	}

	// and send first second in advance to avoid buffer underrun
	prepare1sSamples();	// generate a 1s long 48k stream into txStereosamples
	if ((err = snd_pcm_writei(playback_handle, txStereosamples, CAPTURE_RATE)) != CAPTURE_RATE) {
		deb_printf("SOUND","1: playsound: write to audio interface failed (%s)\n", snd_strerror(err));
		return -1;
	}

	prepare1sSamples();	// generate a 1s long 48k stream into txStereosamples
	if ((err = snd_pcm_writei(playback_handle, txStereosamples, CAPTURE_RATE)) != CAPTURE_RATE) {
		deb_printf("SOUND","2: playsound: write to audio interface failed (%s)\n", snd_strerror(err));
		return -1;
	}

	// start the playback
	snd_pcm_start(playback_handle);
    return 0;
}

void stopPlayback()
{
    snd_pcm_drain(playback_handle); // stops playback if the buffer is sent
    // snd_pcm_drop(playback_handle); stops immediately, but we have to send the buffer
    deb_printf("SOUND","playback beendet\n");
}

// play all WSPR samples
int playback1s()
{
	int err;

	if(prepare1sSamples() == 1)
	{		
		//deb_printf("SOUND","sende %d. TX samples",txpos);
		if ((err = snd_pcm_writei(playback_handle, txStereosamples, CAPTURE_RATE)) != CAPTURE_RATE) {
			deb_printf("SOUND","playsound: write to audio interface failed (%s)", snd_strerror(err));
			return -1;
		}
        return 0;
	}
	return 1;
}

// in %, card:  "hw1:0" o.ä.
void SetAlsaMasterVolume(long volume, char *card)
{
    long min, max;
    snd_mixer_t *handle;
    snd_mixer_selem_id_t *sid;
    const char *selem_name = "Master";

    snd_mixer_open(&handle, 0);
    snd_mixer_attach(handle, card);
    snd_mixer_selem_register(handle, NULL, NULL);
    snd_mixer_load(handle);

    snd_mixer_selem_id_alloca(&sid);
    snd_mixer_selem_id_set_index(sid, 0);
    snd_mixer_selem_id_set_name(sid, selem_name);
    snd_mixer_elem_t* elem = snd_mixer_find_selem(handle, sid);

    snd_mixer_selem_get_playback_volume_range(elem, &min, &max);
    snd_mixer_selem_set_playback_volume_all(elem, volume * max / 100);

    snd_mixer_close(handle);
}

// in %
void SetAlsaCaptureVolume(long volume, char *card)
{
    long min, max;
    snd_mixer_t *handle;
    snd_mixer_selem_id_t *sid;
    const char *selem_name = "Master";

    snd_mixer_open(&handle, 0);
    snd_mixer_attach(handle, card);
    snd_mixer_selem_register(handle, NULL, NULL);
    snd_mixer_load(handle);

    snd_mixer_selem_id_alloca(&sid);
    snd_mixer_selem_id_set_index(sid, 0);
    snd_mixer_selem_id_set_name(sid, selem_name);
    snd_mixer_elem_t* elem = snd_mixer_find_selem(handle, sid);

    snd_mixer_selem_get_capture_volume_range(elem, &min, &max);
    snd_mixer_selem_set_capture_volume_all(elem, volume * max / 100);

    snd_mixer_close(handle);
}
