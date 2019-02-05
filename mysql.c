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
#include "mysql_func.h"
#include "mysql.h"
#include "hopping.h"
#include "kmtools.h"

void queryStatistics();
int ConnectToServer();
int SendQToServer(char *cmd);
int ReadFromServer(char *buf, int maxsize);
int readSpotsRX();
int readSpotsTXstat();
int readSpotsRXstat();
int readSpotsTX();
int readStatistics();
int read2way();
int getBandNumber();
int readQTHlocs(int mode);
int SendCmdToServer(char *cmd, char *type);
void setTimeMeasure(int no);
void sendTypeToServer(char *type);
int readRanking();
int startServerConnection();
void stopServerConnection();
void queryNewSpots();
void querySpots();

char wsprserver[50] = {"wx.spdns.de"};
#define WSPRPORT 9094

int wsprsock;
struct hostent *wsprhost;
struct sockaddr_in wspr_sock_addr;
char db_txbuf[10000];
char db_rxbuf[1000000];
unsigned int ID; 
int read_new_spots = 0;
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
            if(((tt->tm_min & 1) == 0 && tt->tm_sec == rndspots) || read_new_spots == 1)
            {
                rndspots = (rndspots + 60)%60;
                
                read_new_spots = 0;
                querySpots();
                sleep(1);
            }

            // update statistic files only if requested by the GUI
            if(read_db == 1 || (tt->tm_min==rndstat && tt->tm_sec==0))
            {
                read_db = 0;
                queryStatistics();
                sleep(1);
            }
        }
        
        usleep(500000);
    }
    deb_printf("MYSQL","stopping MYSQL process");
    pthread_exit(NULL);
}

/*
 * ein Tick in c# sind 100ns = 10e-7s
 * from.Ticks / 100000000; : 10e-7 / 10e-8 = 10s
 * der hier Benutzte Zeitstempel ist also in 0,1s Schritten
 * 
 * Die Linuxzeit beginnt am 1.1.1970
 * in c# sind das  62135596800 Sekunden
 * 0 Linux-Sekunden sind also 62135596800 Net-Sekunden
 *                            6368152061
 *                            6368158980
 * */

int64_t getDbTicks(int64_t epochticks)
{
    // berechnet Net Sekunden dieser Zeitstempel
    int64_t netSecs = epochticks + (int64_t)62135596800;
    
    // die Datenbank hat 10s Auflösung, also:
    return netSecs / (int64_t)10;
}


void querySpots()
{
    // make times
    // current time (=end time)
    time_t currt = time(NULL);
    int64_t dbcurrtime = getDbTicks(currt);
    
    // starttime for timespan
    int64_t hours6 = currt - (3*3600); // max 3 hours, too large lists will slow the system down and nobody will read it
    int64_t dbstarttime = getDbTicks(hours6);
    
    deb_printf("MYSQL","Read spots from database, update tables, %" PRId64 " seconds",(dbcurrtime-dbstarttime)*10);
    
    if(startServerConnection() == 0) return;
    
    sendTypeToServer("CALLS");
    
    // before generating the new files delete the old files
    // the GUI will then display : loading...
    char fn[256];
    snprintf(fn,sizeof(fn),"rm %s/SPOTS_JSON.txt %s/SPOTS_JSONTX.txt 2>/dev/nul",htmldir,htmldir);
    system(fn);
    
    // send preparations of tables ... , these queries have no response
    if(create_all_temptabs(dbstarttime, dbcurrtime, 0, getBandNumber(), callsign, call_ur[call_db_idx]) == 0)
        return; // cannot send to server

    // also query the spots if the band has changed
    // RX spots
    if(readSpotsRX(dbstarttime, dbcurrtime) == 0) return;
    makeSpotsJSONtable(db_rxbuf,"SPOTS_JSON.txt");

    // TX spots
    if(readSpotsTX(dbstarttime, dbcurrtime) == 0) return;
    makeSpotsJSONtable(db_rxbuf,"SPOTS_JSONTX.txt");

    deb_printf("MYSQL","Spots update finished, CLOSE database and disconnect");
    sendTypeToServer("CLOSE");
    stopServerConnection();
}

