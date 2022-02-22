#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <linux/types.h> 
#include <linux/input.h>
#include <sys/time.h>
#include <unistd.h>

#define bool int
enum BOOL {FALSE = 0, TRUE = !FALSE};

typedef struct tagHIDTable{
	int	nHID;
	int	nVK;
	int	extended;
	int	nScan;
} SHIDTable;

#define	KEYEVENTF_EXTENDEDKEY	0x00

SHIDTable g_HIDTable[] = {
#if 0
{ 0x00, 0x00 0x00, 0x00 }, { 0x01, 0x00 0x00, 0x00 }, { 0x03, 0x0 0x00, 0x00 },
{ 0x04, KEY_A }, { 0x05, KEY_B }, { 0x06, KEY_C }, { 0x07, KEY_D },
{ 0x08, KEY_E }, { 0x09, KEY_F }, { 0x0A, KEY_G }, { 0x0B, KEY_H }, 
{ 0x0C, KEY_I }, { 0x0D, KEY_J }, { 0x0E, KEY_K }, { 0x0F, KEY_L },
{ 0x10, KEY_M }, { 0x11, KEY_N }, { 0x12, KEY_O }, { 0x13, KEY_P },
{ 0x14, KEY_Q }, { 0x15, KEY_R }, { 0x16, KEY_S }, { 0x17, KEY_T },
{ 0x18, KEY_U }, { 0x19, KEY_V }, { 0x1A, KEY_W }, { 0x1B, KEY_X },
{ 0x1C, KEY_Y }, { 0x1D, KEY_Z }, { 0x1E, KEY_1 }, { 0x1F, KEY_2 },
{ 0x20, KEY_3 }, { 0x21, KEY_4 }, { 0x22, KEY_5 }, { 0x23, KEY_6 },
{ 0x24, KEY_7 }, { 0x25, KEY_8 }, { 0x26, KEY_9 }, { 0x27, KEY_0 },
{ 0x28, KEY_ENTER}, { 0x29, KEY_ESC }, { 0x2A, KEY_BACKSPACE }, { 0x2B, KEY_TAB },
{ 0x2C, KEY_SPACE }, { 0x2D, KEY_MINUS /*-_*/},{0x2E, KEY_EQUAL /*=+*/}, 
{ 0x2F, KEY_LEFTBRACE /*[{*/}, { 0x30, KEY_RIGHTBRACE /*]}*/ }, { 0x31, KEY_BACKSLASH /*\|*/ }, { 0x32, 0 /*Europe1*/ },
{ 0x33, KEY_SEMICOLON /*;:*/ }, { 0x34, KEY_APOSTROPHE /*'*/ }, { 0x35, KEY_GRAVE /*`~*/ }, { 0x36, KEY_COMMA /*,<*/ },
{ 0x37, KEY_DOT /*.>*/ }, { 0x38, KEY_SLASH /*/?*/ }, { 0x39, KEY_CAPSLOCK /*Caps Lock*/ },
{ 0x3A, KEY_F1 /*F1*/ }, { 0x3B, KEY_F2 /*F2*/ }, { 0x3C, KEY_F3 /*F3*/ }, { 0x3D, KEY_F4 },
{ 0x3E, KEY_F5 /*F5*/ }, { 0x3F, KEY_F6 /*F6*/ }, { 0x40, KEY_F7 /*F7*/ }, { 0x41, KEY_F8 }, 
{ 0x42, KEY_F9 /*F9*/ }, { 0x43, KEY_F10 /*F10*/ }, { 0x44, KEY_F11 /*F11*/ }, { 0x45, KEY_F12 /*F12*/ },
{ 0x46, KEY_PRINT /*Print Screen, SysRq*/ }, { 0x47, KEY_SCROLLLOCK /*Scroll Lock*/ }, { 0x48, KEY_PAUSE /*Pause*/ },
{ 0x49, KEY_INSERT /*Insert*/ }, { 0x4A, KEY_HOME /*Home*/ }, { 0x4B, KEY_PAGEUP /*Page Up*/ }, 
{ 0x4C, KEY_DELETE /*Delete*/}, { 0x4D, KEY_END /*End*/ }, { 0x4E, KEY_NEXT /*Page Down*/ }, 
{ 0x4F, KEY_RIGHT /*Right Arrow*/ }, { 0x50, KEY_LEFT /*Left Arrow*/ }, { 0x51, KEY_DOWN /*Down Arrow*/ },
{ 0x52, KEY_UP /*Up Arrow*/ }, { 0x53, KEY_NUMLOCK /*Num Lock*/ }, { 0x54, KEY_KEYBOARD /*Keypad /*/ }, 
{ 0x55, KEY_KPASTERISK /*Keypad * */ }, { 0x56, KEY_KPMINUS /*Keypad - */ }, { 0x57, KEY_KPPLUS /*Keypad +*/ },
{ 0x58, KEY_KPENTER /*Keypad enter*/ }, { 0x59, KEY_KP1 /*Keypad 1 End*/ }, { 0x5A, KEY_KP2 /*Keypad 2 Down*/ },
{ 0x5B, KEY_KP3 /*Keypad 3 PageDn*/ }, { 0x5C, KEY_KP4 /*Keypad 4 Left*/}, { 0x5D, KEY_KP5 /*Keypad 5*/ },
{ 0x5E, KEY_KP6 /*Keypad 6 Right*/ }, { 0x5F, KEY_KP7 /*Keypad 7 Home*/ }, { 0x60, KEY_KP8 /*Keypad 8*/ },
{ 0x61, KEY_KP9 /*Keypad 9 PageUp*/ }, { 0x62, KEY_KP0 /*Keypad 0 Insert*/ }, { 0x63, KEY_KPSLASH /*Keypad . Delete*/ }
};
#endif
{ 0x00, 0x00, 0x00, 0x00 }, { 0x01, 0x00, 0x00, 0x00 }, { 0x03, 0x00, 0x00, 0x00 },
{ 0x04,	KEY_A, 0x00, 0x1e }, { 0x05, KEY_B, 0x00, 0x30 }, { 0x06, KEY_C, 0x00, 0x2e }, { 0x07, KEY_D, 0x00, 0x20 },
{ 0x08, KEY_E, 0x00, 0x12 }, { 0x09, KEY_F, 0x00, 0x21 }, { 0x0A, KEY_G, 0x00, 0x22 }, { 0x0B, KEY_H, 0x00, 0x23 }, 
{ 0x0C, KEY_I, 0x00, 0x17 }, { 0x0D, KEY_J, 0x00, 0x24 }, { 0x0E, KEY_K, 0x00, 0x25 }, { 0x0F, KEY_L, 0x00, 0x26 },
{ 0x10, KEY_M, 0x00, 0x32 }, { 0x11, KEY_N, 0x00, 0x31 }, { 0x12, KEY_O, 0x00, 0x18 }, { 0x13, KEY_P, 0x00, 0x19 },
{ 0x14, KEY_Q, 0x00, 0x10 }, { 0x15, KEY_R, 0x00, 0x13 }, { 0x16, KEY_S, 0x00, 0x1f }, { 0x17, KEY_T, 0x00, 0x14 },
{ 0x18, KEY_U, 0x00, 0x16 }, { 0x19, KEY_V, 0x00, 0x2f }, { 0x1A, KEY_W, 0x00, 0x11 }, { 0x1B, KEY_X, 0x00, 0x2d },
{ 0x1C, KEY_Y, 0x00, 0x15 }, { 0x1D, KEY_Z, 0x00, 0x2c }, { 0x1E, KEY_1, 0x00, 0x02 }, { 0x1F, KEY_2, 0x00, 0x03 },
{ 0x20, KEY_3, 0x00, 0x04 }, { 0x21, KEY_4, 0x00, 0x05 }, { 0x22, KEY_5, 0x00, 0x06 }, { 0x23, KEY_6, 0x00, 0x07 },
{ 0x24, KEY_7, 0x00, 0x08 }, { 0x25, KEY_8, 0x00, 0x09 }, { 0x26, KEY_9, 0x00, 0x0a }, { 0x27, KEY_0, 0x00, 0x0b },
{ 0x28, KEY_ENTER, 0x00, 0x1c}, { 0x29, KEY_ESC, 0x00, 0x01}, { 0x2A, KEY_BACKSPACE, 0x00, 0x0e }, { 0x2B, KEY_TAB, 0x00, 0x0f	 },
{ 0x2C, KEY_SPACE, 0x00, 0x39 }, { 0x2D, KEY_MINUS, 0x00, 0x0c },{0x2E, KEY_EQUAL, 0x00, 0x0d }, 
{ 0x2F, KEY_LEFTBRACE, 0x00, 0x1a /*[{*/}, { 0x30, KEY_RIGHTBRACE, 0x00, 0x1b /*]}*/ }, { 0x31, KEY_BACKSLASH, 0x00, 0x2b/*\|*/ }, { 0x32, 0, 0x00, 0x00 /*Europe1*/ },
{ 0x33, KEY_SEMICOLON, 0x00, 0x27 /*;:*/ }, { 0x34, KEY_APOSTROPHE, 0x00, 0x28 /*'*/ }, { 0x35, KEY_GRAVE, 0x00, 0x29 /*`~*/ }, { 0x36, KEY_COMMA, 0x00, 0x33 /*,<*/ },
{ 0x37, KEY_DOT, 0x00, 0x34 /*.>*/ }, { 0x38, KEY_SLASH, 0x00, 0x35 /*/?*/ }, { 0x39, KEY_CAPSLOCK, 0x00, 0x3A /*Caps Lock*/ },
{ 0x3A, KEY_F1, 0x00, 0x3B /*F1*/ }, { 0x3B, KEY_F2, 0x00, 0x3c /*F2*/ }, { 0x3C, KEY_F3, 0x00, 0x3d /*F3*/ }, { 0x3D, KEY_F4, 0x00, 0x3e },
{ 0x3E, KEY_F5, 0x00, 0x3f /*F5*/ }, { 0x3F, KEY_F6, 0x00, 0x40 /*F6*/ }, { 0x40, KEY_F7, 0x00, 0x41 /*F7*/ }, { 0x41, KEY_F8, 0x00, 0x42 }, 
{ 0x42, KEY_F9, 0x00, 0x43 /*F9*/ }, { 0x43, KEY_F10, 0x00, 0x44 /*F10*/ }, { 0x44, KEY_F11, 0x00, 0x57 /*F11*/ }, { 0x45, KEY_F12, 0x00, 0x58 /*F12*/ },
{ 0x46, KEY_PRINT, 0x00, 0x00 /*Print Screen*/ }, { 0x47, KEY_SCROLLLOCK, 0x00, 0x46 /*Scroll Lock*/ }, { 0x48, KEY_PAUSE, 0x00, 0x00 /*Pause*/ },
{ 0x49, KEY_INSERT, KEYEVENTF_EXTENDEDKEY, 0x52 /*Insert*/ }, { 0x4A, KEY_HOME, KEYEVENTF_EXTENDEDKEY, 0x47 /*Home*/ }, 
{ 0x4B, KEY_PAGEUP, KEYEVENTF_EXTENDEDKEY, 0x49/*Page Up*/ }, { 0x4C, KEY_DELETE, KEYEVENTF_EXTENDEDKEY, 0x53 /*Delete*/},
{ 0x4D, KEY_END, KEYEVENTF_EXTENDEDKEY, 0x4f /*End*/ }, { 0x4E, KEY_NEXT, KEYEVENTF_EXTENDEDKEY, 0x51 /*Page Down*/ }, 
{ 0x4F, KEY_RIGHT, KEYEVENTF_EXTENDEDKEY, 0x4d /*Right Arrow*/ }, { 0x50, KEY_LEFT, KEYEVENTF_EXTENDEDKEY, 0x4b /*Left Arrow*/ },
{ 0x51, KEY_DOWN, KEYEVENTF_EXTENDEDKEY,0x50 /*Down Arrow*/ },{ 0x52, KEY_UP, KEYEVENTF_EXTENDEDKEY, 0x48 /*Up Arrow*/ }, 
{ 0x53, KEY_NUMLOCK, 0x00, 0x45 /*Num Lock*/ }, { 0x54, KEY_KEYBOARD, KEYEVENTF_EXTENDEDKEY, 0x35 /*Keypad /*/ }, 
{ 0x55, KEY_KPASTERISK, 0x00, 0x37 /*Keypad * */ }, { 0x56, KEY_KPMINUS, 0x00, 0x4a /*Keypad - */ }, { 0x57, KEY_KPPLUS, 0x00, 0x4e /*Keypad +*/ },
{ 0x58, KEY_KPENTER, KEYEVENTF_EXTENDEDKEY, 0x1c /*Keypad enter*/ }, { 0x59, KEY_KP1, 0x00, 0x4f /*Keypad 1 End*/ },
{ 0x5A, KEY_KP2, 0x00, 0x50 /*Keypad 2 Down*/ },{ 0x5B, KEY_KP3, 0x00, 0x51 /*Keypad 3 PageDn*/ },
{ 0x5C, KEY_KP4, 0x00, 0x4b /*Keypad 4 Left*/}, { 0x5D, KEY_KP5, 0x00, 0x4c /*Keypad 5*/ },
{ 0x5E, KEY_KP6, 0x00, 0x4d /*Keypad 6 Right*/ }, { 0x5F, KEY_KP7, 0x00, 0x47 /*Keypad 7 Home*/ }, { 0x60, KEY_KP8, 0x00, 0x48 /*Keypad 8*/ },
{ 0x61, KEY_KP9, 0x00, 0x49 /*Keypad 9 PageUp*/ }, { 0x62, KEY_KP0, 0x00, 0x52 /*Keypad 0 Insert*/ }, { 0x63, KEY_KPSLASH, 0x00, 0x53 /*Keypad . Delete*/ }
};

