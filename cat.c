/*
 * WSPR Toolkit for Linux
 * ======================
 * by DJ0ABR 
 * 
 * cat.c ... Transceiver CAT interface
 * 
 * mainly handles the serial interfaceand routes commends to the CAT modules
 * 
 * */

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <stdlib.h>
#include <stdint.h>
#include <wchar.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <sys/file.h>
#include <pthread.h>
#include "wsprtk.h"
#include "debug.h"
#include "soundcard.h"
#include "fft.h"
#include "config.h"
#include "coder.h"
#include "civ.h"
#include "hopping.h"
#include "status.h"
#include "dds.h"
#include "kmtools.h"
#include "hamlib.h"

void closeSerial();
int activate_serial(char *device, int baudrate);
void openSerialInterface();
int read_port();
void write_port(unsigned char *data, int len);
void do_command();

void *catproc(void *pdata);
pthread_t cat_tid; 

#define SERSPEED_CIV B4800
#define SERSPEED_DDS B115200

int fd_ser = -1; // handle of the ser interface
char device[20] = {"/dev/ttyUSB0"};
int ser_command = 10;    // 0=no action 1=queryQRG 2=setPTT 3=releasePTT 4=setQRG

// creates a thread to run all serial specific jobs
// call this once after program start
// returns 0=failed, 1=ok
int cat_init()
{
    // automatically choose an USB port
    // start a new process
    int ret = pthread_create(&cat_tid,NULL,catproc, 0);
    if(ret)
    {
        deb_printf("CAT","catproc NOT started");
        return 0;
    }
    
    deb_printf("CAT","OK: catproc started");
    return 1;
}

void *catproc(void *pdata)
{
static int last_txmode = -1;

    pthread_detach(pthread_self());
    
    while(running)
    {
        if(txmode == 0)
        {
            // TX is OFF, no action
            sleep(1);
            continue;
        }
        
        if(txmode == 1)
        {
            // ICOM mode selected
            // open the serial interface and handle all messages through the CIV module
            if(fd_ser == -1)
            {
                // serial IF is closed, try to open it
                sleep(1);
                openSerialInterface(SERSPEED_CIV);
            }
            else
            {
                // serial IF is open, read one byte and pass it to the CIV message receiver routine
                int reti = read_port();
                if(reti == -1) 
                    usleep(10000);  // no data received
                else
                    readCIVmessage(reti);
            }
        }
        
        if(txmode == 2)
        {
            //  U02-DDS mode
            // init DDS at 1:53 (no other DDS job are running at this time)
            if(checkTime(1,53))
                init_dds();
                        
            // init DDS if the DDS mode was switched on
            if(last_txmode != txmode && fd_ser != -1)
                init_dds();
            
            if(dds_update_power == 1)
                dds_updatePower();

            if(dds_update_cal == 1)
                dds_updateCalib();

            // open the serial interface and handle all messages through the DDS module
            // init DDS after successful opening of the serial IF
            if(fd_ser == -1)
            {
                // serial IF is closed, try to open it
                sleep(1);
                openSerialInterface(SERSPEED_DDS);
                if(fd_ser != -1)
                    init_dds();
            }
            else
            {
                // serial IF is open, read one byte and pass it to the DDS message receiver routine
                int reti = read_port();
                if(reti == -1) 
                    usleep(10000);  // no data received
                else
                    readDDSmessage(reti);
            }
        }
        
        if(txmode == 3)
        {
            // PTT control via RTS/DTR line
            // open the serial interface and just keep it open for RTS/DTR actions
            if(fd_ser == -1)
            {
                // serial IF is closed, try to open it
                sleep(1);
                closeSerial();
                openSerialInterface(SERSPEED_CIV);
            }
            else
            {
                // serial IF is open, read incoming bytes but ignore it
                int reti = read_port();
                if(reti == -1) 
                    usleep(10000);  // no data received
                else
                    usleep(10);  // data received
            }
        }
        
        if(txmode == 4)
        {
            // HAMLIB mode
            // open the serial interface just once to find the device name
            // then close it for use by hamlib
            if(fd_ser == -1)
            {
                // device unknown, try to find a serial port
                closeSerial();
                openSerialInterface(SERSPEED_CIV);
                deb_printf("SERIAL","found HAMLIB serial device: %s",device);
                sleep(1);
            }
            usleep(100000);
        }
        
        if(ser_command) do_command();   // a process requested a CAT action
        
        last_txmode = txmode;
    }
    closeSerial();
    pthread_exit(NULL);
}

void closeSerial()
{
    if(fd_ser != -1) close(fd_ser);
}

