/*
 * WSPR Toolkit for Linux
 * ======================
 * by DJ0ABR 
 * 
 * wsprtk.c
 * 
 * all jobs are started here
 * 
 * to compile this program you need to install:
 * libsndfile
 * libsndfile-dev(el)
 * alsa
 * alsa-dev(el)
 * libfftw3-3
 * fftw3-dev(el)
 * libgd3-dev
 * php
 * 
 * /etc/apache2/apache2.conf:
 * under htaccess: required all denied, change to require all granted
 * 
 * Installation on a fresh Raspian-Lite:
 * =====================================
 * libasound2-dev
 * libfftw3-dev
 * libgd3
 * libgd-dev
 * apache2
 * sndfile-tools
 * libsndfile-dev
 * php
 * 
 * * Installation on a fresh Raspi-tinyCore
 * ========================================
 * WebWSPR must be already compiled on a normal Raspi (Raspbian)
 * (it cannot be compiled on tinyCore since too many libs are missing)
 * we need to install these additional packages (with the tce command, searching for the following packages):
 * libsndfile-dev.tcz
 * fftw.tcz
 * libgfortran.tcz
 * freetype-dev.tcz
 * fontconfig.tcz
 * libXpm-dev.tcz
 * libgd ... not available, copy it from a normal raspian installation
 * 
 * 
 * */

#include <unistd.h>
#include <alsa/asoundlib.h>
#include <signal.h>
#include <time.h>
#include <sys/time.h>
#include "debug.h"
#include "soundcard.h"
#include "decode.h"
#include "wsprtk.h"
#include "upload.h"
#include "JSON.h"
#include "fft.h"
#include "config.h"
#include "kmtools.h"
#include "coder.h"
#include "cat.h"
#include "mysql.h"
#include "hopping.h"
#include "dds.h"
#include "hamlib.h"
#include "waterfall.h"
#include "status.h"

void INThandler(int sig);
void silent_stop();
void isRunning();

char wavdir[256] = {"/tmp"};    // recorded wav files are stored here (using a RAM disk will save your SD card !)
char htmldir[256] = { "." };
char phpdir[256] = { "phpdir" };// located always under htmldir
char decoderfile[256];
int systype = SYSTYPE_ARM32;    // 0=PC 1=ARM ...    
int running = 1;                // set to 0 to end this process
int alive=0;

void searchHTMLpath()
{
    if(htmldir[0] == '.')
    {
        // search for the apache woking directory
        system("find  /srv -xdev -name htdocs  -print > pfad 2>/dev/null");
        system("find  /var -xdev -name htdocs  -print >> pfad 2>/dev/null");
        system("find  /var -xdev -name html  -print >> pfad 2>/dev/null");
        system("find  /usr -xdev -name htdocs  -print >> pfad 2>/dev/null");
        // if the directory was found its name is in file: "pfad"
        FILE *fp=fopen("./pfad","r");
        if(fp)
        {
            char p[256];
            fgets(p,255,fp);
            char *cp= cleanString(p,0);
            if(strlen(cp)>3)
            {
                strcpy(htmldir,cp);
                deb_printf("SYSTEM","Webserver Path: %s",htmldir);
            }
            else
            {
                deb_printf("SYSTEM","Path to apache webserver files not found");
                exit(0);
            }
            
            fclose(fp);
        }
        else
        {
            deb_printf("SYSTEM","Path to apache webserver files not found");
            exit(0);
        }
    }
}

/*
 * this directory is used to store data entered by the web user and received by PHP
 * (mainly configuration data)
 * 
 * these date are then read, verified and processed by this program 
 * 
 * this folder must have the owner: apache
 * which is in ubuntu:  www-data:www-data
 * and in opensuse: wwwrun:wwwrun
 * and in tinyCore: tc  staff
   * */

