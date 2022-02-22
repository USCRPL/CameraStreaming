#ifndef __RFFC2072_H__
#define __RFFC2072_H__
#include <linux/delay.h>    //for linux
#include "demodulator.h"


#define reg_LF			0x00
#define reg_XO			0x01
#define reg_CAL_TIME	0x02
#define reg_VCO_CTRL	0x03
#define reg_CT_CAL1		0x04
#define reg_CT_CAL2		0x05
#define reg_PLL_CAL1	0x06
#define reg_PLL_CAL2	0x07
#define reg_VCO_AUTO	0x08
#define reg_PLL_CTRL	0x09
#define reg_PLL_BIAS	0x0A
#define reg_MIX_CONT	0x0B
#define reg_P1_FREQ1	0x0C
#define reg_P1_FREQ2	0x0D
#define reg_P1_FREQ3	0x0E
#define reg_P2_FREQ1	0x0F
#define reg_P2_FREQ2	0x10
#define reg_P2_FREQ3	0x11
#define reg_FN_CTRL		0x12
#define reg_EXT_MOD		0x13
#define reg_FMOD		0x14
#define reg_SDI_CTRL	0x15
#define reg_GPO			0x16
#define reg_T_VCO		0x17
#define reg_IQMOD1		0x18
#define reg_IQMOD2		0x19
#define reg_IQMOD3		0x1A
#define reg_IQMOD4		0x1B
#define reg_T_CTRL		0x1C
#define reg_DEV_CTRL	0x1D
#define reg_TEST		0x1E
#define reg_READBACK0	0x1F
#define reg_READBACK1	0x20
#define reg_READBACK2	0x21
#define reg_READBACK3	0x22
#define reg_READBACK4	0x23
#define reg_READBACK5	0x24
#define reg_READBACK6	0x25
#define reg_READBACK7	0x26

#define LO_Frequency 1597000

typedef struct {
	Handle	userData;
	Dword	frequency;
	Word	reg[39];
} RFFC2072INFO;

typedef enum {
	RFFC2071_Path1 = 0,
	RFFC2071_Path2
	
} RFFC2071_PATH;

Dword RFFC2072_readRegister (
	RFFC2072INFO *RFFC2072,
	Byte regAddr,
	Word *value
);


Dword RFFC2072_writeRegister (
	RFFC2072INFO *RFFC2072,
	Byte regAddr,
	Word value
);

Dword RFFC2072_initialize(
	RFFC2072INFO *RFFC2072
);

Dword RFFC2071_initialize(
	RFFC2072INFO *RFFC2072
);


Dword RFFC2072_setOperatingFrequency(RFFC2072INFO *RFFC2072, Dword f_lo_KHz);
Dword RFFC2071_setOperatingFrequency(RFFC2072INFO *RFFC2072, RFFC2071_PATH Path, Dword f_lo_KHz);
Dword RFFC2072_enableDevice(RFFC2072INFO *RFFC2072, Bool enable);
Dword RFFC2071_enableDevice(RFFC2072INFO *RFFC2072, Bool enable);
Dword RFFC2071_switchInputSource(RFFC2072INFO *RFFC2072, RFFC2071_PATH Path, Bool is2400KHzOut); //0: Path1 1: Path2
Dword RFFC2072_setMixerCurrent(
	RFFC2072INFO *RFFC2072,
	Byte			value
);

Dword RFFC2072_setChargePumpCurrent(
	RFFC2072INFO *RFFC2072,
	Byte			value
);

Dword RFFC2072_setChargePumpLeakage(
	RFFC2072INFO *RFFC2072,
	Byte			value
);

Dword RFFC2072_setReLock(
	RFFC2072INFO *RFFC2072
);

Dword RFFC2072_checkLock(
	RFFC2072INFO *RFFC2072
	);
#endif
