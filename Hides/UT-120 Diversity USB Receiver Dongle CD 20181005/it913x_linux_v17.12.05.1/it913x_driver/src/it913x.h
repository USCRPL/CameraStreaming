#ifndef _IT913x_H_
#define _IT913x_H_

#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/kref.h>
#include <linux/usb.h>
#include <linux/version.h>
#include <linux/mutex.h>
#include <linux/stddef.h>
#include <asm/uaccess.h>
#include "iocontrol.h"
#include "ADRF6655.h"
#include "RFFC2072.h"
#include "demodulator.h"
#include "userdef.h"
#include "type.h"
#include "common.h"
#include "firmware.h"
#include "firmware_V2.h"
#include "firmware_V2I.h"
#include "firmware_V2W.h"
#include "dvb-usb.h"
#include "dvb_math.h"

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 39)
	#include <linux/smp_lock.h>
#endif

#ifdef CONFIG_COMPAT
#include <linux/compat.h>
#endif

/* Get a minor range for your devices from the usb maintainer */
#ifdef CONFIG_USB_DYNAMIC_MINORS
	#define USB_ITE_MINOR_BASE	0
#else
	#define USB_ITE_MINOR_BASE	191
#endif

#define DRIVER_RELEASE_VERSION	"v17.12.05.1"

/***************** customization ****************/
//#define DVB_USB_ADAP_NEED_PID_FILTER
#define IT913X_RC_INTERVAL 500
#define HOT_PLUGGING 0

#define DEBUG 1
#if DEBUG 
	#define deb_data(args...)	printk(KERN_NOTICE args)
#else
	#define deb_data(args...)
#endif

/***************** from device.h *****************/
#define USB_VID_ITETECH					0x048d
#define USB_PID_ITETECH_IT913X			0x9135
#define USB_PID_ITETECH_IT913X_9005	0x9005
#define USB_PID_ITETECH_IT913X_9006	0x9006

/***************** from afdrv.h *****************/
#define EEPROM_FLB_OFS 8

#define EEPROM_IRMODE			(OVA_EEPROM_CFG+EEPROM_FLB_OFS+0x10)	//00:disabled, 01:HID
#define EEPROM_DCA_TIME_FLAG	(OVA_EEPROM_CFG+EEPROM_FLB_OFS+0x20+4)  //DCA get FPGA time flag (0x2C)
#define EEPROM_DCA_MA_TIME		(OVA_EEPROM_CFG+EEPROM_FLB_OFS+0x20+5)  //DCA MA time (0x2D) 
#define EEPROM_DCA_SLA_TIME		(OVA_EEPROM_CFG+EEPROM_FLB_OFS+0x20+6)  //DCA SLA time (0x2E)
#define EEPROM_SELSUSPEND		(OVA_EEPROM_CFG+EEPROM_FLB_OFS+0x28)	//Selective Suspend Mode
#define EEPROM_TSMODE			(OVA_EEPROM_CFG+EEPROM_FLB_OFS+0x28+1)	//0:one ts, 1:dual ts
#define EEPROM_2WIREADDR		(OVA_EEPROM_CFG+EEPROM_FLB_OFS+0x28+2)	//MPEG2 2WireAddr
#define EEPROM_SUSPEND			(OVA_EEPROM_CFG+EEPROM_FLB_OFS+0x28+3)	//Suspend Mode
#define EEPROM_IRTYPE			(OVA_EEPROM_CFG+EEPROM_FLB_OFS+0x28+4)	//0:NEC, 1:RC6
#define EEPROM_SAWBW1			(OVA_EEPROM_CFG+EEPROM_FLB_OFS+0x28+5)
#define EEPROM_XTAL1			(OVA_EEPROM_CFG+EEPROM_FLB_OFS+0x28+6)	//0:28800, 1:20480
#define EEPROM_SPECINV1 		(OVA_EEPROM_CFG+EEPROM_FLB_OFS+0x28+7)
#define EEPROM_IFFREQL			(OVA_EEPROM_CFG+EEPROM_FLB_OFS+0x30)
#define EEPROM_IFFREQH			(OVA_EEPROM_CFG+EEPROM_FLB_OFS+0x30+1)
#define EEPROM_IF1L	        	(OVA_EEPROM_CFG+EEPROM_FLB_OFS+0x30+2)
#define EEPROM_IF1H	        	(OVA_EEPROM_CFG+EEPROM_FLB_OFS+0x30+3)
#define EEPROM_TUNERID			(OVA_EEPROM_CFG+EEPROM_FLB_OFS+0x30+4)
#define EEPROM_BOARDID			(OVA_EEPROM_CFG+EEPROM_FLB_OFS+0x30+7)  //distinguish using 6655 from using 2072   (0x4994 + 0x3F)
#define EEPROM_SHIFT			(0x10)	//EEPROM Addr Shift for slave front end

/***************** from device.h *****************/


/*************************************************
 * EEPROM_DownConverter_TYPE:
 *   is for recognizing different DownConverter from EEPROM
 *
 *   DownConverter_ADRF6655 = 0x31(h) = 49(d)
 *   DownConverter_RFFC2072 = 0x32(h) = 50(d)
 *   DownConverter_RFFC2071 = 0x33(h) = 51(d)
 ************************************************/
typedef enum{
	DownConverter_ADRF6655 = 49,
	DownConverter_RFFC2072,
	DownConverter_RFFC2071,
}EEPROM_DownConverter_TYPE;

