/*
 * WSPR Toolkit for Linux
 * ======================
 * by DJ0ABR 
 * 
 * mysql.c
 * 
 * handles SQL queries by downloading from HELITRON server
 * 
 * */

#include <pthread.h>
#include <alsa/asoundlib.h>
#include <sndfile.h>
#include <time.h> 
#include <sys/time.h>
#include <inttypes.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <zlib.h>
#include "wsprtk.h"
#include "debug.h"
#include "soundcard.h"
#include "fft.h"
#include "config.h"
#include "coder.h"
#include "civ.h"
#include "JSON.h"
#include "mysql.h"
#include "hopping.h"
#include "kmtools.h"

void queryStatistics();
int ConnectToServer();
int SendQToServer(char *cmd);
int ReadFromServer(char *buf, int maxsize);
int getBandNumber();
int SendCmdToServer(char *cmd, char *type);
void setTimeMeasure(int no);
void sendTypeToServer(char *type);
int startServerConnection();
void stopServerConnection();
void queryNewSpots();
void querySpots();
int SendRequestToServer(char *rxbuf, char *reqname);
unsigned long getMonthStartTime();
unsigned long getSTime(int diff_h);

char wsprserver[50] = {"wx.spdns.de"};
//char wsprserver[50] = {"192.168.0.109"};
#define WSPRPORT 9095

#define RXBUF_SIZE 1000000

int wsprsock;
struct hostent *wsprhost;
struct sockaddr_in wspr_sock_addr;
char db_txbuf[10000];
char db_rxbuf[RXBUF_SIZE];
unsigned int ID; 
int read_new_spots = 0;
int read_new_stats = 0;
int read_db = 0;

void *mysqlproc(void *pdata);
pthread_t mysql_tid; 

int mysql_init()
{
    // init ID
    ID = rand();
    
    // start a new process
    int ret = pthread_create(&mysql_tid,NULL,mysqlproc, 0);
    if(ret)
    {
        deb_printf("MYSQL","mysqlproc NOT started");
        return 0;
    }
    
    deb_printf("MYSQL","OK: mysqlproc started");
    return 1;
}

void *mysqlproc(void *pdata)
{
    pthread_detach(pthread_self());
    read_new_spots = 1;
    read_new_stats = 1;
    deb_printf("MYSQL","running");
    int rndspots = rand()%60;
    int rndstat = rand()%60;
    while(running)
    {
        if(strcmp(callsign,"NOCALL") && strcmp(qthloc,"AA0AA"))
        {
            // update the spots, every  minute
            time_t  tv_now = time(NULL);
            struct tm *tt = gmtime(&tv_now);
            if(((tt->tm_min & 1) == 1 && tt->tm_sec == rndspots) || read_new_spots == 1)
            {
                read_new_spots = 0;
                querySpots();
                sleep(1);
            }

            // update statistic files only if requested by the GUI, or once per hour
            if(read_db == 1 || (tt->tm_min==rndstat && tt->tm_sec==0) || read_new_stats == 1)
            {
                read_db = 0;
                read_new_stats = 0;
                queryStatistics();
                sleep(1);
            }
        }
        
        usleep(500000);
    }
    deb_printf("MYSQL","stopping MYSQL process");
    pthread_exit(NULL);
}

void querySpots()
{
    deb_printf("MYSQL","Read spots from database, update tables");
    
    // before generating the new files delete the old files
    // the GUI will then display : loading...
    char fn[256];
    snprintf(fn,sizeof(fn),"rm %s/SPOTS_JSON.txt %s/SPOTS_JSONTX.txt 2>/dev/nul",htmldir,htmldir);
    system(fn);

    // open TCP connection to the WSPR Server
    if(startServerConnection() == 0) return;

    // tell calls from config to the server, mycall and the 5 other calls
    sendTypeToServer("CALLS");  
    
    // request RX spots, reported by us
    int ret = SendRequestToServer(db_rxbuf,"REQU1");
    if(ret == 1) makeSpotsJSONtableSpots(db_rxbuf,"SPOTS_JSON.txt");
    
    // request TX spots, when we are transmitting
    ret = SendRequestToServer(db_rxbuf,"REQU2");
    if(ret == 1) makeSpotsJSONtableSpots(db_rxbuf,"SPOTS_JSONTX.txt");
    
    // close the database
    deb_printf("MYSQL","Spot update finished, CLOSE database and disconnect");
    sendTypeToServer("CLOSE");
    // close TCP connection
    stopServerConnection();
}