void detectOS()
{
    char s[256];
    
    // check if this system is x86 or ARM based with uname -a
    // PC-opensuse: Linux linux-on8s 4.12.14-lp150.12.25-default #1 SMP Thu Nov 1 06:14:23 UTC 2018 (3fcf457) x86_64 x86_64 x86_64 GNU/Linux
    // PC-Mint: Linux josef-X550WE 4.15.0-43-generic #46~16.04.1-Ubuntu SMP Fri Dec 7 13:31:08 UTC 2018 x86_64 x86_64 x86_64 GNU/Linux
    // tinycode: Linux box 4.9.22-piCore-v7 #1 SMP Sat Apr 15 12:27:07 UTC 2017 armv7l GNU/Linux
    // odroid 32bit: Linux odroid 4.14.94-155 #1 SMP PREEMPT Wed Jan 23 05:19:08 -02 2019 armv7l armv7l armv7l GNU/Linux
    // odroid 64 bit: Linux odroid 3.16.61-34 #1 SMP PREEMPT Fri Dec 21 13:51:02 -02 2018 aarch64 aarch64 aarch64 GNU/Linux
    // Raspberry: Linux raspi 4.14.79-v7+ #1159 SMP Sun Nov 4 17:50:20 GMT 2018 armv7l GNU/Linux
    
    snprintf(s,255,"uname -a > /tmp/systemtype");
    system(s);

    FILE *fp = fopen("/tmp/systemtype","r");
    if(fp)
    {
        if(fgets(s,255,fp))
        {
            if(strstr(s,"x86"))
            {
                deb_printf("SYSTEM","x86 based OS detected");
                if(strstr(s,"Ubuntu"))
                {
                    deb_printf("SYSTEM","Ubuntu based OS detected");
                    systype = SYSTYPE_PC_UBUNTU;
                }
                else
                {
                    deb_printf("SYSTEM","rpm based OS detected");
                    systype = SYSTYPE_PC_SUSE;
                }
            }
            else if(strstr(s,"piCore"))
            {
                deb_printf("SYSTEM","tinyCore OS detected");
                systype = SYSTYPE_TINYCORE;
            }
            else if(strstr(s,"aarch64"))
            {
                deb_printf("SYSTEM","arm 64bit based OS detected");
                systype = SYSTYPE_ARM64;
            }
            else if(strstr(s,"arm"))
            {
                deb_printf("SYSTEM","arm 32bit based OS detected");
                systype = SYSTYPE_ARM32;
            }
        }
        fclose(fp);
    }
    else
    {
        deb_printf("SYSTEM","cannot detect OS type, unknown OS");
        exit(0);
    }
    
    // select the decoder file for the OS version
    strcpy(decoderfile,"wsprd_pc");
    if(systype == SYSTYPE_TINYCORE || systype == SYSTYPE_ARM32)
        strcpy(decoderfile,"wsprd_arm");
    if(systype == SYSTYPE_ARM64)
        strcpy(decoderfile,"wsprd_aarch64");
    
    // check if a valid decoder exists
    int len = getFileSize(decoderfile);
    if(len == -1 || len < 40000)
    {
        deb_printf("SYSTEM","a valid WSPR decoder file \"wsprd_xx\" does not exist. Installation not complete\n");
        INThandler(0);
    }
    
    // wsprd exists, set executable permissions for all files
    // it is not required for all files, but we have  some scripts, so lets do it for all
    char cmd[256];
    sprintf(cmd,"chmod 755 *");
    system(cmd);
}

void setFilePermissions()
{
char s[256];

    if(systype == SYSTYPE_PC_SUSE)
    {
        snprintf(s,255,"chown -R wwwrun:www %s/*",htmldir);
        system(s);
    }
    
    else if(systype == SYSTYPE_TINYCORE)
    {
        snprintf(s,255,"chown -R tc:staff %s/*",htmldir);
        system(s);
    }
    
    else
    {
        snprintf(s,255,"chown -R www-data:www-data %s/*",htmldir);
        system(s);
    }
    
    snprintf(s,255,"chmod -R 644 %s/*",htmldir);
    system(s);
    snprintf(s,255,"chmod 755 %s/phpdir",htmldir);
    system(s);
}

void checkPHPdir()
{
char s[256];
int cpdefault = 0;

    // check the type of linux system
    detectOS();
    
    // check if the configfile is in its folder
    snprintf(s,255,"%s/%s/wsprconfig.js",htmldir,phpdir);
    int sz = getFileSize(s);
    if(sz == -1)
    {
        // it's not there, copy the default file to phpdir
        cpdefault = 1;
        // create phpdir if not exists
        snprintf(s,255,"mkdir %s/%s",htmldir,phpdir);
        system(s);
    }
    else
    {
        // check if the default file is larger then the existing file
        snprintf(s,255,"%s/wsprconfig.default",htmldir);
        int defsz = getFileSize(s);
        if(defsz > sz)
            cpdefault = 1;
    }
    
    if(cpdefault == 1)
    {
        deb_printf("SYSTEM","set DEFAULT configuration, enter your callsing + locator !");
        snprintf(s,255,"cp %s/wsprconfig.default %s/%s/wsprconfig.js",htmldir,htmldir,phpdir);
        system(s);
    }
    
    setFilePermissions();
}

