/*
 * This program is used to device_init and create all API function from DemodAPI code
 */

#include "it913x.h"

int dvb_usb_it913x_debug;
module_param_named(debug, dvb_usb_it913x_debug, int, 0644);
MODULE_PARM_DESC(debug, "Set debugging level.(info = 1, deb_fw = 2, deb_fwdata = 4, deb_data = 8)" DVB_USB_DEBUG_STATUS);

DWORD DRV_IrTblDownload(Device_Context *DC)
{
	DWORD result = Error_NO_ERROR;
	struct file *filp;
	unsigned char b_buf[512];
	int fileSize;
	mm_segment_t oldfs;

	deb_data("- Enter %s Function -\n", __func__);

	oldfs = get_fs();
	set_fs(KERNEL_DS);
	filp = filp_open("/lib/firmware/it913xirtbl.bin", O_RDWR, 0644);
	if (IS_ERR(filp)) {
		deb_data("\t Error: open it913xirtbl.bin fail\n");
		goto exit;
	}

	if ((filp->f_op) == NULL) {
		deb_data("\t Error: file operation method error\n");
		goto exit;
	}

	filp->f_pos = 0x00;
	fileSize = filp->f_op->read(filp, b_buf, sizeof(b_buf), &filp->f_pos);

	result = Demodulator_loadIrTable((Demodulator *)&DC->Demodulator, (Word)fileSize, b_buf);
	if (result) {
		deb_data("\t Error: Demodulator_loadIrTable [0x%08lx]\n", result);
		goto exit;
	}

	filp_close(filp, NULL);
	set_fs(oldfs);

	deb_data("- %s success -\n", __func__);
	return result;

exit:
	deb_data("- %s fail -\n", __func__);
	return result;
}

/* Set tuner Frequency and BandWidth */
DWORD DRV_SetFreqBw(
	Device_Context *DC,
	BYTE chip_index,
	DWORD dwFreq,
	WORD ucBw)
{
	DWORD result = Error_NO_ERROR;
	Dword tmpLoFreq = 1800000;

	if(DC->board_id == DownConverter_ADRF6655)
		tmpLoFreq = 1800000;
	else if(DC->board_id == DownConverter_RFFC2072)
		tmpLoFreq = LO_Frequency;
	else if(DC->board_id == DownConverter_RFFC2071)
	{
		if(dwFreq > 1000000)
			tmpLoFreq = LO_Frequency;
		else
			tmpLoFreq = 0;
	}


	//deb_data("- Enter %s Function -\n", __func__);
	deb_data("\t chip[%d] desire to set Freq = %ld, BW = %d\n", chip_index, dwFreq, ucBw * 1000);

	if (!DC->fc[chip_index].tunerinfo.bTunerInited) {
		deb_data("\t Error: tuner power is off\n");
		goto exit;
	}

	if (dwFreq)
	{
		if( DC->board_id == DownConverter_ADRF6655 || 
			DC->board_id == DownConverter_RFFC2072 ||
			DC->board_id == DownConverter_RFFC2071 )
		{
			/*set DownConverter Lo Frequency*/
			result = DRV_DownConverter_setLoFrequency (DC, &tmpLoFreq);
			if(result) {
				deb_data("\t DRV_DownConverter_setLoFrequency failed\n");
				goto exit;
			}

			if(tmpLoFreq == 0) //if not using 2.4G
				DC->fc[chip_index].ulDesiredFrequency = dwFreq;
			else if(dwFreq > tmpLoFreq)
				DC->fc[chip_index].ulDesiredFrequency = dwFreq - tmpLoFreq;
			else
				DC->fc[chip_index].ulDesiredFrequency = tmpLoFreq - dwFreq;
		}
		else //ordinary not 6655 , 2072 or 2071 case
			DC->fc[chip_index].ulDesiredFrequency = dwFreq;
	}
	if (ucBw)
		DC->fc[chip_index].ucDesiredBandWidth = ucBw * 1000;

	DC->fc[chip_index].tunerinfo.bTunerOK = false;
	if (DC->fc[chip_index].ulDesiredFrequency != 0 && DC->fc[chip_index].ucDesiredBandWidth != 0) {
		result = Demodulator_acquireChannel((Demodulator *)&DC->Demodulator, chip_index, DC->fc[chip_index].ucDesiredBandWidth, DC->fc[chip_index].ulDesiredFrequency);
		if (result) {
			deb_data("\t Error: chip[%d] Demodulator_acquireChannel fail [0x%08lx]\n", chip_index, result);
			goto exit;
		}
		else {
			/* When success acquireChannel, record currentFreq/currentBW */
			DC->fc[chip_index].ulCurrentFrequency = DC->fc[chip_index].ulDesiredFrequency;
			DC->fc[chip_index].ucCurrentBandWidth = DC->fc[chip_index].ucDesiredBandWidth;
		}
	}
	else {
		deb_data("\t Warning: parameter not correct\n");
		goto exit;
	}

	DC->fc[chip_index].tunerinfo.bTunerOK = true;

	/* For perfomance. if only 1 on, set the others to the same freq.*/
	if (DC->Demodulator.chipNumber > atomic_read(&DC->FilterCnt) && DC->fc[chip_index].bTimerOn && !DC->fc[!chip_index].tunerinfo.bTunerLock) {
		result = Demodulator_acquireChannel((Demodulator *)&DC->Demodulator, (!chip_index), DC->fc[chip_index].ucCurrentBandWidth, DC->fc[chip_index].ulCurrentFrequency);
		if (result)
			deb_data("\t Warning: chip[%d] set the same parameter fail [0x%08lx]\n", !chip_index, result);
		else
			deb_data("\t Chip[%d] set the same parameter success\n", !chip_index);
	}

	deb_data("- chip[%d] set Freq = %ld, setOmega Freq = %ld, BW = %d, success -\n", chip_index, dwFreq, DC->fc[chip_index].ulCurrentFrequency, DC->fc[chip_index].ucCurrentBandWidth);
	return result;

exit:
	deb_data("- chip[%d] set Freq = %ld, BW = %d, fail -\n", chip_index, DC->fc[chip_index].ulDesiredFrequency, DC->fc[chip_index].ucDesiredBandWidth);
	return result;
}

/* Reset PID table */
DWORD DRV_ResetPID(
	Device_Context *DC,
	BYTE chip)
{
	DWORD result = Error_NO_ERROR;

	deb_data("- Enter %s Function -\n", __func__);

	/* Clear pid Table */
	result = Demodulator_resetPid((Demodulator *)&DC->Demodulator, chip);

	return result;
}

/* Add pid to filter */
DWORD DRV_AddPID(
	Device_Context *DC,
	BYTE chip_index,
	Byte index,
	Pid pid)
{
	DWORD result = Error_NO_ERROR;

	deb_data("- Enter %s Function, filter[%d] add pid[0x%04x] -\n", __func__, index, pid.value);

	result = Demodulator_addPidToFilter((Demodulator *)&DC->Demodulator, chip_index, index, pid);

	return result;
}

/* Remove pid from filter*/
DWORD DRV_RemovePID(
	Device_Context *DC,
	BYTE chip_index,
	Byte index,
	Pid pid)
{
	DWORD result = Error_NO_ERROR;

	deb_data("- Enter %s Function, filter[%d] remove pid[%x] -\n", __func__, index, pid.value);

	result = Demodulator_removePidAt((Demodulator *)&DC->Demodulator, chip_index, index, pid);

	return result;
}

/* Set PID onoff */
DWORD DRV_PIDOnOff(
	Device_Context *DC,
	BYTE chip_index,
	bool bOn)
{
	DWORD result = Error_NO_ERROR;

	deb_data("- Enter %s Function PID = %s -\n", __func__, bOn ? "ON" : "OFF");

	if (bOn)
		result = Demodulator_controlPidFilter((Demodulator *)&DC->Demodulator, chip_index, 1);
	else
		result = Demodulator_controlPidFilter((Demodulator *)&DC->Demodulator, chip_index, 0);

	return result;
}

DWORD DRV_SetArchitecture(
	Device_Context *DC,
	Architecture architecture)
{
	DWORD result = Error_NO_ERROR;

	result= Demodulator_setArchitecture((Demodulator *)&DC->Demodulator, architecture);

	return result;
}

/* Check tuner lock signal or not */
DWORD DRV_isLocked(
	Device_Context *DC,
	BYTE chip_index,
	Bool *bLock)
{
	DWORD result = Error_NO_ERROR;
	*bLock = true;

	//deb_data("- Enter %s Function -\n", __func__);

	result = Demodulator_isLocked((Demodulator *)&DC->Demodulator, chip_index, bLock);
	if (result)
		deb_data("\t Error: Demodulator_isLocked fail [0x%08lx]\n", result);

	return result;
}

/* Get signal strength */
DWORD DRV_getSignalStrength(
	Device_Context *DC,
	BYTE chip_index,
	BYTE *strength)
{
	DWORD result = Error_NO_ERROR;

	//deb_data("- Enter %s Function -\n", __func__);

	result = Demodulator_getSignalStrength((Demodulator *)&DC->Demodulator, chip_index, strength);

	return result;
}

/* Get signal strengthDbm */
DWORD DRV_getSignalStrengthDbm(
	Device_Context *DC,
	BYTE chip_index,
	Long *strengthDbm)
{
	DWORD result = Error_NO_ERROR;

	//deb_data("- Enter %s Function -\n", __func__);

	result = Demodulator_getSignalStrengthDbm((Demodulator *)&DC->Demodulator, chip_index, strengthDbm);

	return result;
}

DWORD DRV_getSignalStrengthIndication(
	Device_Context *DC,
	BYTE chip_index,
	Byte *strength)
{
	DWORD result = Error_NO_ERROR;

	//deb_data("- Enter %s Function -\n", __func__);

	result = Demodulator_getSignalStrengthIndication((Demodulator *)&DC->Demodulator, chip_index, strength);

	return result;
}

