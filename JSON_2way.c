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
#include "status.h"

/* --------------------------------------------------------------------------------------------
 * calculate the 2way statistic, use the previously requested RX and TX statistic data
    sdata1 contains the RX statistics
    sdata  contains the TX statistics 

    Format of the RX data:
      0              1        2        3         4          5       6=Marker   7..12=copy from sdata (if Marker=1), 13..18 RX of otcall, 19..24 TX of otcall
+----------+---------------+---------+------+----------+----------+
| callsign | MAX(datetime) | qrg     | snr  | distance | reporter |
+----------+---------------+---------+------+----------+----------+
| 0B1FOV   |     287313720 | 7040103 |   12 |    13766 | DJ0ABR   |
| 169CNE   |     287651880 | 7040081 |  -24 |     4192 | DJ0ABR   |
| 2E0DSS   |     287619120 | 7040026 |  -22 |     1142 | DJ0ABR   |
| 2E0EUI   |     287592720 | 7040091 |  -30 |     1064 | DH5RAE   |
| 2E0EUI   |     287603520 | 7040088 |  -19 |     1050 | DJ0ABR   |
| 2E0GWF   |     287608560 | 7040119 |  -21 |     1187 | DJ0ABR   |
| 2R8IJC   |     287662200 | 7040148 |  -20 |     9613 | DH5RAE   |
| 3B8FO    |     287625960 | 7040099 |  -12 |     8838 | DH5RAE   |
| 3B8FO    |     287615160 | 7040095 |  -17 |     8843 | DJ0ABR   |
+----------+---------------+---------+------+----------+----------+

    Format of the TX data:
+----------+---------------+---------+------+----------+----------+
| reporter | MAX(datetime) | qrg     | snr  | distance | callsign |
+----------+---------------+---------+------+----------+----------+
| 2E0DSS   |     287609640 | 7040064 |  -17 |     1142 | DJ0ABR   |
| 2E0DSS   |     287609880 | 7040120 |  -15 |     1154 | DH5RAE   |
| 2W0BPJ   |     287517480 | 7040053 |  -10 |     1214 | DJ0ABR   |
| 3B8FO    |     287613960 | 7040070 |  -24 |     8843 | DJ0ABR   |
| 3B8FO    |     287630760 | 7040127 |  -13 |     8838 | DH5RAE   |
| 7L4IOU   |     287624040 | 7040063 |  -27 |     9244 | DJ0ABR   |
| 7L4IOU   |     287623560 | 7040120 |  -22 |     9225 | DH5RAE   |
+----------+---------------+---------+------+----------+----------+
*/

void print2way(FILE *fjson, int i, int mode);

extern int sdata_len,sdata1_len;
extern char sdata[MAXROWS][MAXFIELDS][MAXELEMLEN];
extern char sdata1[MAXROWS][MAXFIELDS][MAXELEMLEN];

int my2way = 0;
int ot2way = 0;

// the lists use index 0..5 for data
// in index 6 (first byte) we set 1 for a 2way of mycall