void installHTMLfiles()
{
    // the HTML files are located in the htdocs folder below the wspr folder
    // copy these files into the Apache HTML folder (only if newer: update)
    char fn[512];
    snprintf(fn,sizeof(fn),"cp ./htdocs/* %s",htmldir);
    deb_printf("SYSTEM","copy Web Site files to: %s",htmldir);
    FILE *fp = popen(fn,"r");
    if(fp)
    {
        // Liest die Ausgabe von rsync
        while (fgets(fn, sizeof(fn)-1, fp) != NULL) 
        {
            //printf("Output: %s\n",fn);
        }
        pclose(fp);
    }
}

void killOtherInstances()
{
    char s[512];
    snprintf(s,sizeof(s),"mypid=$$; echo mypid: $mypid; declare pids=$(pgrep -f wsprtk); for pid in ${pids[@]/$mypid/}; do echo $pid; kill $pid; sleep 1; done");
    //printf("%s\n",s);
    //system(s);
    // sudo muss auch noch ausgenommen werden damit das geht
}

// check if the user want to switch on/off wsprtk
// this is signaled by a file in phpdir/onoff.cmd
// returns: -1=no action, 0=user wants OFF, 1=user wants ON
int check_onoff()
{
    int onoff = -1;
    char fn[256];
    
    snprintf(fn,sizeof(fn),"%s/%s/onoff.cmd",htmldir,phpdir);
    FILE *fr = fopen(fn,"r");
    if(fr)
    {
        char s[11];
        if(fgets(s,10,fr) != NULL)
        {
            deb_printf("SYSTEM","System Command: %s\n",s);
            if(s[1] == 'F')
                onoff = 0;
            else
                onoff = 1;
        }
        printf("%s end read\n",fn);
        fclose(fr);
        // and remove the signaling file
        snprintf(fn,sizeof(fn),"rm %s/%s/onoff.cmd 2>/dev/null",htmldir,phpdir);
        system(fn);
    }

    return onoff;
}

void handle_user_switchON(int a)
{
    // wsprtk must run in an endless loop (batch script)
    // the first call in this script must be without any argument
    // then the loop starts, and all calls in the loop are with one random argument
    // if no command line arg is given then it was started manualy or for the first time
    // in this case just start
    // if an argument is give, check onoff.cmd to see if we should start or wait 
char s[256];

    if(a >= 3)
    {
        deb_printf("SYSTEM", "wsprtk waiting for START command ...");
        // system was stopped by user command
        // wait for user command ON
        while(check_onoff() != 1)
        {
            usleep(100000);
            sleep(1);
        }
        running = 1;
    }
    else
    {
        deb_printf("SYSTEM", "normal start ...");
        // normal start, ignore onoff.cmd
        snprintf(s,sizeof(s),"rm %s/%s/onoff.cmd 2>/dev/null",htmldir,phpdir);
        system(s);
    }
}

