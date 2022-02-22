#include "it913x_testkit.h"

int g_EnalbePID;
int g_numPID;
int g_TSmode;
/****************************************************************
 * Omega_Get_Driver_Info:
 * 
 * Description:
 *    The purpose of this function is to get the driver version
 *****************************************************************/
int Omega_Get_Driver_Info(int handle)
{
	int result = Error_NO_ERROR;
	DemodDriverInfo driver_info;

	if(handle > 0){
		if(ioctl(handle, IOCTL_ITE_DEMOD_GETDRIVERINFO, (void *)&driver_info))
			result = ENOENT;//ERR_CANT_FIND_USB_DEV
		else
			result = driver_info.error;
	}
	else {
		result = Error_USB_INVALID_HANDLE;
	}

	if(result){
		printf("[IOCTL] IOCTL_ITE_DEMOD_GETDRIVERINFO  fail [0x%02x]\n", result);
		return result;
	}
	else{
		printf("======== ITEtech Linux DTV Information ========\n");
		printf("DriverVerion  : %s\n", driver_info.DriverVersion);
		printf("APIVerion     : %s\n", driver_info.APIVersion);
		printf("FWVerionLink  : %s\n", driver_info.FWVersionLink);
		printf("FWVerionOFDM  : %s\n", driver_info.FWVersionOFDM);
		printf("Company       : %s\n", driver_info.Company);
		printf("SupportHWInfo : %s\n", driver_info.SupportHWInfo);
		printf("Board ID      : 0x%02X\n", driver_info.BoardId);
		printf("Tuner ID      : 0x%02X\n", driver_info.TunerId);
		printf("HW PID Filter : %s\n", driver_info.enablePID?"ON":"OFF");
		switch(driver_info.TS_mode) {
			case 1:
				printf("Architecture  : DCA+PIP\n");
				break;
			case 2:
				printf("Architecture  : DCA\n");
				break;
			case 3:
				printf("Architecture  : PIP\n");
				break;
			case 0:
			default:
				printf("Architecture  : TS1\n");
				break;
		}
		printf("===============================================\n");
		g_EnalbePID = driver_info.enablePID;
		g_TSmode = driver_info.TS_mode;
	}
	return 0;
}
//----------------------------------------------------------------

int Omega_Lock_Channel(int frontend)
{
	struct dvb_frontend_parameters param;
	struct dvb_frontend_info fe_info;
	fe_status_t status;
	int set_frequency = 0, set_bandwidth = 0, res = 0;

	if (ioctl(frontend, FE_GET_INFO, &fe_info) < 0) {
		printf("ioctl FE_GET_INFO failed\n");
		return -1;
	}

	if (fe_info.type != FE_OFDM) {
		printf("Frontend is not a OFDM (DVB-T) device\n");
		return -1;
	}

	printf("\n=> Please Input Frequency (KHz): ");
	res = scanf("%d", &set_frequency);
	param.frequency = set_frequency * 1000;

	printf("\n=> Please Input Bandwidth (KHz) (Ex: 8000 KHz): ");
	res = scanf("%d", &set_bandwidth);

	switch (set_bandwidth) {
		case 8000:
			param.u.ofdm.bandwidth = BANDWIDTH_8_MHZ;
			break;
		case 7000:
			param.u.ofdm.bandwidth = BANDWIDTH_7_MHZ;
			break;
		case 6000:
			param.u.ofdm.bandwidth = BANDWIDTH_6_MHZ;
			break;
#if LINUX_VERSION_CODE > KERNEL_VERSION(3, 0, 0)
		case 5000:
			param.u.ofdm.bandwidth = BANDWIDTH_5_MHZ;
			break;
		case 2000:
			param.u.ofdm.bandwidth = BANDWIDTH_1_712_MHZ;
			break;
#endif
		default:
			printf("Bandwidth not correct!\n");
			return -1;
	}

	printf("\n================= Lock Channel ================\n");
	printf("Frequency = %d KHz\n", set_frequency);
	printf("Bandwidth = %d KHz\n", set_bandwidth);

	if (ioctl(frontend, FE_SET_FRONTEND, &param) < 0) {
		printf("ioctl FE_SET_FRONTEND failed\n");
		return -1;
	}

	usleep(2500000);
	if(ioctl(frontend, FE_READ_STATUS, &status))
	{
		printf("ioctl FE_READ_STATUS failed\n");
		return -1;
	}
	
	printf("Channel %slock\n", (status & FE_HAS_LOCK) ? "" : "un");
	printf("===============================================\n");

	return 0;
}

void Omega_Lock_Multi_Channel(int frontend, int handle)
{
	struct dvb_frontend_parameters param;
	struct dvb_frontend_info fe_info;
	fe_status_t status;
	int frequency_start = 0, frequency_end = 0, set_frequency = 0, set_bandwidth = 0, channels = 0, res = 0, is_lock = 0;
	GetSignalStrengthDbmRequest request;
	uint16_t snr = 0, signal = 0;
	uint32_t ber = 0, uncorrected_blocks = 0;

	request.chip = 0; //chip_index, not device_index, only PIP mode use

	if (ioctl(frontend, FE_GET_INFO, &fe_info) < 0) {
		printf("ioctl FE_GET_INFO failed\n");
		return ;
	}

	if (fe_info.type != FE_OFDM) {
		printf("Frontend is not a OFDM (DVB-T) device\n");
		return ;
	}

	printf("\n=> Please Input Start Frequency (KHz): ");
	res = scanf("%d", &frequency_start);

	printf("\n=> Please Input End Frequency (KHz): ");
	res = scanf("%d", &frequency_end);

	printf("\n=> Please Input Bandwidth (KHz) (Ex: 8000 KHz): ");
	res = scanf("%d", &set_bandwidth);

	#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 3, 0)
		switch (set_bandwidth) {
		case 8000:
			param.u.ofdm.bandwidth = 0;
			break;
		case 7000:
			param.u.ofdm.bandwidth = 1;
			break;
		case 6000:
			param.u.ofdm.bandwidth = 2;
			break;
		default:
			printf("Bandwidth not correct!\n");
			return ;
		}
	#else
		/* Defined in "dvb/frontend.h"->fe_bandwidth_t.
		typedef enum fe_bandwidth {
			BANDWIDTH_8_MHZ,
			BANDWIDTH_7_MHZ,
			BANDWIDTH_6_MHZ,
			BANDWIDTH_AUTO,
			BANDWIDTH_5_MHZ,
			BANDWIDTH_10_MHZ,
			BANDWIDTH_1_712_MHZ,
		}fe_bandwidth_t;
		*/

		switch (set_bandwidth) {
		case 8000: //8M
			param.u.ofdm.bandwidth = 0;
			break;
		case 7000: //7M
			param.u.ofdm.bandwidth = 1;
			break;
		case 6000: //6M
			param.u.ofdm.bandwidth = 2;
			break;
		case 5000: //5M
			param.u.ofdm.bandwidth = 4;
			break;
		case 10000: //10M
			param.u.ofdm.bandwidth = 5;
			break;
		case 2000: //2M
			param.u.ofdm.bandwidth = 6;
			break;
		default: //AUTO
			param.u.ofdm.bandwidth = 3;
			set_bandwidth = -1;
			break;
		}
	#endif

	set_frequency = frequency_start;
	while (set_frequency <= frequency_end) {
		param.frequency = set_frequency * 1000;
		printf("Scan %d KHz\n", set_frequency);

		if (ioctl(frontend, FE_SET_FRONTEND, &param) < 0) {
			printf("ioctl FE_SET_FRONTEND failed\n");
			return ;
		}

		usleep(2500000);

		if(ioctl(frontend, FE_READ_STATUS, &status))
		{
			printf("Read staus failed\n");
			return;
		}

		if (status & FE_HAS_LOCK)
			is_lock = 1;
		else
			is_lock = 0;

		if (is_lock) {
			if(ioctl(frontend, FE_READ_SIGNAL_STRENGTH, &signal))
			{
				printf("ioctl FE_READ_SIGNAL_STRENGTH failed\n");
				return;
			}
			if(ioctl(frontend, FE_READ_SNR, &snr))
			{
				printf("ioctl FE_READ_SNR failed\n");
				return;
			}
			if(ioctl(frontend, FE_READ_BER, &ber))
			{
				printf("ioctl FE_READ_BER failed\n");
				return;
			}
			if(ioctl(frontend, FE_READ_UNCORRECTED_BLOCKS, &uncorrected_blocks))
			{
				printf("ioctl FE_READ_UNCORRECTED_BLOCKS failed\n");
				return;
			}

			request.chip = 0;
			if (ioctl(handle, IOCTL_ITE_DEMOD_GETSIGNALSTRENGTHDBM, (void *)&request) < 0)
			{
				printf("IOCTL_ITE_DEMOD_GETSIGNALSTRENGTHDBM fail\n");
				return;
			}
			channels++;
			printf("*** Channel Statistics: ***\n");
			printf("Frequency    : %d KHz\n", set_frequency);
			printf("Demod locked : %d\n", is_lock);
			printf("Strength     : %02d\n", (signal * 100 / 0xFFFF));
			printf("BER          : %.2e\n", (float)ber / 0xFFFFFFFF);
			printf("Abort Count  : %d\n", uncorrected_blocks);
			printf("SNR          : %02d dB\n", (snr * 29 / 0xFFFF));
			printf("InBandPwr    : %ld dBm\n", request.strengthDbm);
			printf("***************************\n");
		}

		if (set_bandwidth > 0)
			set_frequency += set_bandwidth;
		else
			set_frequency += 6000;
	}

	printf("================== Analysis ===================");
	printf("\n Frequency %d(KHz) ~  %d(KHz)", frequency_start, frequency_end);
	printf("\n Scan Channel : %d channels Locked", channels);
	printf("\n===============================================\n");
}