void calc2waystat()
{
    int loops = 0;
    my2way = 0;
    ot2way = 0;
    // init index 6
    for(int i=0; i<sdata1_len; i++)
    {
        sdata1[i][6][0] = 0;
        sdata1[i][6][1] = 0;
    }
    
    // for mycall: go through all RX calls and check if a matching TX exists
    int xstart = 0; // to reduce number of loops
    for(int i=0; i<sdata1_len; i++)
    {
        if(!strcmp(sdata1[i][5],callsign))
        {
            // entry belongs to mycall
            // check if sdata1[i][0] exists in sdata
            
            for(int x=xstart; x<sdata_len; x++)
            {
                int r = strcmp(sdata[x][0],sdata1[i][0]);
                if(r > 0) break; // call in sdata too high, we can stop searching
                if(r == 0)
                {
                    // call found, does it belong to mycall ?
                    if(!strcmp(sdata[x][5],callsign))
                    {
                        
                        // yes, we found a 2way, mark it
                        sdata1[i][6][0] = 1;
                        // copy the 2way data from sdata to sdata1
                        for(int e=0; e<6; e++)
                        {
                            strcpy(sdata1[i][e+7],sdata[x][e]);
                        }
                        
                        my2way++;
                        xstart = x;
                    }
                }
                loops++;
            }
        }
    }
    
    // ------------------------------------------------------------
    // do the same as above for otcall, and set a marker in byte 2
    if(strlen(call_ur[call_db_idx]) >= 3)
    {
        // for otcall: go through all RX calls and check if a matching TX exists
        xstart = 0; // to reduce number of loops
        for(int i=0; i<sdata1_len; i++)
        {
            if(!strcmp(sdata1[i][5],call_ur[call_db_idx]))
            {
                // entry belongs to otcall
                // check if sdata1[i][0] exists in sdata
                for(int x=xstart; x<sdata_len; x++)
                {
                    int r = strcmp(sdata[x][0],sdata1[i][0]);
                    if(r > 0) break; // call in sdata too high, we can stop searching
                    if(r == 0)
                    {
                        // call found, does it belong to mycall ?
                        if(!strcmp(sdata[x][5],call_ur[call_db_idx]))
                        {
                            // yes, we found a 2way, mark it
                            sdata1[i][6][1] = 1;
                            // copy the 2way data from sdata to sdata1
                            for(int e=0; e<6; e++)
                            {
                                strcpy(sdata1[i][e+7],sdata[x][e]);
                            }
                            
                            ot2way++;
                            xstart = x;
                        }
                    }
                    loops++;
                }
            }
        }
    }
    
    fsave_status(MY2WAY,my2way);
    fsave_status(OT2WAY,ot2way);
    
    /*
    // print 2ways
    for(int i=0; i<sdata1_len; i++)
    {
        if(sdata1[i][6][0])
        {
            printf("my %d:",i);
            for(int e=0; e<13; e++)
            {
                if(e!=6)
                    printf("<%s>",sdata1[i][e]);
                else
                    printf("<%d,%d>",sdata1[i][e][0],sdata1[i][e][1]); 
            }
            printf("\n");
        }
        
        if(sdata1[i][6][1])
        {
            printf("ot %d:",i);
            for(int e=0; e<13; e++)
            {
                if(e!=6)
                    printf("<%s>",sdata1[i][e]);
                else
                    printf("<%d,%d>",sdata1[i][e][0],sdata1[i][e][1]); 
            }
            printf("\n");
        }
    }
    printf("my2way: %d\n",my2way);
    printf("ot2way: %d\n",ot2way);
    */
    
    /*
     * sdata1 has now this format:
     * if[6][0] is 1 ... my 2way RX spots are in cols 0..5, TX spots are in cols 7..12
     * if[6][1] is 1 ... ot 2way RX spots are in cols 0..5, TX spots are in cols 7..12
     * all rows are sorted by call, so if two following lines have the same call, my+ot have 2way, make it to one line
     * the first part are the reported RX spots, the second part are the TX spots
     * 
ot 732:<WB2TQE><287553600><7040093><-28><8085><DH5RAE><0,1><WB2TQE><287656680><7040131><-27><8129><DH5RAE>
my 733:<WB2TQE><287646000><7040090><-18><8071><DJ0ABR><1,0><WB2TQE><287747400><7040074><-28><8115><DJ0ABR>
ot 735:<WD4AH><287545200><7040193><-26><7960><DH5RAE><0,1><WD4AH><287545080><7040119><-19><7960><DH5RAE>
ot 736:<WD9EPF><287746560><7040052><-24><7345><DH5RAE><0,1><WD9EPF><287711400><7040123><-22><7345><DH5RAE>
my 737:<WD9EPF><287713680><7040049><-25><7336><DJ0ABR><1,0><WD9EPF><287712120><7040065><-28><7336><DJ0ABR>
my 739:<WE4X><287729640><7040078><-25><7709><DJ0ABR><1,0><WE4X><287739480><7040066><-28><7709><DJ0ABR>
ot 740:<WE4X><287700840><7039994><-19><7720><DH5RAE><0,1><WE4X><287740200><7040123><-25><7720><DH5RAE>
my 741:<YO4XPF><287318040><7040099><5><1131><DJ0ABR><1,0><YO4XPF><287321400><7040066><-11><1131><DJ0ABR>
ot 744:<ZL1TJK><287566920><7040099><-23><18180><DH5RAE><0,1><ZL1TJK><287691240><7040125><-27><18180><DH5RAE>
ot 747:<ZS1TX><287552880><7040104><-21><9216><DH5RAE><0,1><ZS1TX><287552280><7040121><-23><9216><DH5RAE>
my 749:<ZS3D><287722560><7040036><-26><8628><DJ0ABR><1,0><ZS3D><287550600><7040072><-27><8634><DJ0ABR>
     * */
 
    // build the 2way list
    FILE *fjson;
    char fs[256];
    int fi = 1;

    // left table
    snprintf(fs,sizeof(fs),"/tmp/JSON_2WAY.txt");
    fjson = fopen(fs,"w");
    if(fjson)
    {
        // write JSON header
        fprintf(fjson,"{\"data\":[");
        
        for(int i=0; i<sdata1_len; i++)
        {
            if(sdata1[i][6][0] == 0 && sdata1[i][6][1] == 0) continue;
            
            if(!fi) fprintf(fjson,"\n,");
            fi=0;
                
            // check if the actual line belongs to me or the other station
            //if(!strcmp(sdata1[i][5],callsign))
            if(sdata1[i][6][0])
            {
                // its my spots, check if the next line has the same 2way for other call
                if(!strcmp(sdata1[i+1][5],call_ur[call_db_idx]) && !strcmp(sdata1[i][0],sdata1[i+1][0]) && sdata1[i+1][6][1])
                    print2way(fjson,i++,0);         // yes, ot has also a 2way
                else
                    print2way(fjson,i,2);           // its only my 2way
            }
            else
            {
                // its ot spots, check if the next line has the same 2way for my call
                if(!strcmp(sdata1[i+1][5],callsign) && !strcmp(sdata1[i][0],sdata1[i+1][0]) && sdata1[i+1][6][0])
                    print2way(fjson,i++,1);         // yes, my has also a 2way
                else
                    print2way(fjson,i,3);           // its only ot 2way
            }
        }
        // write JSON tail
        fprintf(fjson,"]}");
        
        fclose(fjson);
        
        // copy the tmp file over the real file, use cat to preserve the file owner
        snprintf(fs,sizeof(fs),"cat /tmp/JSON_2WAY.txt > %s/JSON_2WAY.txt",htmldir);
        system(fs);
    }
}