/* Get Channel Statistic */
DWORD DRV_getChannelStatistic(
	Device_Context *DC,
	BYTE chip_index,
	ChannelStatistic *channelStatistic)
{
	DWORD result = Error_NO_ERROR;

	//deb_data("- Enter %s Function -\n", __func__);

	result = Demodulator_getChannelStatistic((Demodulator *)&DC->Demodulator, chip_index, channelStatistic);

	return result;
}

/* Get Channel Modulation */
DWORD DRV_getChannelModulation(
	Device_Context *DC,
	BYTE chip_index,
	ChannelModulation *channelModulation)
{
	DWORD result = Error_NO_ERROR;

	//deb_data("- Enter %s Function -\n", __func__);

	result = Standard_getChannelModulation((Demodulator *)&DC->Demodulator, chip_index, channelModulation);

	return result;
}

/* Get OFDM /LINK Firmware version */
DWORD DRV_getFirmwareVersionFromFile(
	Device_Context *DC,
	Processor processor,
	DWORD *version)
{
	BYTE chip_version = 0;
	DWORD chip_Type;
	BYTE var[2];
	DWORD result = Error_NO_ERROR;

	DWORD OFDM_VER1, OFDM_VER2, OFDM_VER3, OFDM_VER4;
	DWORD LINK_VER1, LINK_VER2, LINK_VER3, LINK_VER4;

	deb_data("- Enter %s Function -\n", __func__);

	/* Get chip version */
	result = Demodulator_readRegister((Demodulator *)&DC->Demodulator, 0, Processor_LINK, chip_version_7_0, &chip_version);
	if (result) {
		deb_data("\t Error: Demodulator_readRegister [chip_version_7_0] fail [0x%08lx]\n", result);
		goto exit;
	}

	/* Get chip type */
	result = Demodulator_readRegisters((Demodulator *)&DC->Demodulator, 0, Processor_LINK, (chip_version_7_0 + 1), 2, var);
	if (result) {
		deb_data("\t Error: Demodulator_readRegister [chip_version_7_0 + 1] fail [0x%08lx]\n", result);
		goto exit;
	}

	chip_Type = (var[1] << 8) | var[0];

	switch (DC->fc[0].tunerinfo.TunerId) {
	case 0x64:
		OFDM_VER1 = DVB_V2I_OFDM_VERSION1;
		OFDM_VER2 = DVB_V2I_OFDM_VERSION2;
		OFDM_VER3 = DVB_V2I_OFDM_VERSION3;
		OFDM_VER4 = DVB_V2I_OFDM_VERSION4;

		LINK_VER1 = DVB_V2I_LL_VERSION1;
		LINK_VER2 = DVB_V2I_LL_VERSION2;
		LINK_VER3 = DVB_V2I_LL_VERSION3;
		LINK_VER4 = DVB_V2I_LL_VERSION4;
		break;
	default:
	case 0x38:
		if (chip_version == 2) {
			OFDM_VER1 = DVB_V2_OFDM_VERSION1;
			OFDM_VER2 = DVB_V2_OFDM_VERSION2;
			OFDM_VER3 = DVB_V2_OFDM_VERSION3;
			OFDM_VER4 = DVB_V2_OFDM_VERSION4;

			LINK_VER1 = DVB_V2_LL_VERSION1;
			LINK_VER2 = DVB_V2_LL_VERSION2;
			LINK_VER3 = DVB_V2_LL_VERSION3;
			LINK_VER4 = DVB_V2_LL_VERSION4;
		}
		else {
			OFDM_VER1 = OFDM_VERSION1;
			OFDM_VER2 = OFDM_VERSION2;
			OFDM_VER3 = OFDM_VERSION3;
			OFDM_VER4 = OFDM_VERSION4;

			LINK_VER1 = LL_VERSION1;
			LINK_VER2 = LL_VERSION2;
			LINK_VER3 = LL_VERSION3;
			LINK_VER4 = LL_VERSION4;
		}
		break;
	}

	if (processor == Processor_OFDM)
		*version = (DWORD)((OFDM_VER1 << 24) + (OFDM_VER2 << 16) + (OFDM_VER3 << 8) + OFDM_VER4);
	else //LINK
		*version = (DWORD)((LINK_VER1 << 24) + (LINK_VER2 << 16) + (LINK_VER3 << 8) + LINK_VER4);

	deb_data("- %s : success -\n", __func__);
	return result;

exit:
	deb_data("- %s : fail -\n", __func__);
	return result;
}

DWORD DRV_WriteRegister(
	IN Device_Context *DC,
	IN BYTE chip_index,
	IN Processor processor,
	IN DWORD wReg,
	IN BYTE ucpValue)
{
	DWORD result = Error_NO_ERROR;

	result = Demodulator_writeRegister((Demodulator *)&DC->Demodulator, chip_index, processor, wReg, ucpValue);

	return result;
}

DWORD DRV_WriteRegisters(
	IN Device_Context *DC,
	IN BYTE chip_index,
	IN Processor processor,
	IN DWORD wReg,
	IN BYTE ucpValueLength,
	IN BYTE *ucpValue)
{
	DWORD result = Error_NO_ERROR;

	result = Demodulator_writeRegisters((Demodulator *)&DC->Demodulator, chip_index, processor, wReg, ucpValueLength, ucpValue);

	return result;
}

DWORD DRV_WriteRegisterBits(
	IN Device_Context *DC,
	IN Byte chip,
	IN Processor processor,
	IN Dword registerAddress,
	IN Byte position,
	IN Byte length,
	IN Byte value)
{
	DWORD result = Error_NO_ERROR;

	result = Demodulator_writeRegisterBits((Demodulator *)&DC->Demodulator, chip, processor, registerAddress, position, length, value);

	return result;
}

DWORD DRV_ReadRegister(
	IN Device_Context *DC,
	IN BYTE chip_index,
	IN Processor processor,
	IN DWORD wReg,
	OUT BYTE *ucpValue)
{
	DWORD result = Error_NO_ERROR;

	result = Demodulator_readRegister((Demodulator *)&DC->Demodulator, chip_index, processor, wReg, ucpValue);

	return result;
}

Dword DRV_ReadRegisters(
	IN Device_Context *DC,
	IN BYTE chip_index,
	IN Processor processor,
	IN DWORD wReg,
	IN BYTE ucpValueLength,
	OUT BYTE *ucpValue)
{
	DWORD result = Error_NO_ERROR;

	result = Demodulator_readRegisters((Demodulator *)&DC->Demodulator, chip_index, processor, wReg, ucpValueLength, ucpValue);

	return result;
}

/* For strapping, gpioh1/gpioh5/ghioh7 have been program, just in IT913x */
DWORD DRV_DisableGPIOPins(IN Device_Context *DC) 
{
	DWORD result = Error_NO_ERROR;

	deb_data("- Enter %s Function -\n", __func__);

	result = Demodulator_writeRegisterBits((Demodulator *)&DC->Demodulator, 0, Processor_LINK, p_reg_top_gpioh1_en, reg_top_gpioh1_en_pos, reg_top_gpioh1_en_len, 0);
	result = Demodulator_writeRegisterBits((Demodulator *)&DC->Demodulator, 0, Processor_LINK, p_reg_top_gpioh1_on, reg_top_gpioh1_on_pos, reg_top_gpioh1_on_len, 0);
	result = Demodulator_writeRegisterBits((Demodulator *)&DC->Demodulator, 0, Processor_LINK, p_reg_top_gpioh5_en, reg_top_gpioh5_en_pos, reg_top_gpioh5_en_len, 0);
	result = Demodulator_writeRegisterBits((Demodulator *)&DC->Demodulator, 0, Processor_LINK, p_reg_top_gpioh5_on, reg_top_gpioh5_on_pos, reg_top_gpioh5_on_len, 0);
	result = Demodulator_writeRegisterBits((Demodulator *)&DC->Demodulator, 0, Processor_LINK, p_reg_top_gpioh7_en, reg_top_gpioh7_en_pos, reg_top_gpioh7_en_len, 0);
	result = Demodulator_writeRegisterBits((Demodulator *)&DC->Demodulator, 0, Processor_LINK, p_reg_top_gpioh7_on, reg_top_gpioh7_on_pos, reg_top_gpioh7_on_len, 0);

	return result;
}

DWORD DRV_ApCtrl(
	Device_Context *DC,
	Byte chip_index,
	Bool bOn);