typedef struct tagIRMap{
	unsigned char IRRaw3;
	unsigned char IRRaw2;
	unsigned char IRRaw1;
	unsigned char IRRaw0;
	int nHID;
	int nControl;
	unsigned char nMask;
} SIRKeyMap;

typedef struct tagIRTable{
	SIRKeyMap IRKey[50];
	unsigned short nToggleMask;
	int nKeyNum;
	int nFNExpire;
	int nRepeat;
} SIRTable;

#define	HK_LCTRL	0x01
#define	HK_LSHIFT	0x02
#define HK_LALT		0x04
#define HK_LWIN		0x08
#define	HK_RCTRL	0x10
#define HK_RSHIFT	0x20
#define HK_RALT		0x40
#define HK_RWIN		0x80

#define IRFILESIZE	356

void simulate_key(
	int fd,
	int key,
	int value)
{
	struct input_event event;
	
	event.type = EV_KEY;
	event.code = key;//event.value = value;//(1:down, 0:up)
	gettimeofday(&event.time, 0);

	if(write(fd,&event,sizeof(event)) < 0)
		printf("simulate key error~~~\n");
}

long doGetFileSize(
	const char* filePath,
	const char* mode)
{
	long fileLength = 0;
	FILE *fp;
	
	if(!(fp = fopen(filePath, mode))){
		//File Open Error
		return -1;
	}
	
	//move pointer to file end
	if(fseek(fp, 0, SEEK_END)){
		//File seek error.
		return -1;
	}
	
	//get file size unit(byte)
	fileLength = ftell(fp);
	//move pointer to file begin
	rewind(fp);
	
	return fileLength;
}

