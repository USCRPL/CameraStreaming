/*
Copyright (c) 2006-2009 ITEtech Corporation. All rights reserved.

Module Name:
    iocontrol.c

Abstract:
    Demodulator IOCTL Query and Set functions
*/

#include "it913x.h"

/*****************************************************************************
*
*  Function:   DemodIOCTLFun
*
*  Arguments:  demodulator             - The handle of demodulator.
*              IOCTLCode               - Device IO control code
*              pIOBuffer               - buffer containing data for the IOCTL
*              number                  - chip index (for PIP mode)
*
*  Returns:    Error_NO_ERROR: successful, non-zero error code otherwise.
*
*  Notes:
*
*****************************************************************************/
// IRQL:DISPATCH_LEVEL
DWORD DemodIOCTLFun(
	IN void *demodulator,
	IN Dword IOCTLCode,
	IN unsigned long pIOBuffer,
	IN Byte number)
{
	Dword error = Error_NO_ERROR;
	Device_Context *DC = (Device_Context *)(((Demodulator *)demodulator)->userData);

	switch (IOCTLCode) {
		case IOCTL_ITE_DEMOD_ADDPIDAT:{
			AddPidAtRequest Request;
			
			if (copy_from_user((void *)&Request, (void *)pIOBuffer, sizeof(AddPidAtRequest)))
				return -EFAULT;

			if(DC->architecture == Architecture_PIP || DC->bDualTs == True)
				Request.chip = number % 2;

			/** Inverse pid filter switch **/
			Request.error = Standard_writeRegisterBits ((Demodulator *)demodulator, Request.chip, Processor_OFDM, p_mp2if_pid_complement, mp2if_pid_complement_pos, mp2if_pid_complement_len, Request.complementFlag);
			if (Request.error) return -EFAULT;

			Request.error = Demodulator_addPidAt((Demodulator *)demodulator, Request.chip, Request.index, Request.pid);
			if (Request.error) return -EFAULT;

			if (copy_to_user((void *)pIOBuffer, (void *)&Request, sizeof(AddPidAtRequest)))
				return -EFAULT;

			break;
		}
		case IOCTL_ITE_DEMOD_REMOVEPIDAT:{
			RemovePidAtRequest Request;
			
			if (copy_from_user((void *)&Request, (void *)pIOBuffer, sizeof(RemovePidAtRequest)))
				return -EFAULT;
			
			if(DC->architecture == Architecture_PIP || DC->bDualTs == True)
				Request.chip = number % 2;

			Request.error = Demodulator_removePidAt((Demodulator *)demodulator, Request.chip, Request.index, Request.pid);
			
			if (copy_to_user((void *)pIOBuffer, (void *)&Request, sizeof(RemovePidAtRequest)))
				return -EFAULT;

			break;
		}
		case IOCTL_ITE_DEMOD_RESETPID:{
			ResetPidRequest Request;
			
			if (copy_from_user((void *)&Request, (void *)pIOBuffer, sizeof(ResetPidRequest)))
				return -EFAULT;
			
			if(DC->architecture == Architecture_PIP || DC->bDualTs == True)
				Request.chip = number % 2;

			Request.error = Demodulator_resetPid((Demodulator *)demodulator, Request.chip);
			
			if (copy_to_user((void *)pIOBuffer, (void *)&Request, sizeof(ResetPidRequest)))
				return -EFAULT;
			
			break;
		}
		case IOCTL_ITE_DEMOD_GETCHANNELMODULATION:{
			GetChannelModulationRequest Request;
			
			if (copy_from_user((void *)&Request, (void *)pIOBuffer, sizeof(GetChannelModulationRequest)))
				return -EFAULT;
			
			if(DC->architecture == Architecture_PIP || DC->bDualTs == True)
				Request.chip = number % 2;

			Request.error = Demodulator_getChannelModulation((Demodulator *)demodulator, Request.chip, &Request.channelModulation);
			
			if (copy_to_user((void *)pIOBuffer, (void *)&Request, sizeof(GetChannelModulationRequest)))
				return -EFAULT;
			
			break;
		}
		case IOCTL_ITE_DEMOD_ACQUIRECHANNEL:{
			AcquireChannelRequest Request;
			
			if (copy_from_user((void *)&Request, (void *)pIOBuffer, sizeof(AcquireChannelRequest)))
				return -EFAULT;
			
			Request.error = Demodulator_acquireChannel((Demodulator *)demodulator, Request.chip, Request.bandwidth, Request.frequency);
			if (!Request.error)
				deb_data("- chip[%d] set Freq = %u, BW = %d, success -\n", Request.chip, Request.frequency, Request.bandwidth);
			else
				deb_data("- chip[%d] set Freq = %u, BW = %d, fail -\n", Request.chip, Request.frequency, Request.bandwidth);

			if (copy_to_user((void *)pIOBuffer, (void *)&Request, sizeof(AcquireChannelRequest)))
				return -EFAULT;
			
			break;
		}
		case IOCTL_ITE_DEMOD_ISLOCKED:{
			IsLockedRequest Request;
			
			if (copy_from_user((void *)&Request, (void *)pIOBuffer, sizeof(IsLockedRequest)))
				return -EFAULT;
			
			Request.error = Demodulator_isLocked((Demodulator *)demodulator, Request.chip, &Request.locked);
			
			if (copy_to_user((void *)pIOBuffer, (void *)&Request, sizeof(IsLockedRequest)))
				return -EFAULT;
			
			break;
		}
		case IOCTL_ITE_DEMOD_SETSTATISTICRANGE:{
			SetStatisticRangeRequest Request;
			
			if (copy_from_user((void *)&Request, (void *)pIOBuffer, sizeof(SetStatisticRangeRequest)))
				return -EFAULT;
			
			if(DC->architecture == Architecture_PIP || DC->bDualTs == True)
				Request.chip = number % 2;

			Request.error = Demodulator_setViterbiRange((Demodulator *)demodulator, Request.chip, Request.superFrameCount, Request.packetUnit);
			
			if (copy_to_user((void *)pIOBuffer, (void *)&Request, sizeof(SetStatisticRangeRequest)))
				return -EFAULT;
			
			break;
		}
		case IOCTL_ITE_DEMOD_GETCHANNELSTATISTIC:{
			GetChannelStatisticRequest Request;
			
			if (copy_from_user((void *)&Request, (void *)pIOBuffer, sizeof(GetChannelStatisticRequest)))
				return -EFAULT;
			
			if(DC->architecture == Architecture_PIP || DC->bDualTs == True)
				Request.chip = number % 2;

			Request.error = Demodulator_getChannelStatistic((Demodulator *)demodulator, Request.chip, &Request.channelStatistic);
			
			if (copy_to_user((void *)pIOBuffer, (void *)&Request, sizeof(GetChannelStatisticRequest)))
				return -EFAULT;
			
			break;
		}
		case IOCTL_ITE_DEMOD_GETSTATISTIC:{
			GetStatisticRequest Request;
			Device_Context *DC = (Device_Context *)(((Demodulator *)demodulator)->userData);
			Statistic statistic2;
			
			if (copy_from_user((void *)&Request, (void *)pIOBuffer, sizeof(GetStatisticRequest)))
				return -EFAULT;
			
			if ((number > 0) && (DC->Demodulator.chipNumber == 1)) {
				Request.error = error = Error_NOT_SUPPORT;
				break;
			}
			
			if(DC->architecture == Architecture_PIP || DC->bDualTs == True)
				Request.chip = number % 2;

			Request.error = Demodulator_getStatistic((Demodulator *)demodulator, Request.chip, &Request.statistic);
			
			if ((DC->Demodulator.chipNumber == 2) && (DC->architecture == Architecture_DCA) && (number == 0) && Request.chip == 1) {
				Request.error = Demodulator_getStatistic((Demodulator *)demodulator, 1, &statistic2);
				
				Request.statistic.signalPresented |= statistic2.signalPresented;
				Request.statistic.signalStrength = (Request.statistic.signalStrength > statistic2.signalStrength) ? Request.statistic.signalStrength : statistic2.signalStrength;
			}
				
			if (copy_to_user((void *)pIOBuffer, (void *)&Request, sizeof(GetStatisticRequest)))
				return -EFAULT;
			
			break;
		}
		case IOCTL_ITE_DEMOD_CONTROLPIDFILTER:{
			ControlPidFilterRequest Request;
			
			if (copy_from_user((void *)&Request, (void *)pIOBuffer, sizeof(ControlPidFilterRequest)))
				return -EFAULT;
			
			if(DC->architecture == Architecture_PIP || DC->bDualTs == True)
				Request.chip = number % 2;

			Request.error = Demodulator_controlPidFilter((Demodulator *)demodulator, Request.chip, Request.control);
						
			if (copy_to_user((void *)pIOBuffer, (void *)&Request, sizeof(ControlPidFilterRequest)))
				return -EFAULT;
			
			break;
		}
		case IOCTL_ITE_DEMOD_CONTROLPOWERSAVING:{
			ControlPowerSavingRequest Request;
			
			if (copy_from_user((void *)&Request, (void *)pIOBuffer, sizeof(ControlPowerSavingRequest)))
				return -EFAULT;
			
			if(DC->architecture == Architecture_PIP || DC->bDualTs == True)
				Request.chip = number % 2;

			Request.error = Demodulator_controlPowerSaving((Demodulator *)demodulator, Request.chip, Request.control);
			
			if (copy_to_user((void *)pIOBuffer, (void *)&Request, sizeof(ControlPowerSavingRequest)))
				return -EFAULT;
			
			break;
		}
		case IOCTL_ITE_DEMOD_GETSIGNALSTRENGTH:{
			GetSignalStrengthRequest Request;
			Device_Context *DC = (Device_Context *)(((Demodulator *)demodulator)->userData);
			
			if (copy_from_user((void *)&Request, (void *)pIOBuffer, sizeof(GetSignalStrengthRequest)))
				return -EFAULT;
			
			if ((Request.chip > 0) && (DC->Demodulator.chipNumber == 1)) {
				Request.error = error = Error_NOT_SUPPORT;
				break;
			}
			
			if(DC->architecture == Architecture_PIP || DC->bDualTs == True)
				Request.chip = number % 2;

			Request.error = Demodulator_getSignalStrengthIndication((Demodulator *)demodulator, Request.chip, &(Request.strength));
			
			if (copy_to_user((void *)pIOBuffer, (void *)&Request, sizeof(GetSignalStrengthRequest)))
				return -EFAULT;
			
			break;
		}
		case IOCTL_ITE_DEMOD_GETDRIVERINFO:{
			DemodDriverInfo DriverInfo;
			Device_Context *DC = (Device_Context *)(((Demodulator *)demodulator)->userData);
			Dword dwFWVersionLink = 0;
			Dword dwFWVersionOFDM = 0;

			strcpy((char *)DriverInfo.DriverVersion, DRIVER_RELEASE_VERSION);
			sprintf((char *)DriverInfo.APIVersion, "%X.%X.%X.%X", (BYTE)(Version_NUMBER >> 8), (BYTE)(Version_NUMBER), Version_DATE, Version_BUILD);

			Demodulator_getFirmwareVersion((Demodulator*)demodulator, Processor_LINK, &dwFWVersionLink);
			sprintf((char *)DriverInfo.FWVersionLink, "%X.%X.%X.%X", (BYTE)(dwFWVersionLink >> 24), (BYTE)(dwFWVersionLink >> 16), (BYTE)(dwFWVersionLink >> 8), (BYTE)dwFWVersionLink);

			Demodulator_getFirmwareVersion((Demodulator *)demodulator, Processor_OFDM, &dwFWVersionOFDM);
			sprintf((char *)DriverInfo.FWVersionOFDM, "%X.%X.%X.%X", (BYTE)(dwFWVersionOFDM >> 24), (BYTE)(dwFWVersionOFDM >> 16), (BYTE)(dwFWVersionOFDM >> 8), (BYTE)dwFWVersionOFDM);

			strcpy((char *)DriverInfo.Company, "ITEtech");
			strcpy((char *)DriverInfo.SupportHWInfo, "DVB-T");
			DriverInfo.BoardId = DC->board_id;
			DriverInfo.TunerId = DC->fc[0].tunerinfo.TunerId;
			#ifdef DVB_USB_ADAP_NEED_PID_FILTER
			DriverInfo.enablePID = 1;
			#else
			DriverInfo.enablePID = 0;
			#endif
			if(DC->Demodulator.chipNumber == 2) {
				if (DC->bDualTs == False && DC->architecture == Architecture_DCA)
					DriverInfo.TS_mode = 2;//DCA
				else if (DC->bDualTs == True && DC->architecture == Architecture_PIP)
					DriverInfo.TS_mode = 3;//PIP
				else if (DC->bDualTs == True && DC->architecture == Architecture_DCA) 
					DriverInfo.TS_mode = 1;//DCA+PIP
			}
			else
				DriverInfo.TS_mode = 0;//to recognize TS1
			DriverInfo.error = Error_NO_ERROR;
						
			if (copy_to_user((void *)pIOBuffer, (void *)&DriverInfo, sizeof(DemodDriverInfo)))
				return -EFAULT;
			
			break;
		}
		case IOCTL_ITE_DEMOD_WRITEREGISTERS:{
			RegistersRequest Request;
			
			if (copy_from_user((void *)&Request, (void *)pIOBuffer, sizeof(RegistersRequest)))
				return -EFAULT;
			
			if(DC->architecture == Architecture_PIP || DC->bDualTs == True)
				Request.chip = number % 2;

			Request.error = Demodulator_writeRegisters((Demodulator *)demodulator, Request.chip, Request.processor, Request.registerAddress, Request.bufferLength, Request.buffer);
						
			if (copy_to_user((void *)pIOBuffer, (void *)&Request, sizeof(RegistersRequest)))
				return -EFAULT;
			
			break;
		}
		case IOCTL_ITE_DEMOD_WRITEEEPROMVALUES:{
			EepromValuesRequest Request;
			
			if (copy_from_user((void *)&Request, (void *)pIOBuffer, sizeof(EepromValuesRequest)))
				return -EFAULT;
			
			Request.error = Demodulator_writeEepromValues((Demodulator *)demodulator, Request.chip, Request.registerAddress, Request.bufferLength, Request.buffer);
						
			if (copy_to_user((void *)pIOBuffer, (void *)&Request, sizeof(EepromValuesRequest)))
				return -EFAULT;
			
			break;
		}
		case IOCTL_ITE_DEMOD_READREGISTERS:{
			RegistersRequest Request;
			
			if (copy_from_user((void *)&Request, (void *)pIOBuffer, sizeof(RegistersRequest)))
				return -EFAULT;
			
			if(DC->architecture == Architecture_PIP || DC->bDualTs == True)
				Request.chip = number % 2;

			Request.error = Demodulator_readRegisters((Demodulator *)demodulator, Request.chip, Request.processor, Request.registerAddress, Request.bufferLength, Request.buffer);
						
			if (copy_to_user((void *)pIOBuffer, (void *)&Request, sizeof(RegistersRequest)))
				return -EFAULT;
			
			break;
		}
		case IOCTL_ITE_DEMOD_READEEPROMVALUES:{
			EepromValuesRequest Request;
			
			if (copy_from_user((void *)&Request, (void *)pIOBuffer, sizeof(EepromValuesRequest)))
				return -EFAULT;
			
			Request.error = Demodulator_readEepromValues((Demodulator *)demodulator, Request.chip, Request.registerAddress, Request.bufferLength, Request.buffer);
						
			if (copy_to_user((void *)pIOBuffer, (void *)&Request, sizeof(EepromValuesRequest)))
				return -EFAULT;
			
			break;
		}
		case IOCTL_ITE_DEMOD_GETSIGNALSTRENGTHDBM:{
			GetSignalStrengthDbmRequest Request;
			
			if (copy_from_user((void *)&Request, (void *)pIOBuffer, sizeof(GetSignalStrengthDbmRequest)))
				return -EFAULT;
			
			if(DC->architecture == Architecture_PIP || DC->bDualTs == True)
				Request.chip = number % 2;

			Request.error = Demodulator_getSignalStrengthDbm((Demodulator *)demodulator, Request.chip, &Request.strengthDbm);
						
			if (copy_to_user((void *)pIOBuffer, (void *)&Request, sizeof(GetSignalStrengthDbmRequest)))
				return -EFAULT;
			
			break;
		}
		case IOCTL_ITE_DEMOD_SETRFENCRYPT:{
			SetRfEncryptRequest Request;
			
			if (copy_from_user((void *)&Request, (void *)pIOBuffer, sizeof(SetRfEncryptRequest)))
				return -EFAULT;
			
			if(DC->architecture == Architecture_PIP || DC->bDualTs == True)
				Request.chip = number % 2;

			Request.error = Demodulator_setDecrypt((Demodulator *)demodulator, Request.chip, Request.key, Request.isEnable);
						
			if (copy_to_user((void *)pIOBuffer, (void *)&Request, sizeof(SetRfEncryptRequest)))
				return -EFAULT;
			
			break;
		}
		default:{
			deb_data("\t Error: command[0x%08lu] not support\n", IOCTLCode);
			return ENOTTY;
		}
	}

	return error;
}