void Find_ScanRangePower (Long *powerScanLocalMax, Long *powerScan, 
		                  Dword *frequency_Scan, int *localMaxMin_index, 
						  int scan_index, Byte bw)
{
	int i = 0;

	for(i = bw ; i < (scan_index-bw) ; i++)
	{
		powerScanLocalMax[i] = powerScan[i];

		if(powerScanLocalMax[i] < powerScan[i-bw] - 20)
			powerScanLocalMax[i] = powerScan[i-bw] - 20;
		if(powerScanLocalMax[i] < powerScan[i+bw] - 20)
			powerScanLocalMax[i] = powerScan[i+bw] - 20;
	}

	//Find Local Maximun RF power  Minum index 
	*localMaxMin_index = bw;
	for(i = bw ; i < (scan_index-bw) ; i++)
	{
		if(powerScanLocalMax[(*localMaxMin_index)] > powerScanLocalMax[i])
			*localMaxMin_index = i;
	}

	//print final result
	printf("\n\n---------------- Free Channel RF Power: ----------------\n");
	for(i = bw ; i < (scan_index-bw) ; i++)
		printf("Frequency: %ld KHz, RF Local Max: %ld dBm\n", frequency_Scan[i], powerScanLocalMax[i]);

}

void Omega_Find_Free_Channel(int frontend, int handle)
{
	struct dvb_frontend_parameters param;
	struct dvb_frontend_info fe_info;
	int res = 0, i = 0, scan_index = 0, localMaxMin_index = 0, scan_Scale = 0;
	Dword frequency_start = 0, frequency_end = 0, set_bandwidth = 0;
	Byte bw = 0;
	Dword frequency_Scan[1024];
	Long powerScan[1024], powerScanLocalMax[1024];
	GetSignalStrengthDbmRequest request;

	request.chip = 0; //chip_index, not device_index, only PIP mode use
	
	if (ioctl(frontend, FE_GET_INFO, &fe_info) < 0) {
		printf("ioctl FE_GET_INFO failed\n");
		return ;
	}

	if (fe_info.type != FE_OFDM) {
		printf("Frontend is not a OFDM (DVB-T) device\n");
		return ;
	}
	// initial all buffer
	for(i = 0 ; i < 1024 ; i++)
	{
		frequency_Scan[i] = 0;
		powerScan[i] = 0;
		powerScanLocalMax[i] = 0;
	}

	printf("\n=> Please Input Start Frequency (KHz): ");
	res = scanf("%ld", &frequency_start);

	printf("\n=> Please Input End Frequency (KHz): ");
	res = scanf("%ld", &frequency_end);

	printf("\n=> Please Input Bandwidth (KHz) (Ex: 8000 KHz): ");
	res = scanf("%ld", &set_bandwidth);
	printf("\n=> Please Input Scan Scale:");
	printf("\n   0: 1 MHz           (recommend for 900MHz)");
	printf("\n   1: 1/2 bandwidth   (recommend for 2.4G)");
	printf("\n Select:");
	res = scanf("%d", &scan_Scale);

	if((scan_Scale != SignalSource_0_9G) && (scan_Scale != SignalSource_2_4G)) {
		printf("WRONG Scan Scale!!!\n");
		return;
	}

	#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 3, 0)
		switch (set_bandwidth) {
		case 8000:
			param.u.ofdm.bandwidth = 0;
			break;
		case 7000:
			param.u.ofdm.bandwidth = 1;
			break;
		case 6000:
			param.u.ofdm.bandwidth = 2;
			break;
		default:
			printf("Bandwidth not correct!\n");
			return ;
		}
	#else
		/* Defined in "dvb/frontend.h"->fe_bandwidth_t.
		typedef enum fe_bandwidth {
			BANDWIDTH_8_MHZ,
			BANDWIDTH_7_MHZ,
			BANDWIDTH_6_MHZ,
			BANDWIDTH_AUTO,
			BANDWIDTH_5_MHZ,
			BANDWIDTH_10_MHZ,
			BANDWIDTH_1_712_MHZ,
		}fe_bandwidth_t;
		*/

		switch (set_bandwidth) {
		case 8000: //8M
			param.u.ofdm.bandwidth = 0;
			break;
		case 7000: //7M
			param.u.ofdm.bandwidth = 1;
			break;
		case 6000: //6M
			param.u.ofdm.bandwidth = 2;
			break;
		case 5000: //5M
			param.u.ofdm.bandwidth = 4;
			break;
		case 10000: //10M
			param.u.ofdm.bandwidth = 5;
			break;
		case 2000: //2M
			param.u.ofdm.bandwidth = 6;
			break;
		default: //AUTO
			param.u.ofdm.bandwidth = 3;
			set_bandwidth = -1;
			break;
		}
	#endif
	
	if(set_bandwidth < 0) return; //check bandwidth > 0

	if(scan_Scale == SignalSource_0_9G)
		bw = (Byte)(set_bandwidth / 1000);
	else
		bw = 2;
	
	scan_index = 0;
	frequency_Scan[0] = frequency_start - (set_bandwidth / 2); 
	
	//This extra FE_SET_FRONTEND : 
	//   much more accurate at getting the first point frequency
	param.frequency = frequency_Scan[scan_index] * 1000;
	if (ioctl(frontend, FE_SET_FRONTEND, &param) < 0) {
		printf("ioctl FE_SET_FRONTEND failed\n");
		return ;
	}
	usleep(2500000);


	while(frequency_Scan[scan_index] <= (frequency_end + (set_bandwidth / 2)) )
	{
		param.frequency = frequency_Scan[scan_index] * 1000;
		printf("Scan %ld KHz:\n", frequency_Scan[scan_index]);

		if (ioctl(frontend, FE_SET_FRONTEND, &param) < 0) {
			printf("ioctl FE_SET_FRONTEND failed\n");
			return ;
		}
		usleep(2500000);

		for(i = 0 ; i < 1 ; i++)
		{
			request.chip = 0;
			if (ioctl(handle, IOCTL_ITE_DEMOD_GETSIGNALSTRENGTHDBM, (void *)&request) < 0)
			{
				printf("IOCTL_ITE_DEMOD_GETSIGNALSTRENGTHDBM fail\n");
				return;
			}
			powerScan[scan_index] =  request.strengthDbm;
			printf("\t Frequency = %ld KHz,  RF power = %ld dBm\n",frequency_Scan[scan_index], powerScan[scan_index]);
		}

		scan_index++;
		if(scan_Scale == SignalSource_0_9G)
			frequency_Scan[scan_index] = frequency_Scan[scan_index - 1] + 1000;
		else
			frequency_Scan[scan_index] = frequency_Scan[scan_index - 1] + (set_bandwidth/2);
	}
		
	//Find Local Maximun RF power
	Find_ScanRangePower(powerScanLocalMax, powerScan, frequency_Scan, &localMaxMin_index, scan_index, bw);

	printf("-----------------------------------------------\n\n");
	printf("=========== Best Free Frequency : =============");
	printf("\n \t Frequency    : %ld KHz", frequency_Scan[localMaxMin_index]);
	printf("\n \t InBandPwr    : %ld dBm", powerScanLocalMax[localMaxMin_index]);
	printf("\n===============================================\n");
}

