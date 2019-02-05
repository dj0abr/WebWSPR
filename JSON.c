/*
 * WSPR Toolkit for Linux
 * ======================
 * by DJ0ABR 
 * 
 * JSON.c 
 * 
 * formats data in JSON format for use with JS data tables
 * 
 * */

#include <pthread.h>
#include <alsa/asoundlib.h>
#include <sndfile.h>
#include <inttypes.h>
#include <time.h>
#include <string.h>
#include <sys/time.h>
#include "wsprtk.h"
#include "debug.h"
#include "soundcard.h"
#include "fft.h"
#include "JSON.h"
#include "kmtools.h"

/* wspr_spots.txt format: 
181223 1204   6   6 -1.0   7.040003  DL0PBS JO33 23         -1     1    0
0      1      2  3  4      5         6                       7     8    9
item[0] ... date
item[1] ... time
item[2] ... 
item[3] ... dBm Report
item[4] ... DT
item[5] ... QRG
item[6] ... Call
item[7] ... QTHloc
item[8] ... TXpwr
item[9] ... DF
*/

/* JSON format:
{
    "data": [
        {
            "date": "Tiger Puhhps",
            "call": "System Architect"
        },
        {
            "date": "18.6.63",
            "call": "huhu"
        },
    ]
}
 * */

#define MAXFIELDS   20
#define MAXROWS     10000
#define MAXELEMLEN  51


int splitData(char *rxbuf, int *pelem);
int splitline(char *line, char arr[MAXFIELDS][MAXELEMLEN]);
void saveJSONdata(char *fn, int rows, int elems);
void addJSONnewdates(char *rxbuf, char *fn);

char sheader[MAXFIELDS][MAXELEMLEN];
char sdata[MAXROWS][MAXFIELDS][MAXELEMLEN];

void saveSpotsJSON() 
{
FILE *fspots, *fjson;
char fn[256];
char fnjson[256];
int n = 1000;
long pos;
int count = 0;
char line[256];

    snprintf(fn,sizeof(fn)-1,"%s/ALL_WSPR.TXT",wavdir);
    snprintf(fnjson,sizeof(fnjson)-1,"%s/_JSON.txt",wavdir);
    fspots = fopen(fn,"r");
    if(fspots)
    {
        fjson = fopen(fnjson,"w");
        if(fjson)
        {
            // write JSON header
            fprintf(fjson,"{\"data\":[");
            
            // goto lastline - n lines in ALL_WSPR.TXT
            fseek(fspots, 0, SEEK_END);
            pos = ftell(fspots);
            int found = 0;
            while (pos) {
                fseek(fspots, --pos, SEEK_SET);
                if (fgetc(fspots) == '\n') {
                    if (count++ == n) 
                    {
                        found = 1;
                        break;
                    }
                }
            }
            if(!found)
            {
                // file still small, go to beginning
                fseek(fspots, 0, SEEK_SET);
            }
            
            // now go through the last n lines and write JSON
            int f=1;
            while (fgets(line, sizeof(line)-1, fspots) != NULL) {
                char *json = buildJSON(line);
                if(json == NULL) continue;  // invalid line
                if(f==0) fprintf(fjson,",\n");
                f=0;
                fprintf(fjson,"%s",json);
            }
            
            // write JSON tail
            fprintf(fjson,"]}");
            
            fclose(fjson);
            
            snprintf(fn,sizeof(fn)-1,"cp %s %s/JSON.txt",fnjson,htmldir);
            system(fn);
        }
        
        fclose(fspots);
    }
}

// a "good" line contains 12 elements
// separate them into the string array "spotelems"
// return 1=ok, 0=invalid line
char spotelems[12][40];

int extractWSPRline(char *ln)
{
    if(*ln<'0' || *ln>'9') 
    {
        deb_printf("JSON","invalid start char\n");
        return 0;    // check if line starts with a number
    }
    
    if(strlen(ln) > 120) 
    {
        deb_printf("JSON","invalid length\n");
        return 0;       // invalid length
    }
    
    memset(spotelems,0,sizeof(spotelems));
    int arridx = 0;
    int sidx = 0;
    int firstspc = 1;
    for(int i=0; i<strlen(ln); i++)
    {
        if(arridx >= 16) 
        {
            deb_printf("JSON","number of elems too high\n");
            return 0;   // invalid line
        }
        if(sidx >= 40)
        {
            deb_printf("JSON","invalid elem length\n");
            return 0;   // invalid element length
        }
        
        if(ln[i] != ' ' && ln[i] != '\n')
        {
            spotelems[arridx][sidx++] = ln[i];
            firstspc = 1;
        }
        else if(ln[i] == '\n')
        {
            // end of line
            break;
        }
        else
        {
            // we are on a SPC
            if(firstspc == 1) 
            {
                // js tables do not like < or >, remove it from the callsign
                if(spotelems[arridx][0] == '<')
                {
                    memmove(spotelems[arridx],spotelems[arridx]+1,strlen(spotelems[arridx])-1);
                    spotelems[arridx][strlen(spotelems[arridx])-2] = 0;
                }
                arridx++;
                sidx = 0;
            }
            firstspc = 0;
        }
    }
    if(arridx < 11) 
    {
        //deb_printf("JSON","number of elems too low\n");
        return 0;   // invalid number of elements
    }
    return 1;
}