int LoadIRTable(
	SIRTable* IRMap)
{
	long fileLength;
	int KeyNum = 50, bOld = 0, dwRead = 0, i, locate = 0, err = 0;
	char *fileBuf;
	FILE *fp;
	
	IRMap->nKeyNum = 0;
	
	fileLength = doGetFileSize("/lib/firmware/it913xirtbl.bin", "r+");
	
	if(fileLength < IRFILESIZE) {
		printf("LoadIRTable: File Size is small than 356 : %ld\n", fileLength);
		bOld = 1;
	}
	
	fileBuf = (char *)malloc(fileLength); 
	
	if(!(fp = fopen("/lib/firmware/it913xirtbl.bin", "r+"))){
		//File Open Error
		return -1;
	}
	
	dwRead = fread(fileBuf, 1, fileLength, fp);

	if(dwRead > fileLength){
		printf("LoadIRTable: File Size readed is over!\n");
		err = -1;
		goto exit;
	}

	if(bOld){
		KeyNum = dwRead / 7;
	}
	
	for(i = 0; i < KeyNum; i++){
		IRMap->IRKey[i].IRRaw0 = fileBuf[7*i];
		IRMap->IRKey[i].IRRaw1 = fileBuf[7*i+1];
		IRMap->IRKey[i].IRRaw2 = fileBuf[7*i+2];
		IRMap->IRKey[i].IRRaw3 = fileBuf[7*i+3];
		IRMap->IRKey[i].nHID = fileBuf[7*i+4];
		IRMap->IRKey[i].nControl = fileBuf[7*i+5];
		IRMap->IRKey[i].nMask = fileBuf[7*i+6];
	
		/*printf ("fileBuf[%d] : %02X %02X %02X %02X %02X %02X %02X\n", 
		i , fileBuf[7*i], fileBuf[7*i+1], fileBuf[7*i+2], fileBuf[7*i+3],fileBuf[7*i+4],
		fileBuf[7*i+5], fileBuf[7*i+6]);
	
		printf ("IRMap[%d] : %02X %02X %02X %02X %02X %02X %02X\n\n", 
		i , IRMap->IRKey[i].IRRaw0, IRMap->IRKey[i].IRRaw1, IRMap->IRKey[i].IRRaw2, 
		IRMap->IRKey[i].IRRaw3,IRMap->IRKey[i].nHID,
		IRMap->IRKey[i].nControl, IRMap->IRKey[i].nMask);*/
	}

	if(bOld){
		IRMap->nToggleMask = 0x7FFF;
		IRMap->nKeyNum = KeyNum;
		IRMap->nFNExpire = 62;
		IRMap->nRepeat = 25;
	}
	else{
		locate = 7*KeyNum;
		IRMap->nToggleMask = (unsigned short)(fileBuf[locate]<<8)+fileBuf[locate+1];
		IRMap->nKeyNum = fileBuf[locate+2];
		IRMap->nFNExpire = fileBuf[locate+3];	//Timer in IRMonitor is 100ms
		IRMap->nRepeat = fileBuf[locate+4];		//Timer in IRMonitor is 100ms
	}

	/*printf ("IRMap : %04X, %02X, %02X, %02X\n", IRMap->nToggleMask, IRMap->nKeyNum,
		IRMap->nFNExpire, IRMap->nRepeat);

	printf("IRMap.nKeyNum : %lx\n",IRMap->nKeyNum);*/
	fclose(fp);

exit:
	free(fileBuf);
	return 0;
}

