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
#include "config.h"

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

int splitline(char *line, char arr[MAXFIELDS][MAXELEMLEN]);
void saveJSONdata(char *fn, int rows, int elems);
void addJSONnewdates(char *rxbuf, char *fn);
void calc2waystat();
char *printDBtime(unsigned long t);

char sheader[MAXFIELDS][MAXELEMLEN];
char sdata[MAXROWS][MAXFIELDS][MAXELEMLEN];
char sdata1[MAXROWS][MAXFIELDS][MAXELEMLEN]; // used to store data from first statistics request for the 1way calculation
int sdata_len,sdata1_len;

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

// --------------------------------------------------------------------------------------------------------

// fill sheader and sdata with the values from rxdata
int splitData(char sdata[MAXROWS][MAXFIELDS][MAXELEMLEN], char *rxbuf, int *pelem)
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

/* --------------------------------------------------------------------------------------------------------
 build the table of the actual RX (or TX) spots
 
 Format:
 
{"data":[{"QRG1":"7040053","QRG2":" ","DIST1":"354","DIST2":" ","SNR1":"-23","SNR2":" ","DATE":"6368541384","C1":"DJ0ABR","C2":" ","HEARD":"DF4IE"},
{"QRG1":"7040179","QRG2":"7040182","DIST1":"38","DIST2":"55","SNR1":"-11","SNR2":"-19","DATE":"6368541384","C1":"DJ0ABR","C2":"DH5RAE","HEARD":"DL1EV"},
{"QRG1":"7040128","QRG2":" ","DIST1":"1329","DIST2":" ","SNR1":"11","SNR2":" ","DATE":"6368541708","C1":"DJ0ABR","C2":" ","HEARD":"M0BSI"},
{"QRG1":"7040114","QRG2":" ","DIST1":"1039","DIST2":" ","SNR1":"-16","SNR2":" ","DATE":"6368541708","C1":"DJ0ABR","C2":" ","HEARD":"M1DOX"}]}

the database returns a list with all spots sorted by date and call.
we have to check calls and if is of both stations, put it into one line

Datebase format:

datetime;  callsign; qrg;    snr; qthloc; dBm; reporter; rptr_qthloc; distance; band
   0          1       2       3     4      5      6         7           8        9
6368541708;G0ACQ;    7040122;1;   JN68QV; 37;  DJ0ABR;   JN68NT;      21;       6
*/
// checks if two rows have equal datetime,callsigns
// returns: 
// 0=next row is equal, make it to one row
// 1=single row belongs to mycall
// 2=single row belongs to other call
int testRow(int row, int rowanz)
{
    if(row<(rowanz-1) && !strcmp(sdata[row][0],sdata[row+1][0]) && !strcmp(sdata[row][1],sdata[row+1][1]))
        return 0;
    
    if(!strcmp(sdata[row][6],callsign))
        return 1;
    
    return 2;
}

