void init_dds();
void readDDSmessage(int reti);
void dds_setQRG(double trx_frequency);
void dds_updatePower();
void dds_updateCalib();

extern int dds_update_power;
extern int dds_update_cal;
