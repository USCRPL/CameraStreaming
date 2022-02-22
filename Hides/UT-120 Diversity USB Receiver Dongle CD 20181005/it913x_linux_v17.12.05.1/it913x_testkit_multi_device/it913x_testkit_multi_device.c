#include "it913x_testkit_multi_device.h"

static void ThreadCtrl(DevInfo_t *d_info, ThreadCtrlMode t_mode)
{
	DevInfo_t *devInfo = d_info;

	pthread_mutex_lock(&mutex);
	++dev_cond;

	if (dev_cond != devInfo->DevCount) {
		pthread_cond_wait(&cond, &mutex);
	}
	else {
		printf("%s", menu_list);
		if (t_mode) {
			if (scanf("%d", &menu_chose) == EOF)
				printf("Input Err!");
		}
		dev_cond = 0;
		pthread_cond_broadcast(&cond);
	}
	pthread_mutex_unlock(&mutex);
}

int Omega_Lock_Multi_Channel(DevInfo_t *d_info, int frontend, int handle)
{
	struct dvb_frontend_parameters param;
	struct dvb_frontend_info fe_info;
	fe_status_t status;
	int frequency_start = 0, frequency_end = 0, set_frequency = 0, set_bandwidth = 0, channels = 0, is_lock = 0, handle_number = 0;
	GetSignalStrengthDbmRequest request;
	uint16_t snr = 0, signal = 0;
	uint32_t ber = 0, uncorrected_blocks = 0;
	FILE *pFile = NULL;
	char StatisticsPath[50];

	request.chip = 0; //chip_index, not device_index, only PIP mode use
	handle_number = d_info->DevId;

	if (ioctl(frontend, FE_GET_INFO, &fe_info) < 0) {
		printf("(%d) ioctl FE_GET_INFO failed\n", handle_number);
		return -1;
	}

	if (fe_info.type != FE_OFDM) {
		printf("(%d) Frontend is not a OFDM (DVB-T) device\n", handle_number);
		return -1;
	}

	frequency_start = d_info->m_sFrequency;
	frequency_end = d_info->m_eFrequency;
	set_bandwidth = d_info->m_Bandwidth;

	sprintf(StatisticsPath, "./Output Msg/scanlog Device(%d).txt", handle_number);
	pFile = fopen(StatisticsPath, "w");
	if (!pFile) {
		printf("(%d) write %s fail\n", handle_number, StatisticsPath);
		return -1;
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
			return -1;
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
		printf("(%d) Scan %d KHz\n", handle_number, set_frequency);

		if (ioctl(frontend, FE_SET_FRONTEND, &param) < 0) {
			printf("(%d) ioctl FE_SET_FRONTEND failed\n", handle_number);
			return -1;
		}

		usleep(2500000);
		ioctl(frontend, FE_READ_STATUS, &status);

		if (status & FE_HAS_LOCK)
			is_lock = 1;
		else
			is_lock = 0;

		if (is_lock) {
			ioctl(frontend, FE_READ_SIGNAL_STRENGTH, &signal);
			ioctl(frontend, FE_READ_SNR, &snr);
			ioctl(frontend, FE_READ_BER, &ber);
			ioctl(frontend, FE_READ_UNCORRECTED_BLOCKS, &uncorrected_blocks);

			if (ioctl(handle, IOCTL_ITE_DEMOD_GETSIGNALSTRENGTHDBM, (void *)&request) < 0)
				printf("(%d) IOCTL_ITE_DEMOD_GETSIGNALSTRENGTHDBM fail\n", handle_number);

			channels++;
			printf("**** (%d) Channel Statistics: ****\n", handle_number);
			printf("Frequency    : %d KHz\n", set_frequency);
			printf("Demod locked : %d\n", is_lock);
			printf("Strength     : %02d\n", (signal * 100 / 0xFFFF));
			printf("BER          : %.2e\n", (float)ber / 0xFFFFFFFF);
			printf("Abort Count  : %d\n", uncorrected_blocks);
			printf("SNR          : %02d dB\n", (snr * 29 / 0xFFFF));
			printf("InBandPwr    : %ld dBm\n", request.strengthDbm);
			printf("*********************************\n");

			if (pFile) {
				fprintf(pFile, "**** (%d) Channel Statistics: ****\n", handle_number);
				fprintf(pFile, "Frequency    : %d KHz\n", set_frequency);
				fprintf(pFile, "Demod locked : %d\n", is_lock);
				fprintf(pFile, "Strength     : %02d\n", (signal * 100 / 0xFFFF));
				fprintf(pFile, "BER          : %.2e\n", (float)ber / 0xFFFFFFFF);
				fprintf(pFile, "Abort Count  : %d\n", uncorrected_blocks);
				fprintf(pFile, "SNR          : %02d dB\n", (snr * 29 / 0xFFFF));
				fprintf(pFile, "InBandPwr    : %ld dBm\n", request.strengthDbm);
				fprintf(pFile, "*********************************\n");
			}
		}

		set_frequency += set_bandwidth;
	}

	printf("================== (%d) Analysis ===================", handle_number);
	printf("\n Frequency %d(KHz) ~  %d(KHz)", frequency_start, frequency_end);
	printf("\n Scan Channel : %d channels Locked", channels);
	printf("\n===================================================\n");

	if (pFile) {
		fprintf(pFile, "================== (%d) Analysis ====================", handle_number);
		fprintf(pFile, "\n Frequency %d(KHz) ~  %d(KHz)", frequency_start, frequency_end);
		fprintf(pFile, "\n Scan Channel : %d channels Locked", channels);
		fprintf(pFile, "\n===================================================\n");
		fclose(pFile);

		printf("Save log to file scanlog Device(%d).txt\n", handle_number);
	}

	return 0;
}