void Omega_Find_2_4G_Free_Band(int frontend, int handle)
{
	struct dvb_frontend_parameters param;
	struct dvb_frontend_info fe_info;
	int res = 0, i = 0, j = 0, scan_interval = 0, scan_times = 0, print_count = 0, free_channel = 0, select = 0, scan_idx = 0, set_bandwidth = 0;
	struct timeval start, end;
	unsigned long diff = 0;
	Dword freqScan[6] = {0};
	Long free_channel_power = 0, powerScanLocalMax[32] = {0};
	GetSignalStrengthDbmRequest request;
	RegistersRequest reg;

	printf("\n=> Please select the Free Band width you want :");
	printf("\n   0: 11 chan");
	printf("\n   1: 13 chan");
	printf("\n Select: ");
	res = scanf("%d", &select);
	switch (select) {
		case 0:
			scan_idx = 5;
			break;
		case 1:
			scan_idx = 6;
			break;
		default:
			return;
	}
	request.chip = 0; //chip_index, not device_index, only PIP mode use
	
	if (ioctl(frontend, FE_GET_INFO, &fe_info) < 0) {
		printf("ioctl FE_GET_INFO failed\n");
		return ;
	}

	if (fe_info.type != FE_OFDM) {
		printf("Frontend is not a OFDM (DVB-T) device\n");
		return ;
	}
	
	// initial buffer value
	scan_interval = 100 * 1000;// 100 ms *1000 = 100000us 
	scan_times = 50;
	free_channel = free_channel_power = 0;
	set_bandwidth = 6000;
	if(select == 0) { //for 11 chan
		freqScan[0] = 2404000;
		freqScan[1] = 2470000;
		freqScan[2] = 2437000;
		freqScan[3] = 2424500;
		freqScan[4] = 2449500;
	}
	else if(select == 1) { //for 13 chan
		freqScan[0] = 2404000;
		freqScan[1] = 2480000;
		freqScan[2] = 2427000;
		freqScan[3] = 2429500;
		freqScan[4] = 2454500;
		freqScan[5] = 2457000;
	}
	else
		return;

	#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 3, 0)
		switch (set_bandwidth) {
		case 8000:
			param.u.ofdm.bandwidth = 0;
			break;
		case 7000:
			param.u.ofdm.bandwidth = 1;
			break;
		case 6000:
			param.u.ofdm.bandwidth = 2;
			break;
		default:
			printf("Bandwidth not correct!\n");
			return ;
		}
	#else
		/* Defined in "dvb/frontend.h"->fe_bandwidth_t.
		typedef enum fe_bandwidth {
			BANDWIDTH_8_MHZ,
			BANDWIDTH_7_MHZ,
			BANDWIDTH_6_MHZ,
			BANDWIDTH_AUTO,
			BANDWIDTH_5_MHZ,
			BANDWIDTH_10_MHZ,
			BANDWIDTH_1_712_MHZ,
		}fe_bandwidth_t;
		*/

		switch (set_bandwidth) {
		case 8000: //8M
			param.u.ofdm.bandwidth = 0;
			break;
		case 7000: //7M
			param.u.ofdm.bandwidth = 1;
			break;
		case 6000: //6M
			param.u.ofdm.bandwidth = 2;
			break;
		case 5000: //5M
			param.u.ofdm.bandwidth = 4;
			break;
		case 10000: //10M
			param.u.ofdm.bandwidth = 5;
			break;
		case 2000: //2M
			param.u.ofdm.bandwidth = 6;
			break;
		default: //AUTO
			param.u.ofdm.bandwidth = 3;
			set_bandwidth = -1;
			break;
		}
	#endif
	if(set_bandwidth < 0) return; //check bandwidth > 0

	gettimeofday(&start,NULL);
	for ( i = 0 ; i < scan_idx ; i++ )	
	{
		param.frequency = freqScan[i] * 1000;
		printf("Frequency: %ld KHz:\n", freqScan[i]);

		if (ioctl(frontend, FE_SET_FRONTEND, &param) < 0) {
			printf("ioctl FE_SET_FRONTEND failed\n");
			return;
		}
		usleep(300000);

		powerScanLocalMax[i] = -100;
		print_count = 0;
		for(j = 0 ; j < scan_times ; j++)
		{
			reg.chip = 0;
			reg.bufferLength = 1;
			reg.processor = Processor_OFDM;
			reg.registerAddress = trigger_ofsm;
			reg.buffer[0] = 0xD0;
			if (ioctl(handle, IOCTL_ITE_DEMOD_WRITEREGISTERS, (void *)&reg) < 0)
			{
				printf("ioctl IOCTL_ITE_DEMOD_WRITEREGISTERS fail\n");
				break;
			}

			request.chip = 0;
			if (ioctl(handle, IOCTL_ITE_DEMOD_GETSIGNALSTRENGTHDBM, (void *)&request) < 0)
			{
				printf("IOCTL_ITE_DEMOD_GETSIGNALSTRENGTHDBM fail\n");
				break;
			}

			if(request.strengthDbm > powerScanLocalMax[i]) {
				print_count = 0;
				powerScanLocalMax[i] = request.strengthDbm;
				printf("powerScanMax = %ld\n",request.strengthDbm);
				if(request.strengthDbm > free_channel_power) {
					printf("Power over best free channel, Skip\n");
					break;
				}
			}
			usleep(scan_interval);
		}
		if(powerScanLocalMax[i] < free_channel_power) {
			free_channel = i;
			free_channel_power = powerScanLocalMax[i];
		}

		if(select == 0) {
			if (i == 1 || i == 2) {
				if (free_channel_power <= -60) {
					printf("Found best free channel, Stop\n");
					break;
				}
			}
		}
		else if(select == 1) {
			if (i >= 1) {
				if (free_channel_power <= -60) {
					printf("Found best free channel, Stop\n");
					break;
				}					
			}
		}
	}
	gettimeofday(&end,NULL);

	diff = 1000 * (end.tv_sec-start.tv_sec) + (end.tv_usec-start.tv_usec)/1000;
	//Find Local Maximun RF power
	printf("-----------------------------------------------");
	printf("\n===============================================");
	printf("\n    Scan Time               : %ld ms", diff);
	printf("\n    Best Free Bnad Frequency: %ld KHz", freqScan[free_channel]);
	printf("\n===============================================\n");
}


int Check_Keyboard(void)
{
	struct termios old_term, new_term;
	int get_char = 0;
	int old_fcntl;

	if(tcgetattr(STDIN_FILENO, &old_term))
		return 1;
	new_term = old_term;
	new_term.c_lflag &= ~(ICANON | ECHO);
	if(tcsetattr(STDIN_FILENO, TCSANOW, &new_term))
		return 1;

	old_fcntl = fcntl(STDIN_FILENO, F_GETFL, 0);
	if(fcntl(STDIN_FILENO, F_SETFL, old_fcntl | O_NONBLOCK)<0)
		return 1;

	get_char = getchar();
	usleep(1000);
	if(tcsetattr(STDIN_FILENO, TCSANOW, &old_term))
		return 1;
	if(fcntl(STDIN_FILENO, F_SETFL, old_fcntl)<0)
		return 1;

	if ((get_char != EOF) && (get_char != 10)) {
		ungetc(get_char, stdin);
		return 1;
	}

	return 0;
}

