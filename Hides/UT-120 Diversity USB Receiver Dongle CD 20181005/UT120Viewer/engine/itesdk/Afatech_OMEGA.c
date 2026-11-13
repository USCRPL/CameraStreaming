/**
 * @(#)Afatech_OMEGA.cpp
 *
 * ==========================================================
 * Version: 2.0
 * Date:    2009.06.15
 * ==========================================================
 *
 * ==========================================================
 * History:
 *
 * Date         Author      Description
 * ----------------------------------------------------------
 *
 * 2009.06.15   M.-C. Ho    new tuner
 * ==========================================================
 *
 * Copyright 2009 Afatech, Inc. All rights reserved.
 *
 */


//#include <stdio.h> //for linux
#include "type.h"
#include "error.h"
#include "user.h"
#include "register.h"
#include "standard.h"
#include "omega.h"
#include "Firmware_Afa_Omega_Script.h"
#include "Firmware_Afa_Omega_LNA_Config_1_Script.h"
#include "Firmware_Afa_Omega_LNA_Config_2_Script.h"
#include "Firmware_Afa_Omega_Script_V2.h"
#include "Firmware_Afa_Omega_LNA_Config_1_Script_V2.h"
#include "Firmware_Afa_Omega_LNA_Config_2_Script_V2.h"
#include "Firmware_Afa_Omega_LNA_Config_3_Script_V2.h"
#include "Firmware_Afa_Omega_LNA_Config_4_Script_V2I.h"
#include "Firmware_Afa_Omega_LNA_Config_5_Script_V2I.h"
#include "Firmware_Afa_Omega_LNA_Config_3_Script_V2W.h"



Dword OMEGA_open (
	IN  Demodulator*	demodulator,
	IN  Byte			chip
) {
	Dword error = Error_NO_ERROR;

	error = omega_init(demodulator, chip);

	return (error);
}

Dword OMEGA_close (
	IN  Demodulator*	demodulator,
	IN  Byte			chip
) {
	return (Error_NO_ERROR);
}

Dword OMEGA_set (
	IN  Demodulator*	demodulator,
	IN  Byte			chip,
	IN  Word			bandwidth,
	IN  Dword			frequency
) {
	Dword error = Error_NO_ERROR;

	error = omega_setfreq(demodulator, chip, (unsigned int)bandwidth, (unsigned int)frequency);
	
	return (error);
}


TunerDescription tuner_OMEGA = {
    OMEGA_open,
    OMEGA_close,
    OMEGA_set,
    NULL,
    NULL,
    OMEGA_ADDRESS,			/** tuner i2c address */
    2,						/** length of tuner register address */
    0,						/** tuner if */
    False,					/** spectrum inverse */
    0x38,					/** tuner id */
};

Dword OMEGA_supportLNA (
    IN  Demodulator*    demodulator,
    IN  Byte            supporttype
 ) {
     Dword error = Error_INVALID_DEV_TYPE;
	
     Byte chip_version = 0;
	 Dword chip_Type;
	 Byte var[2];
	
	error = Standard_readRegister(demodulator, 0, Processor_LINK, chip_version_7_0, &chip_version);
	if (error) goto exit;
	error = Standard_readRegisters(demodulator, 0, Processor_LINK, chip_version_7_0+1, 2, var);
	if (error) goto exit;
	chip_Type = var[1]<<8 | var[0];	

	if(chip_Type==0x9135 && chip_version == 2){	
		switch (supporttype){
			case OMEGA_NORMAL:			
				tuner_OMEGA.tunerScript = V2_OMEGA_scripts;
				tuner_OMEGA.tunerScriptSets = V2_OMEGA_scriptSets;
                tuner_OMEGA.tunerId = 0x60;				
				error = Error_NO_ERROR;
				break;
			case OMEGA_LNA_Config_1:
				tuner_OMEGA.tunerScript = V2_OMEGA_LNA_Config_1_scripts;
				tuner_OMEGA.tunerScriptSets = V2_OMEGA_LNA_Config_1_scriptSets;
				tuner_OMEGA.tunerId = 0x61;
				error = Error_NO_ERROR;
				break;
			case OMEGA_LNA_Config_2:
				tuner_OMEGA.tunerScript = V2_OMEGA_LNA_Config_2_scripts;
				tuner_OMEGA.tunerScriptSets = V2_OMEGA_LNA_Config_2_scriptSets;
				tuner_OMEGA.tunerId = 0x62;
				error = Error_NO_ERROR;
				break;
			case OMEGA_LNA_Config_3:
				tuner_OMEGA.tunerScript = V2W_OMEGA_LNA_Config_3_scripts;
				tuner_OMEGA.tunerScriptSets = V2W_OMEGA_LNA_Config_3_scriptSets;
				tuner_OMEGA.tunerId = 0x63;
				error = Error_NO_ERROR;
				break;
			case OMEGA_LNA_Config_4:
				tuner_OMEGA.tunerScript = V2I_OMEGA_LNA_Config_4_scripts;
				tuner_OMEGA.tunerScriptSets = V2I_OMEGA_LNA_Config_4_scriptSets;
				tuner_OMEGA.tunerId = 0x64;
				error = Error_NO_ERROR;
				break;
			case OMEGA_LNA_Config_5:
				tuner_OMEGA.tunerScript = V2I_OMEGA_LNA_Config_5_scripts;
				tuner_OMEGA.tunerScriptSets = V2I_OMEGA_LNA_Config_5_scriptSets;
				tuner_OMEGA.tunerId = 0x65;
				error = Error_NO_ERROR;
				break;
			default:
				tuner_OMEGA.tunerScript = V2_OMEGA_scripts;
				tuner_OMEGA.tunerScriptSets = V2_OMEGA_scriptSets;
                tuner_OMEGA.tunerId = 0x60;				
				error = Error_NO_ERROR;
		        break;
	   }
	}else{
		switch (supporttype){
			case OMEGA_NORMAL:			
				tuner_OMEGA.tunerScript = OMEGA_scripts;
				tuner_OMEGA.tunerScriptSets = OMEGA_scriptSets;
				tuner_OMEGA.tunerId = 0x38;
				error = Error_NO_ERROR;
				break;
			case OMEGA_LNA_Config_1:
				tuner_OMEGA.tunerScript = OMEGA_LNA_Config_1_scripts;
				tuner_OMEGA.tunerScriptSets = OMEGA_LNA_Config_1_scriptSets;
				tuner_OMEGA.tunerId = 0x51;
				error = Error_NO_ERROR;
				break;
			case OMEGA_LNA_Config_2:
				tuner_OMEGA.tunerScript = OMEGA_LNA_Config_2_scripts;
				tuner_OMEGA.tunerScriptSets = OMEGA_LNA_Config_2_scriptSets;
				tuner_OMEGA.tunerId = 0x52;
				error = Error_NO_ERROR;
				break;
			default:
				tuner_OMEGA.tunerScript = OMEGA_scripts;
				tuner_OMEGA.tunerScriptSets = OMEGA_scriptSets;
				tuner_OMEGA.tunerId = 0x38;
				error = Error_NO_ERROR;
				break;

		 }
	}
exit:

	return error;
}
