#include "demodulatorextend.h"

/* For driver, only include these & implement */
#include "usb2impl.h"
#include "cmd.h"
#include "Afatech_OMEGA.h"

static PidInfo pidInfo;
BusDescription busDesc[] =
{
    /** 0: NULL bus */
    {
        NULL,
        NULL,
        NULL,
        NULL,
    },
    /** 1: I2C bus */
    {
        NULL,
        NULL,
        NULL,
        NULL,
    },
    /** 2: USB bus */
    {
        Usb2_getDriver,
        Usb2_writeControlBus,
        Usb2_readControlBus,
        Usb2_readDataBus,
    },
    /** 3: SPI bus */
    {
        NULL,
        NULL,
        NULL,
        NULL,
    },
    /** 4: SDIO bus */
    {
        NULL,
        NULL,
        NULL,
        NULL,
    },
    /** 5: USB11 bus */
    {
        Usb2_getDriver,
        Usb2_writeControlBus,
        Usb2_readControlBus,
        Usb2_readDataBus,
    },
    /** 6: I2M bus - I2C for old mail box */
    {
        NULL,
        NULL,
        NULL,
        NULL,
    },
    /** 7: I2U bus - 9015 USB for old mail box */
    {
        NULL,
        NULL,
        NULL,
        NULL,
    },
    /** 8: I2U2 bus - 9015 USB for new mail box */
    {
        NULL,
        NULL,
        NULL,
        NULL,
    },
    /** 9: 9035U2I bus - 9035 USB to I2C */
    {
        NULL,
        NULL,
        NULL,
        NULL,
    },
};

CmdDescription cmdDesc[] =
{
    /** NULL Bus */
    {
        0,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL
    },
    /** 1:I2C Bus */
    {
        255,
        &busDesc[1],
        Cmd_writeRegisters,
        Cmd_writeTunerRegisters,
        Cmd_writeEepromValues,
        Cmd_readRegisters,
        Cmd_readTunerRegisters,
        Cmd_readEepromValues,
        NULL,
        Cmd_loadFirmware,
        Cmd_reboot,
        Cmd_sendCommand,
        Cmd_receiveData
    },
    /** 2:USB Bus */
    {
        63,
        &busDesc[2],
        Cmd_writeRegisters,
        Cmd_writeTunerRegisters,
        Cmd_writeEepromValues,
        Cmd_readRegisters,
        Cmd_readTunerRegisters,
        Cmd_readEepromValues,
        NULL,
        Cmd_loadFirmware,
        Cmd_reboot,
        Cmd_sendCommand,
        Cmd_receiveData
    },
    /** 3:SPI Bus */
    {
        255,
        &busDesc[3],
        Cmd_writeRegisters,
        Cmd_writeTunerRegisters,
        Cmd_writeEepromValues,
        Cmd_readRegisters,
        Cmd_readTunerRegisters,
        Cmd_readEepromValues,
        NULL,
        Cmd_loadFirmware,
        Cmd_reboot,
        Cmd_sendCommand,
        Cmd_receiveData
    },
    /** 4:SDIO Bus */
    {
        255,
        &busDesc[4],
        Cmd_writeRegisters,
        Cmd_writeTunerRegisters,
        Cmd_writeEepromValues,
        Cmd_readRegisters,
        Cmd_readTunerRegisters,
        Cmd_readEepromValues,
        NULL,
        Cmd_loadFirmware,
        Cmd_reboot,
        Cmd_sendCommand,
        Cmd_receiveData
    },
    /** 5:USB11 Bus */
    {
        63,
        &busDesc[5],
        Cmd_writeRegisters,
        Cmd_writeTunerRegisters,
        Cmd_writeEepromValues,
        Cmd_readRegisters,
        Cmd_readTunerRegisters,
        Cmd_readEepromValues,
        NULL,
        Cmd_loadFirmware,
        Cmd_reboot,
        Cmd_sendCommand,
        Cmd_receiveData
    },
	/** 6:I2C for old mailbox */
    {
        16,
        &busDesc[6],
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL
    },
    /** 7:USB for old mailbox */
    {
        16,
        &busDesc[7],
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL
    },

    /** 8:USB for new mailbox */
    {
        55,
        &busDesc[8],
        Cmd_writeRegisters,
        Cmd_writeTunerRegisters,
        Cmd_writeEepromValues,
        Cmd_readRegisters,
        Cmd_readTunerRegisters,
        Cmd_readEepromValues,
        NULL,
        Cmd_loadFirmware,
        Cmd_reboot,
        Cmd_sendCommand,
        Cmd_receiveData
    },
    /** 9:9035 USB to I2C Bus */
    {
        54,
        &busDesc[9],
        Cmd_writeRegisters,
        Cmd_writeTunerRegisters,
        Cmd_writeEepromValues,
        Cmd_readRegisters,
        Cmd_readTunerRegisters,
        Cmd_readEepromValues,
        NULL,
        Cmd_loadFirmware,
        Cmd_reboot,
        Cmd_sendCommand,
        Cmd_receiveData
    },
};

