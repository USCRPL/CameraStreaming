#define _GNU_SOURCE

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/time.h> 
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <ctype.h>
#include <errno.h>
#include <termios.h>
#include <linux/dvb/frontend.h>
#include <linux/dvb/dmx.h>
#include <linux/version.h> // fix for kernel 3.7 ver.

#include "iocontrol.h"
#include "error.h"

#define READ_DATA_SIZE (188 * 348)
#define PID_SIZE 0x2001 //13bit + 1, ref spec
#define SYNC_BYTE 0x47

//register address
#define trigger_ofsm           0x0000

// EEPROM particular items address
#define EEPROM_TSMODE_ADDRESS  0x31
#define EEPROM_TUNERID_ADDRESS 0x3C
#define EEPROM_BOARDID_ADDRESS 0x3F

#define EEPROM_RW_SIZE 8

// Define printf color
#define RED_BOLD "\x1b[;31;1m"
#define BLU_BOLD "\x1b[;34;1m"
#define YEL_BOLD "\x1b[;33;1m"
#define GRN_BOLD "\x1b[;32;1m"
#define CYAN_BOLD_ITALIC "\x1b[;36;1;3m"
#define RESET "\x1b[0;m"


//
// The type defination of Signal Source.
//
typedef enum {
    SignalSource_0_9G = 0,			// Signal source 900MHz.
    SignalSource_2_4G 				// Signal source 2.4G.
} DTVSignalSource;

//
// The type defination of TS_mode.
//
typedef enum {
	Mode_TS = 0,            // TS mode.
	Mode_DCA = 2,           // DCA mode.
	Mode_PIP                // PIP mode.
} DTVMode;


typedef struct {
	uint32_t count;
	uint32_t ulErrCount;
	uint32_t ulLostCount;
	uint16_t pid;
	uint8_t  sequence1;
	uint8_t  sequence2;
	uint8_t  dup_flag;
} PIDINFO, *PPIDINFO;