// get a line from ALL_WSPR.TXT and build a JSON string to feed the GUI list
char *buildJSON(char *ln)
{
    // read all elements from the line into the array spot elems
    if(extractWSPRline(ln) == 0) return NULL;
	// build JSON element
    
static char jsonelem[1000];

	snprintf(jsonelem, sizeof(jsonelem) - 1,
"{\"DateTime\":\"%s %s\",\
\"dBm\":\"%s\",\
\"Dtime\":\"%s\",\
\"QRG\":\"%s\",\
\"Call\":\"%s\",\
\"QTHloc\":\"%s\",\
\"TXpwr\":\"%s\",\
\"Drift\":\"%s\"}",
spotelems[0], spotelems[1], spotelems[3], spotelems[4], spotelems[5], spotelems[6], spotelems[7], spotelems[8], spotelems[9]);

    return jsonelem;
}

void makeSpotsJSONtable(char *rxbuf, char *fn)
{
    int elems = 0;
    int rows = splitData(rxbuf,&elems);
    saveJSONdata(fn,rows,elems);
}

// fill sheader and sdata with the values from rxdata
int splitData(char *rxbuf, int *pelem)
{
    char *pline_end;
    char *pline_start = rxbuf;
    pline_end = strchr(pline_start,'\n');
    // read header
    if(!pline_end) 
    {
        return 0;  // invalid string
    }
    int fields = splitline(pline_start,sheader);

    // read data
    int row = 0;
    while(1)
    {
        pline_end = strchr(pline_start,'\n');
        if(!pline_end) break;
        pline_start = pline_end + 1;
        int anz = splitline(pline_start,sdata[row]);
        if(anz == fields)   // ignore lines with an invalid number of elements
            row++;
    }
    *pelem = fields;
    return row;
}

// split a NL terminated line (delimiter= ;)and fill it into the string array
int splitline(char *line, char arr[MAXFIELDS][MAXELEMLEN])
{
    char *s = line;             // line start
    char *e = strchr(s,'\n');
    if(!e) e = s + strlen(s);   // line end
    int linelen = (int)(e-s);
    char *p;
    
    int field = 0;
    while(1)
    {
        p = strchr(s,';');
        if(!p) p = strchr(s,'\n');
        if(!p) p = e;
        if(p>e) p=e;
        int elemlen  = (int)(p-s);
        
        if(elemlen != 0)
            memcpy(arr[field], s, elemlen);
        
        arr[field][elemlen] = 0;
        field++;

        if(p >= (line + linelen))
            break;
        
        s = p+1;
    }
    
    return field;
}

// save array as JSON file
// if the date is newer then fromdate
void saveJSONdata(char *fn, int rows, int elems)
{
FILE *fjson;
char fs[256];

    snprintf(fs,sizeof(fs),"/tmp/%s",fn);

    fjson = fopen(fs,"w");
    if(fjson)
    {
        // write JSON header
        fprintf(fjson,"{\"data\":[");
        
        for(int i=0; i<rows; i++)
        {
            fprintf(fjson,"{");
            for(int e=0; e<elems; e++)
            {
                fprintf(fjson,"\"%s\":\"%s\"",sheader[e],sdata[i][e]);
                if(e < (elems-1))
                    fprintf(fjson,",");
            }
            fprintf(fjson,"}");
            if(i<(rows-1))
                fprintf(fjson,",\n");
        }
        
        // write JSON tail
        fprintf(fjson,"]}");
        
        fclose(fjson);
        
        // copy the tmp file over the real file, use cat to preserve the file owner
        snprintf(fs,sizeof(fs),"cat /tmp/%s > %s/%s",fn,htmldir,fn);
        system(fs);
    }
}
