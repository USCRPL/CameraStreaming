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
#include <linux/dvb/frontend.h>
#include <linux/dvb/dmx.h>
#include <linux/version.h> // fix for kernel 3.7 ver.
#include <pthread.h>

#include "iocontrol.h"

#define READBUFFER_SIZE 188
#define PID_SIZE 0x2001 //13bit + 1, ref spec
#define SYNC_BYTE 0x47

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t init_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

/* Thread control */
int dev_cond = 0;
int menu_chose;
char *menu_list = "";

typedef struct {
	int DevCount;
	int DevId;
	Dword frequency;
	Dword bandwidth;
	Dword m_sFrequency;
	Dword m_eFrequency;
	Dword m_Bandwidth;
	int rw_Flag;
	int rw_processor;
	uint32_t rw_regaddr;
	uint32_t w_value;
} DevInfo_t, *PDevInfo_t;

typedef enum ThreadMode {
	THREAD_SYNC_MODE= 0,
	THREAD_MENU_MODE
} ThreadCtrlMode;

typedef struct {
	uint32_t count;
	uint32_t ulErrCount;
	uint32_t ulLostCount;
	uint16_t pid;
	uint8_t  sequence1;
	uint8_t  sequence2;
	uint8_t  dup_flag;
} PIDINFO, *PPIDINFO;
