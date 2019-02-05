/*
 * WSPR Toolkit for Linux
 * ======================
 * by DJ0ABR 
 * 
 * upload.c
 * 
 * this program uploads new spots to wsprnet.org
 * it keeps the spots until they are uploaded successfully, even
 * if the system is stoppen and restarted
 * 
 * */

#include <pthread.h>
#include <alsa/asoundlib.h>
#include <dirent.h>
#include "wsprtk.h"
#include "debug.h"
#include "decode.h"
#include "upload.h"
#include "config.h"

void *uploadproc(void *pdata);
pthread_t upload_tid; 

int upload_init()
{
    // start a new process
    int ret = pthread_create(&upload_tid,NULL,uploadproc, 0);
    if(ret)
    {
        deb_printf("UPLOAD","uploadproc NOT started");
        return 0;
    }
    
    deb_printf("UPLOAD","OK: uploadproc started");
    return 1;
}

void *uploadproc(void *pdata)
{
char t[512];

    pthread_detach(pthread_self());
    while(running)
    {
        // check if the user has entered his callsign and qthloc already
        if(!strcmp(callsign,"NOCALL") || !strcmp(qthloc,"AA0AA"))
        {
            deb_printf("UPLOAD", "please enter your callsign and QTH locator !!!");
            char *fn = getDECfilename(wavdir);
            if(fn != NULL)
            {
                // delete all decoder files
                char t[256];
                snprintf(t,255,"rm %s",fn);
                system(t);
            }
            sleep(1);
        }
        
        char *fn = getDECfilename(wavdir);
        if(fn != NULL)
        {
            // decoded spots file found, upload to wsprnet.org
            // ignore if file too small (invalid)
            int size = getFileSize(fn);
			if(size < 5)
			{	
				// File is da, aber ohne sinnvollen Inhalt, löschen
				deb_printf("UPLOAD", "decoder file too small, ignored and deleted");
                snprintf(t,255,"rm %s",fn);
				system(t);
				continue;
			}
			// lade zu wsprnet hoch, mit 100s Timeout
			snprintf(t,255,"curl -s -S -F allmept=@%s -F call=%s -F grid=%s -m 20 http://wsprnet.org/meptspots.php",fn,callsign,qthloc); 
			deb_printf("UPLOAD", "Upload Spots: %s",t);
			// starte curl und benutze dabei eine Pipe um die Ausgaben zurückzulesen
			FILE *fp = popen(t,"r");
			if(fp)
			{
				// Liest die Ausgabe von curl
				int success = 0;
				while (fgets(t, sizeof(t)-1, fp) != NULL) 
				{
                    if(strstr(t,"out of"))
                    {
                        char *p = strchr(t,'<');
                        if(p) *p=0;
                        deb_printf("UPLOAD", "%s",t);
                    }
					if(strstr(t,"<html>"))
					{
						// erfolgreich hochgeladen, lösche Sammeldatei
						success = 1;
						deb_printf("UPLOAD", "Upload OK");
                        snprintf(t,255,"rm %s",fn);
                        system(t);
						//break;
					}
				}

				if(success == 0)
					deb_printf("UPLOAD", "Timeout: WSPR SPOTS not uploaded, try again later");

				pclose(fp);
			}
			else
				deb_printf("UPLOAD", "curl ERROR: WSPR SPOTS not uploaded, try again later");
        }
        sleep(1);
    }
    pthread_exit(NULL);
}

// scan for .wsprdec files in path and return the filename of the first one (or NULL if no file was found)
char *getDECfilename(char *path)
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
			p += (len - 8);
			if(strstr(p,".wsprdec"))
			{
				snprintf(filename,599,"%s/%s",path,dir->d_name);
				deb_printf("WSPRD","Decode-file found: %s", filename);
                closedir(d);
				return filename;
			}
		}
		closedir(d);
	}

	return NULL;
}