// query statistics but also spots, this is used when the user changes a selection
void queryStatistics()
{
    deb_printf("MYSQL","Read statistics from database, update tables");
    
    // before generating the new files delete the old files
    // the GUI will then display : loading...
    char fn[256];
    snprintf(fn,sizeof(fn),"rm %s/SPOTS_JSON_TX.txt %s/SPOTS_JSON_RX.txt %s/JSON_2WAY.txt%s/JSON_STAT.txt 2>/dev/nul",htmldir,htmldir,htmldir,htmldir);
    system(fn);
    
    // open TCP connection to the WSPR Server
    if(startServerConnection() == 0) return;

    // tell calls from config to the server, mycall and the 5 other calls
    sendTypeToServer("CALLS");
    sendTypeToServer("MYURL");  
    
    // request RX spots, reported by us
    int ret = SendRequestToServer(db_rxbuf,"REQU1");
    if(ret == 1) makeSpotsJSONtableSpots(db_rxbuf,"SPOTS_JSON.txt");
    
    // request TX spots, when we are transmitting
    ret = SendRequestToServer(db_rxbuf,"REQU2");
    if(ret == 1) makeSpotsJSONtableSpots(db_rxbuf,"SPOTS_JSONTX.txt");

    // request RX statistics, stations reported by us
    ret = SendRequestToServer(db_rxbuf,"REQU3");
    if(ret == 1) makeSpotsJSONtableRXTXstat(db_rxbuf,"SPOTS_JSON_RX.txt");
    
    // request TX statistics, stations reporting us
    ret = SendRequestToServer(db_rxbuf,"REQU4");
    if(ret == 1) makeSpotsJSONtableRXTXstat(db_rxbuf,"SPOTS_JSON_TX.txt");
    
    // request statistic counters
    ret = SendRequestToServer(db_rxbuf,"REQU5");
    if(ret == 1) makeSpotsJSONtableStatCounters(db_rxbuf,"JSON_STAT.txt");

    // request ranking
    ret = SendRequestToServer(db_rxbuf,"REQU6");
    if(ret == 1) makeSpotsJSONtableRanking(db_rxbuf);

    // request qthlocs for maps
    ret = SendRequestToServer(db_rxbuf,"REQU7");
    if(ret == 1) makeSpotsJSONtableQTHLOCs(db_rxbuf);
    
    // request WebWSPR stations
    ret = SendRequestToServer(db_rxbuf,"REQU8");
    if(ret == 1) makeStationsJSONtable(db_rxbuf);
    
    // close the database
    deb_printf("MYSQL","Statistics update finished, CLOSE database and disconnect");
    sendTypeToServer("CLOSE");
    // close TCP connection
    stopServerConnection();

    // when all file are written, create the file htmldir/ready.sig to signal the GUI that the files are updated
    // this signal file will be deleted if the user selects an other range of spots
    snprintf(fn,sizeof(fn),"%s/%s/ready.sig",htmldir,phpdir);
    FILE *fp=fopen(fn,"w");
    if(fp)
    {
        fprintf(fp,"x");
        fclose(fp);
    }
    else
        deb_printf("MYSQL","cannot write signal file: %s\n",fn);
}

// overwrite the spot files with: "{\"data\":[]}" and the lists will be empty
void writeFile(char *filename)
{
    char fn[256];
    snprintf(fn,sizeof(fn),"%s/%s",htmldir,filename);
    FILE *fp=fopen(fn,"w");
    if(fp)
    {
        fprintf(fp,"{\"data\":[]}\n");
        fclose(fp);
    }
}