void Get_Statistic(int frontend, int handle, int target_times)
{
	int get_times = 0, is_lock = 0, dca_mode = 0, get_char = 0;
	GetSignalStrengthDbmRequest request, request2;
	GetStatisticRequest statistic_req, statistic_req2;
	fe_status_t status;
	uint16_t snr = 0, signal = 0;
	uint32_t ber = 0, uncorrected_blocks = 0;
	long strengthDbm = 0;
	
	while (get_times < target_times) {
		if(ioctl(frontend, FE_READ_STATUS, &status))
		{
			printf("ioctl FE_READ_STATUS fail\n");
			return;
		}
		if(ioctl(frontend, FE_READ_SIGNAL_STRENGTH, &signal))
		{
			printf("ioctl FE_READ_SIGNAL_STRENGTH fail\n");
			return;
		}
		if(ioctl(frontend, FE_READ_SNR, &snr))
		{
			printf("ioctl FE_READ_SNR fail\n");
			return;
		}
		if(ioctl(frontend, FE_READ_BER, &ber))
		{
			printf("ioctl FE_READ_BER fail\n");
			return;
		}
		if(ioctl(frontend, FE_READ_UNCORRECTED_BLOCKS, &uncorrected_blocks))
		{
			printf("ioctl FE_READ_UNCORRECTED_BLOCKS fail\n");
			return;
		}

		request.chip = 0; //chip_index, not device_index, only PIP mode use
		request.error = 0;
		if (ioctl(handle, IOCTL_ITE_DEMOD_GETSIGNALSTRENGTHDBM, (void *)&request) < 0)
		{
			printf("ioctl IOCTL_ITE_DEMOD_GETSIGNALSTRENGTHDBM fail\n");
			return;
		}
		else
			strengthDbm = request.strengthDbm;

		request2.chip = 1; // DCA mode
		request2.error = 0;
		if (ioctl(handle, IOCTL_ITE_DEMOD_GETSIGNALSTRENGTHDBM, (void *)&request2) < 0) {
			printf("IOCTL_ITE_DEMOD_GETSIGNALSTRENGTHDBM 2 fail\n");
			return;
		}
		else {
			if (request2.error == 0) {
				if (request2.strengthDbm > request.strengthDbm)
					strengthDbm = request2.strengthDbm;
				
				dca_mode = 1;
			}
		}
		
		if (dca_mode && g_TSmode == Mode_DCA ) {
			statistic_req.chip = 0; //chip_index, not device_index, only PIP mode use
			if (ioctl(handle, IOCTL_ITE_DEMOD_GETSTATISTIC, (void *)&statistic_req) < 0)
			{
				printf("IOCTL_ITE_DEMOD_GETSTATISTIC fail\n");
				return;
			}
			
			statistic_req2.chip = 1; //chip_index, not device_index, only PIP mode use
			if (ioctl(handle, IOCTL_ITE_DEMOD_GETSTATISTIC, (void *)&statistic_req2) < 0)
			{
				printf("IOCTL_ITE_DEMOD_GETSTATISTIC 2 fail\n");
				return;
			}
		}

		if (status & FE_HAS_LOCK)
			is_lock = 1;
		else
			is_lock = 0;

		printf("\n*** Channel Statistics %d: ***\n", get_times);
		printf("Demod locked : %d\n", is_lock);
		printf("Strength     : %02d\n", (signal * 100 / 0xFFFF));
		printf("BER          : %.2e\n", (float)ber / 0xFFFFFFFF);
		printf("Abort Count  : %d\n", uncorrected_blocks);
		printf("SNR          : %02d dB\n", (snr * 32 / 0xFFFF));
		printf("InBandPwr    : %ld dBm\n", strengthDbm);
		if (dca_mode && g_TSmode == Mode_DCA) {
			printf("DCA Strength 1 : %02d\n", (statistic_req.statistic.signalStrength * (0xFFFF/100)) * 100 / 0xFFFF);
			printf("DCA Strength 2 : %02d\n", (statistic_req2.statistic.signalStrength * (0xFFFF/100)) * 100 / 0xFFFF);
		}
		printf("*****************************\n");
		
		printf("Press 'A' or 'a' to Stop...\n");
		if (Check_Keyboard()!=0) {
			get_char = (char)getchar();
			usleep(10000);
			if (get_char == 'a' || get_char == 'A')
				break;
		}

		sleep(1);
		get_times++;
	}
}

void Omega_Get_Statistic_Menu(int frontend, int handle)
{
	int chose_function = 0, target_times = 0, res = 0;

	printf("\n1. Display Statistics");
	printf("\n2. Monitor Statistics");
	printf("\n0. Return to Main Menu");
	printf("\n=> Please Input Your Choice: ");
	res = scanf("%d", &chose_function);

	switch (chose_function) {
	case 1:
		target_times = 1;
		break;
	case 2:
		printf("\n=> Monitor Times:");
		res = scanf("%d", &target_times);
		break;
	default:
		return ;
	}

	Get_Statistic(frontend, handle, target_times);
}