int m_bFunctionKey;
unsigned char m_cFunctionCnt;
unsigned char g_StartUtime = 0;

int GetHIDTable(
	SIRTable* IRMap, 
	unsigned long RawIRvalue,
	unsigned char *irData)
{
	int i;
	unsigned long mapData = 0, end_utime = 0, TickInterval;
	struct timeval tv;
	
	gettimeofday(&tv,NULL);
	end_utime = tv.tv_sec * 1000000 + tv.tv_usec;

	TickInterval = ((end_utime - g_StartUtime)/1000)/20;
	//Check if Function Key expired
	if(m_bFunctionKey){
		m_cFunctionCnt += (unsigned char)TickInterval;
		
		if(m_cFunctionCnt > (IRMap->nFNExpire)){
			m_bFunctionKey = FALSE;
			m_cFunctionCnt = 0;
		}
	}
	
	for(i = 0; i < IRMap->nKeyNum; i++){
		memcpy(&mapData, &(IRMap->IRKey[i]), 4);
		/*printf("for %d ********************\n", i);
		printf("RawIRvalue : %lX\n",RawIRvalue);
		printf("mapData : %lX\n",mapData);
		printf ("IRMap[%d] : %02X%02X%02X%02X\n", 
		i , IRMap->IRKey[i].IRRaw0, IRMap->IRKey[i].IRRaw1, IRMap->IRKey[i].IRRaw2, 
		IRMap->IRKey[i].IRRaw3);*/

		if(RawIRvalue == mapData){
			//It is Function Key 
			if(IRMap->IRKey[i].nMask & 0x80){
				m_bFunctionKey = 1;
				m_cFunctionCnt = 0;
			}
			else{
				if(m_bFunctionKey && (IRMap->IRKey[i].nMask & 0x40)){
					//Has Alternative Key
					irData[0] = IRMap->IRKey[i+1].nControl;
					irData[2] = IRMap->IRKey[i+1].nHID;
				}
				else{//No Alternative key or not press function key before
					irData[0] = IRMap->IRKey[i].nControl;
					irData[2] = IRMap->IRKey[i].nHID;
				}
				
				m_cFunctionCnt = 0;
				return 1;
			}
		}
	}
	return 0;
}