/* Device initialize */
DWORD DRV_Initialize(Device_Context *DC)
{
	DWORD result = Error_NO_ERROR;
	DWORD fileVersion = 0, cmdVersion = 0;
	Byte usb_dma_reg = 0, chip_index = 0;

	deb_data("- Enter %s Function -\n", __func__);

	if (DC->Demodulator.booted) {
		/* From Standard_setBusTuner() -> Standard_getFirmwareVersion() */
		result = DRV_getFirmwareVersionFromFile(DC, Processor_OFDM, &fileVersion);
		if (result) {
			deb_data("\t Error: getFirmwareVersionFromFile fail [0x%08lx]\n", result);
			goto exit;
		}

		/* Use "Command_QUERYINFO" to get firmware version */
		result = Demodulator_getFirmwareVersion((Demodulator *)&DC->Demodulator, Processor_OFDM, &cmdVersion);
		if (result) {
			deb_data("\t Error: getFirmwareVersion fail [0x%08lx]\n", result);
			goto exit;
		}

		if (cmdVersion != fileVersion) {
			deb_data("\t Warning: Outside_Fw = 0x%lX, Inside_Fw = 0x%lX, Reboot\n", cmdVersion, fileVersion);

			/* patch for 2 chips reboot */
			if (DC->Demodulator.chipNumber == 2) {
				result = DRV_DisableGPIOPins(DC);
				if (result) {
					deb_data("\t Error: DisableGPIOPins fail [0x%08lx]\n", result);
					goto exit;
				}
			}

			result = Demodulator_reboot((Demodulator *)&DC->Demodulator);
			if (result) {
				deb_data("\t Error: Reboot fail [0x%08lx]\n", result);
				goto exit;
			}
			else {
				result = Error_NOT_READY;
				goto exit;
			}
		}
		else {
			deb_data("\t Firmware version is the same, Fw = 0x%lX\n", cmdVersion);
			for (chip_index = 0 ; chip_index < DC->Demodulator.chipNumber; chip_index++) {
				result = DRV_ApCtrl(DC, chip_index, true);
			}
		}
	}

	/* ReInit: Patch for NIM fail or disappear, Maggie */
	result = Demodulator_initialize((Demodulator *)&DC->Demodulator, DC->Demodulator.chipNumber, 8000, DC->StreamType, DC->architecture);
	if (result) {
		deb_data("\t Error: ReInit fail [0x%08lx]\n", result);
		goto exit;
	}

	/*IT913x DownConverter_device_init here*/
	if(DC->board_id == DownConverter_ADRF6655 ||
			DC->board_id == DownConverter_RFFC2072 ||
			DC->board_id == DownConverter_RFFC2071 )
	{
		result = DL_DownConverter_device_init(DC);
		if(result) {
			deb_data("\t Error: DownConverter_device_init fail [0x%08lx]\n", result);
			result = -ENOMEM;
			goto exit;
		}
	}


	Demodulator_getFirmwareVersion((Demodulator *)&DC->Demodulator, Processor_OFDM, &cmdVersion);
	deb_data("\t FwVer OFDM = 0x%lX\n", cmdVersion);
	Demodulator_getFirmwareVersion((Demodulator *)&DC->Demodulator, Processor_LINK, &cmdVersion);
	deb_data("\t FwVer LINK = 0x%lX\n", cmdVersion);

	/* Patch for 0-byte USB packet error. write Link 0xDD8D[3] = 1 */
	for (chip_index = 0 ; chip_index < DC->Demodulator.chipNumber; chip_index++) {
		result = Demodulator_readRegister((Demodulator *)&DC->Demodulator, chip_index, Processor_LINK, 0xdd8d, &usb_dma_reg);
		usb_dma_reg |= 0x08; //reg_usb_min_len
		result = Demodulator_writeRegister((Demodulator *)&DC->Demodulator, chip_index, Processor_LINK, 0xdd8d, usb_dma_reg);
	}

	deb_data("- %s success -\n", __func__);
	return result;

exit:
	deb_data("- %s fail -\n", __func__);
	return result;
}

DWORD DRV_GetStatistic(
	IN Device_Context *DC,
	IN Byte chip,
	OUT Statistic *statistic)
{
	DWORD result = Error_NO_ERROR;

	//patch for MT2266
	if (DC->fc[chip].tunerinfo.bTunerOK == False) {
		(*statistic).signalPresented = False;
		(*statistic).signalLocked = False;
		(*statistic).signalQuality = 0;
		(*statistic).signalStrength = 0;
	}
	else {
		result = Demodulator_getStatistic((Demodulator *)&DC->Demodulator, chip, statistic);
	}

	return result;
}

/* Get init device information */
void DRV_InitDevInfo(
	Device_Context *DC,
	BYTE chip_index)
{
	DC->fc[chip_index].ulCurrentFrequency = 0;
	DC->fc[chip_index].ucCurrentBandWidth = 0;
	DC->fc[chip_index].ulDesiredFrequency = 0;
	DC->fc[chip_index].ucDesiredBandWidth = 6000;

	/* For PID Filter Setting */
	DC->fc[chip_index].tunerinfo.bTunerOK = false;
}

/* Get EEPROM_IRMODE/bIrTblDownload/bRAWIr/architecture config from EEPROM */
DWORD DRV_GetEEPROMConfig(Device_Context *DC)
{
	DWORD result = Error_NO_ERROR;
	BYTE chip_version = 0;
	DWORD chip_type = 0;
	BYTE var[2];
	Byte read_register = 0;
	Byte chip_index = 0;

	deb_data("- Enter %s Function -\n", __func__);

	/* Patch for read EEPROM valid bit */
	result = Demodulator_readRegister((Demodulator *)&DC->Demodulator, 0, Processor_LINK, chip_version_7_0, &chip_version);
	result = Demodulator_readRegisters((Demodulator *)&DC->Demodulator, 0, Processor_LINK, (chip_version_7_0 + 1), 2, var);

	if (result) {
		deb_data("\t Error: Read chip version fail [0x%08lx]\n", result);
		goto exit;
	}

	chip_type = var[1] << 8 | var[0];
	if (chip_type == 0x9135 && chip_version == 2) { /* Omega 2 */
		DC->chip_version = 2;
		result = Demodulator_readRegisters((Demodulator *)&DC->Demodulator, 0, Processor_LINK, 0x461d, 1, &read_register);
		deb_data("\t Chip Version = %d, valid bit = 0x%02X\n", chip_version, read_register);
	}
	else { /* Omega 1 */
		DC->chip_version = 1;
		result = Demodulator_readRegisters((Demodulator *)&DC->Demodulator, 0, Processor_LINK, 0x461b, 1, &read_register);
		deb_data("\t Chip Version = %d, valid bit = 0x%02X\n", chip_version, read_register);
	}

	if (result) {
		deb_data("\t Error: EEPROM valid bit read fail [0x%08lx]\n", result);
		goto exit;
	}

	if (read_register == 0) {
		deb_data("\t ========== No read EEPROM, using default ==========\n");
		deb_data("\t TSMode = DCA\n");
		DC->bIrTblDownload = false;
		DC->bProprietaryIr = false;
		DC->bDualTs = false;
		DC->architecture = Architecture_DCA;
		DC->Demodulator.chipNumber = 1;
		DC->bDCAPIP = false;
		DC->fc[0].tunerinfo.TunerId = 0x38;
	}
	else {
		deb_data("\t ========== Read EEPROM ==========\n");
		
		/* get board_id value from EEPROM: in order to know using DownConverter or not using DownConverter*/	
		read_register = 0;
		result = Demodulator_readRegister((Demodulator *)&DC->Demodulator, 0, Processor_LINK, EEPROM_BOARDID, &read_register);
		if (result) {
			deb_data("\t Error: Read Device ID fail [0x%08lx]\n", result);
			goto exit;
		}
		DC->board_id = read_register;
		deb_data("\t OMEGA Board ID = 0x%02lx\n",DC->board_id);


		result = Demodulator_readRegisters((Demodulator *)&DC->Demodulator, 0, Processor_LINK, EEPROM_IRMODE, 1, &read_register);
		if (result) {
			deb_data("\t Error: Read bIrTblDownload fail [0x%08lx]\n", result);
			goto exit;
		}

		/* IR mode option */
		DC->bIrTblDownload = read_register ? true : false;
		deb_data("\t EEPROM_IRMODE = 0x%02X\n", read_register);
		deb_data("\t bIrTblDownload = %s\n", DC->bIrTblDownload ? "ON" : "OFF");

		if (DC->bIrTblDownload) {
			DC->bProprietaryIr = (read_register == 0x05) ? true : false;
			deb_data("\t bRAWIr = %s\n", DC->bProprietaryIr ? "ON" : "OFF");
			if (DC->bProprietaryIr)
				deb_data("\t IT913x proprietary (raw) mode\n");
			else
				deb_data("\t IT913x HID (keyboard) mode\n");

			/* EE chose NEC RC5 RC6 threshhold table */
			result = Demodulator_readRegisters((Demodulator *)&DC->Demodulator, 0, Processor_LINK, EEPROM_IRTYPE, 1, &read_register);
			if (result) {
				deb_data("\t Error: Read bIrType fail [0x%08lx]\n", result);
				goto exit;
			}

			DC->bIrType = read_register;
			deb_data("\t bIrType = 0x%02X\n", DC->bIrType);
		}

		/* bDualTs option */
		DC->architecture = Architecture_DCA;
		DC->Demodulator.chipNumber = 1;
		DC->bDualTs = false;
		DC->bDCAPIP = false;

		result = Demodulator_readRegisters((Demodulator *)&DC->Demodulator, 0, Processor_LINK, EEPROM_TSMODE, 1, &read_register);
		if (result) {
			deb_data("\t Error: Read EEPROM_TSMODE fail [0x%08lx]\n", result);
			goto exit;
		}
		deb_data("\t EEPROM_TSMODE = 0x%02X\n", read_register);
		switch (read_register) {
		case 1:
			deb_data("\t TSMode = DCA + PIP\n");
			DC->architecture = Architecture_DCA;
			DC->Demodulator.chipNumber = 2;
			//DC->bDualTs = true;   //tmp not support PIP. If in DCA+PIP would use DCA mode behavior
			//DC->bDCAPIP = true;
			break;
		case 2:
			deb_data("\t TSMode = DCA\n");
			DC->architecture = Architecture_DCA;
			DC->Demodulator.chipNumber = 2;
			break;
		case 3:
			deb_data("\t TSMode = PIP\n");
			DC->architecture = Architecture_PIP;
			DC->Demodulator.chipNumber = 2;
			DC->bDualTs = true;
			break;
		case 0:
		default:
			deb_data("\t TSMode = TS1\n");
			break;
		}

		/* TunerID option */
		result = Demodulator_readRegisters((Demodulator *)&DC->Demodulator, 0, Processor_LINK, EEPROM_TUNERID, 1, &read_register);
		switch (read_register) {
		case 0x51:
		case 0x52:
		case 0x60:
		case 0x61:
		case 0x62:
		case 0x63:
		case 0x64:
		case 0x65:
			DC->fc[0].tunerinfo.TunerId = read_register;
			break;
		default:
			deb_data("\t Warning: exception TunerId [0x%x], using default [0x38]\n", read_register);
		case 0x38:
			DC->fc[0].tunerinfo.TunerId = 0x38;
			break;
		}

		deb_data("\t fc[0].TunerId = 0x%x\n", DC->fc[0].tunerinfo.TunerId);
		if (DC->bDualTs) {
			DC->fc[1].tunerinfo.TunerId = DC->fc[0].tunerinfo.TunerId;
			deb_data("\t fc[1].TunerId = 0x%x\n", DC->fc[1].tunerinfo.TunerId);
		}

		result = Demodulator_readRegisters((Demodulator *)&DC->Demodulator, 0, Processor_LINK, EEPROM_SUSPEND, 1, &read_register);
		if (result) {
			deb_data("\t Error: Read EEPROM_SUSPEND fail [0x%08lx]\n", result);
			goto exit;
		}
		deb_data("\t EEPROM susped mode = %d\n", read_register);

		result = Demodulator_readRegisters((Demodulator *)&DC->Demodulator, 0, Processor_LINK, 
				EEPROM_DCA_TIME_FLAG, 1, &read_register);
		if (result) {
			deb_data("\t Error: Read EEPROM_DCA_TIME_FLAG fail [0x%08lx]\n", result);
			goto exit;
		}
		deb_data("\t EEPROM DCA Time Flag = %d\n", read_register);
		DC->DCA_time_info.enable_flag = read_register;
		if(DC->DCA_time_info.enable_flag)
		{
			result = Demodulator_readRegisters((Demodulator *)&DC->Demodulator, 0, Processor_LINK, 
					EEPROM_DCA_MA_TIME, 1, &read_register);
			if (result) {
				deb_data("\t Error: Read EEPROM_DCA_MA_TIME fail [0x%08lx]\n", result);
				goto exit;
			}

			deb_data("\t EEPROM DCA Master time= %d\n", read_register);
			DC->DCA_time_info.Master_time = read_register;
			result = Demodulator_readRegisters((Demodulator *)&DC->Demodulator, 0, Processor_LINK, 
					EEPROM_DCA_SLA_TIME, 1, &read_register);
			deb_data("\t EEPROM DCA Slave time = %d\n", read_register);
			DC->DCA_time_info.Slave_time = read_register;
		}
		else
		{
			DC->DCA_time_info.Master_time = DC->DCA_time_info.Slave_time = 0x44;
			deb_data("\t EEPROM DCA Master time= %d\n", DC->DCA_time_info.Master_time);
			deb_data("\t EEPROM DCA Slave time = %d\n", DC->DCA_time_info.Slave_time);
		}
	}

	/* Init some device info */
	for (chip_index = 0; chip_index <= (BYTE)DC->bDualTs; chip_index++)
		DRV_InitDevInfo(DC, chip_index);

	deb_data("- %s success -\n", __func__);
	return result;

exit:
	deb_data("- %s fail -\n", __func__);
	return result;
}