Dword Demodulator_setBusTuner (
    IN  Demodulator*    demodulator,
    IN  Word            busId,
    IN  Word            tunerId
) {
    Dword error = Error_NO_ERROR;

    IT9130* it9130;

    it9130 = (IT9130*) demodulator;

    it9130->cmdDescription = &cmdDesc[busId];
    it9130->busId = busId;

	//for driver, no I2U
    //if (busId == Bus_I2U2) {
    //    I2u_setChipAddress (demodulator, 0x38);
    //}

    switch (tunerId)
    {

	
    case Tuner_Afatech_OMEGA:
        it9130->tunerDescription = &tuner_OMEGA;
		error = OMEGA_supportLNA (demodulator, 0);
        break;
	case Tuner_Afatech_OMEGA_LNA_Config_1:
        it9130->tunerDescription = &tuner_OMEGA;
		error = OMEGA_supportLNA (demodulator, 1);
        break;
	case Tuner_Afatech_OMEGA_LNA_Config_2:
        it9130->tunerDescription = &tuner_OMEGA;
		error = OMEGA_supportLNA (demodulator, 2);
        break;
	case Tuner_Afatech_OMEGA_V2:
        it9130->tunerDescription = &tuner_OMEGA;
		error = OMEGA_supportLNA (demodulator, 0);
        break;
	case Tuner_Afatech_OMEGA_V2_LNA_Config_1:
        it9130->tunerDescription = &tuner_OMEGA;
		error = OMEGA_supportLNA (demodulator, 1);
        break;
	case Tuner_Afatech_OMEGA_V2_LNA_Config_2:
        it9130->tunerDescription = &tuner_OMEGA;
		error = OMEGA_supportLNA (demodulator, 2);
        break;
	case Tuner_Afatech_OMEGA_V2_LNA_Config_3:
        it9130->tunerDescription = &tuner_OMEGA;
		error = OMEGA_supportLNA (demodulator, 3);
        break;
	case Tuner_Afatech_OMEGA_V2_LNA_Config_4:
        it9130->tunerDescription = &tuner_OMEGA;
		error = OMEGA_supportLNA (demodulator, 4);
        break;
	case Tuner_Afatech_OMEGA_V2_LNA_Config_5:
        it9130->tunerDescription = &tuner_OMEGA;
		error = OMEGA_supportLNA (demodulator, 5);
        break;
    /* for driver, no this mode
	case Tuner_Afatech_PICTORV32:
        it9130->tunerDescription = &tuner_PICTORV32;
        break;
	*/
    default:
        error = Error_INVALID_TUNER_TYPE;
		it9130->tunerDescription = NULL;
        goto exit;
        break;
    }

    if (it9130->tunerDescription->tunerScript == NULL) {
        it9130->tunerDescription->tunerScript = NULL;
        it9130->tunerDescription->tunerScriptSets = NULL;
    }


exit:
    return(error);
}
Dword Demodulator_getChannelStatistic (
    IN  Demodulator*            demodulator,
    IN  Byte                    chip,
    OUT ChannelStatistic*       channelStatistic
) {
    Dword error = Error_NO_ERROR;

#if User_USE_DRIVER
    Dword number;
    BOOL result;
    GetChannelStatisticRequest request;
    IT9130* it9130;

    it9130 = (IT9130*) demodulator;

    if (it9130->driver != NULL) {
        request.chip = chip;
        request.channelStatistic = channelStatistic;
        result = DeviceIoControl (
                    it9130->driver,
                    IOCTL_AFA_DEMOD_GETCHANNELSTATISTIC,
                    &request,
                    sizeof (request),
                    NULL,
                    0,
                    &number,
                    NULL
        );
        error = request.error;
    } else {
        error = Error_DRIVER_INVALID;
    }
#else
    Dword postErrCnt;
    Dword postBitCnt;
    Word rsdAbortCnt;
    IT9130* it9130;


    it9130 = (IT9130*) demodulator;


    /** Get BER if couter is ready, error = Error_RSD_COUNTER_NOT_READY if counter is not ready */
    if (it9130->architecture == Architecture_PIP) {
        error = Standard_getPostVitBer (demodulator, chip, &postErrCnt, &postBitCnt, &rsdAbortCnt);
        if (error == Error_NO_ERROR) {
            it9130->channelStatistic[chip].postVitErrorCount = postErrCnt;
            it9130->channelStatistic[chip].postVitBitCount = postBitCnt;
            it9130->channelStatistic[chip].abortCount = rsdAbortCnt;
        }
    } else {
        error = Standard_getPostVitBer (demodulator, 0, &postErrCnt, &postBitCnt, &rsdAbortCnt);
        if (error == Error_NO_ERROR) {
            it9130->channelStatistic[chip].postVitErrorCount = postErrCnt;
            it9130->channelStatistic[chip].postVitBitCount = postBitCnt;
            it9130->channelStatistic[chip].abortCount = rsdAbortCnt;
        }
    }

    *channelStatistic = it9130->channelStatistic[chip];

#endif

    return (error);
}