int main(int argc, char *argv[]){
	char irData[8] = {0};
	unsigned long RawIRvalue = 0;
	int fd, fd_kbd, ret; 
	bool bUseVK = FALSE;
	int nVK = 0, nEx = 0, nScan = 0, num = 0, idx;
	SIRTable IRMap;
	struct timeval tv;
		
	fd_kbd = open("/dev/input/event1", O_RDWR);
	if(fd_kbd < 0){
		//printf("error open keyboard\n");
		return -1;
	}

	fd = open("/dev/usb-it913x0", O_RDWR);
	if(fd < 0){
		//printf(" device open failed!!\n");
		return -1;
	}
	
	ret = LoadIRTable(&IRMap);
	if(ret < 0){
		//printf("LoadIRTable failed!!\n");
		return -1;
	}

	while(1){	
		//fd= open("/dev/usb-it913x0", O_RDWR);
		//if (fd < 0) {/*printf("af901x device open failed!!\n");*/continue;}

		ret = read(fd, &RawIRvalue, 4);
		if(!ret)
			goto exit;
		//printf("main_RawIRvalue : %lx\n",RawIRvalue);
		
		//if(irData)
		//printf ("irData : %X %X %X %X\n", irData[0], irData[1], irData[2], irData[3]);
		//printf("\nap: irData[0] = 0x%2x, irData[2] = 0x%2x", irData[0], irData[2]);

		gettimeofday(&tv, NULL);
		g_StartUtime = tv.tv_sec * 1000000 + tv.tv_usec;
		
		ret = GetHIDTable(&IRMap, RawIRvalue, irData);
		if(!ret) 
			goto exit;
		if(irData)
		//printf ("main_irData : %X %X %X %X\n", irData[0], irData[1], irData[2], irData[3]);
//#if 0
		if((irData[2] <= 0x63) && (irData[2] > 0x3)){
			idx = irData[2] - 1;
			nVK = g_HIDTable[idx].nVK;
			nEx = g_HIDTable[idx].extended;
			nScan = g_HIDTable[idx].nScan;
			bUseVK = TRUE;

			if((int)irData[0] & HK_LCTRL)
				simulate_key(fd_kbd, KEY_LEFTCTRL, 1);

			if((int)irData[0] & HK_LSHIFT)
				simulate_key(fd_kbd, KEY_LEFTSHIFT, 1);

			if((int)irData[0] & HK_LALT)
				simulate_key(fd_kbd, KEY_LEFTALT, 1);
		
			if((int)irData[0] & HK_RCTRL )
				simulate_key(fd_kbd, KEY_RIGHTCTRL, 1);

			if((int)irData[0] & HK_RSHIFT)
				simulate_key(fd_kbd, KEY_RIGHTSHIFT, 1);

			if((int)irData[0] & HK_RALT)
				simulate_key(fd_kbd, KEY_RIGHTALT, 1);

			if(nScan==0){
				//printf(", nVK= %d\n", nVK);
			}
			else{
				simulate_key(fd_kbd, nVK, 1);
				//printf(", nVK= %d\n", nVK);
			}		
		}
			
		if( bUseVK == TRUE )
		{
			usleep(100000);
			simulate_key(fd_kbd, nVK, 0);

			if((int)irData[0] & HK_LCTRL)
				simulate_key(fd_kbd, KEY_LEFTCTRL, 0);

			if((int)irData[0] & HK_LSHIFT)
				simulate_key(fd_kbd, KEY_LEFTSHIFT, 0);

			if((int)irData[0] & HK_LALT)
				simulate_key(fd_kbd, KEY_LEFTALT, 0);
		
			if((int)irData[0] & HK_RCTRL)
				simulate_key(fd_kbd, KEY_RIGHTCTRL, 0);

			if((int)irData[0] & HK_RSHIFT)
				simulate_key(fd_kbd, KEY_RIGHTSHIFT, 0);

			if((int)irData[0] & HK_RALT)
				simulate_key(fd_kbd, KEY_RIGHTALT, 0);
		} 
//#endif
//		usleep(100000);
exit:
//		close(fd);
		usleep(100000);
	}

	close(fd);
	close(fd_kbd);
	return 0;
}