/* Set Bus Tuner */
DWORD DRV_SetBusTuner(
	Device_Context *DC,
	Word busId,
	Word tunerId)
{
	DWORD result = Error_NO_ERROR;
	DWORD version = 0;

	//deb_data("- Enter %s Function -\n", __func__);
	//deb_data("\t Set busId = 0x%x, tunerId = 0x%x\n", busId, tunerId);

	if ((DC->UsbMode == 0x0110) && (busId == Bus_USB))
		busId = Bus_USB11;

	result = Demodulator_setBusTuner((Demodulator *)&DC->Demodulator, busId, tunerId);
	if (result) {
		deb_data("\t Error: Demodulator_setBusTuner fail [0x%08lx]\n", result);
		goto exit;
	}

	result = Demodulator_getFirmwareVersion((Demodulator *)&DC->Demodulator, Processor_LINK, &version);
	if (version != 0)
		DC->Demodulator.booted = True;
	else
		DC->Demodulator.booted = False;

	if (result) {
		deb_data("\t Error: Demodulator_getFirmwareVersion fail [0x%08lx]\n", result);
		goto exit;
	}

exit:
	return result;
}

/* Set tuner power control */
DWORD DRV_TunerPowerCtrl(
	Device_Context *DC,
	BYTE chip_index,
	Bool bPowerOn)
{
	DWORD result = Error_NO_ERROR;

	deb_data("- Enter %s Function, chip[%d] -\n", __func__, chip_index);

	/* Omega has no GPIOH7 */
	if (bPowerOn) { /* Power on */
		result = Demodulator_writeRegisterBits((Demodulator *)&DC->Demodulator, chip_index, Processor_OFDM, p_reg_p_if_en, reg_p_if_en_pos, reg_p_if_en_len, 0x01);
		msleep(50);
	}
	else { /* Power down */
		result = Demodulator_writeRegisterBits((Demodulator *)&DC->Demodulator, chip_index, Processor_OFDM, p_reg_p_if_en, reg_p_if_en_pos, reg_p_if_en_len, 0x00);

		result = Demodulator_writeRegister((Demodulator *)&DC->Demodulator, chip_index, Processor_OFDM, 0xEC02, 0x3F);
		result = Demodulator_writeRegister((Demodulator *)&DC->Demodulator, chip_index, Processor_OFDM, 0xEC03, 0x1F);//DCX, 110m
		result = Demodulator_writeRegister((Demodulator *)&DC->Demodulator, chip_index, Processor_OFDM, 0xEC04, 0x3F);//73m, 46m
		result = Demodulator_writeRegister((Demodulator *)&DC->Demodulator, chip_index, Processor_OFDM, 0xEC05, 0x3F);//57m, 36m

		result = Demodulator_writeRegister((Demodulator *)&DC->Demodulator, chip_index, Processor_OFDM, 0xEC3F, 0x01);
	}

	return result;
}

DWORD DRV_TunerWakeup(Device_Context *DC)
{
	DWORD result = Error_NO_ERROR;

	deb_data("- Enter %s Function -\n", __func__);

	/* Tuner power on */
	result = Demodulator_writeRegisterBits((Demodulator *)&DC->Demodulator, 0, Processor_LINK, p_reg_top_gpioh7_o, reg_top_gpioh7_o_pos, reg_top_gpioh7_o_len, 1);

	return result;
}

DWORD DRV_Reboot(Device_Context *DC)
{
	DWORD result = Error_NO_ERROR;

	deb_data("- Enter %s Function -\n", __func__);

	result = Demodulator_reboot((Demodulator *)&DC->Demodulator);

	return result;
}

DWORD DRV_USBSetup(Device_Context *DC)
{
	DWORD result = Error_NO_ERROR;
	Byte chip_index = 0;

	deb_data("- Enter %s Function -\n", __func__);

	if (DC->Demodulator.chipNumber == 2) {
		//timing
		for (chip_index = 0; chip_index < 2; chip_index++) {
			result = Demodulator_writeRegisterBits((Demodulator *)&DC->Demodulator, chip_index, Processor_OFDM, p_reg_dca_fpga_latch, reg_dca_fpga_latch_pos, reg_dca_fpga_latch_len, 0x66);
			if (result)
				goto exit;

			result = Demodulator_writeRegisterBits((Demodulator *)&DC->Demodulator, chip_index, Processor_OFDM, p_reg_dca_platch, reg_dca_platch_pos, reg_dca_platch_len, 1);
			if (result)
				goto exit;
		}
	}

	deb_data("- %s success -\n", __func__);
	return result;

exit:
	deb_data("- %s fail -\n", __func__);
	return result;
}

/* Control GPIOH5, bSuspend: true(sleep), false(resume) */
DWORD DRV_NIMSuspend(
	Device_Context *DC,
	bool bSuspend)
{
	DWORD result = Error_NO_ERROR;

	if (DC->Demodulator.chipNumber == 1)
		return Error_NO_ERROR;

	deb_data("- Enter %s Function : Suspend = %s -\n", __func__, bSuspend ? "ON" : "OFF");

	if (bSuspend) { /* Sleep */
		result = Demodulator_writeRegisterBits((Demodulator *)&DC->Demodulator, 0, Processor_LINK, p_reg_top_gpioh5_o, reg_top_gpioh5_o_pos, reg_top_gpioh5_o_len, 1);
		if (result)
			return result;

		msleep(50);
	}
	else { /* Resume */
		result = Demodulator_writeRegisterBits((Demodulator *)&DC->Demodulator, 0, Processor_LINK, p_reg_top_gpioh5_o, reg_top_gpioh5_o_pos, reg_top_gpioh5_o_len, 0);
		if (result)
			return result;

		msleep(100);
	}

	return result;
}

DWORD DRV_InitNIMSuspendRegs(Device_Context *DC)
{
	DWORD result = Error_NO_ERROR;

	deb_data("- Enter %s Function -\n", __func__);

	result = Demodulator_writeRegisterBits((Demodulator *)&DC->Demodulator, 0, Processor_LINK, p_reg_top_gpioh5_en, reg_top_gpioh5_en_pos, reg_top_gpioh5_en_len, 1);
	result = Demodulator_writeRegisterBits((Demodulator *)&DC->Demodulator, 0, Processor_LINK, p_reg_top_gpioh5_on, reg_top_gpioh5_on_pos, reg_top_gpioh5_on_len, 1);
	result = Demodulator_writeRegisterBits((Demodulator *)&DC->Demodulator, 0, Processor_LINK, p_reg_top_gpioh5_o, reg_top_gpioh5_o_pos, reg_top_gpioh5_o_len, 0);
	msleep(10);

	result = Demodulator_writeRegisterBits((Demodulator *)&DC->Demodulator, 1, Processor_LINK, p_reg_top_pwrdw, reg_top_pwrdw_pos, reg_top_pwrdw_len, 1);
	result = Demodulator_writeRegisterBits((Demodulator *)&DC->Demodulator, 1, Processor_LINK, p_reg_top_pwrdw_hwen, reg_top_pwrdw_hwen_pos, reg_top_pwrdw_hwen_len, 1);

	return result;
}

DWORD DRV_NIMReset(Device_Context *DC)
{
	DWORD result = Error_NO_ERROR;

	deb_data("- Enter %s Function -\n", __func__);

	/* Set AF0350 GPIOH1 to 0 to reset AF0351 */
	result = Demodulator_writeRegisterBits((Demodulator *)&DC->Demodulator, 0, Processor_LINK, p_reg_top_gpioh1_en, reg_top_gpioh1_en_pos, reg_top_gpioh1_en_len, 1);
	result = Demodulator_writeRegisterBits((Demodulator *)&DC->Demodulator, 0, Processor_LINK, p_reg_top_gpioh1_on, reg_top_gpioh1_on_pos, reg_top_gpioh1_on_len, 1);
	result = Demodulator_writeRegisterBits((Demodulator *)&DC->Demodulator, 0, Processor_LINK, p_reg_top_gpioh1_o, reg_top_gpioh1_o_pos, reg_top_gpioh1_o_len, 0);

	msleep(50);

	result = Demodulator_writeRegisterBits((Demodulator *)&DC->Demodulator, 0, Processor_LINK, p_reg_top_gpioh1_o, reg_top_gpioh1_o_pos, reg_top_gpioh1_o_len, 1);

	return result;
}

