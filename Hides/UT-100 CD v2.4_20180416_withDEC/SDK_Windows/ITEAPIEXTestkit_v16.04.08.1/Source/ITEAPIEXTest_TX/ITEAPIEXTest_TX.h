#ifndef ITEAPIEXTTEST_H_INCLUDED
#define ITEAPIEXTTEST_H_INCLUDED



#define CONTROL_DEVICE 0
#define QUIT_APPLICATION 2


#define TRANSMITTER_BLOCK_SIZE 188*348L

typedef struct _DEVICE_INFO {
    WORD  UsbMode;  //0x0200, 0x0110
    WORD  VendorID;
    WORD  ProductID;    
} DEVICE_INFO, *PDEVICE_INFO;

typedef struct _DRIVER_INFO {
    DWORD DriverVersion;
	DWORD FwVersion;
	BYTE  TunerID;
} DRIVER_INFO, *PDRIVER_INFO;

typedef struct _PVBER_VAL {
    DWORD dwPostErrCnt;   // 24 bits
    DWORD dwPostCWCnt;    // 16 bits   
    WORD  wPostAbortCnt;
} PVBER_VAL, *PPVBER_VAL;

typedef struct _DRV_INFO {
    DWORD DriverPID;
    DWORD DriverVersion;     
    DWORD FwVersion_LINK;       
    DWORD FwVersion_OFDM;      
    BYTE  TunerID;
} DRV_INFO, *PDRV_INFO;

typedef struct _MODULATION_PARAM{
	DWORD IOCTLCode;
    BYTE highCodeRate;
    BYTE transmissionMode;
    BYTE constellation; 
    BYTE interval;   
} MODULATION_PARAM, *PMODULATION_PARAM;

/**
 * Get DLL version.
 *
 * @return:					dll version.
 */
typedef	char* (WINAPIV *ITE_GetDllVersion)();

/**
 * Get the driver information.
 *
 * @param pDriverInfo:		The data structure of driver information.
  * @param DevNo:			The device index number.
 * @return:					ERROR_NO_ERROR: successful, non-zero error code otherwise.
 */
typedef	int (WINAPIV *ITE_TxGetDrvInfo)(PDRV_INFO pDriverInfo, int DevNo);

/**
 * Get device information.
 *
 * @param pDeviceInfo:		The data structure of device information.
  * @param DevNo:			The device index number.
 * @return:					true: successful, false: fail.
 */
typedef	bool (WINAPIV *ITE_TxGetDeviceInfo)(PDEVICE_INFO pDeviceInfo, int DevNo);

/**
 * Initial modulator (IT9500) device. This function will create a IT9500 device handle to
 * access device interfaces.
  * @param Tx_DevNo:			The device index number of IT9500.
 * @return:					true: open device handle successful, false: open device handle failure
 */
typedef	int (WINAPIV *ITE_TxDeviceInit)(int Tx_DevNo);

/**
 * Exit modulator (IT9500) device. This function will close a IT9500 device handle.
 *
 * @param Tx_DevNo:			The device index number of IT9500.
 * @return:					true: Close device handle successful, false: Close device handle failure
 */
typedef	int (WINAPIV *ITE_TxDeviceExit)(int Tx_DevNo);

/**
 * Control the device power.
 *
 * @param bOn:				Power on/off (true: on, false: off).
  * @param DevNo:			The device index number.
 * @return:					Error_NO_ERROR:	successful, non-zero error code otherwise.
 */
typedef	int (WINAPIV *ITE_TxPowerCtl)(bool bOn, int DevNo);

/**
 * Set the frequency and bandwidth to IT9500 TX.
 * @param frequency:		Specify the frequency, in KHz.
 * @param bandwidth:		Specify the bandwidth, in KHz.
 * @param Tx_DevNo:			The device index number of IT9500.
 * @return:					ERROR_NO_ERROR: successful, non-zero error code otherwise.
 */
typedef	int (WINAPIV *ITE_TxSetChannel)(DWORD Frequency, WORD Bandwidth, int DevNo);