// Öffne die serielle Schnittstelle
int activate_serial(char *device,int baudrate)
{
	struct termios tty;
    
	closeSerial();
	fd_ser = open(device, O_RDWR | O_NDELAY);
	if (fd_ser < 0) {
		deb_printf("SERIAL","error when opening %s, errno=%d", device,errno);
		return -1;
	}

	if (tcgetattr(fd_ser, &tty) != 0) {
		deb_printf("SERIAL","error %d from tcgetattr %s", errno,device);
		return -1;
	}

	cfsetospeed(&tty, baudrate);
	cfsetispeed(&tty, baudrate);

	tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;
	tty.c_iflag &= ~ICRNL; // binary mode (no CRLF conversion)
	tty.c_lflag = 0;

	tty.c_oflag = 0;
	tty.c_cc[VMIN] = 0; // 0=nonblocking read, 1=blocking read
	tty.c_cc[VTIME] = 0;

	tty.c_iflag &= ~(IXON | IXOFF | IXANY);

	tty.c_cflag |= (CLOCAL | CREAD);

	tty.c_cflag &= ~(PARENB | PARODD);
	tty.c_cflag |= CSTOPB;
	tty.c_cflag &= ~CRTSCTS;
    

	if (tcsetattr(fd_ser, TCSANOW, &tty) != 0) {
		deb_printf("SERIAL","error %d from tcsetattr %s", errno, device);
		return -1;
	}
	
	// set RTS/DTR
    int flags;
    ioctl(fd_ser, TIOCMGET, &flags);
    flags &= ~TIOCM_DTR;
    flags &= ~TIOCM_RTS;
    ioctl(fd_ser, TIOCMSET, &flags);
    
	return 0;
}

void direct_ptt(int onoff)
{
    // set/reset RTS/DTR
    int flags;
    ioctl(fd_ser, TIOCMGET, &flags);
    if(onoff)
    {
        flags |= TIOCM_DTR;
        flags |= TIOCM_RTS;
    }
    else
    {
        flags &= ~TIOCM_DTR;
        flags &= ~TIOCM_RTS;
    }
    ioctl(fd_ser, TIOCMSET, &flags);
}

void openSerialInterface(int baudrate)
{
int ret = -1;
static int ttynum = 0;
int lasttxmode = txmode;
    
    while(ret == -1)
    {
        if(!running || txmode != lasttxmode) return;
        ret = activate_serial(device,baudrate);
        if(ret == 0) 
        {
            deb_printf("SERIAL","%s is now OPEN",device);
            sleep(1);
            break;
        }
        fd_ser = -1;
        sleep(1);
        if(++ttynum >= 4) ttynum = 0;
        sprintf(device,"/dev/ttyUSB%d",ttynum);
    }
}

// read one byte non blocking
int read_port()
{
static unsigned char c;

    int rxlen = read(fd_ser, &c, 1);
    
    if(rxlen == 0)
    {
        return -1;
    }

	return (unsigned int)c;
}

// schreibe ein
void write_port(unsigned char *data, int len)
{
    write(fd_ser, data, len);
}

// execute a CAT command
void do_command()
{
    if(txmode == 0)
    {
        // no action
    }
    
    if(txmode == 1)
    {
        switch (ser_command)
        {
            case 1: civ_queryQRG();
                    break;
            case 2: civ_ptt(1, civ_adr);
                    fsave_status(PTT,1);
                    break;
            case 3: civ_ptt(0, civ_adr);
                    fsave_status(PTT,0);
                    break;
            case 4: civ_setQRG(trx_frequency);
                    break;
        }
    }

    if(txmode == 2)
    {
        switch (ser_command)
        {
            case 2: // DDS starts by itself
                    fsave_status(PTT,1);
                    break;
            case 3: fsave_status(PTT,0);
                    break;
            case 4: dds_setQRG(trx_frequency);
                    break;
        }
    }

    if(txmode == 3)
    {
        switch (ser_command)
        {
            case 2: // PTT on
                    direct_ptt(1);
                    fsave_status(PTT,1);
                    break;
            case 3: // PTT off
                    direct_ptt(0);
                    fsave_status(PTT,0);
                    break;
        }
    }
    
    if(txmode == 4)
    {
        switch (ser_command)
        {
            case 2: hamlib_ptt(1);
                    fsave_status(PTT,1);
                    break;
            case 3: hamlib_ptt(0);
                    fsave_status(PTT,0);
                    break;
            case 4: hamlib_setQRG(trx_frequency);
                    break;
        }
    }

    ser_command = 0;
}