DWORD DRV_DummyCmd(Device_Context *DC)
{
	int count = 0;
	DWORD result = Error_NO_ERROR;

	//deb_data("- Enter %s Function -\n", __func__);

	for (count = 0; count < 5 ; count++) {
		//deb_data("%s: patch for KJS/EEPC = %d\n", __func__, count);
		result = DRV_SetBusTuner(DC, Bus_USB, Tuner_Afatech_OMEGA);
		msleep(1);
	}

	return result;
}

DWORD NIM_ResetSeq(Device_Context *DC)
{
	DWORD result = Error_NO_ERROR;
	BYTE bootbuffer[6];
	bootbuffer[0] = 0x05;
	bootbuffer[1] = 0x00;
	bootbuffer[2] = 0x23;
	bootbuffer[3] = 0x01;
	bootbuffer[4] = 0xFE;
	bootbuffer[5] = 0xDC;

	/* Reset 9133 -> boot -> demod init */

	//GPIOH5 init
	result = Demodulator_writeRegisterBits((Demodulator *)&DC->Demodulator, 0, Processor_LINK, p_reg_top_gpioh5_en, reg_top_gpioh5_en_pos, reg_top_gpioh5_en_len, 0);
	result = Demodulator_writeRegisterBits((Demodulator *)&DC->Demodulator, 0, Processor_LINK, p_reg_top_gpioh5_on, reg_top_gpioh5_on_pos, reg_top_gpioh5_on_len, 0);

	//result = Demodulator_writeRegisterBits((Demodulator*)&DC->Demodulator, 0, Processor_LINK, p_reg_top_gpioh5_o, reg_top_gpioh5_o_pos, reg_top_gpioh5_o_len, 0);
	//mdelay(100);

	result = DRV_NIMReset(DC);
	result = DRV_InitNIMSuspendRegs(DC);
	result = Demodulator_writeGenericRegisters ((Demodulator *)&DC->Demodulator, 0, 0x01, 0x3a, 0x06, bootbuffer);
	result = Demodulator_readGenericRegisters ((Demodulator *)&DC->Demodulator, 0, 0x01, 0x3a, 0x05, bootbuffer);

	msleep(50); //Delay for Fw boot

	/* Demod & Tuner init */
	result = DRV_Initialize(DC);

	return result;
}

/* Set tuner power saving and control */
DWORD DRV_ApCtrl(
	Device_Context *DC,
	Byte chip_index,
	Bool bOn)
{
	DWORD result = Error_NO_ERROR;
	int count;
	Dword version = 0;

	//deb_data("- Enter %s Function -\n", __func__);

	/* clock from tuner, so close sequence demod->tuner, and 9133->9137. vice versa. */
	/* BUT! demod->tuner:57mADC, tuner->demod:37mADC */

	if (bOn) { /* Power on */
		if (chip_index == 1) { /* chip[1] */
			switch (DC->chip_version) {
			case 1: /* Omega 1 */
				result = NIM_ResetSeq(DC);
				if (result)
					NIM_ResetSeq(DC);

				break;
			case 2: /* Omega 2 */
				result = DRV_NIMSuspend(DC, false);
				if (result) {
					deb_data("\t Error: GPIOH5 ON fail [0x%08lx]\n", result);
					goto exit;
				}
				break;
			default:
				deb_data("\t Error: chip version not defined [%d]\n", DC->chip_version);
				goto exit;
				break;
			}
		}
		else { /* chip[0] */
			/* DummyCmd */
			for (count = 0; count < 5 ;count++) {
				result = Demodulator_getFirmwareVersion((Demodulator *)&DC->Demodulator, Processor_LINK, &version);
				msleep(1);
			}
		}

		result = Demodulator_controlTunerPowerSaving((Demodulator *)&DC->Demodulator, chip_index, bOn);
		if (result) {
			deb_data("\t Error: chip[%d] Demodulator_controlTunerPowerSaving fail [0x%08lx]\n", chip_index, result);
			goto exit;
		}

		result = Demodulator_controlPowerSaving((Demodulator *)&DC->Demodulator, chip_index, bOn);
		if (result) {
			deb_data("\t Error: chip[%d] Demodulator_controlPowerSaving fail [0x%08lx]\n", chip_index, result);
			goto exit;
		}
	}
	else { /* Power down */
		/* DCA DUT: 36(all) -> 47-159(no GPIOH5, sequence change) */
		if ((chip_index == 0) && (DC->Demodulator.chipNumber == 2)) {
			result = Demodulator_controlTunerLeakage((Demodulator *)&DC->Demodulator, 1, bOn);
			if (result) {
				deb_data("\t chip[1] Demodulator_controlTunerLeakage fail [0x%08lx]\n", result);
				goto exit;
			}

			result = DRV_NIMSuspend(DC, true);
			if (result) {
				deb_data("\t Error: GPIOH5 OFF fail [0x%08lx]\n", result);
				goto exit;
			}
		}

		result = Demodulator_controlPowerSaving((Demodulator *)&DC->Demodulator, chip_index, bOn);
		if (result) {
			deb_data("\t Error: chip[%d] Demodulator_controlPowerSaving fail [0x%08lx]\n", chip_index, result);
			goto exit;
		}

		result = Demodulator_controlTunerPowerSaving((Demodulator *)&DC->Demodulator, chip_index, bOn);
		if (result) {
			deb_data("\t Error: chip[%d] Demodulator_controlTunerPowerSaving fail [0x%08lx]\n", chip_index, result);
			goto exit;
		}
		
		DC->fc[chip_index].ulCurrentFrequency = 0;
		DC->fc[chip_index].ucCurrentBandWidth = 0;
		DC->fc[chip_index].ulDesiredFrequency = 0;
		DC->fc[chip_index].ucDesiredBandWidth = 0;
	}
	
	deb_data("- chip[%d] set POWER %s success -\n", chip_index, bOn ? "ON" : "DOWN");
	return result;

exit:
	deb_data("- chip[%d] set POWER %s fail -\n",chip_index, bOn ? "ON" : "DOWN");
	return result;
}

DWORD DRV_ApReset(
	Device_Context *DC,
	Byte chip_index,
	Bool bOn)
{
	DWORD result = Error_NO_ERROR;

	deb_data("- Enter %s Function -\n", __func__);

	DC->fc[chip_index].ulCurrentFrequency = 0;
	DC->fc[chip_index].ucCurrentBandWidth = 6000; //for BDAUtil

	result = DRV_ApCtrl(DC, chip_index, bOn);
	if (result)
		deb_data("\t Error: chip[%d] DRV_ApCtrl fail [0x%08lx]\n", chip_index, result);

	return result;
}

DWORD DRV_LoadDesFreq(
	IN Device_Context *DC,
	IN BYTE chip_index)
{
	DWORD result = Error_NO_ERROR;
	DWORD dwFreq = DC->fc[chip_index].ulDesiredFrequency;
	WORD ucBw = DC->fc[chip_index].ucDesiredBandWidth;

	deb_data("- Enter %s Function -\n", __func__);

	result = DRV_SetFreqBw(DC, chip_index, dwFreq, ucBw);
	return result;
}

DWORD DRV_WriteEeprom(
	IN Device_Context *DC,
	IN BYTE index,
	IN BYTE value)
{
	DWORD result = Error_NO_ERROR;

	deb_data("- Enter %s Function, index = %02x, value = %02x -\n", __func__, index, value);

	result = Demodulator_writeEepromValues((Demodulator *)&DC->Demodulator, 0, index, 1, &value);

	return result;
}

DWORD DRV_ReadEeprom(
	IN Device_Context *DC,
	IN BYTE index,
	IN BYTE *value)
{
	DWORD result = Error_NO_ERROR;

	deb_data("- Enter %s Function -\n", __func__);

	result = Demodulator_readEepromValues((Demodulator *)&DC->Demodulator, 0, index, 1, value);

	return result;
}