void Get_Statistic(int handle_number, int frontend, int handle, int target_times)
{
	int get_times = 0, is_lock = 0;
	GetSignalStrengthDbmRequest request;
	fe_status_t status;
	uint16_t snr = 0, signal = 0;
	uint32_t ber = 0, uncorrected_blocks = 0;
	char statistics_path[50];
	FILE *pFile = NULL;

	request.chip = 0; //chip_index, not device_index, only PIP mode use

	if (target_times > 1) {
		sprintf(statistics_path, "./Output Msg/Statistics Device(%d).txt", handle_number);
		pFile = fopen(statistics_path, "w");
		if (!pFile) {
			printf("(%d) write %s fail\n", handle_number, statistics_path);
			return ;
		}
	}

	while (get_times < target_times) {
		ioctl(frontend, FE_READ_STATUS, &status);
		ioctl(frontend, FE_READ_SIGNAL_STRENGTH, &signal);
		ioctl(frontend, FE_READ_SNR, &snr);
		ioctl(frontend, FE_READ_BER, &ber);
		ioctl(frontend, FE_READ_UNCORRECTED_BLOCKS, &uncorrected_blocks);

		if (ioctl(handle, IOCTL_ITE_DEMOD_GETSIGNALSTRENGTHDBM, (void *)&request) < 0)
			printf("(%d) IOCTL_ITE_DEMOD_GETSIGNALSTRENGTHDBM fail\n", handle_number);

		if (status & FE_HAS_LOCK)
			is_lock = 1;
		else
			is_lock = 0;

		printf("*** (%d) Channel Statistics %d: ***\n", handle_number, get_times);
		printf("Demod locked : %d\n", is_lock);
		printf("Strength     : %02d\n", (signal * 100 / 0xFFFF));
		printf("BER          : %.2e\n", (float)ber / 0xFFFFFFFF);
		printf("Abort Count  : %d\n", uncorrected_blocks);
		printf("SNR          : %02d dB\n", (snr * 29 / 0xFFFF));
		printf("InBandPwr    : %ld dBm\n", request.strengthDbm);
		printf("*********************************\n\n");

		if (pFile) {
			fprintf(pFile, "*** (%d) Channel Statistics %d: ***\n", handle_number, get_times);
			fprintf(pFile, "Demod locked : %d\n", is_lock);
			fprintf(pFile, "Strength     : %02d\n", (signal * 100 / 0xFFFF));
			fprintf(pFile, "BER          : %.2e\n", (float)ber / 0xFFFFFFFF);
			fprintf(pFile, "Abort Count  : %d\n", uncorrected_blocks);
			fprintf(pFile, "SNR          : %02d dB\n", (snr * 32 / 0xFFFF));
			fprintf(pFile, "InBandPwr    : %ld dBm\n", request.strengthDbm);
			fprintf(pFile, "*********************************\n\n");
		}

		sleep(1);
		get_times++;
	}

	if (pFile)
		fclose(pFile);
}