// Array format:
//   0         1        2     3      4     5      6     7      8         9       10   11     12
// <ZS3D><287722560><7040036><-26><8628><DJ0ABR><1,0><ZS3D><287550600><7040072><-27><8634><DJ0ABR>

// JSON Format:
// CS, MYCALL, MYRXD, MYRXQRG, MYSNR, MYQTH, MYTXD, MYTXQRG, MYTXSNR, MYTXQTH, 
// URCALL, URRXD, URRXQRG, URSNR, URQTH, URTXD, URTXQRG, URTXSNR, URTXQTH

// mode: 0=both (my in first line)  1=both (ot in first line)  2=only my  3=only ot
void print2way(FILE *fjson, int i, int mode)
{
    //printf("mode:%d: ",mode);
    if(mode == 0)
    {
        // both 2way, my in first line
        fprintf(fjson,"{\"CS\":\"%s\",\"MYCALL\":\"%s\",\"MYRXD\":\"%s\",\"MYRXQRG\":\"%s\",\"MYSNR\":\"%s\",\"MYQTH\":\"%s\",\"MYTXD\":\"%s\",\"MYTXQRG\":\"%s\",\"MYTXSNR\":\"%s\",\"MYTXQTH\":\"%s\",\"URCALL\":\"%s\",\"URRXD\":\"%s\",\"URRXQRG\":\"%s\",\"URSNR\":\"%s\",\"URQTH\":\"%s\",\"URTXD\":\"%s\",\"URTXQRG\":\"%s\",\"URTXSNR\":\"%s\",\"URTXQTH\":\"%s\"}",
            sdata1[i][0],sdata1[i][5],sdata1[i][1],sdata1[i][2],sdata1[i][3],sdata1[i][4],sdata1[i][8],sdata1[i][9],sdata1[i][10],sdata1[i][11],
            sdata1[i+1][5],sdata1[i+1][1],sdata1[i+1][2],sdata1[i+1][3],sdata1[i+1][4],sdata1[i+1][8],sdata1[i+1][9],sdata1[i+1][10],sdata1[i+1][11]);
        /*        
        printf(" my RX: ");
        for(int e=0; e<6; e++)
        {
            printf("<%10.10s>",sdata1[i][e]);
        }
        // print my TX spot
        printf(" my TX: ");
        for(int e=7; e<13; e++)
        {
            printf("<%10.10s>",sdata1[i][e]);
        }

        printf(" ot RX: ");
        for(int e=0; e<6; e++)
        {
            printf("<%10.10s>",sdata1[i+1][e]);
        }
        // print my TX spot
        printf(" ot TX: ");
        for(int e=7; e<13; e++)
        {
            printf("<%10.10s>",sdata1[i+1][e]);
        }*/
    }
    
    if(mode == 1)
    {
        // both 2way, ot in first line
        fprintf(fjson,"{\"CS\":\"%s\",\"MYCALL\":\"%s\",\"MYRXD\":\"%s\",\"MYRXQRG\":\"%s\",\"MYSNR\":\"%s\",\"MYQTH\":\"%s\",\"MYTXD\":\"%s\",\"MYTXQRG\":\"%s\",\"MYTXSNR\":\"%s\",\"MYTXQTH\":\"%s\",\"URCALL\":\"%s\",\"URRXD\":\"%s\",\"URRXQRG\":\"%s\",\"URSNR\":\"%s\",\"URQTH\":\"%s\",\"URTXD\":\"%s\",\"URTXQRG\":\"%s\",\"URTXSNR\":\"%s\",\"URTXQTH\":\"%s\"}",
            sdata1[i+1][0],sdata1[i+1][5],sdata1[i+1][1],sdata1[i+1][2],sdata1[i+1][3],sdata1[i+1][4],sdata1[i+1][8],sdata1[i+1][9],sdata1[i+1][10],sdata1[i+1][11],
            sdata1[i][5],sdata1[i][1],sdata1[i][2],sdata1[i][3],sdata1[i][4],sdata1[i][8],sdata1[i][9],sdata1[i][10],sdata1[i][11]);
        /*
        printf(" my RX: ");
        for(int e=0; e<6; e++)
        {
            printf("<%10.10s>",sdata1[i+1][e]);
        }
        // print my TX spot
        printf(" my TX: ");
        for(int e=7; e<13; e++)
        {
            printf("<%10.10s>",sdata1[i+1][e]);
        }

        printf(" ur RX: ");
        for(int e=0; e<6; e++)
        {
            printf("<%10.10s>",sdata1[i][e]);
        }
        // print my TX spot
        printf(" ur TX: ");
        for(int e=7; e<13; e++)
        {
            printf("<%10.10s>",sdata1[i][e]);
        }*/
    }
    
    if(mode == 2)
    {
        // my 2way only
        fprintf(fjson,"{\"CS\":\"%s\",\"MYCALL\":\"%s\",\"MYRXD\":\"%s\",\"MYRXQRG\":\"%s\",\"MYSNR\":\"%s\",\"MYQTH\":\"%s\",\"MYTXD\":\"%s\",\"MYTXQRG\":\"%s\",\"MYTXSNR\":\"%s\",\"MYTXQTH\":\"%s\",\"URCALL\":\"%s\",\"URRXD\":\"%s\",\"URRXQRG\":\"%s\",\"URSNR\":\"%s\",\"URQTH\":\"%s\",\"URTXD\":\"%s\",\"URTXQRG\":\"%s\",\"URTXSNR\":\"%s\",\"URTXQTH\":\"%s\"}",
            sdata1[i][0],sdata1[i][5],sdata1[i][1],sdata1[i][2],sdata1[i][3],sdata1[i][4],sdata1[i][8],sdata1[i][9],sdata1[i][10],sdata1[i][11],
            " "," "," "," "," "," "," "," "," ");
        /*
        printf(" my RX: ");
        for(int e=0; e<6; e++)
        {
            printf("<%10.10s>",sdata1[i][e]);
        }
        // print my TX spot
        printf(" my TX: ");
        for(int e=7; e<13; e++)
        {
            printf("<%10.10s>",sdata1[i][e]);
        }

        printf(" ot RX: ");
        for(int e=0; e<6; e++)
        {
            printf("<%10.10s>"," ");
        }
        // print my TX spot
        printf(" ot TX: ");
        for(int e=7; e<13; e++)
        {
            printf("<%10.10s>"," ");
        }*/
    }
    
    if(mode == 3)
    {
        // ot 2way only
        fprintf(fjson,"{\"CS\":\"%s\",\"MYCALL\":\"%s\",\"MYRXD\":\"%s\",\"MYRXQRG\":\"%s\",\"MYSNR\":\"%s\",\"MYQTH\":\"%s\",\"MYTXD\":\"%s\",\"MYTXQRG\":\"%s\",\"MYTXSNR\":\"%s\",\"MYTXQTH\":\"%s\",\"URCALL\":\"%s\",\"URRXD\":\"%s\",\"URRXQRG\":\"%s\",\"URSNR\":\"%s\",\"URQTH\":\"%s\",\"URTXD\":\"%s\",\"URTXQRG\":\"%s\",\"URTXSNR\":\"%s\",\"URTXQTH\":\"%s\"}",
            sdata1[i+1][0]," "," "," "," "," "," "," "," "," ",
            sdata1[i][5],sdata1[i][1],sdata1[i][2],sdata1[i][3],sdata1[i][4],sdata1[i][8],sdata1[i][9],sdata1[i][10],sdata1[i][11]);
        /*
        printf(" my RX: ");
        for(int e=0; e<6; e++)
        {
            printf("<%10.10s>"," ");
        }
        // print my TX spot
        printf(" my TX: ");
        for(int e=7; e<13; e++)
        {
            printf("<%10.10s>"," ");
        }

        printf(" ur RX: ");
        for(int e=0; e<6; e++)
        {
            printf("<%10.10s>",sdata1[i][e]);
        }
        // print my TX spot
        printf(" ur TX: ");
        for(int e=7; e<13; e++)
        {
            printf("<%10.10s>",sdata1[i][e]);
        }*/
    }
    //printf("\n");
}

