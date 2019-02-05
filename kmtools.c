/*
 * WSPR Toolkit for Linux
 * ======================
 * by DJ0ABR 
 * 
 * kmtools.c
 * 
 * various useful functions
 * 
 * */

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include "kmtools.h"

BANDLIST myband[NUMOFBANDS] = 
{
    // do not change, it has no meaning !
    // these are just default values which are overwritten
    // at program start with the values from ...phpdir/wsprconfig.js
{"2200",0.1360},
{"630" ,0.4742},
{"160" ,1.8366},
{"80"  ,3.5926},
{"60"  ,5.3647},
{"40"  ,7.0386},
{"30"  ,10.1387},
{"20"  ,14.0956},
{"17"  ,18.1046},
{"15"  ,21.0946},
{"12"  ,24.9246},
{"10"  ,28.1246},
{"6"   ,50.2930},
{"4"   ,70.0910},
{"2"   ,144.4890},
{"70"  ,432.3000},
{"23"  ,1296.500}
};

// cleans white spaces from beginning, middle and end of a string
char *cleanString(char *str, int cleanspace)
{
static char hs[256];
char *hp = str;
int idx = 0;

    // putze den Anfang
    while(*hp == ' ' || *hp == ',' || *hp == '\n' || *hp == '\r' || *hp == '\'' || *hp == '\"')
    {
 hp++;
    }
    
    // putze die Mitte
    if(cleanspace)
    {
 while(*hp)
 {
     if(*hp != ' ' && *hp != ',' && *hp != '\n' && *hp != '\r' && *hp != '\'' && *hp != '\"')
  hs[idx++] = *hp;
     hp++;
 }
    }
    else
    {
 while(*hp)
     hs[idx++] = *hp++;
    }
    
    // putze das Ende
    hp = hs+idx-1;

    while(*hp == ' ' || *hp == ',' || *hp == '\n' || *hp == '\r' || *hp == '\'' || *hp == '\"')
    {
 *hp = 0;
 hp--;
    }
    

    hs[idx] = 0;

    return hs;
}

char* strupr(char* s)
{
  	char* p = s;
  	while(*p)
	{
    	*p = toupper((int)*p);
		p++;
	}
  return s;
}

int getFileSize(char *fn)
{
FILE *fp;
int size=0;

	if((fp = fopen(fn,"r")))
	{
		fseek(fp,0L,SEEK_END);
		size = ftell(fp);
		fclose(fp);
        return size;
	}
	return -1;
}

// returns: 1=the requested time is there
int checkTime(int min, int sec)
{
struct tm *tm;

    struct timeval  tv;
    gettimeofday(&tv, NULL);
    tm = gmtime(&tv.tv_sec);
    
    if((tm->tm_min & 1) == min && tm->tm_sec == sec)
        return 1;
    
    return 0;
}

// returns: second of the actual interval
int getIntervalSecond()
{
struct tm *tm;

    struct timeval  tv;
    gettimeofday(&tv, NULL);
    tm = gmtime(&tv.tv_sec);
    
    return (tm->tm_min & 1)*60 + tm->tm_sec;
}

// returns band string for a frequency or NULL if not found
char *bandFromQrg(double f)
{
    for(int i=0; i<NUMOFBANDS; i++)
    {
        if(f > (myband[i].qrg - 0.1) && f < (myband[i].qrg + 0.1))
        {
            return myband[i].band;
        }
    }

    return NULL;
}

/*
 * Find the first occurrence of find in s, where the search is limited to the
 * first slen characters of s.
 */
char *strnstr(const char *s, const char *find, size_t slen)
{
	char c, sc;
	size_t len;

	if ((c = *find++) != '\0') {
		len = strlen(find);
		do {
			do {
				if (slen-- < 1 || (sc = *s++) == '\0')
					return (NULL);
			} while (sc != c);
			if (len > slen)
				return (NULL);
		} while (strncmp(s, find, len) != 0);
		s--;
	}
	return ((char *)s);
}