DWORD DRV_TunerSuspend(
	IN Device_Context *DC,
	IN BYTE ucChip,
	IN bool bOn)
{
	DWORD result = Error_NO_ERROR;

	deb_data("- Enter %s Function -\n", __func__);

	if (bOn) {
		/* EC40 */
		result = DRV_WriteRegisterBits(DC, ucChip, Processor_OFDM, p_reg_p_if_en, reg_p_if_en_pos, reg_p_if_en_len, 0);
		if (result)
			goto exit;

		/* current = 190,	EC02~EC0F */
		result = DRV_WriteRegisterBits(DC, ucChip, Processor_OFDM, p_reg_pd_a, reg_pd_a_pos, reg_pd_a_len, 0);
		result = DRV_WriteRegister(DC, ucChip, Processor_OFDM, 0xEC03, 0x0C);
		result = DRV_WriteRegisterBits(DC, ucChip, Processor_OFDM, p_reg_pd_c, reg_pd_c_pos, reg_pd_c_len, 0);
		result = DRV_WriteRegisterBits(DC, ucChip, Processor_OFDM, p_reg_pd_d, reg_pd_d_pos, reg_pd_d_len, 0);
		result = DRV_WriteRegisterBits(DC, ucChip, Processor_OFDM, p_reg_tst_a, reg_tst_a_pos, reg_tst_a_len, 0);
		result = DRV_WriteRegisterBits(DC, ucChip, Processor_OFDM, p_reg_tst_b, reg_tst_b_pos, reg_tst_b_len, 0);
		if (result)
			goto exit;

		/* current = 172 ,KEY: p_reg_ctrl_a: 0 fail/ 7x/ 1x/ 2x/ 3x/ 4x/ 5/ 6 */
		result = DRV_WriteRegisterBits(DC, ucChip, Processor_OFDM, p_reg_ctrl_a, reg_ctrl_a_pos, reg_ctrl_a_len, 0);
		result = DRV_WriteRegisterBits(DC, ucChip, Processor_OFDM, p_reg_ctrl_b, reg_ctrl_b_pos, reg_ctrl_b_len, 0);
		result = DRV_WriteRegisterBits(DC, ucChip, Processor_OFDM, p_reg_cal_freq_a, reg_cal_freq_a_pos, reg_cal_freq_a_len, 0);
		result = DRV_WriteRegisterBits(DC, ucChip, Processor_OFDM, p_reg_cal_freq_b, reg_cal_freq_b_pos, reg_cal_freq_b_len, 0);
		result = DRV_WriteRegisterBits(DC, ucChip, Processor_OFDM, p_reg_cal_freq_c, reg_cal_freq_c_pos, reg_cal_freq_c_len, 0);
		result = DRV_WriteRegisterBits(DC, ucChip, Processor_OFDM, p_reg_lo_freq_a, reg_lo_freq_a_pos, reg_lo_freq_a_len, 0);
		result = DRV_WriteRegisterBits(DC, ucChip, Processor_OFDM, p_reg_lo_freq_b, reg_lo_freq_b_pos, reg_lo_freq_b_len, 0);
		result = DRV_WriteRegisterBits(DC, ucChip, Processor_OFDM, p_reg_lo_freq_c, reg_lo_freq_c_pos, reg_lo_freq_c_len, 0);
		if (result)
			goto exit;

		/* current = 139, EC10~EC15 */
		result = DRV_WriteRegisterBits(DC, ucChip, Processor_OFDM, p_reg_lo_cap, reg_lo_cap_pos, reg_lo_cap_len, 0);
		result = DRV_WriteRegisterBits(DC, ucChip, Processor_OFDM, p_reg_clk02_select, reg_clk02_select_pos, reg_clk02_select_len, 0);
		result = DRV_WriteRegisterBits(DC, ucChip, Processor_OFDM, p_reg_clk01_select, reg_clk01_select_pos, reg_clk01_select_len, 0);
		result = DRV_WriteRegisterBits(DC, ucChip, Processor_OFDM, p_reg_lna_g, reg_lna_g_pos, reg_lna_g_len, 0);
		result = DRV_WriteRegisterBits(DC, ucChip, Processor_OFDM, p_reg_lna_cap, reg_lna_cap_pos, reg_lna_cap_len, 0);
		result = DRV_WriteRegisterBits(DC, ucChip, Processor_OFDM, p_reg_lna_band, reg_lna_band_pos, reg_lna_band_len, 0);
		result = DRV_WriteRegisterBits(DC, ucChip, Processor_OFDM, p_reg_pga, reg_pga_pos, reg_pga_len, 0);
		if (result)
			goto exit;

		/* current=119, EC17 -> EC1F */
		result = DRV_WriteRegisterBits(DC, ucChip, Processor_OFDM, p_reg_pgc, reg_pgc_pos, reg_pgc_len, 0);
		result = DRV_WriteRegisterBits(DC, ucChip, Processor_OFDM, p_reg_lpf_cap, reg_lpf_cap_pos, reg_lpf_cap_len, 0);
		result = DRV_WriteRegisterBits(DC, ucChip, Processor_OFDM, p_reg_lpf_bw, reg_lpf_bw_pos, reg_lpf_bw_len, 0);
		result = DRV_WriteRegisterBits(DC, ucChip, Processor_OFDM, p_reg_ofsi, reg_ofsi_pos, reg_ofsi_len, 0);
		result = DRV_WriteRegisterBits(DC, ucChip, Processor_OFDM, p_reg_ofsq, reg_ofsq_pos, reg_ofsq_len, 0);
		result = DRV_WriteRegisterBits(DC, ucChip, Processor_OFDM, p_reg_dcxo_a, reg_dcxo_a_pos, reg_dcxo_a_len, 0);
		result = DRV_WriteRegisterBits(DC, ucChip, Processor_OFDM, p_reg_dcxo_b, reg_dcxo_b_pos, reg_dcxo_b_len, 0);
		result = DRV_WriteRegisterBits(DC, ucChip, Processor_OFDM, p_reg_tddo, reg_tddo_pos, reg_tddo_len, 0);
		result = DRV_WriteRegisterBits(DC, ucChip, Processor_OFDM, p_reg_strength_setting, reg_strength_setting_pos, reg_strength_setting_len, 0);
		if (result)
			goto exit;

		/* EC22~EC2B */
		result = DRV_WriteRegisterBits(DC, ucChip, Processor_OFDM, p_reg_gi, reg_gi_pos, reg_gi_len, 0);
		result = DRV_WriteRegisterBits(DC, ucChip, Processor_OFDM, p_reg_clk_del_sel, reg_clk_del_sel_pos, reg_clk_del_sel_len, 0);
		result = DRV_WriteRegisterBits(DC, ucChip, Processor_OFDM, p_reg_p2s_ck_sel, reg_p2s_ck_sel_pos, reg_p2s_ck_sel_len, 0);
		result = DRV_WriteRegisterBits(DC, ucChip, Processor_OFDM, p_reg_rssi_sel, reg_rssi_sel_pos, reg_rssi_sel_len, 0);
		result = DRV_WriteRegisterBits(DC, ucChip, Processor_OFDM, p_reg_tst_sel, reg_tst_sel_pos, reg_tst_sel_len, 0);
		result = DRV_WriteRegisterBits(DC, ucChip, Processor_OFDM, p_reg_ctrl_c, reg_ctrl_c_pos, reg_ctrl_c_len, 0);
		result = DRV_WriteRegisterBits(DC, ucChip, Processor_OFDM, p_reg_ctrl_d, reg_ctrl_d_pos, reg_ctrl_d_len, 0);
		result = DRV_WriteRegisterBits(DC, ucChip, Processor_OFDM, p_reg_ctrl_e, reg_ctrl_e_pos, reg_ctrl_e_len, 0);
		result = DRV_WriteRegisterBits(DC, ucChip, Processor_OFDM, p_reg_ctrl_f, reg_ctrl_f_pos, reg_ctrl_f_len, 0);
		result = DRV_WriteRegisterBits(DC, ucChip, Processor_OFDM, p_reg_lo_bias, reg_lo_bias_pos, reg_lo_bias_len, 0);
		result = DRV_WriteRegisterBits(DC, ucChip, Processor_OFDM, p_reg_ext_lna_en, reg_ext_lna_en_pos, reg_ext_lna_en_len, 0);
		result = DRV_WriteRegisterBits(DC, ucChip, Processor_OFDM, p_reg_pga_bak, reg_pga_bak_pos, reg_pga_bak_len, 0);
		result = DRV_WriteRegisterBits(DC, ucChip, Processor_OFDM, p_reg_cpll_cap, reg_cpll_cap_pos, reg_cpll_cap_len, 0);
		if (result)
			goto exit;

		/* EC20 */
		result = DRV_WriteRegister(DC, ucChip, Processor_OFDM, 0xEC20, 0x00);
		if (result)
			goto exit;

		/* current = 116 */
		result = DRV_WriteRegister(DC, ucChip, Processor_OFDM, 0xEC3F, 0x01);
		if (result)
			goto exit;
	}
	else {
		result = DRV_WriteRegisterBits(DC, ucChip, Processor_OFDM, p_reg_p_if_en, reg_p_if_en_pos, reg_p_if_en_len, 1);
		msleep(50);
	}

	deb_data("- %s success -\n", __func__);
	return result;

exit:
	deb_data("- %s fail -\n", __func__);
	return result;
}

DWORD DRV_GPIOCtrl(
	IN Device_Context *DC,
	IN BYTE ucIndex,
	IN bool bHigh)
{
	DWORD result = Error_NO_ERROR;

	deb_data("- Enter %s Function, index = %d, High = %d -\n", __func__, ucIndex, bHigh);

	//output
	result = Demodulator_writeRegisterBits((Demodulator *)&DC->Demodulator, 0, Processor_LINK, p_reg_top_gpioh3_en, reg_top_gpioh3_en_pos, reg_top_gpioh3_en_len, 1);
	result = Demodulator_writeRegisterBits((Demodulator *)&DC->Demodulator, 0, Processor_LINK, p_reg_top_gpioh3_on, reg_top_gpioh3_on_pos, reg_top_gpioh3_on_len, 1);
	//control
	result = Demodulator_writeRegisterBits((Demodulator *)&DC->Demodulator, 0, Processor_LINK, p_reg_top_gpioh3_o, reg_top_gpioh3_o_pos, reg_top_gpioh3_o_len, 0);

	return result;
}

DWORD DRV_ReadRawIR(
	Device_Context *DC,
	DWORD *ucpReadBuf)
{
	DWORD result = Error_NO_ERROR;

	//deb_data("- Enter %s Function -\n", __func__);

	if (DC->bProprietaryIr)
		result = Demodulator_getIrCode((Demodulator *)&DC->Demodulator, ucpReadBuf);
	else
		deb_data("\t Warning: RawIR can't be used\n");

	return result;
}

DWORD DRV_getSNR(
	Device_Context *DC,
	BYTE chip_index,
	Constellation *constellation,
	BYTE *snr)
{
	DWORD result = Error_NO_ERROR;
	ChannelModulation channelModulation;

	//deb_data("- Enter %s Function -\n", __func__);

	result = DRV_getChannelModulation(DC, chip_index, &channelModulation);
	if (result) {
		deb_data("\t Error: DRV_getChannelModulation fail [0x%08lx]\n", result);
		goto exit;
	}

	*constellation = channelModulation.constellation;

	result = Demodulator_getSNR((Demodulator *)&DC->Demodulator, chip_index, snr);
	if (result) {
		deb_data("\t Error: Demodulator_getSNR fail [0x%08lx]\n", result);
		goto exit;
	}

	//deb_data("- %s success -\n", __func__);
	return result;

exit:
	deb_data("- %s fail -\n", __func__);
	return result;
}