/**
 * Set the channel modulation of IT9500 TX.
 *
 * @param pModulationParam:	Modulation Transmission parameter setting.
 * @param Tx_DevNo:			The device index number of IT9500.
 * @return:					ERROR_NO_ERROR: successful, non-zero error code otherwise.
 */
typedef	int (WINAPIV *ITE_TxSetChannelModulation)(PMODULATION_PARAM pModulationParam, int DevNo);

/**
 * Set TX output enable/disable(output data).
 *
 * @param bEnable:			The flag for TX oupt enable/disable(1:on / 0:off).
 * @param Tx_DevNo:			The device index number of IT9500.
 * @return:					ERROR_NO_ERROR: successful, non-zero error code otherwise.
 */
typedef	int (WINAPIV *ITE_TxSetModeEnable)(bool bEnable, int DevNo);

/**
 * Send Ts data to IT9500 TX.
 *
 * @param bufferLength:		Buffer size in bytes.
 * @param buffer:			Byte array which is used to store Ts data to be sent.
 * @param Tx_DevNo:			The device index number of IT9500.
 * @return:					ERROR_NO_ERROR: successful, non-zero error code otherwise.
 */
typedef	int (WINAPIV *ITE_TxSendTSData)(DWORD* BufferLength, BYTE* Buffer, int DevNo);

/**
 * Set device type.
 *
 * @param deviceType:		The board type of the device 
 * 0: EVB,   1:DB-01-01 v01.   2:DB-01-02 v01,   3:DB-01-01 v03
 * @param Tx_DevNo:			The device index number of IT9500.
 * @return:					ERROR_NO_ERROR: successful, non-zero error code otherwise.
 */
typedef	int (WINAPIV *ITE_TxSetDeviceType)(BYTE DeviceType, int Tx_DevNo);
/**
 * Get device type.
 *
 * @param deviceType:		The board type of the device 
 * 0: EVB,   1:DB-01-01 v01.   2:DB-01-02 v01,   3:DB-01-01 v03
 * @param Tx_DevNo:			The device index number of IT9500.
 * @return:					ERROR_NO_ERROR: successful, non-zero error code otherwise.
 */
typedef	int (WINAPIV *ITE_TxGetDeviceType)(BYTE *pDeviceType, int Tx_DevNo);


/**
 * Set Output Gain/Attenuation
 *
 * @param gain:				Specify the gain in dB
 * @param Tx_DevNo:			The device index number of IT9500.
 * @return:					ERROR_NO_ERROR: successful, non-zero error code otherwise.
 */
typedef	int (WINAPIV *ITE_TxAdjustOutputGain)(int *Gain, int Tx_DevNo);

/**
 * Write one byte (8 bits) to a specific LINK register in Modulator.
 *
 * @param RegAddr:			The LINK address of demodulator to be written.
 * @param value:			The value you write into the LINK address.
 * @param Tx_DevNo:			The device number of IT9507.
 * @return:					Error_NO_ERROR:	successful, non-zero error code otherwise.
 */
typedef	int (WINAPIV *ITE_TxWriteLinkReg)(ULONG RegAddr, int value, int Tx_DevNo);

/**
 * Read one byte (8 bits) to a specific LINK register in Modulator.
 *
 * @param idx:				The LINK address of demodulator to be read.
 * @param pVal:				The value you read from the LINK address.
 * @param Tx_DevNo:			The device number of IT9507.
 * @return:					Error_NO_ERROR: successful, non-zero error code otherwise.
 */
typedef	int (WINAPIV *ITE_TxReadLinkReg)(ULONG RegAddr, int* value, int Tx_DevNo);

/**
 * Write one byte (8 bits) to a specific OFDM register in Modulator.
 *
 * @param RegAddr:			The OFDM address of demodulator to be written.
 * @param value:			The value you write into the OFDM address.
 * @param Tx_DevNo:			The device number of IT9507.
 * @return:					Error_NO_ERROR:	successful, non-zero error code otherwise.
 */
typedef	int (WINAPIV *ITE_TxWriteOFDMReg)(ULONG RegAddr, int value, int Tx_DevNo);

