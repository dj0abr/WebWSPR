void saveSpotsJSON();
char *buildJSON(char *ln);
void makeSpotsJSONtable(char *rxbuf, char *fn);
void makeSpotsJSONtableSpots(char *rxbuf, char *fn);
void makeSpotsJSONtableRXTXstat(char *rxbuf, char *fn);
void makeSpotsJSONtableStatCounters(char *rxbuf, char *fn);
void makeSpotsJSONtableRanking(char *rxbuf);
void makeSpotsJSONtableQTHLOCs(char *rxbuf);
void deleteJSONlistfiles();
int64_t getLastDate(char *s);

#define MAXFIELDS   25          // check also minimum required number for 2way calculation
#define MAXROWS     10000
#define MAXELEMLEN  51      
