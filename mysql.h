int mysql_init();
int sendQueryToServer();
int sendQueryToServerText(char *cmd);
void clearSpotFiles();

extern int read_db;

#define RXTIMEOUT 20000 // 20s