/**
 * Read one byte (8 bits) to a specific register in Modulator(OFDM).
 *
 * @param idx:				The OFDM address of demodulator to be read.
 * @param pVal:				The value you read from the OFDM address.
 * @param Tx_DevNo:			The device number of IT9507.
 * @return:					ERROR_NO_ERROR: successful, non-zero error code otherwise.
 */
typedef	int (WINAPIV *ITE_TxReadOFDMReg)(ULONG RegAddr, int* value, int Tx_DevNo);





ITE_GetDllVersion			g_ITEAPI_GetDllVersion;
ITE_TxGetDrvInfo			g_ITEAPI_GetDrvInfo;
ITE_TxGetDeviceInfo			g_ITEAPI_GetDeviceInfo;
ITE_TxDeviceInit			g_ITEAPI_TxDeviceInit;
ITE_TxDeviceExit			g_ITEAPI_TxDeviceExit;
ITE_TxPowerCtl				g_ITEAPI_PowerCtl;
ITE_TxSetChannel			g_ITEAPI_TxSetChannel;
ITE_TxSetChannelModulation	g_ITEAPI_TxSetChannelModulation;
ITE_TxSetModeEnable			g_ITEAPI_TxSetModeEnable;
ITE_TxSendTSData			g_ITEAPI_TxSendTSData;
ITE_TxSetDeviceType			g_ITEAPI_TxSetDeviceType;
ITE_TxGetDeviceType			g_ITEAPI_TxGetDeviceType;
ITE_TxAdjustOutputGain		g_ITEAPI_TxAdjustOutputGain;

ITE_TxWriteLinkReg			g_ITEAPI_TxWriteLinkReg;
ITE_TxReadLinkReg			g_ITEAPI_TxReadLinkReg;
ITE_TxWriteOFDMReg			g_ITEAPI_TxWriteOFDMReg;
ITE_TxReadOFDMReg			g_ITEAPI_TxReadOFDMReg;



//**************************************************************************** New
/**
 * Get number of IT9500 devices installed
 *
 * @param Tx_DevNo:			Get how many IT9500 devices are installed in system
 * @return:					ERROR_NO_ERROR: successful, non-zero error code otherwise.
 */
typedef	int (WINAPIV *ITE_TxGetNumOfDevice)(int *Tx_NumOfDev);

/**
 * Get Output Gain/Attenuation
 *
 * @param pGain:			the current gain/Attenuation setting in dB
 * @param Tx_DevNo:			The device index number of IT9500.
 * @return:					ERROR_NO_ERROR: successful, non-zero error code otherwise.
 */
typedef	int (WINAPIV *ITE_TxGetOutputGain)(int *pGain, int Tx_DevNo);

/**
 * Identify chip type
 *
 * @param pChipType:		Chip Type: 0:IT9507 1:IT9503
 * @param Tx_DevNo:			The device index number of IT9500
 * @return:					ERROR_NO_ERROR: successful, non-zero error code otherwise.
 */
typedef	int (WINAPIV *ITE_TxGetChipType)(int *pChipType, int Tx_DevNo);

typedef struct _TPS{
    BYTE highCodeRate;
	BYTE lowCodeRate;
    BYTE transmissionMode;
    BYTE constellation; 
    BYTE interval;   
    WORD cellid;
} TPS, *PTPS;

/**
 * GET current TPS settings
 *
 * @param pTPS:				Transmission Parameter Signalling 
 * @param Tx_DevNo:			The device index number of IT9500
 * @return:					ERROR_NO_ERROR: successful, non-zero error code otherwise.
 */
typedef	int (WINAPIV *ITE_TxGetTPS)(TPS *pTps, int Tx_DevNo);

/**
 * Set TPS settings
 *
 * @param TPS:				Transmission Parameter Signalling 
 * @param Tx_DevNo:			The device index number of IT9500
 * @return:					ERROR_NO_ERROR: successful, non-zero error code otherwise.
 */
typedef	int (WINAPIV *ITE_TxSetTPS)(TPS Tps, int Tx_DevNo);