void clearSpotFiles()
{
    writeFile("SPOTS_JSON.txt");
    writeFile("SPOTS_JSONTX.txt");
    writeFile("SPOTS_JSON_TX.txt");
    writeFile("SPOTS_JSON_RX.txt");
    writeFile("JSON_2WAY.txt");
    writeFile("JSON_STAT.txt");
}

int ConnectToServer()
{
    if((wsprhost=gethostbyname(wsprserver)) == NULL)
    {
        deb_printf("MYSQL","WSPR Socket kann nicht aufgeloest werden");
        return 0;
    }
    
    if((wsprsock = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        deb_printf("MYSQL","Socket fuer WSPR kann nicht geoeffnet werden");
        return 0;
    }

    // host byte order
    wspr_sock_addr.sin_family = AF_INET;
    wspr_sock_addr.sin_port = htons(WSPRPORT);
    wspr_sock_addr.sin_addr = *((struct in_addr *)wsprhost->h_addr);
    memset(&(wspr_sock_addr.sin_zero), '\0', 8);

    if(connect(wsprsock, (struct sockaddr *)&wspr_sock_addr, sizeof(struct sockaddr)) == -1)
    {
        deb_printf("MYSQL","WSPR Connect zu %s nicht moeglich",wsprserver);
        return 0;
    }
    //deb_printf("MYSQL","Verbunden mit WSPR-Server: %s",wsprserver);
    return 1;
}

int SendCmdToServer(char *cmd, char *type)
{
    // first four bytes are the length
    int clen = strlen(cmd) + 19; // 19 is the header length
    unsigned char len[4];
    len[0] = clen >> 24;
    len[1] = clen >> 16;
    len[2] = clen >> 8;
    len[3] = clen;
    send(wsprsock, len, 4,MSG_NOSIGNAL);
    
    // then 5 bytes Type, which is "QUERY"
    send(wsprsock, type, 5,MSG_NOSIGNAL); 
    
    // then 10 Bytes callsign
    char cs[10];
    memset(cs,' ',10);
    int ln = strlen(callsign);
    if(ln > sizeof(cs))
    {
        deb_printf("MYSQL","Callsign wrong length");
    }
    memcpy(cs,callsign,ln);
    send(wsprsock,callsign,10,MSG_NOSIGNAL);
    
    // and 4 bytes ID (0...32767)
    ID = ID & 0xefffffff; 
    unsigned char id[4];
    id[0] = ID >> 24;
    id[1] = ID >> 16;
    id[2] = ID >> 8;
    id[3] = ID;
    send(wsprsock, id, 4,MSG_NOSIGNAL);
    
    long bytesTowrite = strlen(cmd);
    void *p = cmd;

    while(1)
    {
        long bytes_written = send(wsprsock, p, bytesTowrite,MSG_NOSIGNAL);
        if(bytes_written < 0)
        {
            deb_printf("MYSQL","SendToServer: TCP Schreibfehler: %s",strerror(errno)); 
            return 0;
        }
        p += bytes_written;
        bytesTowrite -= bytes_written;
        if(bytesTowrite <= 0) break;
    }

    return 1;
}

// calculate the start time based on db_time (db_time is the number of hours to dislay, or 1000 for the full actual month)
unsigned long getStarttime()
{
    if(db_time == 1000)
        return getMonthStartTime();
    
    return getSTime(db_time);
}

int SendRequestToServer(char *rxbuf, char *reqname)
{
    char req[256];
    snprintf(req,sizeof(req),"%-10s%-10s%05d%02d%011ld",callsign,call_ur[call_db_idx],1000,getBandNumber(),getStarttime());
    int ret = SendCmdToServer(req,reqname);
    if(ret)
    {
        //deb_printf("MYSQL","read response");
        memset(rxbuf,0,RXBUF_SIZE);
        ret=ReadFromServer(rxbuf, RXBUF_SIZE);
    }
    return ret;
}

int SendQToServer(char *cmd)
{
    return SendCmdToServer(cmd,"QUERY");
}

void sendTypeToServer(char *type)
{
    int ret = 0;
    char cmd[10000] = {0};
    
    sprintf(cmd,"CALLS%s",callsign);
    for(int i=0; i < MAXURCALLS; i++)
    {
        if(strlen(call_ur[i]) > 2 && strcmp(call_ur[i],"NOCALL"))
        {
            strcat(cmd,";");
            strcat(cmd,call_ur[i]);
        }
    }
    
    if(strstr(type,"MYURL"))
    {
        sprintf(cmd,"MYURL%s,%s",callsign,myurl);
    }
    
    if((ret = SendCmdToServer(cmd,type)))
    {
        memset(db_rxbuf,0,sizeof(db_rxbuf));
        ret=ReadFromServer(db_rxbuf, sizeof(db_rxbuf));
        
        // print response
        if(db_rxbuf[3] == 2)
        {
            db_rxbuf[6] = 0;
            deb_printf("MYSQL","%s\n",db_rxbuf+4);
        }
    }
    return;
}

int startServerConnection()
{
    if(ConnectToServer())
    {
        //deb_printf("MYSQL","Connected to Server");
        return 1;
    }
    
    deb_printf("MYSQL","sendCallsToServer: Verbindung zum Server fehlgeschlagen");
    return 0;
}

void stopServerConnection()
{
    close(wsprsock);
}

int getBandNumber()
{
    int idx = 0;
    
    char b[30];
    strcpy(b,db_band);
    if(b[strlen(b)-1] == 'm') b[strlen(b)-1] = 0;
    if(b[strlen(b)-2] == 'c') b[strlen(b)-2] = 0;
    
    int f = atoi(b);
    //deb_printf("MYSQL","database band: %d\n",f);

    if (f == 2200) idx = 1;
    else if (f == 630) idx = 2;
    else if (f == 160) idx = 3;
    else if (f == 80) idx = 4;
    else if (f == 60) idx = 5;
    else if (f == 40) idx = 6;
    else if (f == 30) idx = 7;
    else if (f == 20) idx = 8;
    else if (f == 17) idx = 9;
    else if (f == 15) idx = 10;
    else if (f == 12) idx = 11;
    else if (f == 10) idx = 12;
    else if (f == 6) idx = 13;
    else if (f == 4) idx = 14;
    else if (f == 2) idx = 15;
    else if (f == 70) idx = 16;
    else idx = 17;

    return idx;

}

// send query and get result
int sendQueryToServer()
{
    int ret = 0;
    //deb_printf("MYSQL","Connected to Server, send query");
    if((ret=SendQToServer(db_txbuf)))
    {
        //deb_printf("MYSQL","read response");
        memset(db_rxbuf,0,sizeof(db_rxbuf));
        ret=ReadFromServer(db_rxbuf, sizeof(db_rxbuf));
        /*
        char txt[1000];
        *txt = 0;
        for(int i=0; i<10; i++) sprintf(txt+strlen(txt),"%02X ",(unsigned char)db_rxbuf[i]);
        deb_printf("MYSQL","response: %s", txt);
            * */
    }
    return ret;
}

// send query without result
int sendQueryToServerText(char *cmd)
{
    strcpy(db_txbuf,cmd);
    
    int ret = 0;
    //deb_printf("MYSQL","Connected to Server, send queryT: %s",cmd);
    if((ret=SendQToServer(db_txbuf)))
    {
        memset(db_rxbuf,0,sizeof(db_rxbuf));
        ret=ReadFromServer(db_rxbuf, sizeof(db_rxbuf));
        if(db_rxbuf[3] == 2)
        {
            db_rxbuf[6] = 0;
            deb_printf("MYSQL","response: %s\n",db_rxbuf+4);
        }
    }
    return ret;
}

// reads data from server and implements
// a read timeout wia polling
// return: -1=error, 0=nodata(Timeout), >0=datalen
int getDataFromServer(char *buf, int maxsize)
{
struct pollfd fd;
int ret;

    fd.fd = wsprsock;
    fd.events = POLLIN;
    
    ret = poll(&fd, 1, RXTIMEOUT); // second for timeout
    if(ret == -1) 
    {
        deb_printf("MYSQL","recv: %s (%d)\n", strerror(errno), errno);
        return -1;
    }
    if(ret == 0) 
    {
        deb_printf("MYSQL","recv timeout");
        return 0;
    }
    
    // short wait, get more data into the network receive buffer
    usleep(1000);
    
    return recv(wsprsock,buf,maxsize, 0);
}

#define MAYZSIZE    500000 // max size of compressed data

int ReadFromServer(char *buf, int maxsize)
{
int numbytes,total = 0;
int maxlen = maxsize;

    char *rxbuf = malloc(MAYZSIZE);

    //deb_printf("MYSQL","ReadFromServer");
    while(1)
    {
        numbytes = getDataFromServer(rxbuf+total,maxlen-total);
        if(numbytes == -1) return 0;
        if(numbytes == 0) 
        {
            deb_printf("MYSQL","receive data from server timeout, read: %ld from len:%d",total,maxlen);
            free(rxbuf);
            return 0;
        }
        
        //deb_printf("MYSQL","recv len:%d",numbytes);

        // die ersten 4 Bytes sind die Länge
        int len = (unsigned char)rxbuf[0];
        len <<= 8;
        len += (unsigned char)rxbuf[1];
        len <<= 8;
        len += (unsigned char)rxbuf[2];
        len <<= 8;
        len += (unsigned char)rxbuf[3];
        //printf("get len:%d\n",len);
        //deb_printf("MYSQL","get len:%d",len);

        // init maxlen to correct value
        if(maxlen > len) maxlen = len+4;
        
        total += numbytes;
        
        //deb_printf("MYSQL","total now:%d into %ld",total,sizeof(db_rxbuf));
        
        if(total >= maxlen)  break;
        
        if(total >= maxsize)
        {
            deb_printf("MYSQL","rx string too long %ld for the buffer %ld",total, maxsize);
            break;
        }
    }
    
    total -=4;   // remove length
    //printf("received %d bytes\n",total);
    
    // compressed data are now in rxbuf
    // uncompress it to buffer
    z_stream infstream;
    infstream.zalloc = Z_NULL;
    infstream.zfree = Z_NULL;
    infstream.opaque = Z_NULL;
    // setup "b" as the input and "c" as the compressed output
    infstream.avail_in = (uInt)total; // size of input
    infstream.next_in = (Bytef *)(rxbuf+4); // input char array
    infstream.avail_out = (uInt)maxsize; // size of output
    infstream.next_out = (Bytef *)buf; // output char array
     
    // the actual decompression work.
    inflateInit(&infstream);
    inflate(&infstream, Z_NO_FLUSH);
    inflateEnd(&infstream);
    
    //printf("uncompressed size: %ld\n",strlen(buf));
    
    //deb_printf("MYSQL","SPOTS list received from server, %d bytes",total);
    free(rxbuf);
    return 1;
}

struct timeval  tv_startmysql;
void setTimeMeasure(int no)
{
    if(no == 0)
    {
        // Start new measurement
        gettimeofday(&tv_startmysql, NULL);
        return;
    }
    
    struct timeval  tv_end;
	gettimeofday(&tv_end, NULL);
    
    long ssec = tv_startmysql.tv_sec - 1546950000;
    long stime = tv_startmysql.tv_usec + ssec*1000000;
    
    long esec = tv_end.tv_sec - 1546950000;
    long etime = tv_end.tv_usec + esec*1000000;
    
    long dtime = etime-stime; // in us
    
    if((dtime/1000) > RXTIMEOUT)
        deb_printf("MYSQL","Job % 2d time: % 5ld ms",no,dtime/1000);
    
    gettimeofday(&tv_startmysql, NULL);
}