void Omega_Get_Statistic_Menu(DevInfo_t *d_info, int frontend, int handle)
{ 
	int target_times = 1, ret = 0, handle_number = d_info->DevId;;

	menu_list = "";
	ret = asprintf(&menu_list, "%s\n1. Display Statistics", menu_list);
	ret = asprintf(&menu_list, "%s\n2. Monitor Statistics", menu_list);
	ret = asprintf(&menu_list, "%s\n3. Return to Main Menu", menu_list);
	ret = asprintf(&menu_list, "%s\n=> Please Choice : ", menu_list);
	ThreadCtrl(d_info, THREAD_MENU_MODE);

	switch (menu_chose) {
	case 1: 
		target_times = 1;
		break;
	case 2:
		menu_list = "";
		ret = asprintf(&menu_list, "%s\n=> Monitor Times: ", menu_list);
		ThreadCtrl(d_info, THREAD_MENU_MODE);

		target_times = menu_chose;
		break;
	default:
		return ;
	}

	Get_Statistic(handle_number, frontend, handle, target_times);
}

void Get_Packet(int handle_number, int demux, int dvr, int target_times, int is_save)
{
	FILE *pFile_ts = NULL;
	FILE *pFile_console = NULL;
	PIDINFO pidt[PID_SIZE];
	char DVBTSPath[50], PIDTPath[50];
	int pid_count = 0, get_times = 0, read_length = 0, return_length = 0, pid = 0, packets = 0, skip_data = 0;
	unsigned char buffer[READBUFFER_SIZE];
	struct timeval time_start, time_pre, time_now;
	uint8_t checkbit = 0, adaptation_field = 0, seq = 0;
	uint32_t time_diff;
	struct dmx_pes_filter_params filter;

	sprintf(PIDTPath, "./Output Msg/PIDTable.Device(%d).txt", handle_number);
	pFile_console = fopen(PIDTPath, "wb");
	if (!pFile_console) {
		printf("(%d) write %s fail\n", handle_number, PIDTPath);
		return ;
	}

	if (is_save) {
		sprintf(DVBTSPath, "./Output Msg/DVB.ts.Device(%d).txt", handle_number);
		pFile_ts = fopen(DVBTSPath, "wb");
		if (!pFile_ts) {
			printf("(%d) write %s fail\n", handle_number, DVBTSPath);
			return ;
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

	ioctl(dvr, DMX_SET_BUFFER_SIZE, 1024 * 1024);

	filter.pid = 0x2000;
	filter.input = DMX_IN_FRONTEND;
	filter.output = DMX_OUT_TS_TAP;
	filter.pes_type = DMX_PES_OTHER;
	filter.flags = 0;

	if (ioctl(demux, DMX_SET_PES_FILTER, &filter) < 0) {
		printf("(%d) ioctl DMX_SET_PES_FILTER fail\n", handle_number);
		goto FREE_SOURCE;
	}

	/* Start capture */
	if (ioctl(demux, DMX_START, 0) < 0) {
		printf("(%d) ioctl DMX_START fail\n", handle_number);
		goto FREE_SOURCE;
	}

	gettimeofday(&time_start, 0);
	time_pre = time_start;

	/* Read data */
	read_length = 188;
	while (get_times < target_times) {
		return_length = read(dvr, buffer, read_length);

		/* Error */
		if (return_length < 0) {
			get_times = target_times;
			printf("(%d) read error[%d]\n", handle_number, return_length);
			continue;
		}

		switch (return_length) {
		case 188: //correct
		case 1: //Search sync byte
			break;
		case 187: //Find sync byte & drop remaining. read next
			read_length = 188;
			continue;
		default: //length not correct, error
			printf("(%d) Only read %d\n", handle_number, return_length);
			get_times = target_times;
			continue;
		}

		/* Check & search sync byte */
		if (buffer[0] != SYNC_BYTE) {
			/* Only show first time */
			if (read_length == 188) {
				printf("(%d) desync data[%x] != sync[%x]\n", handle_number, buffer[0], SYNC_BYTE);
				fprintf(pFile_console, "(%d) desync data[%x] != sync[%x]\n", handle_number, buffer[0], SYNC_BYTE);
			}
			read_length = 1;
			continue;
		}
		else {
			if (read_length == 1) {
				read_length = 187; //Find sync and drop remaining data
				continue;
			}
		}

		/* For skip first error data, only V4L */
		if (skip_data < 2000) {
			skip_data++;
			continue;
		}

		/* Analyse */
		pid = ((((unsigned)buffer[1]) << 8) | ((unsigned) buffer[2])) & 0x1FFF;
		checkbit = buffer[1] & 0x80;
		adaptation_field = (buffer[3] & 0x30) >> 4;
		seq = buffer[3] & 0xf;

		if (pid == 0x1FFF) {
			/* Skip null TS packet */
			continue;
		}

		pidt[pid].count++;

		if (pidt[pid].sequence1 == 0xFF) {
			/* The first packet arrives, nothing to do */
			pidt[pid].sequence1 = seq;
		}
		else {
			if (adaptation_field == 0 || adaptation_field == 2) {
				/* No increment(lost) check for type 0 & 2 */ 
			}
			/* Increment(lost) check for type 1 & 3 */
			else if ((pidt[pid].sequence1 == seq) && (pidt[pid].dup_flag == 0)) {
				/* Duplicate packets may be sent as two, and only two, consecutive Transport Stream packets of the same PID */
				pidt[pid].dup_flag = 1; // No more duplicate allowed
			}
			else {
				if (pidt[pid].sequence1 + 1 == seq) {
					//no packet loss, expected packet arrives
					if(pidt[pid].count == 100000)
						printf("%d", pidt[pid].ulLostCount);
				}
				else{
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

				printf("\n---------------- (%d) -------------------\n", handle_number);
				printf("-PID-------Total----Lost----Error---Kbps-\n");
				fprintf(pFile_console, "\n------------ (%d) ---------------\n", handle_number);
				fprintf(pFile_console, "-PID--Total--Lost--Error---Kbps-\n");

				for (pid_count = 0; pid_count < PID_SIZE; pid_count++) {
					if (pidt[pid_count].count > 10) {
						printf("%04x %10d %7d %7d %8lld\n",
							pid_count,
							pidt[pid_count].count,
							pidt[pid_count].ulLostCount,
							pidt[pid_count].ulErrCount,
							(long long)(pidt[pid_count].count) * 188 * 8 / time_diff);

						fprintf(pFile_console, "%04x %10d %7d %7d %8lld\n",
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
					printf("\n (%d) Save %d KB...\n", handle_number, packets * 188 / 1024);
					fprintf(pFile_console, "\n (%d) Save %d KB...\n", handle_number, packets * 188 / 1024);
				}
			}
		}

		if (is_save) {
			get_times = (packets * 188) / 1024 / 1024; // MB, if save, change times to size
			fwrite(buffer, 1, sizeof(buffer), pFile_ts);
		}
	}

	/* Stop capture */
	ioctl(demux, DMX_STOP, 0);

FREE_SOURCE:
	if (pFile_ts) {
		fclose(pFile_ts);
		printf("\n*** Save File to DVB.ts.Device(%d).txt (%dM) ***\n", handle_number, get_times);
	}

	if (pFile_console) {
		fclose(pFile_console);
		printf("\n*** Save File to PID table.Device(%d).txt (%dM) ***\n", handle_number, get_times);
	}
}

void Omega_Get_Packet_Menu(DevInfo_t *d_info, int demux, int dvr)
{
	int target_times = 0, ret = 0, handle_number = d_info->DevId;

	menu_list = "";
	ret = asprintf(&menu_list, "%s\n1. Packet Error Testing", menu_list);
	ret = asprintf(&menu_list, "%s\n2. Save Packet to File", menu_list);
	ret = asprintf(&menu_list, "%s\n3. Return to Main Menu", menu_list);
	ret = asprintf(&menu_list, "%s\n=> Please Choice : ", menu_list);
	ThreadCtrl(d_info, THREAD_MENU_MODE);

	switch (menu_chose) {
	case 1:
		menu_list = "";
		ret = asprintf(&menu_list, "%s\n=> Testing Times: ", menu_list);
		ThreadCtrl(d_info, THREAD_MENU_MODE);

		target_times = menu_chose;
		Get_Packet(handle_number, demux, dvr, target_times, 0);
		break;
	case 2:
		menu_list = "";
		ret = asprintf(&menu_list, "%s\n=> File Size (MB): ", menu_list);
		ThreadCtrl(d_info, THREAD_MENU_MODE);

		target_times = menu_chose;
		Get_Packet(handle_number, demux, dvr, target_times, 1);
		break;
	default:
		return ;
	}
}

int Omega_Lock_Channel(DevInfo_t *d_info, int frontend)
{
	struct dvb_frontend_parameters param;
	struct dvb_frontend_info fe_info;
	fe_status_t status;
	int set_frequency = d_info->frequency, set_bandwidth = d_info->bandwidth, handle_number = d_info->DevId;

	if (ioctl(frontend, FE_GET_INFO, &fe_info) < 0) {
		printf("(%d) ioctl FE_GET_INFO failed\n", handle_number);
		return -1;
	}

	if (fe_info.type != FE_OFDM) {
		printf("(%d) Frontend is not a OFDM (DVB-T) device\n", handle_number);
		return -1;
	}

	param.frequency = set_frequency * 1000;

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
			return -1;
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

	printf("(%d) Frequency = %d KHz, ", handle_number, set_frequency);
	printf("Bandwidth = %d KHz\n", set_bandwidth);

	if (ioctl(frontend, FE_SET_FRONTEND, &param) < 0) {
		printf("ioctl FE_SET_FRONTEND failed\n");
		return -1;
	}

	usleep(2500000);
	ioctl(frontend, FE_READ_STATUS, &status);

	printf("(%d) Channel %slock\n", handle_number, (status & FE_HAS_LOCK) ? "" : "un");

	return 0;
}

void Omega_RW_Register(DevInfo_t *d_info, int handle)
{
	int RW_flag = 0, processor_set = 0, handle_number = 0;
	uint32_t reg_addr = 0, value = 0;
	Processor processor;
	RegistersRequest request;

	handle_number = d_info->DevId;
	RW_flag = d_info->rw_Flag;
	processor = d_info->rw_processor;
	reg_addr = d_info->rw_regaddr;

	if (processor_set == 0)
		processor = Processor_LINK;
	else
		processor = Processor_OFDM;

	request.chip = 0; //chip_index, not device_index, only PIP mode use
	request.processor = processor;
	request.registerAddress = reg_addr;
	request.bufferLength = 1;

	if (RW_flag == 1) { // Write register
		value = d_info->w_value;

		request.buffer[0] = value;
		if (ioctl(handle, IOCTL_ITE_DEMOD_WRITEREGISTERS, (void *)&request) < 0)
			printf("IOCTL_ITE_DEMOD_WRITEREGISTERS fail\n");
		else
			printf("Write %s Address[0x%X] Value[0x%x] success.\n", (processor_set ? "LINK" : "OFDM"), reg_addr, value);
	}
	else { //Read register
		if (ioctl(handle, IOCTL_ITE_DEMOD_READREGISTERS, (void *)&request) < 0)
			printf("IOCTL_ITE_DEMOD_READREGISTERS fail\n");
		else
			printf("Read %s Address[0x%X] Value[0x%x] success.\n", (processor_set ? "LINK" : "OFDM"), reg_addr, request.buffer[0]);
	}
}

void *DTV_Menu(void *arg)
{
	int menu_flag = 1, ret = 0;
	int handle_number = 0, frontend_number = 0, demux_number = 0, dvr_number = 0;
	int handle = 0, frontend = 0, demux = 0, dvr = 0;
	char handle_path [50], frontend_path[80], demux_path[80], dvr_path[80];
	DevInfo_t d_info = *(DevInfo_t *) arg;

	/* Init & open handle */
	handle_number = d_info.DevId;
	frontend_number = 0;
	demux_number = 0;
	dvr_number = 0;

	snprintf(handle_path, sizeof(handle_path), "/dev/usb-it913x%i", handle_number);
	snprintf(frontend_path, sizeof(frontend_path), "/dev/dvb/adapter%i/frontend%i", handle_number, frontend_number);
	snprintf(demux_path, sizeof(demux_path), "/dev/dvb/adapter%i/demux%i", handle_number, demux_number);
	snprintf(dvr_path, sizeof(demux_path), "/dev/dvb/adapter%i/dvr%i", handle_number, dvr_number);

	pthread_mutex_lock(&init_mutex);
		printf("======== ITEtech Linux DTV (%d) Information ========\n", handle_number);
		printf("OPEN PATH:\n\t%s\n\t%s\n\t%s\n\t%s\n", handle_path, frontend_path, demux_path, dvr_path);
		printf("===================================================\n");

		if ((handle = open(handle_path, O_RDWR)) < 0) {
			printf("(%d) Open %s fail\n", handle_number, handle_path);
			goto FREE_SOURCE;
		}

		if ((frontend = open(frontend_path, O_RDWR)) < 0) {
			printf("(%d) Open %s fail\n", handle_number, frontend_path);
			goto FREE_SOURCE;
		}

		if ((demux = open(demux_path, O_RDWR)) < 0) {
			printf("(%d) Open %s fail\n", handle_number, demux_path);
			goto FREE_SOURCE;
		}

		if ((dvr = open(dvr_path, O_RDONLY)) < 0) {
			printf("(%d) Open %s fail\n", handle_number, dvr_path);
			goto FREE_SOURCE;
		}
	pthread_mutex_unlock(&init_mutex);

	/* Menu */	
	while (menu_flag) {
		menu_list = "";
		ret = asprintf(&menu_list, "%s\n============ ITEtech Linux DTV Testkit ============", menu_list);
		ret = asprintf(&menu_list, "%s\n1. Lock Channel              ", menu_list);
		ret = asprintf(&menu_list, "%s\n2. Singal Quality Statistics ", menu_list);
		ret = asprintf(&menu_list, "%s\n3. Record & Analyze Packets  ", menu_list);
		ret = asprintf(&menu_list, "%s\n4. Mutil-Channel Lock Test   ", menu_list);
		ret = asprintf(&menu_list, "%s\n5. Read/Write Register	     ", menu_list);
		ret = asprintf(&menu_list, "%s\n0. Quit                      ", menu_list);
		ret = asprintf(&menu_list, "%s\n=> Please Input Your Choice: ", menu_list);
		ThreadCtrl(&d_info, THREAD_MENU_MODE);

		switch (menu_chose) {
		case 1: 
			menu_list = "";
			ret = asprintf(&menu_list, "%s\n**************** Device Lock Status ***************\n", menu_list);
			ThreadCtrl(&d_info, THREAD_SYNC_MODE);
			Omega_Lock_Channel(&d_info, frontend);
			break;
		case 2:
			Omega_Get_Statistic_Menu(&d_info, frontend, handle);
			break;
		case 3:
			Omega_Get_Packet_Menu(&d_info, demux, dvr);
			break;
		case 4:
			menu_list = "";
			ret = asprintf(&menu_list, "%s\n**************** Device Lock Status ***************\n", menu_list);
			ThreadCtrl(&d_info, THREAD_SYNC_MODE);
			Omega_Lock_Multi_Channel(&d_info, frontend, handle);
			break;
		case 5:
			Omega_RW_Register(&d_info, handle);
			break;
		case 0:
			menu_flag = 0;
			break;
		}
	}

FREE_SOURCE:
	if (handle > 0)
		close(handle);

	if (frontend > 0)
		close(frontend);

	if (demux > 0)
		close(demux);

	if (dvr > 0)
		close(dvr);

	pthread_exit((void *)0);
}

Byte Get_Params_From_File(PDevInfo_t *d_info)
{
	int type_count = 0, handle_count = 0;
	int open_handle_number = 0, type_number = 0, retErr = 0;
	char cmd[20];
	FILE *pFILE = NULL;

	pFILE = fopen("info.txt", "r");
	if (!pFILE) {
		printf("Fail to open info.txt\n");
		return -1;
	}

	retErr = fscanf(pFILE, "<DeviceNum> %d\n", &open_handle_number);
	printf("Geting %d Devices infomation from setting file!\n", open_handle_number);
	*d_info = (PDevInfo_t)malloc(sizeof(DevInfo_t) * open_handle_number);

	retErr = fscanf(pFILE, "<SettingTypeNum> %d\n", &type_number);

	for (type_count = 0; type_count < type_number; type_count++) {
		retErr = fscanf(pFILE, "*%s\n", cmd);
		for (handle_count = 0; handle_count < open_handle_number; handle_count++) {
			if (!strcmp("ChannelLock", cmd)) {
				retErr = fscanf(pFILE, "[%d]\n", &retErr);
				retErr = fscanf(pFILE, "<frequency> %lu\n", &(*d_info)[handle_count].frequency);
				retErr = fscanf(pFILE, "<bandwidth> %lu\n", &(*d_info)[handle_count].bandwidth);
				(*d_info)[handle_count].DevId = handle_count;
				(*d_info)[handle_count].DevCount = open_handle_number;
				if (!open_handle_number || !(*d_info)[handle_count].frequency) {
					printf("Input Data format is wrong!\n");
					exit(0);
				}
			}
			else if (!strcmp("MultiChannel", cmd)) {
				retErr = fscanf(pFILE, "[%d]\n", &retErr);
				retErr = fscanf(pFILE, "<startfreq> %lu\n", &(*d_info)[handle_count].m_sFrequency);
				retErr = fscanf(pFILE, "<endfreq> %lu\n", &(*d_info)[handle_count].m_eFrequency);
				retErr = fscanf(pFILE, "<bandwidth> %lu\n", &(*d_info)[handle_count].m_Bandwidth);
			}
			else if (!strcmp("RWRegister", cmd)) {
				retErr = fscanf(pFILE, "[%d]\n", &retErr);
				retErr = fscanf(pFILE, "<0:Read, 1:Write> %d\n", &(*d_info)[handle_count].rw_Flag);
				retErr = fscanf(pFILE, "<0:LINK, 1:OFDM> %d\n", &(*d_info)[handle_count].rw_processor);
				retErr = fscanf(pFILE, "<Address (Hex)> %x\n", &(*d_info)[handle_count].rw_regaddr);
				if ((*d_info)[handle_count].rw_Flag)
					retErr = fscanf(pFILE, "<Write Value (Hex)> %x\n", &(*d_info)[handle_count].w_value);
			}
		}
		retErr = fscanf(pFILE, "--------------------\n");
	}

	if (pFILE)
		fclose(pFILE);

	return open_handle_number;
}

int main(int argc, char **argv)
{
	Byte handle_count = 0, open_handle_number = 0;
	int ret = 0;
	int kernel_version[3];
	pthread_t *dev_thread;
	PDevInfo_t d_info;
	struct stat buf;
	memset(&d_info, 0, sizeof(d_info));

	kernel_version[0] = LINUX_VERSION_CODE >> 16;
	kernel_version[1] = (LINUX_VERSION_CODE >> 8) - (kernel_version[0] << 8);
	kernel_version[2] = LINUX_VERSION_CODE - (kernel_version[0] << 16) - (kernel_version[1] << 8);
	printf("KERNEL_VERSION(%d, %d, %d)\n", kernel_version[0], kernel_version[1], kernel_version[2]);

	/* Init */
	open_handle_number = Get_Params_From_File(&d_info);
	dev_thread = malloc(sizeof(pthread_t) * open_handle_number);

	if (stat("./Output Msg/",&buf)) {
		if (mkdir("./Output Msg/", S_IRWXU)) {
			printf("create ./Output Msg/ folder fail \n");
			return 0;
		}
	}

	/* Create thread */
	for (handle_count = 0; handle_count < open_handle_number; handle_count++)
		pthread_create(&dev_thread[handle_count], NULL, DTV_Menu, (void*) &d_info[handle_count]);

	for (handle_count = 0; handle_count < open_handle_number; handle_count++)
		pthread_join(dev_thread[handle_count], (void*) &ret);

	/* Free resource */
	pthread_mutex_destroy(&mutex);
	free(menu_list);
	free(d_info);

	return 0;
}
