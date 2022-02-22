/*********************************************************************************************
 *  This is for DownConverter Definition
 *
 *       DownConverter includes:
 *             DownConverter_ADRF6655
 *             DownConverter_RFFC2072
 *             DownConverter_RFFC2071
 *
 *   Copyright 2016 ITE, Inc. All rights reserved.
 *
  * ******************************************************************************************/

#include "it913x.h"

Dword DRV_DCA_SetParticularLatchTime(Device_Context *DC)
{
	//This function is to fix --- in some occasion could not lock frequency
	Dword error = Error_NO_ERROR;

	if(DC->architecture == Architecture_DCA)
	{
		/** Set fpga latch */
		error = Standard_writeRegisterBits ((Demodulator *)&DC->Demodulator, 1, Processor_OFDM,
				p_reg_dca_fpga_latch, reg_dca_fpga_latch_pos, reg_dca_fpga_latch_len, 
				DC->DCA_time_info.Master_time);
		if (error) return error;
		error = Standard_writeRegisterBits ((Demodulator *)&DC->Demodulator, 0, Processor_OFDM,
				p_reg_dca_fpga_latch, reg_dca_fpga_latch_pos, reg_dca_fpga_latch_len, 
				DC->DCA_time_info.Slave_time);
		if (error) return error;
	}
	return error;
}

Dword DRV_DownConverter_device_init(Device_Context *DC)
{
	Dword error = Error_NO_ERROR;
	
	switch(DC->board_id)
	{
		case DownConverter_ADRF6655:{ //board_id = 0x31
			//6655 init
			error = ADRF6655_init((Demodulator *)&DC->Demodulator);
			if(error)	return error;
			error = ADRF6655_setCpRefSource((Demodulator *)&DC->Demodulator, False);
			if(error)	return error;
			error = ADRF6655_setCpCurrentMultiplier((Demodulator *)&DC->Demodulator, 3);
			if(error)	return error;
			deb_data("[DownConverter] Init DownConverter ADRF6655 success\n");
			break;
		}
		case DownConverter_RFFC2072:{ //board_id = 0x32
			//2072 init
			DC->RFFC2072.userData =	(void*)&DC->Demodulator;
			error = RFFC2072_initialize(&DC->RFFC2072);
			if(error) return error;
			deb_data("[DownConverter] Init DownConverter RFFC2072 success\n");
			break;
		}
		case DownConverter_RFFC2071:{ //board_id = 0x33, for DCA and PIP mode
			// Fix some particular freq could not lock
			error = DRV_DCA_SetParticularLatchTime(DC);
			if(error) return error;	

			//2071 init
			DC->RFFC2072.userData =	(void*)&DC->Demodulator;
			error = RFFC2071_initialize(&DC->RFFC2072);
			if(error) return error;

			//set to 2.4G
			error = RFFC2071_switchInputSource(&DC->RFFC2072, RFFC2071_Path1, True);
			if(error) return error;
			error = RFFC2071_switchInputSource(&DC->RFFC2072, RFFC2071_Path2, True);
			if(error) return error;

			deb_data("[DownConverter] Init DownConverter RFFC2071 success\n");
			break;
		}
		default:
			break;
	}
	
	return error;
}

DWORD DRV_ADRF6655_SourceSetting (Device_Context *DC, Dword *loFreq)
{// ADRF6655 set Lofrequency

	DWORD dwError = Error_NO_ERROR;
	dwError = ADRF6655_setLoFrequency((Demodulator*) &DC->Demodulator, *loFreq);
	if(dwError)	goto exit;
	return dwError;
exit:
	deb_data(">>>>> ADRF6655 SourceSetting fail !!!\n\n");
	return dwError;
}

DWORD DRV_RFFC2072_SourceSetting (Device_Context *DC, Dword *loFreq)
{//  RFFC2072 set Lofrequency

	DWORD dwError = Error_NO_ERROR;
	dwError = RFFC2072_setOperatingFrequency(&DC->RFFC2072, *loFreq);
	if(dwError)	goto exit;

	dwError = RFFC2072_enableDevice(&DC->RFFC2072, True);
	if(dwError)	goto exit;
	
	return dwError;
exit:
	deb_data(">>>>> RFFC2072 SourceSetting fail !!!\n\n");
	return dwError;
}

DWORD DRV_RFFC2071_SourceSetting (Device_Context *DC, Dword *loFreq)
{//  RFFC2071 set Lofrequency

	DWORD dwError = Error_NO_ERROR;
	if(*loFreq != 0)
	{

		dwError = RFFC2071_switchInputSource(&DC->RFFC2072, RFFC2071_Path1, True);
		if(dwError) goto exit;
		dwError = RFFC2071_setOperatingFrequency(&DC->RFFC2072, RFFC2071_Path1, LO_Frequency);
		if(dwError) goto exit;

		dwError = RFFC2071_switchInputSource(&DC->RFFC2072, RFFC2071_Path2, True);
		if(dwError) goto exit;
		dwError = RFFC2071_setOperatingFrequency(&DC->RFFC2072, RFFC2071_Path2, LO_Frequency);
		if(dwError) goto exit;
	
		dwError = RFFC2071_enableDevice(&DC->RFFC2072, True);
		if (dwError) goto exit;
	}
	else
	{
		dwError = RFFC2071_enableDevice(&DC->RFFC2072, False);
		if (dwError) goto exit;

		dwError = RFFC2071_switchInputSource(&DC->RFFC2072, RFFC2071_Path1, False);
		if (dwError) goto exit;
		dwError = RFFC2071_switchInputSource(&DC->RFFC2072, RFFC2071_Path2, False);
		if (dwError) goto exit;
	}

	return dwError;
exit:
	deb_data(">>>>> RFFC2071 SourceSetting fail !!!\n\n");
	return dwError;
}

DWORD DRV_DownConverter_setLoFrequency (Device_Context *DC, Dword *loFreq)
{
	DWORD dwError = Error_NO_ERROR;

	switch(DC->board_id){
		case DownConverter_ADRF6655:{
			dwError = DRV_ADRF6655_SourceSetting(DC, loFreq);
			if(dwError)	return dwError;
			break;
		}
		case DownConverter_RFFC2072:{
			dwError = DRV_RFFC2072_SourceSetting(DC, loFreq);
			if(dwError)	return dwError;
			break;
		}
		case DownConverter_RFFC2071:{
			dwError = DRV_RFFC2071_SourceSetting(DC, loFreq);
			if(dwError)	return dwError;
			break;
		}
		default:
			break;
	}

	return dwError;
}
/*IT913x DownConverter device init*/
Dword DL_DownConverter_device_init(Device_Context *DC)
{
	Dword result = Error_NO_ERROR;
	//mutex_lock(&mymutex);
		result = DRV_DownConverter_device_init(DC);
	//mutex_unlock(&mymutex);	
	return result;
}


