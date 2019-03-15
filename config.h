int readConfigSilent();

#define MAXURCALLS 5

extern char callsign[30];
extern char call_ur[MAXURCALLS][30];
extern char myurl[256];
extern int call_db_idx;
extern char qthloc[20];
extern int secondsPerLine;
extern int qrg_WF_left;
extern int qrg_WF_right;
extern int txpower;
extern int txoffset;
extern double refminus80dBm;
extern int hWFrefval;
extern int hWFgain;
extern int hWFauto;
extern int WFmidnum;
extern int txmode;
extern char db_band[25];
extern int db_time;
extern int civ_adr;
extern int dds_txpwr;
extern int dds_cal;
extern int dds_if;
extern int hamlib_trxnr;
extern char sndcard_selection[50];