void Get_Packet(int *demux, int dvr, int target_times, int is_save)
{
	FILE *save_file = NULL;
	PIDINFO pidt[PID_SIZE];
	int pid_count = 0, get_times = 0, read_length = 0, return_length = 0, pid = 0, packets = 0, skip_data = 0, get_char = 0, i = 0, retry = 0;
	unsigned char *pbuf;
	unsigned char buffer[READ_DATA_SIZE];
	struct timeval time_start, time_pre, time_now;
	uint8_t checkbit = 0, adaptation_field = 0, seq = 0;
	uint32_t time_diff;
	struct dmx_pes_filter_params filter;
	Bool bBufferIsEmpty = True;
	int ReadCounter = 0, init = 0;
	
	/* Create save file */
	if (is_save) {
		save_file = fopen("DVB.ts", "wb");
		if (!save_file) {
			printf("DVB.ts create fail\n");
			is_save = 0;
		}
	}

	/* Init pid */
	for (pid_count = 0; pid_count < PID_SIZE; pid_count++) {
		pidt[pid_count].count = 0;
		pidt[pid_count].ulLostCount = 0;
		pidt[pid_count].pid = 0;
		pidt[pid_count].sequence1 = 0xFF;
		pidt[pid_count].sequence2 = 0xFF;
	}

	if(ioctl(dvr, DMX_SET_BUFFER_SIZE, 1024 * 1024)<0)
	{
		printf("ioctl DMX_SET_BUFFER_SIZE fail\n");
		if (save_file)
			fclose(save_file);
		
		return ;
	}

	if(!g_EnalbePID)
	{
		filter.pid = 0x2000;
		filter.input = DMX_IN_FRONTEND;
		filter.output = DMX_OUT_TS_TAP;
		filter.pes_type = DMX_PES_OTHER;
		filter.flags = 0;

		if (ioctl(demux[0], DMX_SET_PES_FILTER, &filter) < 0) {
			printf("ioctl DMX_SET_PES_FILTER fail\n");
			if (save_file)
				fclose(save_file);

			return ;
		}

		/* Start capture */
		if (ioctl(demux[0], DMX_START, 0) < 0) {
			printf("ioctl DMX_START fail\n");
			if (save_file)
				fclose(save_file);

			return ;
		}
	}
	
	if(g_EnalbePID)
	{
		for ( i = 0 ; i < g_numPID ; i++ )
		{
			if (ioctl(demux[i], DMX_START, 0) < 0) {
				printf("ioctl DMX_START fail\n");
				if (save_file)
					fclose(save_file);

				return ;
			}
		}
	}
	gettimeofday(&time_start, 0);
	time_pre = time_start;
	

	/* Read data */
	read_length = READ_DATA_SIZE;//188*348;
	while (get_times < target_times) {
		errno = 0;
		if(bBufferIsEmpty)
		{
			if((return_length = read(dvr, buffer, read_length)) < 0) {
				if( errno == EAGAIN) {
					usleep(5000);
					retry++;

					/*time out */
					if(retry > 200) {
						printf("time out\n");
						goto EXIT;
					}
					continue;
				}
			}
			switch(return_length) {
				case -1:////error, no data and timeout
					goto EXIT;
				case 0:// no data
					usleep(5000);
					retry++;

					/* time out (5 sec) */
					if (retry > 200) {
						printf("time out\n");
						goto EXIT;
					}
					continue;
				default:
					break;
			}
			
			bBufferIsEmpty = False;
			retry = 0;
			ReadCounter = 0;
		}

		pbuf = buffer + (ReadCounter);
		if(skip_data == 0) {
			if (pbuf[0] != SYNC_BYTE) {
				printf("desync data[%x] != sync[%x]\n", pbuf[0], SYNC_BYTE);
				bBufferIsEmpty = True;
				continue;
			}
		}

		/* For skip first error data, only V4L */
		if(init==0)
		{
			if (skip_data < 4) {
				skip_data++;
				bBufferIsEmpty = True;
				continue;
			}
			else {
				init = 1;
				bBufferIsEmpty = False;
			}
		}


		ReadCounter+=188;
		if(ReadCounter >= READ_DATA_SIZE)
			bBufferIsEmpty = True;


		if (pbuf[0] != SYNC_BYTE) {
			printf("desync data[%x] != sync[%x]\n", pbuf[0], SYNC_BYTE);
			bBufferIsEmpty = True;
			continue;
		}

		/* Analyse */
		pid = ((((unsigned)pbuf[1]) << 8) | ((unsigned) pbuf[2])) & 0x1FFF;
		checkbit = pbuf[1] & 0x80;
		adaptation_field = (pbuf[3] & 0x30) >> 4;
		seq = pbuf[3] & 0xf;

		//	if (pid == 0x1FFF) {
		//		/* Skip null TS packet */
		//		continue;
		//	}

		pidt[pid].count++;

		if (pidt[pid].sequence1 == 0xFF) {
			/* The first packet arrives, nothing to do */
			pidt[pid].sequence1 = seq;
		}
		else {
			if (adaptation_field == 0 || adaptation_field == 2) { /* No increment(lost) check for type 0 & 2 */ 
				/* Nothing */
			}
			else if ((pidt[pid].sequence1 == seq) && (pidt[pid].dup_flag == 0)) { /* Increment(lost) check for type 1 & 3 */
				/* Duplicate packets may be sent as two, and only two, consecutive Transport Stream packets of the same PID */
				pidt[pid].dup_flag = 1; // No more duplicate allowed
			}
			else {
				if (pidt[pid].sequence1 + 1 == seq) {
					//no packet loss, expected packet arrives
					if (pidt[pid].count == 100000)
						printf("pid[%d]: loss%d", pid,pidt[pid].ulLostCount);
				}
				else {
					if (pidt[pid].sequence1 + 1 < seq)
						pidt[pid].ulLostCount += seq - (pidt[pid].sequence1 + 1);
					else
						pidt[pid].ulLostCount += (seq + 0x10) - (pidt[pid].sequence1 + 1);
				}
			}

			pidt[pid].dup_flag = 0; //Next duplicate allowed
			pidt[pid].sequence1 = seq;
		}

		/* Check if TEI error */
		if (checkbit != 0x00)
			pidt[pid].ulErrCount++;

		packets++;

		/* Show information */
		if (!(packets & 0xFF)) {
			gettimeofday(&time_now, 0);
			time_diff = (time_now.tv_sec - time_pre.tv_sec) * 1000 + (time_now.tv_usec - time_pre.tv_usec) / 1000;
			if (time_diff > 1000) {
				time_diff = (time_now.tv_sec - time_start.tv_sec) * 1000 + (time_now.tv_usec - time_start.tv_usec) / 1000;

				printf("\n-PID-------Total----Lost----Error---Kbps-\n");
				for (pid_count = 0; pid_count < PID_SIZE; pid_count++) {
					if (pidt[pid_count].count > 10) {
						printf("%04x %10d %7d %7d %8lld\n",
								pid_count,
								pidt[pid_count].count,
								pidt[pid_count].ulLostCount,
								pidt[pid_count].ulErrCount,
								(long long)(pidt[pid_count].count) * 188 * 8 / time_diff);
					}
				}

				time_pre = time_now;
				get_times++;

				if (is_save) {
					printf("\nSave %d KB...\n", packets * 188 / 1024);
					get_times = (packets * 188) / 1024 / 1024; // MB, if save, change times to size

					if (get_times >= target_times)
						break;
				}

				printf("Press 'A' or 'a' to Stop...\n");
				if (Check_Keyboard()!=0) {
					get_char = (char)getchar();
					if (get_char == 'a' || get_char == 'A')
						goto EXIT;
				}
			}
		}

		if (is_save)
			fwrite(pbuf, 188, sizeof(char), save_file);
	}
EXIT:
	if(!g_EnalbePID)
	{
		/* Stop capture */
		if(ioctl(demux[0], DMX_STOP, 0) < 0)
		{
			printf("\n ioctl DMX_STOP fail\n");
			if (save_file)
				fclose(save_file);

			return ;
		}
	}
	if(g_EnalbePID)
	{
		for (i = 0 ; i < g_numPID ; i++ )
		{
			if(ioctl(demux[i], DMX_STOP, 0) < 0)
			{
				printf("\n ioctl DMX_STOP fail\n");
				if (save_file)
					fclose(save_file);

				return ;
			}
		}
	}
	if (save_file) {
		fclose(save_file);
		printf("\n*** Save File to DVB.ts (%dM) ***\n", get_times);
	}
}

void Omega_Get_Packet_Menu(int *demux, int dvr)
{
	int chose_function = 0, target_times = 0, res = 0;

	printf("\n1. Packet Error Testing");
	printf("\n2. Save Packet to File");
	printf("\n0. Return to Main Menu");
	printf("\n=> Please Choice : ");
	res = scanf("%d", &chose_function);

	switch (chose_function) {
	case 1:
		printf("\n=> Testing Times: ");
		res = scanf("%d", &target_times);

		Get_Packet(demux, dvr, target_times, 0);
		break;
	case 2:
		printf("\n=> File Size (MB): ");
		res = scanf("%d", &target_times);

		Get_Packet(demux, dvr, target_times, 1);
		break;
	default:
		return ;
	}
}

void Omega_RW_Register(int handle)
{
	int RW_flag = 0, processor_set = 0, res = 0;
	uint32_t reg_addr = 0, value = 0;
	Processor processor;
	RegistersRequest request;

	printf("\n=> Please Choose Read/Write Register (0: Read, 1: Write): ");
	res = scanf("%d", &RW_flag);

	printf("\n=> Please Choose LINK or OFDM (0: LINK, 1: OFDM): ");
	res = scanf("%d", &processor_set);

	printf("\n=> Please Enter Read/Write Register Address (Hex): ");
	res = scanf("%x", &reg_addr);

	if (processor_set == 0)
		processor = Processor_LINK;
	else
		processor = Processor_OFDM;

	request.chip = 0; //chip_index, not device_index, only PIP mode use
	request.processor = processor;
	request.registerAddress = reg_addr;
	request.bufferLength = 1;

	if (RW_flag == 1) { // Write register
		printf("\n=> Please Enter Write Value (Hex): ");
		res = scanf("%x", &value);

		request.buffer[0] = value;
		if (ioctl(handle, IOCTL_ITE_DEMOD_WRITEREGISTERS, (void *)&request) < 0)
		{
			printf("ioctl IOCTL_ITE_DEMOD_WRITEREGISTERS fail\n");
			return;
		}
		else
			printf("Write %s Address[0x%X] Value[0x%x] success.\n", (processor_set ? "OFDM" : "LINK"), reg_addr, value);
	}
	else { //Read register
		if (ioctl(handle, IOCTL_ITE_DEMOD_READREGISTERS, (void *)&request) < 0)
		{
			printf("ioctl IOCTL_ITE_DEMOD_READREGISTERS fail\n");
			return;
		}
		else
			printf("Read %s Address[0x%X] Value[0x%x] success.\n", (processor_set ? "OFDM" : "LINK"), reg_addr, request.buffer[0]);
	}
}