/**************************** DL ****************************/
DWORD DL_WriteRegisters(
	IN Device_Context *DC,
	IN BYTE chip_index,
	IN Processor processor,
	IN DWORD wReg,
	IN BYTE ucpValueLength,
	IN BYTE *ucpValue)
{
	DWORD result = Error_NO_ERROR;

	mutex_lock(&DC->dev_mutex);
		result = DRV_WriteRegisters(DC, chip_index, processor, wReg, ucpValueLength, ucpValue);
	mutex_unlock(&DC->dev_mutex);

	return result;
}

DWORD DL_ReadRegister(
	IN Device_Context *DC,
	IN BYTE chip_index,
	IN Processor processor,
	IN DWORD wReg,
	OUT BYTE *ucpValue)
{
	DWORD result = Error_NO_ERROR;

	mutex_lock(&DC->dev_mutex);
		result = DRV_ReadRegister(DC, chip_index, processor, wReg, ucpValue);
	mutex_unlock(&DC->dev_mutex);

	return result;
}

//EE
DWORD DL_ReadRegisters(
	IN Device_Context *DC,
	IN BYTE chip_index,
	IN Processor processor,
	IN DWORD wReg,
	IN BYTE ucpValueLength,
	OUT BYTE *ucpValue)
{
	DWORD result = Error_NO_ERROR;

	mutex_lock(&DC->dev_mutex);
		result = DRV_ReadRegisters(DC, chip_index, processor, wReg, ucpValueLength, ucpValue);
	mutex_unlock(&DC->dev_mutex);

	return result;
}

DWORD DL_DummyCmd(IN Device_Context *DC)
{
	DWORD result = Error_NO_ERROR;

	mutex_lock(&DC->dev_mutex);
		result = DRV_DummyCmd(DC);
	mutex_unlock(&DC->dev_mutex);

	return result;
}

DWORD DL_TunerSuspend(
	IN Device_Context *DC,
	IN BYTE ucChip,
	IN bool bOn)
{
	DWORD result = Error_NO_ERROR;

	mutex_lock(&DC->dev_mutex);
		result = DRV_TunerSuspend(DC, ucChip, bOn);
	mutex_unlock(&DC->dev_mutex);

	return result;
}

DWORD DL_GPIOCtrl(
	IN Device_Context *DC,
	IN BYTE ucIndex,
	IN bool bHigh)
{
	DWORD result = Error_NO_ERROR;

	mutex_lock(&DC->dev_mutex);
		result = DRV_GPIOCtrl(DC, ucIndex, bHigh);
	mutex_unlock(&DC->dev_mutex);

	return result;
}

DWORD DL_NIMReset(Device_Context *DC)
{
	DWORD result = Error_NO_ERROR;

	mutex_lock(&DC->dev_mutex);
		result = DRV_NIMReset(DC);
	mutex_unlock(&DC->dev_mutex);

	return result;
}

DWORD DL_USBSetup(Device_Context *DC)
{
	DWORD result = Error_NO_ERROR;

	mutex_lock(&DC->dev_mutex);
		result = DRV_USBSetup(DC);
	mutex_unlock(&DC->dev_mutex);

	return result;
}

DWORD DL_TunerWakeup(Device_Context *DC)
{
	DWORD result = Error_NO_ERROR;

	mutex_lock(&DC->dev_mutex);
		result = DRV_TunerWakeup(DC);
	mutex_unlock(&DC->dev_mutex);

	return result;
}

DWORD DL_NIMSuspend(
	Device_Context *DC,
	bool bSuspend)
{
	DWORD result = Error_NO_ERROR;

	mutex_lock(&DC->dev_mutex);
		result = DRV_NIMSuspend(DC, bSuspend);
	mutex_unlock(&DC->dev_mutex);

	return result;
}

DWORD DL_InitNIMSuspendRegs(Device_Context *DC)
{
	DWORD result = Error_NO_ERROR;

	mutex_lock(&DC->dev_mutex);
		result = DRV_InitNIMSuspendRegs(DC);
	mutex_unlock(&DC->dev_mutex);

	return result;
}

DWORD DL_Initialize(Device_Context *DC)
{
	DWORD result = Error_NO_ERROR;

	mutex_lock(&DC->dev_mutex);
		result = DRV_Initialize(DC);
	mutex_unlock(&DC->dev_mutex);

	return result;
}

DWORD DL_SetBusTuner(
	Device_Context *DC,
	Word busId,
	Word tunerId)
{
	DWORD result = Error_NO_ERROR;

	mutex_lock(&DC->dev_mutex);
		result = DRV_SetBusTuner(DC, busId, tunerId);
	mutex_unlock(&DC->dev_mutex);

	return result;
}

DWORD DL_GetEEPROMConfig(Device_Context *DC)
{
	DWORD result = Error_NO_ERROR;

	mutex_lock(&DC->dev_mutex);
		result = DRV_GetEEPROMConfig(DC);
	mutex_unlock(&DC->dev_mutex);

	return result;
}

/* Load Ir Table */
DWORD DL_IrTblDownload(Device_Context *DC)
{
	DWORD result = Error_NO_ERROR;

	mutex_lock(&DC->dev_mutex);
		result = DRV_IrTblDownload(DC);
	mutex_unlock(&DC->dev_mutex);

	return result;
}

DWORD DL_TunerPowerCtrl(
	Device_Context *DC,
	u8 bPowerOn)
{
	DWORD result = Error_NO_ERROR;
	BYTE chip_index = 0;

	deb_data("- Enter %s Function, POWER %s -\n", __func__, bPowerOn ? "ON" : "OFF");

	for (chip_index = 0; chip_index < DC->Demodulator.chipNumber; chip_index++) {
		mutex_lock(&DC->dev_mutex);
			result = DRV_TunerPowerCtrl(DC, chip_index, bPowerOn);
		mutex_unlock(&DC->dev_mutex);
		if (result) {
			deb_data("\t Error: chip[%d] DRV_TunerPowerCtrl fail [0x%08lx]\n", chip_index, result);
			goto exit;
		}
	}

	deb_data("- %s success -\n", __func__);
	return result;

exit:
	deb_data("- %s fail -\n", __func__);
	return result;
}

DWORD DL_ResetPID(
	Device_Context *DC,
	BYTE chip)
{
	DWORD result = Error_NO_ERROR;

	mutex_lock(&DC->dev_mutex);
		result = DRV_ResetPID(DC, chip);
	mutex_unlock(&DC->dev_mutex);

	return result;
}

DWORD DL_AddPID(
	Device_Context *DC,
	BYTE chip_index,
	Byte index,
	Pid pid)
{
	DWORD result = Error_NO_ERROR;

	mutex_lock(&DC->dev_mutex);
		result = DRV_AddPID(DC, chip_index, index, pid);
	mutex_unlock(&DC->dev_mutex);

	return result;
}

DWORD DL_RemovePID(
	Device_Context *DC,
	BYTE chip_index,
	Byte index,
	Pid pid)
{
	DWORD result = Error_NO_ERROR;

	mutex_lock(&DC->dev_mutex);
		result = DRV_RemovePID(DC, chip_index, index, pid);
	mutex_unlock(&DC->dev_mutex);

	return result;
}

DWORD DL_PIDOnOff(
	Device_Context *DC,
	BYTE chip_index,
	bool bOn)
{
	DWORD result = Error_NO_ERROR;

	mutex_lock(&DC->dev_mutex);
		result = DRV_PIDOnOff(DC, chip_index, bOn);
	mutex_unlock(&DC->dev_mutex);

	return result;
}

DWORD DL_ApCtrl(
	Device_Context *DC,
	BYTE chip_index,
	Bool bOn)
{
	DWORD result = Error_NO_ERROR;

	mutex_lock(&DC->dev_mutex);
		result = DRV_ApCtrl(DC, chip_index, bOn);
	mutex_unlock(&DC->dev_mutex);

	return result;
}

DWORD DL_ApReset(
	Device_Context *DC,
	BYTE chip_index,
	Bool bOn)
{
	DWORD result = Error_NO_ERROR;

	mutex_lock(&DC->dev_mutex);
		result = DRV_ApReset(DC, chip_index, bOn);
	mutex_unlock(&DC->dev_mutex);

	return result;
}

DWORD DL_Tuner_SetFreqBw(
	Device_Context *DC,
	BYTE chip_index,
	u32 dwFreq,
	u8 ucBw)
{
	DWORD result = Error_NO_ERROR;

	mutex_lock(&DC->dev_mutex);
		if (DC->fc[chip_index].ulDesiredFrequency != dwFreq || DC->fc[chip_index].ucDesiredBandWidth != (ucBw * 1000))
			result = DRV_SetFreqBw(DC, chip_index, dwFreq, ucBw);
	mutex_unlock(&DC->dev_mutex);

	return result;
}

DWORD DL_SetArchitecture(
	Device_Context *DC,
	Architecture architecture)
{
	DWORD result = Error_NO_ERROR;

	mutex_lock(&DC->dev_mutex);
		result = DRV_SetArchitecture(DC, architecture);
	mutex_unlock(&DC->dev_mutex);

	return result;
}

DWORD DL_isLocked(
	Device_Context *DC,
	BYTE chip_index,
	Bool *bLock)
{
	DWORD result = Error_NO_ERROR;

	mutex_lock(&DC->dev_mutex);
		result = DRV_isLocked(DC, chip_index, bLock);
	mutex_unlock(&DC->dev_mutex);

	return result;
}

DWORD DL_getSignalStrength(
	Device_Context *DC,
	BYTE chip_index,
	BYTE *strength)
{
	DWORD result = Error_NO_ERROR;

	mutex_lock(&DC->dev_mutex);
		result = DRV_getSignalStrength(DC, chip_index, strength);
	mutex_unlock(&DC->dev_mutex);

	return result;
}

