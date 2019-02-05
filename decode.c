/*
 * WSPR Toolkit for Linux
 * ======================
 * by DJ0ABR 
 * 
 * decode.c 
 * 
 * this program looks for WAV files and decodes it
 * 
 * */

#include <pthread.h>
#include <alsa/asoundlib.h>
#include <dirent.h>
#include <time.h>
#include <sys/time.h>
#include "wsprtk.h"
#include "debug.h"
#include "decode.h"
#include "JSON.h"
#include "config.h"
#include "soundcard.h"
#include "hopping.h"
#include "status.h"

void *wsprdproc(void *pdata);
pthread_t wsprd_tid; 

int decode_wspr_init()
{
    // start a new process
    int ret = pthread_create(&wsprd_tid,NULL,wsprdproc, 0);
    if(ret)
    {
        deb_printf("WSPRD","wsprdproc NOT started");
        return 0;
    }
    
    deb_printf("WSPRD","OK: wsprdproc started");
    return 1;
}

void *wsprdproc(void *pdata)
{
char parameter[512];

    pthread_detach(pthread_self());
    while(running)
    {
        char *longfn = getWAVfilename(wavdir);
        if(longfn != NULL)
        {
            // evaluate wav filename and extract date und freq
            // longfn ... wav filename including date and qrg, Format: YYMMDD_HHMMqxxx.wav, xxx=10 chars freq in Hz
            // fn ... filename including date only and without extension
            
            // extract frequency and fn
            char fn[256];
            strcpy(fn,longfn);
            
            // WAV file found
            // the last 10 chars before the '.' are the frequency in Hz
            char *p = strchr(fn,'.');
            if(!p) 
            {
                deb_printf("WSPRD", "error in wav filename, no dot found");
                snprintf(parameter,511,"rm %s",longfn);
                system(parameter);
                sleep(1);
                continue;
            }
            *p=0;   // terminate qrg
            p = strchr(fn,'q');
            if(!p) 
            {
                deb_printf("WSPRD", "error in wav filename, no \"q\" found");
                snprintf(parameter,511,"rm %s",longfn);
                system(parameter);
                sleep(1);
                continue;
            }
            double decoder_qrg = atof(p+1);
            decoder_qrg /= 1000000;
            if(decoder_qrg < 0.1)
            {
                deb_printf("WSPRD", "error in wav filename, invalid frequency");
                snprintf(parameter,511,"rm %s",longfn);
                system(parameter);                
                sleep(1);
                continue;
            }
            *p=0;   // terminate filename
            
            fsave_status(DECODER, 1);
            
            deb_printf("WSPRD", "wav-frequency:%f  filename(no ext):%s",decoder_qrg,fn); 
            
            // remove frequency from wav filename by renaming it
            snprintf(parameter,sizeof(parameter),"mv %s %s.wav",longfn,fn);
            //deb_printf("WSPRD","renaming wav file for decoding: %s\n",parameter);
            system(parameter);
            
            static int use_new_decoder = 1;
            if(use_new_decoder)
                snprintf(parameter,255,"./%s -d -a %s -w -f %f %s.wav",decoderfile,wavdir,decoder_qrg,fn);
            else
                snprintf(parameter,255,"./%s -a %s -w -f %f %s.wav",decoderfile,wavdir,decoder_qrg,fn);
			
            // start the decoder wsprd and use a pipe to read back the outputs
			FILE *fp = popen(parameter,"r");
			if(fp)
			{
				while (fgets(parameter, sizeof(parameter)-1, fp) != NULL) 
				{
                    // check for error
                    char *p = strstr(parameter,"Usage");
                    if(p)
                    {
                        // error message, wrong parameter
                        // we are using the old decoder
                        use_new_decoder = 0;
                        deb_printf("WSPRD", "old decoder found, switch to old command parameters");
                        break;
                    }
                    deb_printf("WSPRD", "%s", parameter);
				}
				pclose(fp);
			}
			else
				deb_printf("WSPRD", "ERROR: WSPR decoder not started");
			//system(parameter);
            
			// delete wav file
			snprintf(parameter,255,"rm %s.wav",fn);
			system(parameter);
            
            // the resulting spots are in file wspr_spots.txt
            // build the JSON table data format
            saveSpotsJSON();
            
            // copy it to *.wsprdec which is then used i.e. to upload it to wsprnet.org
            snprintf(parameter,255,"cp %s/wspr_spots.txt %s.wsprdec",wavdir,fn);
			//deb_printf("WSPRD", "rename for upload: %s",parameter);
			system(parameter);
            
            fsave_status(DECODER, 0);
        }
        
        usleep(100000);
    }
    
    pthread_exit(NULL);
}

// scan for WAV files in path and return the filename of the first one (or NULL if no file was found)
char *getWAVfilename(char *path)
{
DIR *d;
struct dirent *dir;
static char filename[600] = {0};

	d = opendir(path);
	if (d) 
	{
		while ((dir = readdir(d)) != NULL) 
		{
			char *p = dir->d_name;
			int len = strlen(p);
			if(len < 6) continue;
			p += (len - 4);
			if(strstr(p,".wav"))
			{
				snprintf(filename,599,"%s/%s",path,dir->d_name);
				deb_printf("WSPRD","WAV-file found: %s", filename);
                closedir(d);
				return filename;
			}
		}
		closedir(d);
	}

	return NULL;
}