int main(int argc, char *argv[])
{
char s[256];

    // check if it is already running, if yes then exit
    isRunning();
    
    // delete old OFF file
    // and remove the signaling file
    snprintf(s,sizeof(s),"rm %s/%s/onoff.cmd 2>/dev/null",htmldir,phpdir);
    system(s);

    if(argc == 1)
    {
        deb_printf("SYSTEM", "start:  sudo ./startwspr");
        exit(0);
    }

    deb_printf("SYSTEM", "wsprtk started, setting up system ...");
    
    killOtherInstances();
    
    // init random numer generator
    srand(time(NULL));
    
    // look for the apache HTML path
    deb_printf("SYSTEM","search HTML path");
    searchHTMLpath();
    
    // Install or Update the html files
    deb_printf("SYSTEM","install web files");
    installHTMLfiles();
    
    // set file permissions of the HTML files
    deb_printf("SYSTEM","set file permissions");
    checkPHPdir();

    // handle user command: start/stop
    handle_user_switchON(argc);
    
    // generate the HAMLIB transceiver table
    deb_printf("SYSTEM","generate HAMLIB transceiver table");
    hamlib_maketable();
    
    // read the JS configuration from the HTML directory
    deb_printf("SYSTEM","read initial configuration");
    readConfigSilent();
    
    // make the initial JSON table file from old ALL_WSPR.TXT
    deb_printf("SYSTEM","generate lists");
    saveSpotsJSON();    
    
    // init the hopping table
    deb_printf("SYSTEM","make hopping table");
    readTXhopping();

    // restore WF picture
    restoreWF();

    // initialize FFT
    deb_printf("SYSTEM","init wwf and waterfall");
    init_fft();

    // this handler captures the Ctrl-C key and closes
    // all processes before exiting
    signal(SIGINT, INThandler);  
    
    // resore ALL_WSPR.TXT
    //sprintf(s,"cp %s/ALL_WSPR.TXT %s",htmldir,wavdir);
    //system(s);
    
    // start process to capture WSPR intervals from the sound card
    // and save the WAV files on disk
    soundcard_init();
    
    // start process to look for WAV files
    // if WAV file found, decode it using K1JTs WSPR decoder: wsprd
    // and write the resulting files on disk
    
    decode_wspr_init();
    
    // start the upload process to wsprnet.org
    // if a valid decoder file is available it will be uploaded
    // and then deleted
    upload_init();
    
    // init the serial interface process
    cat_init();
    
    // init the spots downloader from HELITRON server
    mysql_init();
    
    // main loop
    int ucnt = 0;
    while(1) 
    {
        // re-read config 5s befor the end of an interval
        struct timeval  tv;
        struct tm      *tm;
        gettimeofday(&tv, NULL);
        tm = localtime(&tv.tv_sec);
        
        if((tm->tm_min & 1) == 1 && tm->tm_sec == 45)
        {
            readConfigSilent();
            sleep(1);       // avoid redoing above actions in the same second
        }
        
        // every 5s update the (next)frequency display-file status.txt
        if(++ucnt >= 10)
        {
            updateNextFreq();
            ucnt = 0;
        }
        
        // check if the config file wsprconfig.js was updated by php
        snprintf(s,sizeof(s),"%s/%s/updateconfig.cmd",htmldir,phpdir);
        if(getFileSize(s) != -1)
        {
            readConfigSilent();
            
            if(unlink(s) != 0)
                printf("unlink %s error\n",s);
                
            // if we are on tinyCore save to SD card
            if(systype == SYSTYPE_TINYCORE)
            {
                deb_printf("SYSTEM", "save WSPR config to SD-card");
                snprintf(s,sizeof(s),"filetool.sh -b &");
                system(s);
            }
        }
        
        // check if php requests reread of the config and list update
        snprintf(s,sizeof(s),"%s/%s/updatelists.cmd",htmldir,phpdir);
        if(getFileSize(s) != -1)
        {
            // overwrite all SPOT files in html with the empty default file
            clearSpotFiles();
            
            // file exists, read database now
            // and delete this file
            //printf("unlink %s\n",s);
            if(unlink(s) != 0)
                printf("unlink %s error\n",s);
            readConfigSilent();
            read_db = 1;    // tell mysql to read the database
        }
        
        if(check_onoff() == 0)
        {
            // User requests stopping this program
            INThandler(0);
            // the program exists inside the INT handler function
        }
        
        // add some fast counter to the second, because one second is too slow for the alive marker
        int av = alive++;
        av <<= 16;
        av += getIntervalSecond();
        fsave_status(SYSALIVE,av);

        usleep(500000);
    }
    return 0;
}

void INThandler(int sig)
{
    deb_printf("SYSTEM", "\nstopped by Ctrl-C, cleaning system ...");
    deb_printf("SYSTEM", "backup ALL_WSPR.TXT");
    char s[256];
    sprintf(s,"cp %s/ALL_WSPR.TXT %s",wavdir,htmldir);
    system(s);
    ser_command = 3;
    running = 0;
    if(txmode == 2) init_dds(); // init releases the PTT
    exit_fft();
    deb_printf("SYSTEM", "stop and exit.");
    sleep(1);
	exit(0);
}

// check if wsprtk is already running
void isRunning()
{
    int num = 0;
    char s[256];
    sprintf(s,"ps -e | grep wsprtk");
    
    FILE *fp = popen(s,"r");
    if(fp)
    {
        // gets the output of the system command
        while (fgets(s, sizeof(s)-1, fp) != NULL) 
        {
            if(strstr(s,"wsprtk") && !strstr(s,"grep"))
            {
                if(++num == 2)
                {
                    deb_printf("SYSTEM", "wsprtk is already running, do not start twice !");
                    pclose(fp);
                    exit(0);
                }
            }
        }
        pclose(fp);
    }
}