DWORD DL_getSignalStrengthDbm(
	Device_Context *DC,
	BYTE chip_index,
	Long *strengthDbm)
{
	DWORD result = Error_NO_ERROR;

	mutex_lock(&DC->dev_mutex);
		result = DRV_getSignalStrengthDbm(DC, chip_index, strengthDbm);
	mutex_unlock(&DC->dev_mutex);

	return result;
}

DWORD DL_getSignalStrengthIndication(
	Device_Context *DC,
	BYTE chip_index,
	BYTE *strength)
{
	DWORD result = Error_NO_ERROR;

	mutex_lock(&DC->dev_mutex);
		result = DRV_getSignalStrengthIndication(DC, chip_index, strength);
	mutex_unlock(&DC->dev_mutex);

	return result;
}

DWORD DL_getChannelStatistic(
	Device_Context *DC,
	BYTE chip_index,
	ChannelStatistic *channelStatistic)
{
	DWORD result = Error_NO_ERROR;

	mutex_lock(&DC->dev_mutex);
		result = DRV_getChannelStatistic(DC, chip_index, channelStatistic);
	mutex_unlock(&DC->dev_mutex);

	return result;
}

DWORD DL_getChannelModulation(
	Device_Context *DC,
	BYTE chip_index,
	ChannelModulation *channelModulation)
{
	DWORD result = Error_NO_ERROR;

	mutex_lock(&DC->dev_mutex);
		result = DRV_getChannelModulation(DC, chip_index, channelModulation);
	mutex_unlock(&DC->dev_mutex);

	return result;
}

DWORD DL_getSNR(
	Device_Context *DC,
	BYTE chip_index,
	Constellation *constellation,
	BYTE *snr)
{
	DWORD result = Error_NO_ERROR;

	mutex_lock(&DC->dev_mutex);
		result = DRV_getSNR(DC, chip_index, constellation, snr);
	mutex_unlock(&DC->dev_mutex);

	return result;
}

DWORD DL_Reboot(Device_Context *DC)
{
	DWORD result = Error_NO_ERROR;

	mutex_lock(&DC->dev_mutex);
		result = DRV_Reboot(DC);
	mutex_unlock(&DC->dev_mutex);

	return result;
}

DWORD DL_CheckTunerInited(
	Device_Context *DC,
	BYTE chip_index,
	bool *Tuner_inited)
{
	DWORD result = Error_NO_ERROR;

	mutex_lock(&DC->dev_mutex);
		*Tuner_inited = DC->fc[chip_index].tunerinfo.bTunerInited;
	mutex_unlock(&DC->dev_mutex);

	return result;
}

DWORD DL_DemodIOCTLFun(
	Device_Context *DC,
	DWORD IOCTLCode,
	unsigned long pIOBuffer,
	Byte chip_index)
{
	DWORD result = Error_NO_ERROR;

	mutex_lock(&DC->dev_mutex);
		result = DemodIOCTLFun((void *)&(DC->Demodulator), IOCTLCode, pIOBuffer, chip_index);
	mutex_unlock(&DC->dev_mutex);

	return result;
}

DWORD DL_ReadRawIR(
	Device_Context *DC,
	DWORD *ucpReadBuf)
{
	DWORD result = Error_NO_ERROR;

	mutex_lock(&DC->dev_mutex);
		result = DRV_ReadRawIR(DC, ucpReadBuf);
	mutex_unlock(&DC->dev_mutex);

	return result;
}

/* Init device */
DWORD Device_init(
	struct usb_device *usb_dev,
	Device_Context *DC,
	Bool bBoot)
{
	DWORD result = Error_NO_ERROR;
	BYTE chip_index = 0;
	Byte read_register = 5;

	deb_data("- Enter %s Function -\n", __func__);

	DC->Demodulator.driver = (void *)usb_dev;
	dev_set_drvdata(&usb_dev->dev, DC);

	#ifdef DRIVER_RELEASE_VERSION
		deb_data("\t DRIVER_RELEASE_VERSION\t\t: %s\n", DRIVER_RELEASE_VERSION);
	#else
		deb_data("\t DRIVER_RELEASE_VERSION\t\t: v0.0.0\n");
	#endif

	#ifdef FW_RELEASE_VERSION
		deb_data("\t FW_RELEASE_VERSION\t\t: %s\n", FW_RELEASE_VERSION);
	#else
		deb_data("\t FW_RELEASE_VERSION\t\t: v0_0_0_0\n");
	#endif

	#ifdef OMEGA2_FW_RELEASE_VERSION
		deb_data("\t OMEGA2_FW_RELEASE_VERSION\t: %s\n", OMEGA2_FW_RELEASE_VERSION);
	#else
		deb_data("\t OMEGA2_FW_RELEASE_VERSION\t: v0_0_0_0\n");
	#endif

	#ifdef OMEGA2I_FW_RELEASE_VERSION
		deb_data("\t OMEGA2I_FW_RELEASE_VERSION\t: %s\n", OMEGA2I_FW_RELEASE_VERSION);
	#else
		deb_data("\t OMEGA2I_FW_RELEASE_VERSION\t: v0_0_0_0\n");
	#endif

	#ifdef OMEGA2W_FW_RELEASE_VERSION 
		deb_data("\t OMEGA2W_FW_RELEASE_VERSION\t: %s\n", OMEGA2W_FW_RELEASE_VERSION);
	#else
		deb_data("\t OMEGA2W_FW_RELEASE_VERSION\t: v0_0_0_0\n");
	#endif

	#ifdef Version_NUMBER
		deb_data("\t API_RELEASE_VERSION\t\t: %X.%X.%X\n", Version_NUMBER, Version_DATE, Version_BUILD);
	#else
		deb_data("\t API_RELEASE_VERSION\t\t:000.00000000.0\n");
	#endif

	#ifdef DVB_USB_ADAP_NEED_PID_FILTER
		deb_data("\t DVB_USB_ADAP_NEED_PID_FILTERING\n");
	#else
		deb_data("\t DVB_USB_ADAP_NOT_NEED_PID_FILTERING\n");
	#endif

	/* Set Device init Info */
	if (bBoot) {
		DC->Demodulator.userData = (Handle)DC;
		DC->Demodulator.frequency[0] = 666000;
		DC->Demodulator.bandwidth[0] = 8000;
		DC->Demodulator.chipNumber = 1;
		DC->architecture = Architecture_DCA;
		DC->bIrTblDownload = false;
		DC->fc[0].tunerinfo.TunerId = 0;
		DC->fc[1].tunerinfo.TunerId = 0;
		DC->fc[0].bTimerOn = false;
		DC->fc[1].bTimerOn = false;
		atomic_set(&DC->FilterCnt, 0);
		DC->bDualTs = false;
		DC->StreamType = StreamType_DVBT_DATAGRAM;
		DC->disconnect = 0;
		DC->AP_off = 1;
	}

	result = DL_DummyCmd(DC);
	if (result) {
		deb_data("\t Error: DummyCmd fail [0x%08lx]\n", result);
		goto exit;
	}
	else {
		deb_data("\t DummyCmd success\n");
	}

	if (bBoot) {
		/* Set USB Info */
		DC->UsbMode = 0x0200; //Default USB2.0
		deb_data("\t USB mode = 0x%x\n", DC->UsbMode);
	}

	if (bBoot) {
		/* Patch for eeepc */
		result = DL_SetBusTuner(DC, Bus_USB, 0x38);
		if (result) {
			deb_data("\t Error: SetBusTuner fail [0x%08lx]\n", result);
			goto exit;
		}

		result = DL_GetEEPROMConfig(DC);
		if (result) {
			deb_data("\t Error: GetEEPROMConfig fail [0x%08lx]\n", result);
			goto exit;
		}
	}

	deb_data("\t Set busId = 0x%x, tunerId = 0x%x\n", Bus_USB, DC->fc[0].tunerinfo.TunerId);
	result = DL_SetBusTuner(DC, Bus_USB, DC->fc[0].tunerinfo.TunerId);
	result = Demodulator_readRegister((Demodulator *)&DC->Demodulator, 0, Processor_LINK, EEPROM_TUNERID, &read_register);
	if (result) {
		deb_data("\t Error: SetBusTuner fail [0x%08lx]\n", result);
		goto exit;
	}

	if (DC->Demodulator.chipNumber == 2 && !DC->Demodulator.booted) {
		//plug/cold-boot/S4
		result = DL_NIMReset(DC);
	}
	else if (DC->Demodulator.chipNumber == 2 && DC->Demodulator.booted) {
		//warm-boot/(S1)
		result = DL_NIMSuspend(DC, false);
		result = DL_TunerWakeup(DC); //actually for mt2266
	}
	else if (DC->Demodulator.chipNumber == 1 && DC->Demodulator.booted) {
		//warm-boot/(S1)
		result = DL_TunerWakeup(DC);
	}

	if (result) {
		deb_data("\t Error: NIMReset or NIMSuspend or TunerWakeup fail [0x%08lx]\n", result);
		goto exit;
	}

	result = DL_Initialize(DC);
	if (result) {
		deb_data("\t Error: Initialize fail [0x%08lx]\n", result);
		goto exit;
	}

	if (DC->bIrTblDownload) {
		result = DL_IrTblDownload(DC);
		if (result) {
			deb_data("\t Error: IrTblDownload fail [0x%08lx]\n", result);
			goto exit;
		}
	}

	if (DC->Demodulator.chipNumber == 2) {
		result = DL_InitNIMSuspendRegs(DC);
		if (result) {
			deb_data("\t Error: InitNIMSuspendRegs fail [0x%08lx]\n", result);
			goto exit;
		}
	}

	for (chip_index = DC->Demodulator.chipNumber; chip_index > 0; chip_index--) {
		result = DL_ApCtrl(DC, (chip_index - 1), false);
		if (result) {
			deb_data("\t Error: ApCtrl[%d] fail [0x%08lx]\n", chip_index, result);
			goto exit;
		}

		DC->fc[chip_index - 1].bTimerOn = false;
		DC->fc[chip_index - 1].tunerinfo.bTunerInited = false;
	}

	deb_data("- %s success -\n", __func__);
	return result;

exit:
	deb_data("- %s fail -\n", __func__);
	return result;
}