void makeSpotsJSONtableSpots(char *rxbuf, char *fn)
{
    // evaluate the result string, check how many rows and elements we got
    // and insert the column names in sheader
    // and the value in sdata
    int elems = 0;
    int rows = splitData(sdata, rxbuf,&elems);
    
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
            int rowmode = testRow(i, rows);
            
            if(rowmode == 0)
            {
                // both stations have the same spot
                if(!strcmp(sdata[i][6],callsign))
                {
                    // my callsign is in the first row
                    fprintf(fjson,"{\"QRG1\":\"%s\",\"QRG2\":\"%s\",\"DIST1\":\"%s\",\"DIST2\":\"%s\",\"SNR1\":\"%s\",\"SNR2\":\"%s\",\"DATE\":\"%s\",\"C1\":\"%s\",\"C2\":\"%s\",\"HEARD\":\"%s\"}",sdata[i][2],sdata[i+1][2],sdata[i][8],sdata[i+1][8],sdata[i][3],sdata[i+1][3],sdata[i][0],sdata[i][6],sdata[i+1][6],sdata[i][1]);
                }
                else
                {
                    // other callsign is in the first row
                    fprintf(fjson,"{\"QRG1\":\"%s\",\"QRG2\":\"%s\",\"DIST1\":\"%s\",\"DIST2\":\"%s\",\"SNR1\":\"%s\",\"SNR2\":\"%s\",\"DATE\":\"%s\",\"C1\":\"%s\",\"C2\":\"%s\",\"HEARD\":\"%s\"}",sdata[i+1][2],sdata[i][2],sdata[i+1][8],sdata[i][8],sdata[i+1][3],sdata[i][3],sdata[i][0],sdata[i+1][6],sdata[i][6],sdata[i][1]);
                }
                // next row already processed
                i++;
            }
            
            if(rowmode == 1)
            {
                // my spot, other station has no spot
                fprintf(fjson,"{\"QRG1\":\"%s\",\"QRG2\":\"%s\",\"DIST1\":\"%s\",\"DIST2\":\"%s\",\"SNR1\":\"%s\",\"SNR2\":\"%s\",\"DATE\":\"%s\",\"C1\":\"%s\",\"C2\":\"%s\",\"HEARD\":\"%s\"}",sdata[i][2]," ",sdata[i][8]," ",sdata[i][3]," ",sdata[i][0],sdata[i][6]," ",sdata[i][1]);
            }
            
            if(rowmode == 2)
            {
                // other spot, my station has no spot
                fprintf(fjson,"{\"QRG1\":\"%s\",\"QRG2\":\"%s\",\"DIST1\":\"%s\",\"DIST2\":\"%s\",\"SNR1\":\"%s\",\"SNR2\":\"%s\",\"DATE\":\"%s\",\"C1\":\"%s\",\"C2\":\"%s\",\"HEARD\":\"%s\"}"," ",sdata[i][2]," ",sdata[i][8]," ",sdata[i][3],sdata[i][0]," ",sdata[i][6],sdata[i][1]);
            }
            
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

/* --------------------------------------------------------------------------------------------------------
 build the table of the RX (or TX) statistics
 
 Format:
 
{"data":[{"REPORTER1":"2E0EUI","D1":"6368547900","C1":"DJ0ABR","QRG1":"7040088","SNR1":"2","DIST1":"1050","D2":"6368547816","C2":"DH5RAE","QRG2":"7040091","SNR2":"-18","DIST2":"1064"},
{"REPORTER1":"9A3SWO","D1":"6368547792","C1":"DJ0ABR","QRG1":"7040160","SNR1":"-24","DIST1":"354","D2":"6368547744","C2":"DH5RAE","QRG2":"7040177","SNR2":"-26","DIST2":"352"},
{"REPORTER1":"WA4KFZ","D1":"6368547672","C1":"DJ0ABR","QRG1":"7040136","SNR1":"-24","DIST1":"6910","D2":" ","C2":" ","QRG2":" ","SNR2":" ","DIST2":" "}]}

the database returns a list with all calls sorted by call and reporter, always the latest date
we have to check calls and if is of both stations, put it into one line

Datebase format:
      0           1           2         3         4         5
| callsign | MAX(datetime) | qrg     | snr  | distance | reporter |
+----------+---------------+---------+------+----------+----------+
| 000TPW   |     287554080 |  475717 |  -17 |     5347 | DH5RAE   |
| 0B1FOV   |     287313720 | 7040103 |   12 |    13766 | DJ0ABR   |
*/

// checks if two rows have equal datetime,callsigns
// returns: 
// 0=next row is equal, make it to one row
// 1=single row belongs to mycall
// 2=single row belongs to other call

int testRowStat(int row, int rowanz)
{
    if(row<(rowanz-1) && !strcmp(sdata[row][0],sdata[row+1][0]))
        return 0;
    
    if(!strcmp(sdata[row][5],callsign))
        return 1;
    
    return 2;
}

void makeSpotsJSONtableRXTXstat(char *rxbuf, char *fn)
{
    // evaluate the result string, check how many rows and elements we got
    // and insert the column names in sheader
    // and the value in sdata
    int elems = 0;
    int rows = splitData(sdata,rxbuf,&elems);
    
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
            int rowmode = testRowStat(i, rows);
            
            if(rowmode == 0)
            {
                // both stations have the same spot
                if(!strcmp(sdata[i][5],callsign))
                {
                    // my callsign is in the first row
                    fprintf(fjson,"{\"REPORTER1\":\"%s\",\"D1\":\"%s\",\"C1\":\"%s\",\"QRG1\":\"%s\",\"SNR1\":\"%s\",\"DIST1\":\"%s\",\"D2\":\"%s\",\"C2\":\"%s\",\"QRG2\":\"%s\",\"SNR2\":\"%s\",\"DIST2\":\"%s\"}",sdata[i][0],sdata[i][1],sdata[i][5],sdata[i][2],sdata[i][3],sdata[i][4],sdata[i+1][1],sdata[i+1][5],sdata[i+1][2],sdata[i+1][3],sdata[i+1][4]);
                }
                else
                {
                    // other callsign is in the first row
                    fprintf(fjson,"{\"REPORTER1\":\"%s\",\"D1\":\"%s\",\"C1\":\"%s\",\"QRG1\":\"%s\",\"SNR1\":\"%s\",\"DIST1\":\"%s\",\"D2\":\"%s\",\"C2\":\"%s\",\"QRG2\":\"%s\",\"SNR2\":\"%s\",\"DIST2\":\"%s\"}",sdata[i][0],sdata[i+1][1],sdata[i+1][5],sdata[i+1][2],sdata[i+1][3],sdata[i+1][4],sdata[i][1],sdata[i][5],sdata[i][2],sdata[i][3],sdata[i][4]);
                }
                // next row already processed
                i++;
            }
            
            if(rowmode == 1)
            {
                // my spot, other station has no spot
                fprintf(fjson,"{\"REPORTER1\":\"%s\",\"D1\":\"%s\",\"C1\":\"%s\",\"QRG1\":\"%s\",\"SNR1\":\"%s\",\"DIST1\":\"%s\",\"D2\":\"%s\",\"C2\":\"%s\",\"QRG2\":\"%s\",\"SNR2\":\"%s\",\"DIST2\":\"%s\"}",sdata[i][0],sdata[i][1],sdata[i][5],sdata[i][2],sdata[i][3],sdata[i][4]," "," "," "," "," ");
            }
            
            if(rowmode == 2)
            {
                // other spot, my station has no spot
                fprintf(fjson,"{\"REPORTER1\":\"%s\",\"D1\":\"%s\",\"C1\":\"%s\",\"QRG1\":\"%s\",\"SNR1\":\"%s\",\"DIST1\":\"%s\",\"D2\":\"%s\",\"C2\":\"%s\",\"QRG2\":\"%s\",\"SNR2\":\"%s\",\"DIST2\":\"%s\"}",sdata[i][0]," "," "," "," "," ",sdata[i][1],sdata[i][5],sdata[i][2],sdata[i][3],sdata[i][4]);
            }
            
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
    
    if(!strcmp(fn,"SPOTS_JSON_RX.txt"))
    {
        // save data for later use
        for(int i=0; i<rows; i++)
        {
            memcpy(sdata1[i],sdata[i],MAXFIELDS * MAXELEMLEN);
        }
        sdata1_len = rows;
    }
    else
    {
        // sdata1 contains the RX statistics
        // sdata  contains the TX statistics
        sdata_len = rows;
        calc2waystat();
    }
}

/* --------------------------------------------------------------------------------------------------------
 build the table of the stat counters
 
 Format: three fields: 1=description, 2=value mycall, 3=value other call
 
{"data":[{"DESCR":"Start Date (statistics)","CALL1":"6368531376","CALL2":"6368531376"},
{"DESCR":"End Date (statistics)","CALL1":"6368548620","CALL2":"6368548620"},
{"DESCR":"all RX spots","CALL1":"4712","CALL2":"7443"},
{"DESCR":"all TX spots","CALL1":"5721","CALL2":"12461"},
{"DESCR":"all RX stations","CALL1":"239","CALL2":"294"},
{"DESCR":"all TX stations","CALL1":"280","CALL2":"387"},
{"DESCR":"max RX distance","CALL1":"18235","CALL2":"18215"},
{"DESCR":"max TX distance","CALL1":"18201","CALL2":"18180"},
{"DESCR":"mid RX snr","CALL1":"-9.9387","CALL2":"-12.4473"},
{"DESCR":"mid TX snr","CALL1":"-13.0058","CALL2":"-13.7744"},
{"DESCR":"2-way contacts (month)","CALL1":"357","CALL2":"358"}]}

Datebase format:
   0     1        2        3       4         5
+-----+--------+-------+-------+---------+----------+
| dir | cx     | spots | calls | maxdist | midsnr   |
+-----+--------+-------+-------+---------+----------+
| RX  | DJ0ABR |  5420 |   254 |   18235 |  -9.4985 |
| RX  | DH5RAE |  2603 |   166 |   18180 | -11.2148 |
| TX  | DJ0ABR |  6094 |   304 |   18201 | -12.7990 |
| TX  | DH5RAE |  3696 |   225 |   18180 | -13.0952 |
+-----+--------+-------+-------+---------+----------+
*/

void makeSpotsJSONtableStatCounters(char *rxbuf, char *fn)
{
    // evaluate the result string, check how many rows and elements we got
    // and insert the column names in sheader
    // and the value in sdata
    int elems = 0;
    int rows = splitData(sdata,rxbuf,&elems);
    
    // the database response contains 4 lines (see above) for
    // 1h, 6h, 1d and 1month
    // so we get 16 lines
    // check if we really got all 16 lines
    if(rows != 16)
    {
        deb_printf("JSON","invalid statistics counter file from server");
        return;
    }
    
    /*for(int i=0; i<rows; i++)
    {
        printf("%d: ",i);
        for(int e=0; e<elems; e++)
            printf("<%s>",sdata[i][e]);
        printf("\n");
    }*/
    
    FILE *fjson;
    char fs[256];

    // left table
    snprintf(fs,sizeof(fs),"/tmp/%s",fn);
    fjson = fopen(fs,"w");
    if(fjson)
    {
        // write JSON header
        fprintf(fjson,"{\"data\":[");

        int ln = 0;
        fprintf(fjson,"{\"DESCR\":\"1 hour: \",\"CALL1\":\"%s\",\"CALL2\":\"%s\"},",callsign,call_ur[call_db_idx]);
        fprintf(fjson,"{\"DESCR\":\"RX spots\",\"CALL1\":\"%s\",\"CALL2\":\"%s\"},", sdata[ln][2],sdata[ln+1][2]);
        fprintf(fjson,"{\"DESCR\":\"TX spots\",\"CALL1\":\"%s\",\"CALL2\":\"%s\"},", sdata[ln+2][2],sdata[ln+3][2]);
        fprintf(fjson,"{\"DESCR\":\"RX stations\",\"CALL1\":\"%s\",\"CALL2\":\"%s\"},", sdata[ln][3],sdata[ln+1][3]);
        fprintf(fjson,"{\"DESCR\":\"TX stations\",\"CALL1\":\"%s\",\"CALL2\":\"%s\"},", sdata[ln+2][3],sdata[ln+3][3]);
        fprintf(fjson,"{\"DESCR\":\"max RX distance\",\"CALL1\":\"%s\",\"CALL2\":\"%s\"},", sdata[ln][4],sdata[ln+1][4]);
        fprintf(fjson,"{\"DESCR\":\"max TX distance\",\"CALL1\":\"%s\",\"CALL2\":\"%s\"},", sdata[ln+2][4],sdata[ln+3][4]);
        fprintf(fjson,"{\"DESCR\":\"mid RX snr\",\"CALL1\":\"%s\",\"CALL2\":\"%s\"},", sdata[ln][5],sdata[ln+1][5]);
        fprintf(fjson,"{\"DESCR\":\"mid TX snr\",\"CALL1\":\"%s\",\"CALL2\":\"%s\"},", sdata[ln+2][5],sdata[ln+3][5]);
        ln += 4;
        fprintf(fjson,"{\"DESCR\":\"6 hours: \",\"CALL1\":\"%s\",\"CALL2\":\"%s\"},",callsign,call_ur[call_db_idx]);
        fprintf(fjson,"{\"DESCR\":\"RX spots\",\"CALL1\":\"%s\",\"CALL2\":\"%s\"},", sdata[ln][2],sdata[ln+1][2]);
        fprintf(fjson,"{\"DESCR\":\"TX spots\",\"CALL1\":\"%s\",\"CALL2\":\"%s\"},", sdata[ln+2][2],sdata[ln+3][2]);
        fprintf(fjson,"{\"DESCR\":\"RX stations\",\"CALL1\":\"%s\",\"CALL2\":\"%s\"},", sdata[ln][3],sdata[ln+1][3]);
        fprintf(fjson,"{\"DESCR\":\"TX stations\",\"CALL1\":\"%s\",\"CALL2\":\"%s\"},", sdata[ln+2][3],sdata[ln+3][3]);
        fprintf(fjson,"{\"DESCR\":\"max RX distance\",\"CALL1\":\"%s\",\"CALL2\":\"%s\"},", sdata[ln][4],sdata[ln+1][4]);
        fprintf(fjson,"{\"DESCR\":\"max TX distance\",\"CALL1\":\"%s\",\"CALL2\":\"%s\"},", sdata[ln+2][4],sdata[ln+3][4]);
        fprintf(fjson,"{\"DESCR\":\"mid RX snr\",\"CALL1\":\"%s\",\"CALL2\":\"%s\"},", sdata[ln][5],sdata[ln+1][5]);
        fprintf(fjson,"{\"DESCR\":\"mid TX snr\",\"CALL1\":\"%s\",\"CALL2\":\"%s\"}", sdata[ln+2][5],sdata[ln+3][5]);
        
        // write JSON tail
        fprintf(fjson,"]}");
        
        fclose(fjson);
        
        // copy the tmp file over the real file, use cat to preserve the file owner
        snprintf(fs,sizeof(fs),"cat /tmp/%s > %s/%s",fn,htmldir,fn);
        system(fs);
    }
    
    // right table
    snprintf(fs,sizeof(fs),"/tmp/r_%s",fn);
    fjson = fopen(fs,"w");
    if(fjson)
    {
        // write JSON header
        fprintf(fjson,"{\"data\":[");

        int ln = 8;
        fprintf(fjson,"{\"DESCR\":\"1 day: \",\"CALL1\":\"%s\",\"CALL2\":\"%s\"},",callsign,call_ur[call_db_idx]);
        fprintf(fjson,"{\"DESCR\":\"RX spots\",\"CALL1\":\"%s\",\"CALL2\":\"%s\"},", sdata[ln][2],sdata[ln+1][2]);
        fprintf(fjson,"{\"DESCR\":\"TX spots\",\"CALL1\":\"%s\",\"CALL2\":\"%s\"},", sdata[ln+2][2],sdata[ln+3][2]);
        fprintf(fjson,"{\"DESCR\":\"RX stations\",\"CALL1\":\"%s\",\"CALL2\":\"%s\"},", sdata[ln][3],sdata[ln+1][3]);
        fprintf(fjson,"{\"DESCR\":\"TX stations\",\"CALL1\":\"%s\",\"CALL2\":\"%s\"},", sdata[ln+2][3],sdata[ln+3][3]);
        fprintf(fjson,"{\"DESCR\":\"max RX distance\",\"CALL1\":\"%s\",\"CALL2\":\"%s\"},", sdata[ln][4],sdata[ln+1][4]);
        fprintf(fjson,"{\"DESCR\":\"max TX distance\",\"CALL1\":\"%s\",\"CALL2\":\"%s\"},", sdata[ln+2][4],sdata[ln+3][4]);
        fprintf(fjson,"{\"DESCR\":\"mid RX snr\",\"CALL1\":\"%s\",\"CALL2\":\"%s\"},", sdata[ln][5],sdata[ln+1][5]);
        fprintf(fjson,"{\"DESCR\":\"mid TX snr\",\"CALL1\":\"%s\",\"CALL2\":\"%s\"},", sdata[ln+2][5],sdata[ln+3][5]);
        ln += 4;
        fprintf(fjson,"{\"DESCR\":\"this month: \",\"CALL1\":\"%s\",\"CALL2\":\"%s\"},",callsign,call_ur[call_db_idx]);
        fprintf(fjson,"{\"DESCR\":\"RX spots\",\"CALL1\":\"%s\",\"CALL2\":\"%s\"},", sdata[ln][2],sdata[ln+1][2]);
        fprintf(fjson,"{\"DESCR\":\"TX spots\",\"CALL1\":\"%s\",\"CALL2\":\"%s\"},", sdata[ln+2][2],sdata[ln+3][2]);
        fprintf(fjson,"{\"DESCR\":\"RX stations\",\"CALL1\":\"%s\",\"CALL2\":\"%s\"},", sdata[ln][3],sdata[ln+1][3]);
        fprintf(fjson,"{\"DESCR\":\"TX stations\",\"CALL1\":\"%s\",\"CALL2\":\"%s\"},", sdata[ln+2][3],sdata[ln+3][3]);
        fprintf(fjson,"{\"DESCR\":\"max RX distance\",\"CALL1\":\"%s\",\"CALL2\":\"%s\"},", sdata[ln][4],sdata[ln+1][4]);
        fprintf(fjson,"{\"DESCR\":\"max TX distance\",\"CALL1\":\"%s\",\"CALL2\":\"%s\"},", sdata[ln+2][4],sdata[ln+3][4]);
        fprintf(fjson,"{\"DESCR\":\"mid RX snr\",\"CALL1\":\"%s\",\"CALL2\":\"%s\"},", sdata[ln][5],sdata[ln+1][5]);
        fprintf(fjson,"{\"DESCR\":\"mid TX snr\",\"CALL1\":\"%s\",\"CALL2\":\"%s\"}", sdata[ln+2][5],sdata[ln+3][5]);

        
        // write JSON tail
        fprintf(fjson,"]}");
        
        fclose(fjson);
        
        // copy the tmp file over the real file, use cat to preserve the file owner
        snprintf(fs,sizeof(fs),"cat /tmp/r_%s > %s/r_%s",fn,htmldir,fn);
        system(fs);
    }
}

/* --------------------------------------------------------------------------------------------------------
 * build the ranking tables
 * 
 * one table per band, filename: JSON_RANKING_%d.txt",band+1);
 * JSON Format: rank, spots,callsign,locator 
{"data":[{"rank":"142","spots":"1","callsign":"DF4PV","locator":"JN49"},
{"rank":"142","spots":"1","callsign":"VA6THC","locator":"DO33"},
{"rank":"142","spots":"1","callsign":"OE6TLF","locator":"JN77"},
{"rank":"2","spots":"72","callsign":"OZ7IT","locator":"JO65"},
{"rank":"1","spots":"85","callsign":"G3ZJG","locator":"IO92"}]}

Database format: first line is the date of this ranking
+----+--------+------+-------+---------------------+---------+
| ID | band   | rank | spots | callsign            | locator |
+----+--------+------+-------+---------------------+---------+
|  0 |        |    0 |     0 |  February 1-11 2019 |         |
|  1 | LF/VLF |    1 |    47 | DK2DB               | JN48    |
|  1 | LF/VLF |    2 |    45 | DL3GAK              | JN48    |
|  1 | LF/VLF |    3 |    43 | G0MRF               | IO91    |
|  1 | LF/VLF |    4 |    42 | DL7NN               | JO60    |
|  1 | LF/VLF |    5 |    39 | DL2JA               | JN58    |
|  1 | LF/VLF |    6 |    38 | DC0DX               | JO31    |

JSON Format of the TOP list: JSON_RANKING_TOP.txt
{"data":[{"ID":"0","band":" ","rank":"0","spots":"0","callsign":" February 1-11 2019","locator":" "},                                                                  
{"ID":"1","band":"LF/VLF","rank":"1","spots":"47","callsign":"DK2DB","locator":"JN48"},                                                                                
{"ID":"15","band":"70cm","rank":"1","spots":"6","callsign":"PA0KNW","locator":"JO33"},
{"ID":"16","band":"23cm","rank":"1","spots":"2","callsign":"JQ1ZJU","locator":"PM95"}]}

 */
void makeSpotsJSONtableRanking(char *rxbuf)
{
    int elems = 0;
    int rows = splitData(sdata,rxbuf,&elems);
    if(rows == 0) return;
    
    char fn[256];
    // ranking per band
    for(int band=0; band<16; band++)
    {
        snprintf(fn,sizeof(fn),"%s/JSON_RANKING_%d.txt",htmldir,band+1);
        FILE *fjson = fopen(fn,"w");
        if(fjson)
        {
            // write JSON header
            fprintf(fjson,"{\"data\":[");

            int fk=0;
            for(int i=0; i<rows; i++)
            {
                if(atoi(sdata[i][0]) == (band+1))
                {
                    if(fk) fprintf(fjson,",");
                    fk = 1;
                    fprintf(fjson,"{\"rank\":\"%s\",\"spots\":\"%s\",\"callsign\":\"%s\",\"locator\":\"%s\"}", sdata[i][2],sdata[i][3],sdata[i][4],sdata[i][5]);
                    
                }
            }
            
            // write JSON tail
            fprintf(fjson,"]}");
            
            fclose(fjson);
        }
    }
    
    // top list
    snprintf(fn,sizeof(fn),"%s/JSON_RANKING_TOP.txt",htmldir);
    FILE *fjson = fopen(fn,"w");
    if(fjson)
    {
        // write JSON header
        fprintf(fjson,"{\"data\":[");

        // Date
        fprintf(fjson,"{\"ID\":\"0\",\"band\":\" \",\"rank\":\"0\",\"spots\":\"0\",\"callsign\":\"%s\",\"locator\":\" \"},",sdata[0][4]);

        int fk=0;
        for(int band=0; band<16; band++)
        {
            for(int i=0; i<rows; i++)
            {
                if(atoi(sdata[i][0]) == (band+1) && atoi(sdata[i][2]) == 1)
                {
                    if(fk) fprintf(fjson,",");
                    fk = 1;
                    fprintf(fjson,"{\"ID\":\"%s\",\"band\":\"%s\",\"rank\":\"%s\",\"spots\":\"%s\",\"callsign\":\"%s\",\"locator\":\"%s\"}", sdata[i][0],sdata[i][1],sdata[i][2],sdata[i][3],sdata[i][4],sdata[i][5]);                    
                }
            }
        }
        
        // write JSON tail
        fprintf(fjson,"]}");
        
        fclose(fjson);
    }
}

/* --------------------------------------------------------------------------------------------------------
 * build QTHloc tables for use with the maps
 * Filenames: QTH_MY_RX.txt, QTH_MY_TX.txt, QTH_UR_RX.txt, QTH_UR_TX.txt
 * Format:
{"data":[{"_":"EL88VF","__":"KC8HQS/4"},
{"_":"EM81","__":"K4APC"},
{"_":"KO95","__":"RU5C"},
{"_":"KP12IQ","__":"OH6MQM"}]}

the database returns this format:
   0       1       2           3
| RX | QF22NE | VK3MI      | DH5RAE   |
| RX | QF57   | VK2POP     | DH5RAE   |
| TX | EL88VF | KC8HQS/4   | DH5RAE   |

 */
void makeSpotsJSONtableQTHLOCs(char *rxbuf)
{
char fn[256];
FILE *fmytx, *fmyrx, *furtx, *furrx;
int f1=1,f2=1,f3=1,f4=1;

    int elems = 0;
    int rows = splitData(sdata,rxbuf,&elems);
    if(rows == 0) return;
    
    snprintf(fn,sizeof(fn),"%s/QTH_MY_RX.txt",htmldir);
    fmyrx = fopen(fn,"w");
    if(fmyrx)
    {
        fprintf(fmyrx,"{\"data\":[");
        snprintf(fn,sizeof(fn),"%s/QTH_MY_TX.txt",htmldir);
        fmytx = fopen(fn,"w");
        if(fmytx)
        {
            fprintf(fmytx,"{\"data\":[");
            snprintf(fn,sizeof(fn),"%s/QTH_UR_RX.txt",htmldir);
            furrx = fopen(fn,"w");
            if(furrx)
            {
                fprintf(furrx,"{\"data\":[");
                snprintf(fn,sizeof(fn),"%s/QTH_UR_TX.txt",htmldir);
                furtx = fopen(fn,"w");
                if(furtx)
                {
                    fprintf(furtx,"{\"data\":[");
                    for(int i=0; i<rows; i++)
                    {
                        if(!strcmp(sdata[i][0],"RX"))
                        {
                            if(!strcmp(sdata[i][3],callsign))
                            {
                                // my RX qthlocs
                                if(!f1) fprintf(fmyrx,",");
                                f1=0;
                                fprintf(fmyrx,"{\"_\":\"%s\",\"__\":\"%s\"}",sdata[i][1],sdata[i][2]);
                            }
                            else
                            {
                                // other RX qthlocs
                                if(!f2) fprintf(furrx,",");
                                f2=0;
                                fprintf(furrx,"{\"_\":\"%s\",\"__\":\"%s\"}",sdata[i][1],sdata[i][2]);
                            }
                        }
                        else
                        {
                            if(!strcmp(sdata[i][3],callsign))
                            {
                                // my TX qthlocs
                                if(!f3) fprintf(fmytx,",");
                                f3=0;
                                fprintf(fmytx,"{\"_\":\"%s\",\"__\":\"%s\"}",sdata[i][1],sdata[i][2]);
                            }
                            else
                            {
                                // other TX qthlocs
                                if(!f4) fprintf(furtx,",");
                                f4=0;
                                fprintf(furtx,"{\"_\":\"%s\",\"__\":\"%s\"}",sdata[i][1],sdata[i][2]);
                            }
                        }
                    }
                    fprintf(furtx,"]}");
                    fclose(furtx);
                }
                fprintf(furrx,"]}");
                fclose(furrx);
            }
            fprintf(fmytx,"]}");
            fclose(fmytx);  
        }
        fprintf(fmyrx,"]}");
        fclose(fmyrx);
    }
}

/* make table for WebWSPR User
{"data":[
{"lastheard":"12345","callsign":"KC8HQS/4","url","myurl1"},
{"lastheard":"12345","callsign":"DJ0ABR","url","myurl2"}
]}
 * */
void makeStationsJSONtable(char *rxbuf)
{
    char fn[256];
    int elems = 0;
    int rows = splitData(sdata,rxbuf,&elems);
    if(rows == 0) return;
    
    /*for(int i=0; i<rows; i++)
    {
        for(int j=0; j<elems; j++)
        {
            printf("<%s>",sdata[i][j]);
        }
        printf("\n");
    }*/
    
    snprintf(fn,sizeof(fn),"%s/STATIONS.txt",htmldir);
    FILE *fw = fopen(fn,"w");
    if(fw)
    {
        // write JSON header
        fprintf(fw,"{\"data\":[");

        int fk=0;
        for(int i=0; i<rows; i++)
        {
            if(fk) fprintf(fw,",");
            fk = 1;
            fprintf(fw,"{\"lastheard\":\"%s\",\"callsign\":\"%s\",\"url\":\"%s\"}", sdata[i][0],sdata[i][1],sdata[i][2]);
        }
        
        // write JSON tail
        fprintf(fw,"]}");
            
        fclose(fw);
    }
}