Dword Demodulator_addPid (
    IN  Demodulator*    demodulator,
    IN  Byte            chip,
    IN  Pid             pid
) {
    Dword error = Error_NO_ERROR;

#if User_USE_DRIVER
    Dword number;
    BOOL result;
    AddPidRequest request;
    IT9130* it9130;

    it9130 = (IT9130*) demodulator;

    if (it9130->driver != NULL) {
        request.chip = chip;
        request.pid = pid;
        result = DeviceIoControl (
                    it9130->driver,
                    IOCTL_AFA_DEMOD_ADDPID,
                    &request,
                    sizeof (request),
                    NULL,
                    0,
                    &number,
                    NULL
        );
        error = request.error;
    } else {
        error = Error_DRIVER_INVALID;
    }
#else
    Byte writeBuffer[2];
    Byte i, j;
    Bool found;
    IT9130* it9130;

    it9130 = (IT9130*) demodulator;

    if (pidInfo.pidinit == False) {
        for (i = 0; i < it9130->chipNumber; i++) {
            for (j = 0; j < 32; j++) {
                pidInfo.pidtable[i].pid[j] = 0xFFFF;
            }
        }
        pidInfo.pidinit = True;
    }

    /** Enable pid filter */
    if (pidInfo.pidcount == 0) {
        error = Standard_writeRegisterBits (demodulator, chip, Processor_OFDM, p_mp2if_pid_en, mp2if_pid_en_pos, mp2if_pid_en_len, 1);
        if (error) goto exit;
    } else {
        found = False;
        for (i = 0; i < 32; i++) {
            if (pidInfo.pidtable[chip].pid[i] == pid.value) {
                found = True;
                break;
            }
        }
        if (found == True)
            goto exit;
    }

    for (i = 0; i < 32; i++) {
        if (pidInfo.pidtable[chip].pid[i] == 0xFFFF)
            break;
    }
    if (i == 32) {
        error = Error_PID_FILTER_FULL;
        goto exit;
    }

    writeBuffer[0] = (Byte) pid.value;
    writeBuffer[1] = (Byte) (pid.value >> 8);

    error = Standard_writeRegisters (demodulator, chip, Processor_OFDM, p_mp2if_pid_dat_l, 2, writeBuffer);
    if (error) goto exit;

    error = Standard_writeRegisterBits (demodulator, chip, Processor_OFDM, p_mp2if_pid_index_en, mp2if_pid_index_en_pos, mp2if_pid_index_en_len, 1);
    if (error) goto exit;

    error = Standard_writeRegister (demodulator, chip, Processor_OFDM, p_mp2if_pid_index, i);
    if (error) goto exit;

    pidInfo.pidtable[chip].pid[i] = pid.value;
    pidInfo.pidcount++;

exit :
#endif

    return (error);
}