void queryStatistics()
{
    char fn[256];
    
    // make times
    // current time (=end time)
    time_t currt = time(NULL);
    int64_t dbcurrtime = getDbTicks(currt);
    
    // starttime for timespan
    int64_t hours6 = currt - (db_time*3600);
    int64_t dbstarttime = getDbTicks(hours6);
    
    // startime of current month
    struct tm *t = localtime (&currt);
    t->tm_hour = 0;
    t->tm_min = 0;
    t->tm_sec = 0;
    t->tm_mday = 1;
    time_t tt = mktime(t);
    int64_t dbmonthstarttime = getDbTicks(tt);
    
    // Prüfung der Zeitberechnung:
    /*char text[256];
    struct tm *tm = localtime(&tt);
    strftime(text,255,"%Y.%m.%d %H.%M.%S ",tm);
    printf("Monatsbeginn: %s\n",text);
    
    tm = localtime(&currt);
    strftime(text,255,"%Y.%m.%d %H.%M.%S ",tm);
    printf("jetzt: %s\n",text);*/
    
    //printf("updating: %" PRId64 " to %" PRId64 " = %" PRId64 " seconds\n",dbstarttime,dbcurrtime, (dbcurrtime-dbstarttime)*10);
    
    deb_printf("MYSQL","Read statistics from database, update tables, %" PRId64 " seconds",(dbcurrtime-dbstarttime)*10);
    
    if(startServerConnection() == 0) return;
    
    // send preparations of tables ... , these queries have no response
    if(create_all_temptabs(dbstarttime, dbcurrtime, dbmonthstarttime, getBandNumber(), callsign, call_ur[call_db_idx]) == 0)
        return; // cannot send to server

    // also query the spots if the band has changed
    // RX spots
    if(readSpotsRX(dbstarttime, dbcurrtime) == 0) return;
    makeSpotsJSONtable(db_rxbuf,"SPOTS_JSON.txt");

    // TX spots
    if(readSpotsTX(dbstarttime, dbcurrtime) == 0) return;
    makeSpotsJSONtable(db_rxbuf,"SPOTS_JSONTX.txt");
    
    if(readSpotsTXstat() == 0) return;
    makeSpotsJSONtable(db_rxbuf,"SPOTS_JSON_TX.txt");

    if(readSpotsRXstat() == 0) return;
    makeSpotsJSONtable(db_rxbuf,"SPOTS_JSON_RX.txt");
    
    if(read2way() == 0) return;
    makeSpotsJSONtable(db_rxbuf,"JSON_2WAY.txt");

    if(readStatistics() == 0) return;
    makeSpotsJSONtable(db_rxbuf,"JSON_STAT.txt");
    
    if(readQTHlocs(0) == 0) return;
    makeSpotsJSONtable(db_rxbuf,"QTH_MY_RX.txt");
    
    if(readQTHlocs(1) == 0) return;
    makeSpotsJSONtable(db_rxbuf,"QTH_MY_TX.txt");
    
    if(readQTHlocs(2) == 0) return;
    makeSpotsJSONtable(db_rxbuf,"QTH_UR_RX.txt");
    
    if(readQTHlocs(3) == 0) return;
    makeSpotsJSONtable(db_rxbuf,"QTH_UR_TX.txt");
    
    readRanking();

    deb_printf("MYSQL","Statistics update finished, CLOSE database and disconnect");
    sendTypeToServer("CLOSE");
    stopServerConnection();
    
    // when all file are written, create the file htmldir/ready.sig
    // to signal the GUI that the files are updated
    // this signal file will be deleted if the user selects an other range of spots
    snprintf(fn,sizeof(fn),"%s/%s/ready.sig",htmldir,phpdir);
    FILE *fp=fopen(fn,"w");
    if(fp)
    {
        //printf("%s\n",fn);
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
        deb_printf("MYSQL","Connected to Server");
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
    deb_printf("MYSQL","database band: %d\n",f);

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
/* ===========================================
 * evt voranstellen:  set statement max_statement_time=1 for  ...und dann weiter mit select usw.
 * die Zahl ist in Sekunden
 * die Query bricht dann ab, in dem Fall sollte das alte json File nicht überschrieben werden, daher Rückgabewerte machen !
 * */

int readSpotsRX()
{
    // !!!!! just a note: the connection string to the mysql server must contain "Allow User Variables=True;" !!!!!!
    // SQL variables are already set and temp tables are prepared
    // hier keinen eigenen "QUERY"-Header für den wsprserver weil dieser String am vorherigen Kommandos hängt
    strcpy(db_txbuf,"select a.qrg as QRG1,b.qrg as QRG2,a.distance as DIST1,b.distance as DIST2,a.snr as SNR1,b.snr as SNR2,a.datetime as DATE, a.reporter as C1, if(b.reporter is NULL,' ',b.reporter) as C2, a.callsign as HEARD from t_C1C2rep as a left join t_C1C2rep as b on a.datetime=b.datetime and a.callsign=b.callsign and a.reporter<>b.reporter where a.reporter=@CALL1 union all select b.qrg as QRG1,a.qrg as QRG2,b.distance as DIST1,a.distance as DIST2,b.snr as SNR1,a.snr as SNR2,a.datetime , if(b.reporter is NULL,' ',b.reporter) as C1,a.reporter as c2, a.callsign from t_C1C2rep as a left join t_C1C2rep as b on a.datetime=b.datetime and a.callsign=b.callsign and a.reporter<>b.reporter where a.reporter=@CALL2 and b.reporter is NULL order by DATE,HEARD;");

    return sendQueryToServer();
}

int readSpotsTX()
{
    // !!!!! just a note: the connection string to the mysql server must contain "Allow User Variables=True;" !!!!!!
strcpy(db_txbuf,"select a.qrg as QRG1,b.qrg as QRG2,a.distance as DIST1,b.distance as DIST2,a.snr as SNR1,b.snr as SNR2,a.datetime as DATE, a.callsign as C1, if(b.callsign is NULL,' ',b.callsign) as C2, a.reporter as HEARD from t_C1C2cal as a left join t_C1C2cal as b on a.datetime=b.datetime and a.reporter=b.reporter and a.callsign<>b.callsign where a.callsign=@CALL1 union all select b.qrg as QRG1,a.qrg as QRG2,b.distance as DIST1,a.distance as DIST2,b.snr as SNR1,a.snr as SNR2,a.datetime , if(b.callsign is NULL,' ',b.callsign) as C1,a.callsign as c2, a.reporter from t_C1C2cal as a left join t_C1C2cal as b on a.datetime=b.datetime and a.reporter=b.reporter and a.callsign<>b.callsign where a.callsign=@CALL2 and b.callsign is NULL order by DATE,HEARD ;");
    
    return sendQueryToServer();
}

int readSpotsTXstat()
{
    // just a note: the connection string to the mysql server must contain "Allow User Variables=True;"
    
    strcpy(db_txbuf,"select REPORTER1, DATE1 as D1,C1,QRG1,SNR1,DIST1,DATE2 as D2,C2,QRG2,SNR2,DIST2 from (select a.snr as SNR1, a.qrg as QRG1, a.distance as DIST1, b.snr as SNR2, b.qrg as QRG2, b.distance as DIST2,a.datetime as DATE1, a.callsign as C1, a.reporter AS REPORTER1, b.datetime as DATE2, b.callsign as C2, b.reporter AS REPORTER2 from (select MAX(datetime) as datetime,callsign,qrg,MAX(snr)as snr,qthloc,dBm,reporter,rptr_qthloc,distance,band from t_C1C2all where callsign=@CALL1 group by reporter) as a left join (select MAX(datetime) as datetime,callsign,qrg,MAX(snr)as snr,qthloc,dBm,reporter,rptr_qthloc,distance,band from t_C1C2all where callsign=@CALL2 group by reporter) as b on a.reporter=b.reporter and a.callsign<>b.callsign union all select b.snr as SNR1, b.qrg as QRG1,b.distance as DIST1, a.snr as SNR2, a.qrg as QRG2, a.distance as DIST2,b.datetime as DATE1, b.callsign as C1, a.reporter AS REPORTER1, a.datetime as DATE2, a.callsign as C2, b.reporter AS REPORTER2 from (select MAX(datetime) as datetime,callsign,qrg,MAX(snr)as snr,qthloc,dBm,reporter,rptr_qthloc,distance,band from t_C1C2all where callsign=@CALL2 group by reporter) as a left join (select MAX(datetime)as datetime,callsign,qrg,MAX(snr)as snr,qthloc,dBm,reporter,rptr_qthloc,distance,band from t_C1C2all where callsign=@CALL1 group by reporter) as b on a.reporter=b.reporter and a.callsign<>b.callsign) as c group by REPORTER1;");
    
    return sendQueryToServer();
}

int readSpotsRXstat()
{
    // just a note: the connection string to the mysql server must contain "Allow User Variables=True;"
    
    strcpy(db_txbuf,"select REPORTER1, DATE1 as D1,C1,QRG1,SNR1,DIST1,DATE2 as D2,C2,QRG2,SNR2,DIST2 from (select a.snr as SNR1, a.qrg as QRG1, a.distance as DIST1, b.snr as SNR2, b.qrg as QRG2, b.distance as DIST2,a.datetime as DATE1, a.reporter as C1, a.callsign AS REPORTER1, b.datetime as DATE2, b.reporter as C2, b.callsign AS REPORTER2 from (select MAX(datetime) as datetime,reporter,qrg,MAX(snr)as snr,qthloc,dBm,callsign,rptr_qthloc,distance,band from t_C1C2all where reporter=@CALL1 group by callsign) as a left join ( select MAX(datetime) as datetime,reporter,qrg,MAX(snr)as snr,qthloc,dBm,callsign,rptr_qthloc,distance,band from t_C1C2all where reporter=@CALL2 group by callsign) as b on a.callsign=b.callsign and a.reporter<>b.reporter union all select b.snr as SNR1, b.qrg as QRG1,b.distance as DIST1, a.snr as SNR2, a.qrg as QRG2, a.distance as DIST2,b.datetime as DATE1, b.reporter as C1, a.callsign AS REPORTER1, a.datetime as DATE2, a.reporter as C2, b.callsign AS REPORTER2 from (select MAX(datetime) as datetime,reporter,qrg,MAX(snr)as snr,qthloc,dBm,callsign,rptr_qthloc,distance,band from t_C1C2all where reporter=@CALL2 group by callsign) as a left join (select MAX(datetime)as datetime,reporter,qrg,MAX(snr)as snr,qthloc,dBm,callsign,rptr_qthloc,distance,band from t_C1C2all where reporter=@CALL1 group by callsign) as b on a.callsign=b.callsign and a.reporter<>b.reporter) as c group by REPORTER1;");
    
    return sendQueryToServer();
}

void catStatString(char *p, char *command, char *title, char mode, char *unit)
{
    if(mode == 'R')
        sprintf(p," union all select \"%s\" as DESCR, convert(SUM(CNT1),char(50)) as CALL1, convert(SUM(CNT2),char(50)) as CALL2 from (select %s as CNT1, NULL as CNT2 from t_C1C2all where reporter=@CALL1 union all select NULL as CNT1, %s as CNT2 from t_C1C2all where reporter=@CALL2) as taba",title,command,command);
    else
        sprintf(p," union all select \"%s\" as DESCR, convert(SUM(CNT1),char(50)) as CALL1, convert(SUM(CNT2),char(50)) as CALL2 from (select %s as CNT1, NULL as CNT2 from t_C1C2all where callsign=@CALL1 union all select NULL as CNT1, %s as CNT2 from t_C1C2all where callsign=@CALL2) as taba",title,command,command);
}

int readStatistics()
{
    // and add the queries, all queries connected in an union to get one resulting table
    // add start and end date
    strcpy(db_txbuf,
/*"select 'Start Date (statistics)' as DESCR, C1 as CALL1, C2 as CALL2 from (select MIN(datetime) as C1, NULL as C2 from t_C1C2all where (callsign=@CALL1 or reporter=@CALL1) union all select NULL as C1, MIN(datetime) as C2 from t_C1C2all where (callsign=@CALL2 or reporter=@CALL2)) as taba union all select 'End Date (statistics)' as DESCR, C1 as CALL1, C2 as CALL2 from (select MAX(datetime) as C1, NULL as C2 from t_C1C2all where (callsign=@CALL1 or reporter=@CALL1) union all select NULL as C1, MAX(datetime) as C2 from t_C1C2all where (callsign=@CALL2 or reporter=@CALL2)) as taba");*/
"select 'Start Date (statistics)' as DESCR, C1 as CALL1, C1 as CALL2 from (select MIN(datetime) as C1 from t_C1C2all) as xx union all select 'End Date (statistics)' as DESCR, C1 as CALL1, C1 as CALL2 from (select MAX(datetime) as C1 from t_C1C2all) as yy");

    catStatString(db_txbuf+strlen(db_txbuf),"COUNT(*)","all RX spots",'R'," ");
    catStatString(db_txbuf+strlen(db_txbuf),"COUNT(*)","all TX spots",'T'," ");
        
    catStatString(db_txbuf+strlen(db_txbuf),"COUNT(DISTINCT callsign)","all RX stations",'R'," ");
    catStatString(db_txbuf+strlen(db_txbuf),"COUNT(DISTINCT reporter)","all TX stations",'T'," ");

    catStatString(db_txbuf+strlen(db_txbuf),"MAX(distance)","max RX distance",'R',"km");
    catStatString(db_txbuf+strlen(db_txbuf),"MAX(distance)","max TX distance",'T',"km");

    catStatString(db_txbuf+strlen(db_txbuf),"AVG(snr)","mid RX snr",'R',"dBm");
    catStatString(db_txbuf+strlen(db_txbuf),"AVG(snr)","mid TX snr",'T',"dBm");
    
    // and add the 2-way spot numer
    strcat(db_txbuf,
" union all select \"2-way contacts (month)\" as DESCR, sum(x) as CALL1, sum(y) as CALL2 from(select count(0) as x, NULL as y from (select count(*) from t_mytab1 as a join t_urtab1 as b on a.C=b.C group by a.C) as xx union all select NULL as x, count(0) as y from (select count(*) from t_mytab2 as a join t_urtab2 as b on a.C=b.C group by a.C) as xx) as xx");
    
    return sendQueryToServer();
}

int read2way()
{
    strcpy(db_txbuf,"select * from (select a.C as CS, a.R as MYCALL, a.RXDATE as MYRXD, a.Q as MYRXQRG, a.S as MYSNR, a.QTH as MYQTH, a.TXDATE as MYTXD, a.QTX as MYTXQRG, a.STX as MYTXSNR, a.QTHTX as MYTXQTH, b.R as URCALL, b.RXDATE as URRXD, b.Q as URRXQRG, b.S as URSNR, b.QTH as URQTH, b.TXDATE as URTXD, b.QTX as URTXQRG, b.STX as URTXSNR, b.QTHTX as URTXQTH  from t_my2way as a left join t_ur2way as b on a.C=b.C union all select a.C as CS, NULL as MYCALL, b.RXDATE as MYRXD, b.Q as MYRXQRG, b.S as MYSNR, b.QTH as MYQTH, b.TXDATE as MYTXD, b.QTX as MYTXQRG, b.STX as MYTXSNR, b.QTHTX as MYTXQTH, a.R as URCALL, a.RXDATE as URRXD, a.Q as URRXQRG, a.S as URSNR, a.QTH as URQTH, a.TXDATE as URTXD, a.QTX as URTXQRG, a.STX as URTXSNR, a.QTHTX as URTXQTH  from t_ur2way as a left join t_my2way as b on a.C=b.C where b.RXDATE is NULL) as c order by CS;");
    
    return sendQueryToServer();
}

int readQTHlocs(int mode)
{
    if(mode == 0)
        strcpy(db_txbuf,"select qthloc as _,callsign as __ from t_C1C2all where reporter=@CALL1 group by qthloc;");
    if(mode == 1)
        strcpy(db_txbuf,"select rptr_qthloc as _, reporter as __ from t_C1C2all where callsign=@CALL1 group by rptr_qthloc;");
    if(mode == 2)
        strcpy(db_txbuf,"select qthloc as _,callsign as __ from t_C1C2all where reporter=@CALL2 group by qthloc;");
    if(mode == 3)
        strcpy(db_txbuf,"select rptr_qthloc as _, reporter as __ from t_C1C2all where callsign=@CALL2 group by rptr_qthloc;");
    
    return sendQueryToServer();
}

char rankbands [16][10]={
    "LF/VLF",
    "160m",
    "80m",
    "60m",
    "40m",
    "30m",
    "20m",
    "17m",
    "15m",
    "12m",
    "10m",
    "6m",
    "4m",
    "2m",
    "70cm",
    "23cm"
};

int readRanking()
{
    char fn[256];
    int ret;
    // get ranking per band
    for(int band=0; band<16; band++)
    {
        snprintf(db_txbuf,sizeof(db_txbuf),"select rank,spots,callsign,locator from RANKING where band=\"%s\" order by rank desc",rankbands[band]);
        ret = sendQueryToServer();
        if(ret == 0)
        {
            deb_printf("MYSQL","1:read ranking from DB error");
            return 0;
        }
        snprintf(fn,sizeof(fn),"JSON_RANKING_%d.txt",band+1);
        makeSpotsJSONtable(db_rxbuf,fn);
    }
    
    // get top list, only rank 1 for all bands
    snprintf(db_txbuf,sizeof(db_txbuf),"select ID,band,rank,spots,group_concat(callsign separator \", \") as callsign,locator from RANKING where rank<2 group by ID");
    ret = sendQueryToServer();
    if(ret == 0)
    {
        deb_printf("MYSQL","2:read ranking from DB error");
        return 0;
    }
    snprintf(fn,sizeof(fn),"JSON_RANKING_TOP.txt");
    makeSpotsJSONtable(db_rxbuf,fn);
    
    return 1;
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
     
    // the actual DE-compression work.
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