struct Remote_config {
	u8  dual_mode:1;
	u16 mt2060_if1[2];
	u16 firmware_size;
	u16 firmware_checksum;
	u32 eeprom_sum;
};

enum af9015_remote {
	IT913X_REMOTE_NONE                    = 0,
	IT913X_REMOTE_A_LINK_DTU_M,
	IT913X_REMOTE_MSI_DIGIVOX_MINI_II_V3,
	IT913X_REMOTE_MYGICTV_U718,
	IT913X_REMOTE_DIGITTRADE_DVB_T,
	IT913X_REMOTE_AVERMEDIA_KS,
};

struct it913x_state {
	struct i2c_adapter i2c_adap; /* I2C adapter for 2nd FE */
	u8 rc_repeat;
	u32 rc_keycode;
	u8 rc_last[4];
};

typedef struct _TUNER_INFO {
	Bool bTunerInited;
	BYTE TunerId;
	Bool bTunerOK;
	Bool bTunerLock;
	Tuner_struct MXL5005_Info;
} TUNER_INFO, *PTUNER_INFO;

typedef struct _FILTER_CONTEXT_HW {
	DWORD ulCurrentFrequency;
	WORD ucCurrentBandWidth;
	DWORD ulDesiredFrequency;
	WORD ucDesiredBandWidth;
	Bool bTimerOn;
	TUNER_INFO tunerinfo;
} FILTER_CONTEXT_HW, *PFILTER_CONTEXT_HW;

typedef struct _DCA_FPGA_LATCH_INFO {
	Byte enable_flag;
	Byte Master_time;
	Byte Slave_time;
} DCA_FPGA_LATCH_INFO;

typedef struct _DEVICE_CONTEXT {
	FILTER_CONTEXT_HW fc[2];
	struct usb_device *usb_dev;
	Bool bDualTs;
	Bool bIrTblDownload;
	Bool bProprietaryIr;
	Byte bIrType; /* EE chose NEC RC5 RC6 table */
	u16 UsbMode;
	Architecture architecture;
	StreamType StreamType;
	Bool bDCAPIP;
	atomic_t FilterCnt;
	IT9130 Demodulator;
	RFFC2072INFO RFFC2072;
	Byte chip_version; /* Patch for Om1 Om2 EE */
	Byte minor[2];
	Byte *Temp_readbuffer;
	Byte *Temp_writebuffer;
	/* Patch for suspend/resume */
	Bool power_state[2];

	/* Patch for supporting hot plugging */
	Bool disconnect;
	Bool AP_off;
	wait_queue_head_t disconnect_wait;

	struct mutex dev_mutex;
	DCA_FPGA_LATCH_INFO DCA_time_info;
	/*********************board_id:***************
	 * distinguish using DownConverter ADRF6655, 
	 * DownConverter RFFC2072, or ordinary omega
	 *********************************************/
	Dword board_id;
} Device_Context;

struct tuner_priv {
	struct tuner_config *cfg;
	struct i2c_adapter *i2c;
	u32 frequency;
	u32 bandwidth;
	u16 if1_freq;
	u8 fmfreq;
};

struct RC_Setup {
	unsigned int id;
	char *rc_codes;
};

/**********************************************/

extern int it913x_device_count;
extern struct usb_device_id IT913x_usb_id_table[];
extern struct dvb_usb_device_properties DVB_properties[];
extern struct dvb_frontend *DVB_frontend(struct dvb_usb_adapter *adap);

extern DWORD DL_ReadRawIR(Device_Context *DC, DWORD *ucpReadBuf);
extern DWORD DL_ApCtrl(Device_Context *DC, BYTE chip_index, Bool bOn);
extern DWORD DL_SetArchitecture(Device_Context *DC, Architecture architecture);
extern DWORD DL_DemodIOCTLFun(Device_Context *DC, DWORD IOCTLCode, unsigned long pIOBuffer, Byte chip_index);
extern DWORD Device_init(struct usb_device *udev, Device_Context *DC, Bool bBoot);
extern void RemoteConfig_set(struct usb_device *usb_dev, struct dvb_usb_device_properties *props);

extern Dword DL_DownConverter_device_init(Device_Context *DC);
extern DWORD DRV_DownConverter_setLoFrequency (Device_Context *DC, Dword *loFreq);

extern DWORD DL_Tuner_SetFreqBw(Device_Context *DC,BYTE chip_index, u32 ucFreq, u8 ucBw);
extern DWORD DL_isLocked(Device_Context *DC, BYTE chip_index, Bool *bLock);
extern DWORD DL_getSNR(Device_Context *DC, BYTE chip_index, Constellation *constellation, BYTE *snr);
extern DWORD DL_getSignalStrength(Device_Context *DC, BYTE chip_index, BYTE *strength);
extern DWORD DL_getSignalStrengthDbm(Device_Context *DC, BYTE chip_index, Long *strengthDbm);
extern DWORD DL_getSignalStrengthIndication(Device_Context *DC, BYTE chip_index, BYTE *strength);
extern DWORD DL_getChannelStatistic(Device_Context *DC, BYTE chip_index, ChannelStatistic *channelStatistic);

extern DWORD DL_ResetPID(Device_Context *DC, BYTE chip_index);
extern DWORD DL_PIDOnOff(Device_Context *DC, BYTE chip_index, bool on);
extern DWORD DL_AddPID(Device_Context *DC, BYTE chip_index, Byte index, Pid pid);
extern DWORD DL_RemovePID(Device_Context *DC, BYTE chip_index, Byte index, Pid pid);

extern int dvb_usb_it913x_debug;
#endif
