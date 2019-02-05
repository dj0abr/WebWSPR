/*
 * WSPR Toolkit for Linux
 * ======================
 * by DJ0ABR 
 * 
 * mysql_func.c
 * 
 * helper functions for mysql.c
 * 
 * */

#include <alsa/asoundlib.h>
#include <sndfile.h>
#include <inttypes.h>
#include "config.h"
#include "mysql.h"

int create_all_temptabs(int64_t startdate, int64_t enddate, int64_t monthstartdate, int band, char *call1, char *call2)
{
char tabname[256];
char txt[1024];

//printf("=============band: %d freq:%f\n",band,frequency);

    // fill MYSQL variables and Server-Header
    snprintf(txt,sizeof(txt),"SET @startdate=%" PRId64 ";SET @enddate=%" PRId64 ";SET @monthstartdate=%" PRId64 ";SET @CALL1='%s';SET @CALL2='%s';SET @selband=%d;",startdate,enddate,monthstartdate,call1,call2,band);
    sendQueryToServerText(txt);
    
    // this table contains all spots from both calls from startdate until now
    //create_temp_tabs(cmd, "tab_allspan", startdate, enddate, band, call1, call2);
    
    // this table contains all spots with CALL1 or CALL2 at the date interval and band
    // dropping the table is not required because we have a new mysql connection
    strcpy(tabname,"t_C1C2all");
    snprintf(txt,sizeof(txt),"create temporary table %s select * from SPOTS where datetime>=@startdate and datetime<=@enddate and (reporter=@CALL1 or reporter=@CALL2 or callsign=@CALL1 or callsign=@CALL2) and band=@selband order by datetime desc limit 50000;alter table %s add index (datetime,callsign,reporter);",tabname,tabname);
    sendQueryToServerText(txt);
    
    // this table contains all spots where CALL1 or CALL2 are the reporter
    strcpy(tabname,"t_C1C2rep");
    snprintf(txt,sizeof(txt),"create temporary table %s select * from t_C1C2all where reporter=@CALL1 or reporter=@CALL2;alter table %s add index (datetime,callsign,reporter);",tabname,tabname);
    sendQueryToServerText(txt);

    // this table contains all spots where CALL1 or CALL2 are the callsign
    strcpy(tabname,"t_C1C2cal");
    snprintf(txt,sizeof(txt),"create temporary table %s select * from t_C1C2all where callsign=@CALL1 or callsign=@CALL2;alter table %s add index (datetime,callsign,reporter);",tabname,tabname);
    sendQueryToServerText(txt);
    
    if(monthstartdate != 0)
    {
        // these tables are for the 2way calculation
        // Stationen die ich gehört habe
        strcpy(tabname,"t_mytab1");
        snprintf(txt,sizeof(txt),"create temporary table %s select datetime as D,callsign as C, reporter as R, qrg as Q, snr as S, qthloc as QTH from SPOTS where datetime>=@monthstartdate and reporter=@CALL1 and band=@selband group by callsign;alter table %s add index (D,C);",tabname,tabname);
        sendQueryToServerText(txt);

        // Stationen die mich gehört haben
        strcpy(tabname,"t_urtab1");
        snprintf(txt,sizeof(txt),"create temporary table %s select datetime as D,reporter as C, callsign as R, qrg as Q, snr as S, rptr_qthloc as QTH from SPOTS where datetime>=@monthstartdate and  callsign=@CALL1 and band=@selband group by reporter;alter table %s add index (D,C);",tabname,tabname);
        sendQueryToServerText(txt);

        // Stationen die Call2 gehört hat
        strcpy(tabname,"t_mytab2");
        snprintf(txt,sizeof(txt),"create temporary table %s select datetime as D,callsign as C, reporter as R, qrg as Q, snr as S, qthloc as QTH from SPOTS where datetime>=@monthstartdate and  reporter=@CALL2 and band=@selband group by callsign;alter table %s add index (D,C);",tabname,tabname);
        sendQueryToServerText(txt);

        // Stationen die Call2 gehört haben
        strcpy(tabname,"t_urtab2");
        snprintf(txt,sizeof(txt),"create temporary table %s select datetime as D,reporter as C, callsign as R, qrg as Q, snr as S, rptr_qthloc as QTH from SPOTS where datetime>=@monthstartdate and  callsign=@CALL2 and band=@selband group by reporter;alter table %s add index (D,C);",tabname,tabname);
        sendQueryToServerText(txt);

        // tables to join both 2way lists together
        strcpy(tabname,"t_my2way");
        snprintf(txt,sizeof(txt),"create temporary table %s select a.D as RXDATE, a.C,a.R,a.Q,a.S,a.QTH,b.D as TXDATE, b.C as CTX, b.R as RTX,b.Q as QTX,b.S as STX,b.QTH as QTHTX from t_mytab1 as a join t_urtab1 as b on a.C=b.C group by a.C;",tabname);
        sendQueryToServerText(txt);
        
        strcpy(tabname,"t_ur2way");
        snprintf(txt,sizeof(txt),"create temporary table %s select a.D as RXDATE, a.C,a.R,a.Q,a.S,a.QTH,b.D as TXDATE, b.C as CTX, b.R as RTX,b.Q as QTX,b.S as STX,b.QTH as QTHTX from t_mytab2 as a join t_urtab2 as b on a.C=b.C group by a.C;",tabname);
        sendQueryToServerText(txt);
    }
    
    return 1;
}