Dword Demodulator_addPidAt (
    IN  Demodulator*    demodulator,
    IN  Byte            chip,
    IN  Byte            index,
    IN  Pid             pid
) {
	return (Demodulator_addPidToFilter (demodulator, chip, index, pid));
}


Dword Demodulator_removePid (
    IN  Demodulator*    demodulator,
    IN  Byte            chip,
    IN  Pid             pid
) {
    Dword error = Error_NO_ERROR;

#if User_USE_DRIVER
    Dword number;
    BOOL result;
    RemovePidRequest request;
    IT9130* it9130;

    it9130 = (IT9130*) demodulator;

    if (it9130->driver != NULL) {
        request.chip = chip;
        request.pid = pid;
        result = DeviceIoControl (
                    it9130->driver,
                    IOCTL_AFA_DEMOD_REMOVEPID,
                    &request,
                    sizeof (request),
                    NULL,
                    0,
                    &number,
                    NULL
        );
        error = request.error;
    } else {
        error = Error_DRIVER_INVALID;
    }
#else
    Byte i;
    Bool found;
    Interrupts interrupts;
    IT9130* it9130;

    it9130 = (IT9130*) demodulator;

    found = False;
    for (i = 0; i < 32; i++) {
        if (pidInfo.pidtable[chip].pid[i] == pid.value) {
            found = True;
            break;
        }
    }
    if (found == False)
        goto exit;

    error = Standard_writeRegisterBits (demodulator, chip, Processor_OFDM, p_mp2if_pid_index_en, mp2if_pid_index_en_pos, mp2if_pid_index_en_len, 0);
    if (error) goto exit;

    error = Standard_writeRegister (demodulator, chip, Processor_OFDM, p_mp2if_pid_index, i);
    if (error) goto exit;

    pidInfo.pidtable[chip].pid[i] = 0xFFFF;

    /** Disable pid filter */
    if (pidInfo.pidcount == 1) {
        error = Standard_writeRegisterBits (demodulator, chip, Processor_OFDM, p_mp2if_pid_en, mp2if_pid_en_pos, mp2if_pid_en_len, 0);

        error = Standard_getInterrupts (demodulator, &interrupts);
        if (error) goto exit;
        if (interrupts & Interrupt_DVBT) {
            error = Standard_clearInterrupt (demodulator, Interrupt_DVBT);
            if (error) goto exit;
        }
    }

    pidInfo.pidcount--;

exit :
#endif

    return (error);
}


Dword Demodulator_removePidAt (
    IN  Demodulator*    demodulator,
    IN  Byte            chip,
	IN  Byte			index,
    IN  Pid             pid
) {
    Dword error = Error_NO_ERROR;

#if User_USE_DRIVER
	Dword number;
	BOOL result;
	RemovePidAtRequest request;
	IT9130* it9130;

	it9130 = (IT9130*) demodulator;
	
	if (it9130->driver != NULL) {
		request.chip = chip;
		request.index = index;
		request.pid = pid;
		result = DeviceIoControl (
					it9130->driver,
					IOCTL_AFA_DEMOD_REMOVEPIDAT,
					&request,
					sizeof (request),
					NULL,
					0,
					&number,
					NULL
		);
		error = request.error;
	} else {
		error = Error_DRIVER_INVALID;
	}
#else
	IT9130* it9130;

	it9130 = (IT9130*) demodulator;

	error = Standard_writeRegisterBits (demodulator, chip, Processor_OFDM, p_mp2if_pid_index_en, mp2if_pid_index_en_pos, mp2if_pid_index_en_len, 0);
	if (error) goto exit;

	error = Standard_writeRegister (demodulator, chip, Processor_OFDM, p_mp2if_pid_index, index);
	if (error) goto exit;
exit :
#endif

	return (error);
}


