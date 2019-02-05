/*
 * WSPR Toolkit for Linux
 * ======================
 * by DJ0ABR 
 * 
 * config.c
 * 
 * reads configuration values from the config file.
 * The config file has the name wsprconfig.js and
 * is located in the apache's HTML folder
 * 
 * */


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "wsprtk.h"
#include "config.h"
#include "debug.h"
#include "kmtools.h"
#include "dds.h"

void extract_band_array(FILE *fp);


char callsign[30];
char call_ur[MAXURCALLS][30];
int call_db_idx = 0;
char qthloc[20];
int secondsPerLine = 1;
int qrg_WF_left = 1400;
int qrg_WF_right = 1600;
int txpower = 37;
int txoffset = 1450;
double refminus80dBm = 0;
int hWFrefval = -70;
int hWFgain = 50;
int hWFauto = 0;
int WFmidnum = 4;
int txmode = 0;
char db_band[25];
int db_time;
int civ_adr = 0x94;
int dds_txpwr = 50;
int dds_cal = 0;
int dds_if = 9000;
int hamlib_trxnr = 0;

int readConfigSilent()
{
char txt[2000], *hp;

    deb_printf("CONFIG","reading configuration");
    memset(call_ur,0,sizeof(call_ur));
    sprintf(txt,"%s/phpdir/wsprconfig.js",htmldir);
    FILE *fp = fopen(txt,"r");
    if(fp)
    {
        while(fgets(txt,1999,fp))
        {
            hp = strstr(txt,"call:");
            if(hp)
            {
                hp = strchr(txt,':');
                char *p = cleanString(hp+1,1);
                snprintf(callsign,sizeof(callsign)-1,"%s",p);
            }
            
            hp = strstr(txt,"call_ur1:");
            if(hp)
            {
                hp = strchr(txt,':');
                char *p = cleanString(hp+1,1);
                snprintf(call_ur[0],sizeof(call_ur[0])-1,"%s",p);
            }
            
            hp = strstr(txt,"call_ur2:");
            if(hp)
            {
                hp = strchr(txt,':');
                char *p = cleanString(hp+1,1);
                snprintf(call_ur[1],sizeof(call_ur[1])-1,"%s",p);
            }
            
            hp = strstr(txt,"call_ur3:");
            if(hp)
            {
                hp = strchr(txt,':');
                char *p = cleanString(hp+1,1);
                snprintf(call_ur[2],sizeof(call_ur[2])-1,"%s",p);
            }
            
            hp = strstr(txt,"call_ur4:");
            if(hp)
            {
                hp = strchr(txt,':');
                char *p = cleanString(hp+1,1);
                snprintf(call_ur[3],sizeof(call_ur[3])-1,"%s",p);
            }
            
            hp = strstr(txt,"call_ur5:");
            if(hp)
            {
                hp = strchr(txt,':');
                char *p = cleanString(hp+1,1);
                snprintf(call_ur[4],sizeof(call_ur[4])-1,"%s",p);
            }
            
            hp = strstr(txt,"call_ur_sel:");
            if(hp)
            {
                hp = strchr(txt,':');
                char *p = cleanString(hp+1,1);
                call_db_idx = atoi(p+1) - 1;
                if(call_db_idx<0 || call_db_idx>4)
                    call_db_idx = 0;
            }
            
            hp = strstr(txt,"qthloc:");
            if(hp)
            {
                hp = strchr(txt,':');
                char *p = cleanString(hp+1,1);
                snprintf(qthloc,sizeof(qthloc)-1,"%s",p);
            }
                        
            hp = strstr(txt,"secondsPerLine:");
            if(hp)
            {
                hp = strchr(txt,':');
                char *p = cleanString(hp+1,1);
                secondsPerLine = atoi(p);
            }
                        
            hp = strstr(txt,"FrequencyLeft:");
            if(hp)
            {
                hp = strchr(txt,':');
                char *p = cleanString(hp+1,1);
                qrg_WF_left = atoi(p);
            }
                        
            hp = strstr(txt,"FrequencyRight:");
            if(hp)
            {
                hp = strchr(txt,':');
                char *p = cleanString(hp+1,1);
                qrg_WF_right = atoi(p);
            }
            
            hp = strstr(txt,"txmode:");
            if(hp)
            {
                hp = strchr(txt,':');
                char *p = cleanString(hp+1,1);
                txmode = atoi(p);
            }
            
            hp = strstr(txt,"txpower:");
            if(hp)
            {
                hp = strchr(txt,':');
                char *p = cleanString(hp+1,1);
                txpower = atoi(p);
            }
            
            hp = strstr(txt,"txoffset:");
            if(hp)
            {
                hp = strchr(txt,':');
                char *p = cleanString(hp+1,1);
                txoffset = atoi(p);
            }

            hp = strstr(txt,"calib80dBm:");
            if(hp)
            {
                hp = strchr(txt,':');
                char *p = cleanString(hp+1,1);
                refminus80dBm = atof(p);
            }

            hp = strstr(txt,"levelWF:");
            if(hp)
            {
                hp = strchr(txt,':');
                char *p = cleanString(hp+1,1);
                hWFrefval = atoi(p);
            }

            hp = strstr(txt,"gainWF:");
            if(hp)
            {
                hp = strchr(txt,':');
                char *p = cleanString(hp+1,1);
                hWFgain = atoi(p);
            }

            hp = strstr(txt,"autoWF:");
            if(hp)
            {
                hp = strchr(txt,':');
                char *p = cleanString(hp+1,1);
                hWFauto = atoi(p);
            }

            hp = strstr(txt,"midHF:");
            if(hp)
            {
                hp = strchr(txt,':');
                char *p = cleanString(hp+1,1);
                WFmidnum = atoi(p);
            }
            
            hp = strstr(txt,"listband:");
            if(hp)
            {
                hp = strchr(txt,':');
                char *p = cleanString(hp+1,1);
                snprintf(db_band,sizeof(db_band)-1,"%s",p);
            }
            
            hp = strstr(txt,"listtime:");
            if(hp)
            {
                hp = strchr(txt,':');
                char *p = cleanString(hp+1,1);
                char *p1=strchr(p,'h');
                if(p1) *p1=0;
                else continue;
                db_time = atoi(p);
            }

            hp = strstr(txt,"civ_adr:");
            if(hp)
            {
                hp = strchr(txt,':');
                char *p = cleanString(hp+1,1);
                char sciv[20];
                snprintf(sciv,sizeof(sciv)-1,"%s",p);
                // calculate civ address
                char *ph=strchr(sciv,'h');
                if(ph)
                    sscanf(sciv, "%02x", &civ_adr);
                else
                    civ_adr = atoi(sciv);
                //deb_printf("CONFIG","CIV-address: %d %02Xh",civ_adr,civ_adr);
            }
            
            hp = strstr(txt,"dds_txpwr:");
            if(hp)
            {
                hp = strchr(txt,':');
                char *p = cleanString(hp+1,1);
                dds_txpwr = atoi(p);
                dds_update_power = 1;
            }
            
            hp = strstr(txt,"dds_cal:");
            if(hp)
            {
                hp = strchr(txt,':');
                char *p = cleanString(hp+1,1);
                dds_cal = atoi(p);
                dds_update_cal = 1;
            }
            
            hp = strstr(txt,"dds_if:");
            if(hp)
            {
                hp = strchr(txt,':');
                char *p = cleanString(hp+1,1);
                dds_if = atoi(p);
            }

            hp = strstr(txt,"hamlib_trx:");
            if(hp)
            {
                hp = strchr(txt,':');
                char *p = cleanString(hp+1,0);
                // we need the number only
                char *ps = strchr(p,' ');
                if(ps)
                {
                    *ps = 0;
                    hamlib_trxnr = atoi(p);
                }
                else
                {
                    //deb_printf("CONFIG","error in HAMLIB transceiver information string");
                }
            }
            
            hp = strstr(txt,"qrgs:");
            if(hp)
            {
                extract_band_array(fp);
            }
        }

        fclose(fp);
    }
    else
    {
        deb_printf("CONFIG","Kann die Config-Datei %s nicht finden.\n",txt);
        return 0;
    }
    deb_printf("CONFIG","OK, mycall: %s",callsign);
    return 1;
}

// Lese den Inhalt von Config Arrays
// das Ende ist eine Zeile mit }
void extract_band_array(FILE *fp)
{
char txt[1000];
char *band, *qrg, *hp;

    while(fgets(txt,999,fp))
    {
        if(strstr(txt,"}")) return;
        char *p = cleanString(txt,1);
        hp = strchr(p,':');
        if(hp)
        {
            *hp = 0;
            band = p;
            qrg = hp+1;
            double dqrg = atof(qrg);
            for(int i=0; i<NUMOFBANDS; i++)
            {
                if(!strcmp(myband[i].band,band))
                {
                    myband[i].qrg = dqrg;
                    //printf("<%s><%f>\n",myband[i].band, myband[i].qrg);
                    break;
                }
            }
        }
    }
}