//==================================== For EEPROM Function ===================================
// below function and structure is for EEPROM R/W function
uint8_t EEPROM_Default_Data[256] = {
	0x1E, 0xC5, 0xF1, 0x0B, 0x1A, 0x00, 0x00, 0x00, 0x8D, 0x04, 0x35, 0x91, 0x00, 0x02, 0x01, 0x02,
	0x03, 0x80, 0x00, 0xFA, 0xFA, 0x0A, 0x40, 0x01, 0x00, 0x41, 0x46, 0x30, 0x31, 0x30, 0x32, 0x30,
	0x32, 0x30, 0x37, 0x30, 0x30, 0x30, 0x30, 0x31, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x3A, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x65, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x60, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x04, 0x03, 0x09, 0x04, 0x2E, 0x03, 0x49, 0x00,
	0x54, 0x00, 0x45, 0x00, 0x20, 0x00, 0x54, 0x00, 0x65, 0x00, 0x63, 0x00, 0x68, 0x00, 0x6E, 0x00,
	0x6F, 0x00, 0x6C, 0x00, 0x6F, 0x00, 0x67, 0x00, 0x69, 0x00, 0x65, 0x00, 0x73, 0x00, 0x2C, 0x00,
	0x20, 0x00, 0x49, 0x00, 0x6E, 0x00, 0x63, 0x00, 0x2E, 0x00, 0x1E, 0x03, 0x44, 0x00, 0x56, 0x00,
	0x42, 0x00, 0x2D, 0x00, 0x54, 0x00, 0x20, 0x00, 0x54, 0x00, 0x56, 0x00, 0x20, 0x00, 0x53, 0x00,
	0x74, 0x00, 0x69, 0x00, 0x63, 0x00, 0x6B, 0x00, 0x20, 0x03, 0x41, 0x00, 0x46, 0x00, 0x30, 0x00,
	0x31, 0x00, 0x30, 0x00, 0x32, 0x00, 0x30, 0x00, 0x32, 0x00, 0x30, 0x00, 0x37, 0x00, 0x30, 0x00,
	0x30, 0x00, 0x30, 0x00, 0x30, 0x00, 0x31, 0x00, 0x15, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0xC1, 0x03, 0xDD, 0x9F, 0x04, 0xDD, 0xA2, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x12, 0x03,
	0x4D, 0x00, 0x53, 0x00, 0x46, 0x00, 0x54, 0x00, 0x31, 0x00, 0x30, 0x00, 0x30, 0x00, 0x00, 0x00,
	0x68, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static uint16_t Omega_EEPROM_checksum(uint8_t CFG_Length, uint8_t *image)
{
	uint16_t sum = 0;
	int i = 0;

	// No need to do divide operation for remainder, since it will 
	for (i = 2; i<2 + CFG_Length; i++)
	{
		sum += image[i];
	}
	return sum;
}

uint8_t EEPROM_SN_Offset(uint8_t *eep_tmpbuf)
{
	uint8_t sn_offset = 0;
	
	// check Serial Number offset
	sn_offset = eep_tmpbuf[3] * 8;
	// 1st USB String Descriptor
	sn_offset += eep_tmpbuf[sn_offset];
	// 2nd string : Manufacturer
	sn_offset += eep_tmpbuf[sn_offset];
	// 3rd string : Product
	sn_offset += eep_tmpbuf[sn_offset];

	return sn_offset; 
}

int Omega_EEPROM_write (int handle, Word bChangeSN, uint8_t *eep_tmpbuf, uint8_t *sn_buf)
{
	uint8_t sn_offset = 0;
	uint16_t sum = 0;
	int sn_size = 15, i = 0;
	uint8_t CFG_Length = 0;
	EepromValuesRequest request;
	request.chip = 0; //chip_index, not device_index, only PIP mode use
	request.bufferLength = EEPROM_RW_SIZE;

	CFG_Length = eep_tmpbuf[2];
	
	if(bChangeSN)
	{
		sn_offset = EEPROM_SN_Offset(eep_tmpbuf);	
		//4th string : Serial Number
		sn_size = ((eep_tmpbuf[sn_offset] - 2) / 2);
		if (sn_size == 0) {
			printf(RED_BOLD "Please enable USB serial number in EEPROM editor\n"RESET);
			//return -1;
		}
		if (sn_size > 15) { // SN length
			printf("Error: SN number is over 15\n");
			return -1;
		}

		// Update SN
		for (i = 0; i < sn_size; i++) {
			eep_tmpbuf[sn_offset + 2 + 2 * i] = sn_buf[i];
		}
	}

	// update checksum value
	sum = Omega_EEPROM_checksum(CFG_Length, eep_tmpbuf);
	eep_tmpbuf[0] = (uint8_t) (sum >> 8);
	eep_tmpbuf[1] = (uint8_t) sum;

	
	for (i = 0; i < 256; i += EEPROM_RW_SIZE) {
		request.registerAddress = i;
		//memcpy(request.buffer, &EepromData[i], EEPROM_RW_SIZE);
		memcpy(request.buffer, eep_tmpbuf+i, EEPROM_RW_SIZE);
		if (ioctl(handle, IOCTL_ITE_DEMOD_WRITEEEPROMVALUES, (void *)&request) < 0)
		{
			printf("IOCTL_ITE_DEMOD_WRITEEEPROMVALUES fail\n");
			return -1;
		}
	}

	printf("EEPROM Write finish\n");
	return 0;
}


int Omega_EEPROM_read(int handle, uint8_t *eep_tmpbuf, Word *initok_flag)
{
	/************************************************************
	 * 1) *initok_flag:
	 *    1: already done Omega_EEPROM_init()
	 *       In this case, if checksum is error will return -1 .
	 *    
	 *    0: Omega_EEPROM_init() not yet
	 *       In this case, if checksum error, we will load the 
	 *       default value to EEPROM.
	 *
	 *************************************************************/

	uint8_t sn_offset = 0;
	uint16_t sum = 0;
	int sn_size = 15, i = 0;
	uint8_t CFG_Length = 0;
	EepromValuesRequest request;
	request.chip = 0; //chip_index, not device_index, only PIP mode use
	request.bufferLength = EEPROM_RW_SIZE;

	
	if(handle > 0)
	{
		for (i = 0; i < 256; i += EEPROM_RW_SIZE) {
			request.registerAddress = i;
			if (ioctl(handle, IOCTL_ITE_DEMOD_READEEPROMVALUES, (void *)&request) < 0)
			{
				printf("IOCTL_ITE_DEMOD_READEEPROMVALUES fail\n");
				return -1;
			}
			memcpy(eep_tmpbuf+i, request.buffer, EEPROM_RW_SIZE);
		}
	}
	else
	{
		printf("Invalid handle\n");
		return -1;
	}
	CFG_Length = eep_tmpbuf[2];

	//checksum
	sum = Omega_EEPROM_checksum(CFG_Length, eep_tmpbuf);
	if ((eep_tmpbuf[0] != ((uint8_t)(sum >> 8))) || (eep_tmpbuf[1] != ((uint8_t)sum))) {
		
		printf("Check sum error\n");
		if(*initok_flag == 1)
			return -1 ;
		
		else if(*initok_flag == 0)
		{
			printf("!!!!!! WILL USING DEFAULT EEPROM VALUE !!!!!!\n");
			CFG_Length = EEPROM_Default_Data[2]; //load default CFG_Length
			
			//write default value to EEPROM
			for (i = 0; i < 256; i += EEPROM_RW_SIZE) {
				request.registerAddress = i;
				memcpy(request.buffer, &EEPROM_Default_Data[i], EEPROM_RW_SIZE);
				memcpy(eep_tmpbuf+i, &EEPROM_Default_Data[i], EEPROM_RW_SIZE);
				if (ioctl(handle, IOCTL_ITE_DEMOD_WRITEEEPROMVALUES, (void *)&request) < 0)
				{
					printf("IOCTL_ITE_DEMOD_WRITEEEPROMVALUES fail\n");
					return -1;
				}
			}
			printf("EEPROM update finished!!\n");
		}
	}

	//calculate EEPROM Serial Number offset
	sn_offset = EEPROM_SN_Offset(eep_tmpbuf);
	//4th string : Serial Number
	sn_size = ((eep_tmpbuf[sn_offset] - 2) / 2);
	if (sn_size == 0) {
		printf(RED_BOLD "Please enable USB serial number in EEPROM editor\n"RESET);
	}
	if (sn_size > 15) { // SN length
		printf("Error: Serial number is over 15\n");
		return -1;
	}


	printf("\t++++++++++++++++++++++++ Current EEPROM Info ++++++++++++++++++++++++\n");
	printf(" Serial Number:");
	if(sn_size == 0)
		printf("NONE");
	else
	{
		for (i = 0; i < sn_size; i++) {
			printf("0x%02X ", eep_tmpbuf[sn_offset + 2 + 2 * i]);
		}
	}
	printf("\n");
	printf("            ");
	for (i = 0; i < 15; i++) {
		printf("%4c ", eep_tmpbuf[sn_offset + 2 + 2 * i]);
	}
	printf("\n");

	printf(" TS mode : 0x%02X",eep_tmpbuf[EEPROM_TSMODE_ADDRESS]);
	switch(eep_tmpbuf[EEPROM_TSMODE_ADDRESS]) {
		case 0:
			printf(" (TS1)\n");
			break;
		case 2:
			printf(" (DCA)\n");
			break;
		case 3:
			printf(" (PIP)\n");
			break;
		default:
			break;
	}
	printf(" Tuner ID: 0x%02X\n",eep_tmpbuf[EEPROM_TUNERID_ADDRESS]);
	printf(" Board ID: 0x%02X\n",eep_tmpbuf[EEPROM_BOARDID_ADDRESS]);

	return 0;
}

int Omega_EEPROM_init(int handle, uint8_t *eep_tmpbuf, Word *initok_flag)
{
	/**********************************************************
	 * This function will read the original EEPROM value, and 
	 * also check the checksum value.
	 * If checksum value is not correct, we will load the 
	 * defalut value to EEPROM.
	 *********************************************************/

	*initok_flag = 0;

	if(Omega_EEPROM_read(handle, eep_tmpbuf, initok_flag))
	{
		printf("Omega_EEPROM_read failed\n");
		return -1;
	}

	//Omega_EEPROM_init finish this flag will set to 1
	*initok_flag = 1; 
	return 0;
}

void Omega_EEPROM_raw_data_editor(int handle)
{
	int mene_flag = 1, res = 0, select_fun = 0;
	uint8_t eeprom_tmpbuf[256];
	uint8_t sn_buf[15];
	Word bChangeSN = 0 ,bChangeTunerId = 0, bChangeBoardId = 0, bChangeTSmode = 0, initok_flag = 0; 


	memset(eeprom_tmpbuf, 0x00, sizeof(eeprom_tmpbuf));
	if(Omega_EEPROM_init(handle, eeprom_tmpbuf, &initok_flag ))
	{
		printf("Omega_EEPROM_init failed\n");
		return;
	}

	while(mene_flag)
	{
		printf("\n======= Raw Data Editor =======");
		printf("\n1. Read EEPROM Table           ");
		printf("\n2. Modify EEPROM Value      	 ");
		printf("\n0. Return to Main Menu         ");
		printf("\n================================\n");

		printf("=> Please Input Your Choice: ");
		res = scanf("%d", &select_fun);

		switch(select_fun)
		{
			case 1:// Read EEPROM
				memset(eeprom_tmpbuf, 0x00, sizeof(eeprom_tmpbuf));
				if(Omega_EEPROM_read(handle, eeprom_tmpbuf, &initok_flag))
				{
					printf("Omega_EEPROM_read failed\n");
					return;
				}
			break;

			case 2:// Write EEPROM
				bChangeSN = bChangeTunerId = bChangeBoardId = bChangeTSmode= 0;
				memset(sn_buf,0, sizeof(sn_buf));

				//--- Serial Number ---
				printf("\nChange Serial Number? (yes:1, no:0): ");
				res = 0;
				res = scanf("%hu", &bChangeSN);
				if(bChangeSN != 1) bChangeSN = 0;
				else
				{
					printf("Please key in the new Serial Number:(max 15 input) ");
					res = 0;
					res = scanf("%15s",sn_buf);
				}
			
				//--- TS mode  ---
				printf("\nChange TS mode? (yes:1, no:0): ");
				res = 0;
				res = scanf("%hu", &bChangeTSmode);
				if(bChangeTSmode != 1) bChangeTSmode = 0;
				else
				{
					printf("Please choose the mode you want, 0: TS1, 2: DCA, 3:PIP (decimal number): ");
					res = 0;
					res = scanf("%hhu",&eeprom_tmpbuf[EEPROM_TSMODE_ADDRESS]);
				}

				//--- Tuner ID  ---
				printf("\nChange Tuner ID? (yes:1, no:0): ");
				res = 0;
				res = scanf("%hu", &bChangeTunerId);
				if(bChangeTunerId != 1) bChangeTunerId = 0;
				else
				{
					printf("Please key in the new Tuner ID(decimal number): ");
					res = 0;
					res = scanf("%hhu",&eeprom_tmpbuf[EEPROM_TUNERID_ADDRESS]);
				}
				
				//--- Board ID  ---
				printf("\nChange Board ID? (yes:1, no:0): ");
				res = 0;
				res = scanf("%hu", &bChangeBoardId);
				if(bChangeBoardId != 1) bChangeBoardId = 0;
				else
				{
					printf("Please key in the new Board ID(decimal number): ");
					res = 0;
					res = scanf("%hhu",&eeprom_tmpbuf[EEPROM_BOARDID_ADDRESS]);
				}

				//--- Write data to EEPROM
				if(bChangeSN == 0 && bChangeTunerId == 0 && bChangeBoardId == 0 && bChangeTSmode == 0)
				{
					printf("No Update EEPROM!!!!!!\n");
				}
				else
				{
					if(Omega_EEPROM_write(handle, bChangeSN, eeprom_tmpbuf, sn_buf))
					{
						printf("Omega_EEPROM_write failed\n");
						return;
					}
				}
			break;

			default:
			return;
		}
	}
}

void Omega_Set_PID_Filter(int *demux)
{
	struct dmx_pes_filter_params filter;
	int pid[32] = {0}, num = 0;
	int res = 0, i = 0, chose_function = 0;
	if(!g_EnalbePID)
	{
		printf(RED_BOLD"\n DVB_USB_ADAP_NEED_PID_FILTER "RESET);
		printf("should be ");
		printf(RED_BOLD"ENABLE "RESET);
		printf("in driver/src/it913x.h\n");
		return;	
	}
	printf("\n1. Set PIDs");
	printf("\n0. Return to Main Menu");
	printf("\n=> Please Input Your Choice: ");
	res = scanf("%d", &chose_function);

	switch (chose_function) {
		case 1:
			printf("\n Total PIDs Number: ");
			res = scanf("%d",&num);
			if(num > 32) 
			{
				printf("not support more than 32 PID filter!!!!!\n");
				g_numPID = 0;
				return;
			}
			g_numPID = num;
			printf(" Set PIDs:\n");
			for(i = 0 ; i < num ; i++)
			{
				printf("  PID[%d](in Hex): ",i);
				res = scanf("%x",&pid[i]);
				filter.pid = pid[i];
				filter.input = DMX_IN_FRONTEND;
				filter.output = DMX_OUT_TS_TAP;
				filter.pes_type = DMX_PES_OTHER;
				filter.flags = DMX_IMMEDIATE_START;
				if (ioctl(demux[i], DMX_SET_PES_FILTER, &filter) < 0){
					perror("DEMUX DEVICE");
					return ;
				}
			}
			break;
		default:
			break;
	}
}


Dword DTV_ResetPIDTable(
	IN	int handle)
{
	Dword result = Error_NO_ERROR;
	ResetPidRequest request;

	if (handle > 0) {
		request.chip = 0;
		if (ioctl(handle, IOCTL_ITE_DEMOD_RESETPID, (void *)&request))
			result = -1;
		else
			result = request.error;
	}
	else {
		result = Error_USB_INVALID_HANDLE;
	}

	return result;
}

Dword DTV_EnablePIDTable(
		IN  int handle)
{
	Dword result = Error_NO_ERROR;
	ControlPidFilterRequest request;

	if (handle > 0) {
		request.chip = 0;
		request.control = 1;
		if (ioctl(handle, IOCTL_ITE_DEMOD_CONTROLPIDFILTER, (void *)&request))
			result = -1;
		else
			result = request.error;
	}
	else {
		result = Error_USB_INVALID_HANDLE;
	}

	return result;
}

Dword DTV_DisablePIDTable(
		IN  int handle)
{
	Dword result = Error_NO_ERROR;
	ControlPidFilterRequest request;

	if (handle > 0) {
		request.chip = 0;
		request.control = 0;
		if (ioctl(handle, IOCTL_ITE_DEMOD_CONTROLPIDFILTER, (void *)&request))
			result = -1;
		else
			result = request.error;
	}
	else {
		result = Error_USB_INVALID_HANDLE;
	}

	return result;
}

Dword DTV_AddInversePID(
		IN  int handle,
		IN  Byte byIndex,            // 0 ~ 31
		IN  Word wProgId)            // pid number
{
	Dword result = Error_NO_ERROR;
	AddPidAtRequest request;
	Pid pid;
	memset(&pid, 0, sizeof(pid));
	memset(&request, 0, sizeof(AddPidAtRequest));

	pid.value = (Word)wProgId;
	if (handle > 0) {
		request.chip = 0;
		request.pid = pid;
		request.index = byIndex;
		request.complementFlag = 1;//enable complement register
		if (ioctl(handle, IOCTL_ITE_DEMOD_ADDPIDAT, (void *)&request))
			result = -1;
		else
			result = request.error;
	}
	else {
		result = Error_USB_INVALID_HANDLE;
	}

	return result;
}

void Omega_Set_Inverse_NULLPacket_Filter(int handle)
{
	int chose_function = 0, res = 0, pid_index = 0;
	Word program_id = 0;
	Dword result = Error_NO_ERROR;

	if(g_EnalbePID)
	{
		printf(RED_BOLD"\n DVB_USB_ADAP_NEED_PID_FILTER "RESET);
		printf("should be ");
		printf(RED_BOLD"DISABLE "RESET);
		printf("in driver/src/it913x.h\n");
		return;	
	}

	printf("\n=> Enable(1)/Disable(0) Inverse NULL Packet Filter: ");
	res = scanf("%d", &chose_function);

	switch (chose_function) {
		case 0:
			result = DTV_DisablePIDTable(handle);
			if (result)
			{
				printf("DTV_DisablePIDTable fail [0x%08lx]\n", result);
				return;
			}
			break;

		case 1:
			result = DTV_ResetPIDTable(handle);
			if (result)
			{
				printf("DTV_ResetPIDTable fail [0x%08lx]\n", result);
				return;
			}
			
			result = DTV_EnablePIDTable(handle);
			if (result)
			{
				printf("DTV_EnablePIDTable fail [0x%08lx]\n", result);
				return;
			}

			pid_index = 0;
			program_id = 0x1FFF; //NULL packet PID
			result = DTV_AddInversePID(handle, (Byte)pid_index, program_id);
			if (result)
			{
				printf("DTV_AddPID fail [0x%08lx]\n", result);
				return;
			}
			break;

		default:
			return ;
	}
}
//==================================== For EEPROM Function  End ===================================

int main(int argc, char **argv)
{
	int menu_flag = 1, chose_function = 0, res = 0, kernel_version[3], count = 0;
	int handle_number = 0, frontend_number = 0, demux_number = 0, dvr_number = 0;
	int handle = 0, frontend = 0, demux[32] = {0}, dvr = 0;
	char handle_path [50], frontend_path[80], demux_path[80], dvr_path[80];

	/* Check handle number */
	if (argv[1] == NULL) {
		printf("================== Open default device handle =====================\n");
		printf("=== To choose another driver handle. Please input handle number ===\n");
		printf("=== Example: ./it913x_testkit 1 -> for /dev/dvb/adapter1 handle ===\n");
		printf("===================================================================\n\n");
		handle_number = 0;
	}
	else {
		handle_number = atoi(argv[1]);
		if (atoi(argv[1]) < 0) {
			printf("================ Bad handle number! Please input again! ===========\n");
			printf("================== To choose driver handle sample =================\n");
			printf("=== Example: ./it913x_testkit   -> for /dev/dvb/adapter0 handle ===\n");
			printf("=== Example: ./it913x_testkit 1 -> for /dev/dvb/adapter1 handle ===\n");
			printf("===================================================================\n\n");
			return 0;	
		}
	}

	/* Init & open handle */
	frontend_number = 0;
	demux_number = 0;
	dvr_number = 0;
	g_EnalbePID = g_numPID = g_TSmode = 0;

	kernel_version[0] = LINUX_VERSION_CODE >> 16;
	kernel_version[1] = (LINUX_VERSION_CODE >> 8) - (kernel_version[0] << 8);
	kernel_version[2] = LINUX_VERSION_CODE - (kernel_version[0] << 16) - (kernel_version[1] << 8);

	snprintf(handle_path, sizeof(handle_path), "/dev/usb-it913x%i", handle_number);
	snprintf(frontend_path, sizeof(frontend_path), "/dev/dvb/adapter%i/frontend%i", handle_number, frontend_number);
	snprintf(demux_path, sizeof(demux_path), "/dev/dvb/adapter%i/demux%i", handle_number, demux_number);
	snprintf(dvr_path, sizeof(dvr_path), "/dev/dvb/adapter%i/dvr%i", handle_number, dvr_number);

	printf("======== ITEtech Linux DTV Information ========\n");
	printf("KERNEL_VERSION(%d, %d, %d)\n", kernel_version[0], kernel_version[1], kernel_version[2]);
	printf("OPEN PATH:\n\t%s\n\t%s\n\t%s\n\t%s\n", handle_path, frontend_path, demux_path, dvr_path);
	printf("===============================================\n");

	if ((handle = open(handle_path, O_RDWR)) < 0) {
		printf("Open %s fail\n", handle_path);
		goto FREE_SOURCE;
	}

	if ((frontend = open(frontend_path, O_RDWR)) < 0) {
		printf("Open %s fail\n", frontend_path);
		goto FREE_SOURCE;
	}

	for(count = 0 ; count < 32 ; count++ )
	{
		if ((demux[count]= open(demux_path, O_RDWR | O_NONBLOCK)) < 0) {
			printf("Open %s fail\n", demux_path);
			goto FREE_SOURCE;
		}
	}
	
	if ((dvr = open(dvr_path, O_RDONLY | O_NONBLOCK)) < 0) {
		printf("Open %s fail\n", dvr_path);
		goto FREE_SOURCE;
	}

	/*Get driver information*/
	if(Omega_Get_Driver_Info(handle)) {
		printf("Omega_Get_Driver_info failed\n");
		goto FREE_SOURCE;
	}

	/* Menu */	
	while (menu_flag) {
		printf("\n======= ITEtech Linux DTV Testkit Menu ========");
		printf("\n1. Lock Channel              ");
		printf("\n2. Signal Quality Statistics ");
		printf("\n3. Record & Analyze Packets  ");
		printf("\n4. Mutil-Channel Lock Test   ");
		printf("\n5. Find Free Channel		   ");
		printf("\n6. Find 2.4G Free Band	   ");
		printf("\n7. Read/Write Register       ");
		printf("\n8. Read/Write EEPROM 		   ");
		printf("\n9. Set PID Filter 		   ");
		printf("\n10.Inverse NULL Packet Filter");
		printf("\n0. Quit                      ");
		printf("\n===============================================\n");
		printf("=> Please Input Your Choice: ");
		res = scanf("%d", &chose_function);
		fflush(stdin);

		switch (chose_function) {
		case 1:
			if (Omega_Lock_Channel(frontend) < 0)
				printf("Lock channel fail!\n");

			break;
		case 2:
			Omega_Get_Statistic_Menu(frontend, handle);
			break;
		case 3:
			Omega_Get_Packet_Menu(demux, dvr);
			break;
		case 4:
			Omega_Lock_Multi_Channel(frontend, handle);
			break;
		case 5:
			Omega_Find_Free_Channel(frontend, handle);
			break;
		case 6:
			Omega_Find_2_4G_Free_Band(frontend, handle);
			break;
		case 7:
			Omega_RW_Register(handle);
			break;
		case 8:
			Omega_EEPROM_raw_data_editor(handle);
			break;
		case 9:
			Omega_Set_PID_Filter(demux);
			break;
		case 10:
			Omega_Set_Inverse_NULLPacket_Filter(handle);
			break;
		case 0:
			menu_flag = 0;
			break;
		default:
			break;
		}
	}

FREE_SOURCE:
	if (handle > 0)
	{	
		DTV_ResetPIDTable(handle);
		DTV_DisablePIDTable(handle);
		close(handle);
	}

	if (frontend > 0)
		close(frontend);

	for(count = 0 ; count < 32 ; count++ )
	{
		if (demux[count] > 0)
		{
			close(demux[count]);
		}
	}

	if (dvr > 0)
		close(dvr);

	return 0;
}