/**
 * Get the Min/Max Gain range of the specified frequency and bandwidth to IT9500 TX.
 * @param frequency:		Specify the frequency, in KHz.
 * @param bandwidth:		Specify the bandwidth, in KHz.
 * @param pMaxGain:			The Maximum Gain/Attenuation can be set, in dB
 * @param pMinGain:			The Minumum Gain/Attenuation can be set, in dB 
 * @param Tx_DevNo:			The device index number of IT9500.
 * @return:					ERROR_NO_ERROR: successful, non-zero error code otherwise.
 */
typedef	int (WINAPIV *ITE_TxGetGainRange)(DWORD Frequency, WORD Bandwidth, int *pMaxGain, int *pMinGain, int DevNo);

/**
 * Send custom packet once (one shot only)
 *
 * @param Buffer_Length:	the size of Buffer.
 * @param Buffer:			the data that will be write to a HW packet buffer.
 * @param Tx_DevNo:			The device number of IT9500.
 * @return:					Error_NO_ERROR: successful, non-zero error code otherwise.
 */
typedef	int (WINAPIV *ITE_TxSendCustomPacketOnce)(DWORD Buffer_Length, BYTE* Buffer, int Tx_DevNo);

/**
 
 * Set periodical custom packet
 *
 * @param Buffer_Length:	the size of Buffer.
 * @param Buffer:			the data that will be write to a FW packet buffer .
 * @param Index				the index of custom packet, valid value 1~5.
 * @param Tx_DevNo:			The device number of IT9500.
 * @return:					Error_NO_ERROR: successful, non-zero error code otherwise.
 */
typedef	int (WINAPIV *ITE_TxSetPeridicCustomPacket)(DWORD Buffer_Length, BYTE* Buffer, BYTE Index, int Tx_DevNo);

/**
 *  Set periodical custom packet transmission interval
 *
 * @param Index:			the index of custom packet, valid value 1~5.
 * @param Timer:			the timer unit is ms.
 * @param Tx_DevNo:			The device number of IT9500.
 * @return:					Error_NO_ERROR: successful, non-zero error code otherwise.
 */
typedef	int (WINAPIV *ITE_TxSetPeridicCustomPacketTimer)(BYTE Index, BYTE Timer, int Tx_DevNo);

/**
 * The type defination of ValueSet.
 */
typedef struct {
    DWORD frequency;      /**  */
    int dAmp;			  /**  */
	int dPhi;
} IQtable;

/**
 * set IQ fixed table point
 * 
 * @param IQ_table: IQ fixed table point
 * @param Tx_DevNo:	The device number of IT9500.
 * @return:			ERROR_NO_ERROR: successful, non-zero error code otherwise.
 */
typedef	int (WINAPIV *ITE_TxSetIQtable)(IQtable *IQ_table, WORD IQtable_element_number, int Tx_DevNo);

typedef	int (WINAPIV *ITE_TxSetDCCalibrationValue)(int dc_i, int dc_q, int Tx_DevNo);

ITE_TxGetNumOfDevice		g_ITEAPI_TxGetNumOfDevice;
ITE_TxGetOutputGain			g_ITEAPI_TxGetOutputGain;
ITE_TxGetChipType			g_ITEAPI_TxGetChipType;
ITE_TxGetTPS				g_ITEAPI_TxGetTPS;
ITE_TxSetTPS				g_ITEAPI_TxSetTPS;
ITE_TxGetGainRange			g_ITEAPI_TxGetGainRange;

ITE_TxSendCustomPacketOnce			g_ITEAPI_TxSendCustomPacketOnce;
ITE_TxSetPeridicCustomPacket		g_ITEAPI_TxSetPeridicCustomPacket;
ITE_TxSetPeridicCustomPacketTimer	g_ITEAPI_TxSetPeridicCustomPacketTimer;
ITE_TxSetIQtable					g_ITE_TxSetIQtable;
ITE_TxSetDCCalibrationValue			g_ITE_TxSetDCCalibrationValue;

//**************************************************************************** New

#endif // ITEAPIEXTTEST_H_INCLUDED
