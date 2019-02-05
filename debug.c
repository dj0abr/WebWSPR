/*
 * WSPR Toolkit for Linux
 * ======================
 * by DJ0ABR 
 * 
 * debug.c
 * 
 * this program formats nice console messages
 * and is used to print information of the program flow 
 * into the console
 * 
 * */

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <pthread.h>
#include "debug.h"
#include "wsprtk.h"

int makeprot = 0;

void current_time(char *text) 
{
	struct timeval  tv;
	struct tm      *tm;
	
	gettimeofday(&tv, NULL);
	tm = gmtime(&tv.tv_sec);
	
    strftime(text,255,"%Y.%m.%d %H.%M.%S ",tm);
}

void deb_printf(char const *src, char *fmt, ...)
{
	char s[10000];
	char text[10000] = { "" };
	
	va_list ap;
	va_start(ap, fmt);
	
	current_time(text);
	sprintf(text+strlen(text),"[%9.9s] ", src);
	vsprintf(s,fmt,ap);
	sprintf(text+strlen(text),"%s\n",s);
	va_end(ap);

	// drucke in Konsole
    for(int i=0; i<strlen(text); i++)
        if(text[i] == '\n') text[i] = ' ';
        
    printf("%s\n", text);
    
    if(makeprot)
    {
        // und schreibe in Protokolldatei
        FILE *fw=fopen("protokoll.txt","a");
        if(fw)
        {
            fprintf(fw,"%s\n", text);
            fclose(fw);
        }
        else
            printf("kann Protokolldatei nicht öffnen\n");
    }
}