Dword Demodulator_resetPid (
    IN  Demodulator*    demodulator,
    IN  Byte            chip
) {
    Dword error = Error_NO_ERROR;

#if User_USE_DRIVER
#else
    Byte i;
    IT9130* it9130;

    it9130 = (IT9130*) demodulator;

    for (i = 0; i < 32; i++) {
        pidInfo.pidtable[chip].pid[i] = 0xFFFF;
    }
    error = Standard_writeRegisterBits (demodulator, chip, Processor_OFDM, p_mp2if_pid_rst, mp2if_pid_rst_pos, mp2if_pid_rst_len, 1);
    if (error) goto exit;

    pidInfo.pidcount = 0;

exit :
#endif

    return (error);
}

#ifndef __GNUC__
#ifdef UNDER_CE
#else
extern long ActiveSync;
#endif
#endif

Dword Demodulator_controlActiveSync (
    IN Demodulator* demodulator,
    IN Byte         control
) {
#ifndef __GNUC__
#ifdef UNDER_CE
#else
    if (control == 0)
        ActiveSync = 0;
    else
        ActiveSync = 1;
#endif
#endif

    return (Error_NO_ERROR);
}


Dword Demodulator_setStatisticRange (
    IN  Demodulator*    demodulator,
    IN  Byte            chip,
    IN  Byte            superFrameCount,
    IN  Word            packetUnit
) {
    return (Demodulator_setViterbiRange (demodulator, chip, superFrameCount, packetUnit));
}


Dword Demodulator_getStatisticRange (
    IN  Demodulator*    demodulator,
    IN  Byte            chip,
    IN  Byte*           superFrameCount,
    IN  Word*           packetUnit
) {
    return (Demodulator_getViterbiRange (demodulator, chip, superFrameCount, packetUnit));
}

Dword Demodulator_setDecrypt (
	IN  Demodulator*			demodulator,
	IN  Byte					chip,
	IN	Dword					decryptKey,
	IN	Byte					decryptEnable
) {
	Dword error = Error_NO_ERROR;
	Byte key_m[3][2];
	Dword mask[2];
	Byte i, j, k;
	Dword version;
	Byte tuner_id;
	Byte KEY_TABLE[2][8] = 
	{
		{1, 7, 9, 4, 8, 3, 10, 6}, 
		{8, 3, 6, 5, 10, 2, 9, 1}
	};

	error = Demodulator_getFirmwareVersion (demodulator, Processor_OFDM, &version);
	if(error) goto exit;
	error = Demodulator_readRegister (demodulator, chip, Processor_OFDM, tuner_ID, &tuner_id);
	if(error) goto exit;
	if(((version >> 16) & 0xFF) < 0x2F || tuner_id != 0x65)
		return Error_TPS_DECRYPTION_UNSUPPORTED;
		
	for(i = 0; i < 3; i++)
	{
		for(k = 0; k < 2; k++)
		{
			key_m[i][k] = 0;
			for(j = 0; j < 8; j++)
			{
				mask[k] = (0x80000000 >> (KEY_TABLE[k][j] - 1 + 10 * i));
				if((decryptKey & mask[k]) != 0)
					key_m[i][k] |= (0x80 >> j);
			}
		}
	}

	error = Demodulator_writeRegisters (demodulator, chip, Processor_OFDM, 0x45B2, 2, key_m[0]);
	if(error) goto exit;
	error = Demodulator_writeRegisters (demodulator, chip, Processor_OFDM, 0x45B4, 2, key_m[1]);
	if(error) goto exit;
	error = Demodulator_writeRegisters (demodulator, chip, Processor_OFDM, 0x45B6, 2, key_m[2]);
	if(error) goto exit;


	if(decryptEnable == 1)
	{
		error = Demodulator_writeRegister (demodulator, chip, Processor_OFDM, 0x19E, 1);
		if(error) goto exit;
	}
	else
	{
		error = Demodulator_writeRegister (demodulator, chip, Processor_OFDM, 0x19E, 0);
		if(error) goto exit;
	}


exit :
	return (error);
}