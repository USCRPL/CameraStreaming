/**
 * Copyright (c) 2015 ITE Corporation. All rights reserved. 
 * 
 * Date:
 *    2015/01/14
 *
 * Abstract:
 *    ITEAPIEXTest_RX.cpp : Defines the entry point for the console application.
 *    This is a sample testkit for IT9500 device (modulator).
 */

#include "api.h"

#define ERROR(x...)												\
	do {														\
		fprintf(stderr, "ERROR: ");								\
		fprintf(stderr, x);										\
		fprintf (stderr, "\n");									\
	} while (0)

#define PERROR(x...)											\
	do {														\
		fprintf(stderr, "ERROR: ");								\
		fprintf(stderr, x);										\
		fprintf (stderr, " (%s)\n", strerror(errno));			\
	} while (0)


typedef struct {
	char *name;
	int value;
} Param;

typedef struct {
	uint32_t count;
	uint32_t ulErrCount;
	uint32_t ulLostCount;
	uint16_t pid;
	uint8_t  sequence1;
	uint8_t  sequence2;
	uint8_t  dup_flag;
} PIDINFO, *PPIDINFO;

static const Param inversion_list [] = {
	{ "INVERSION_OFF", INVERSION_OFF },
	{ "INVERSION_ON", INVERSION_ON },
	{ "INVERSION_AUTO", INVERSION_AUTO }
};

static const Param bw_list [] = {
	{ "BANDWIDTH_6_MHZ", BANDWIDTH_6_MHZ },
	{ "BANDWIDTH_7_MHZ", BANDWIDTH_7_MHZ },
	{ "BANDWIDTH_8_MHZ", BANDWIDTH_8_MHZ }
};

static const Param fec_list [] = {
	{ "FEC_1_2", FEC_1_2 },
	{ "FEC_2_3", FEC_2_3 },
	{ "FEC_3_4", FEC_3_4 },
	{ "FEC_4_5", FEC_4_5 },
	{ "FEC_5_6", FEC_5_6 },
	{ "FEC_6_7", FEC_6_7 },
	{ "FEC_7_8", FEC_7_8 },
	{ "FEC_8_9", FEC_8_9 },
	{ "FEC_AUTO", FEC_AUTO },
	{ "FEC_NONE", FEC_NONE }
};

static const Param guard_list [] = {
	{"GUARD_INTERVAL_1_16", GUARD_INTERVAL_1_16},
	{"GUARD_INTERVAL_1_32", GUARD_INTERVAL_1_32},
	{"GUARD_INTERVAL_1_4", GUARD_INTERVAL_1_4},
	{"GUARD_INTERVAL_1_8", GUARD_INTERVAL_1_8}
};

static const Param hierarchy_list [] = {
	{ "HIERARCHY_1", HIERARCHY_1 },
	{ "HIERARCHY_2", HIERARCHY_2 },
	{ "HIERARCHY_4", HIERARCHY_4 },
	{ "HIERARCHY_NONE", HIERARCHY_NONE }
};

static const Param constellation_list [] = {
	{ "QPSK", QPSK },
	{ "QAM_128", QAM_128 },
	{ "QAM_16", QAM_16 },
	{ "QAM_256", QAM_256 },
	{ "QAM_32", QAM_32 },
	{ "QAM_64", QAM_64 }
};

static const Param transmissionmode_list [] = {
	{ "TRANSMISSION_MODE_2K", TRANSMISSION_MODE_2K },
	{ "TRANSMISSION_MODE_8K", TRANSMISSION_MODE_8K },
};

#define LIST_SIZE(x) sizeof(x)/sizeof(Param)


int gChoseBand;
int gFreq;
uint32_t gTransferInterval = 0;

static int GetDriverInfo()
{
	int res = 0;
	DemodDriverInfo driverInfo;
	Dword dwError = ERR_NO_ERROR;

	dwError = DTV_GetVersion(&driverInfo);
	if (dwError) {
		printf("\nGet Driver Info failed 0x%X!\n", (unsigned int)dwError);
		res = -1;
	}
	else {
		printf("DriverVerion  = %s\n", driverInfo.DriverVerion);
		printf("APIVerion     = %s\n", driverInfo.APIVerion);
		printf("FWVerionLink  = %s\n", driverInfo.FWVerionLink);
		printf("FWVerionOFDM  = %s\n", driverInfo.FWVerionOFDM);
		printf("Company       = %s\n", driverInfo.Company);
		printf("SupportHWInfo = %s\n", driverInfo.SupportHWInfo);
	}

	return res;
}

static int ChannelLock()
{
	Dword dwFrequency = 0;
    Word  wBandwidth = 0;
	Dword dwStatus = 0;
	Bool boolIsLock = 0;
	DTVStatistic StatisicEx;
	int res;

	printf("\n=> Please Input Frequency (KHz): ");
	res = scanf("%lu", &dwFrequency);
	gFreq = dwFrequency;

	printf("\n=> Please Input Bandwidth in KHz (ex. 2000-8000KHz): ");
	res = scanf("%d",&gChoseBand);

	if(gChoseBand>=2000 && gChoseBand<3000) wBandwidth = 2;
	else if(gChoseBand>=3000 && gChoseBand<4000) wBandwidth = 3;
	else if(gChoseBand>=4000 && gChoseBand<5000) wBandwidth = 4;
	else if(gChoseBand>=5000 && gChoseBand<6000) wBandwidth = 5;
	else if(gChoseBand>=6000 && gChoseBand<7000) wBandwidth = 6;
	else if(gChoseBand>=7000 && gChoseBand<8000) wBandwidth = 7;
	else if(gChoseBand>=8000 && gChoseBand<9000) wBandwidth = 8;
	else {
		printf("No Supported Bandwidth!\n"); 
		return ERR_INVALID_BW;
	}

	printf("\n\n**************** Lock Status ****************");
	printf("\nFrequency = %lu KHz\n", dwFrequency);
	printf("Bandwidth = %i MHz\n\n", wBandwidth);
    
	dwStatus = DTV_AcquireChannel(dwFrequency, wBandwidth);
	if (dwStatus)
		printf("DTV_AcquireChannel() return error!\n");

	dwStatus = DTV_DisablePIDTbl();
	if (dwStatus)
		printf("DTV_DisablePIDTbl() return error!\n");

	dwStatus = DTV_IsLocked((Bool *)&boolIsLock);
	if (dwStatus)
		printf("DTV_IsLocked() return error!\n");

	if (boolIsLock) {
		dwStatus =  DTV_GetStatisticEx(&StatisicEx);
		if (dwStatus)
			printf("DTV_GetStatisticEx() return error!\n");
		
		if (StatisicEx.signalLocked)
			printf("*** Channel Locked ***\n");
		else
			printf("*** Channel Unlocked ***\n");
	} else {
			printf("*** Channel Unlocked ***\n");
	}
	return 0;
}

static int MultiChannelTest()
{
	Dword dwFrequency = 0;
    Word  wBandwidth = 0;
	Dword dwStatus = 0;
	Bool boolIsLock = 0;
	Dword dwpPostErrCnt;    // ErrorBitCount
	Dword dwpPostBitCnt;    // TotalBitCount
    Word  wpAbortCnt;       // number of abort RSD packet
    Word  wpQuality;        // signal quality (0 - 100)
    Word  wpStrength;		// signal strength (0 - 100)
	DTVStatistic StatisicEx;
	int startFreq, stopFreq;
	int channels = 0, res;
	FILE* pFile = NULL;

	pFile = fopen("scanlog.txt", "w");

	printf("\n=> Please Input Start Frequency (KHz): ");
	res = scanf("%d", &startFreq);
	
	printf("\n=> Please Input Stop Frequency (KHz): ");
	res = scanf("%d", &stopFreq);
	
	printf("\n=> Please Choose Bandwidth (0:8MHz  1:7MHz  2:6MHz): ");
	res = scanf("%d",&gChoseBand);

	if (gChoseBand == 0)
		wBandwidth = 8;
	else if (gChoseBand == 1)
		wBandwidth = 7;
	else
		wBandwidth = 6;

	gFreq = startFreq;

	while (gFreq <= stopFreq)
	{
		dwFrequency = gFreq;

		dwStatus = DTV_AcquireChannel(dwFrequency, wBandwidth);
		if (dwStatus)
			printf("DTV_AcquireChannel() return error!\n");

		usleep(2500000);

		dwStatus =  DTV_GetStatistic(10000,
										(Dword *)&dwpPostErrCnt,
										(Dword *)&dwpPostBitCnt,
										(Word *)&wpAbortCnt,
										(Word *)&wpQuality,
										(Word *)&wpStrength);

		if (dwStatus)
			printf("DTV_GetStatistic() return error!\n");

		dwStatus =  DTV_GetStatisticEx(&StatisicEx);
		if (dwStatus)
			printf("DTV_GetStatisticEx() return error!\n");

		dwStatus = DTV_IsLocked((Bool *)&boolIsLock);
		if (dwStatus)
			printf("DTV_IsLocked() return error!\n");

		if (boolIsLock) {
			channels ++;
			printf("\n======  Statistics   ======\n");
			printf("Frequency = %i KHz\n", gFreq);
			printf("Demod locked: %d\n", StatisicEx.signalLocked);
			printf("TPS locked :%d\n", StatisicEx.signalPresented);
			printf("Quality: %d\n", wpQuality);
			printf("Strength: %d\n", wpStrength);
			if (dwpPostBitCnt != 0)
				printf("BER: %3.3e\n", ((float)dwpPostErrCnt)/dwpPostBitCnt);
			else
				printf("BER: %3.3e\n", (double) dwpPostBitCnt);
			printf("Abort Count: %d\n", wpAbortCnt);

			printf("===============================\n");

			if (pFile) {
				fprintf(pFile, "\n======  Statistics   ======\n");
				fprintf(pFile, "Frequency = %i KHz\n", gFreq);
				fprintf(pFile, "Demod locked: %d\n", StatisicEx.signalLocked);
				fprintf(pFile, "TPS locked :%d\n", StatisicEx.signalPresented);
				fprintf(pFile, "Quality: %d\n", wpQuality);
				fprintf(pFile, "Strength: %d\n", wpStrength);
				if (dwpPostBitCnt != 0)
					fprintf(pFile, "BER: %3.3e\n", ((float)dwpPostErrCnt)/dwpPostBitCnt);
				else
					fprintf(pFile, "BER: %3.3e\n", (double) dwpPostBitCnt);
				fprintf(pFile, "Abort Count: %d\n", wpAbortCnt);

				fprintf(pFile, "===============================\n");
			}
		}

		if (gChoseBand == 0)
			gFreq = gFreq + 8000;
		else if (gChoseBand == 1)
			gFreq = gFreq + 7000;
		else
			gFreq = gFreq + 6000;

	}

	printf("\n============ Analysis ============");
	printf("\n Frequency %d(KHz) ~  %d(KHz)", startFreq, stopFreq);
	printf("\n Scan Channel : %d channels Locked  \n", channels);

	if (pFile) {
		fprintf(pFile, "\n============ Analysis ============");
		fprintf(pFile, "\n Frequency %d(KHz) ~  %d(KHz)", startFreq, stopFreq);
		fprintf(pFile, "\n Scan Channel : %d channels Locked  \n", channels);
		fclose(pFile);
		printf("\n Save log to file scanlog.txt\n\n");
	}

	return 0;
}

static int ChannelStatisticTest()
{
	Dword dwpPostErrCnt;    // ErrorBitCount
	Dword dwpPostBitCnt;    // TotalBitCount
    Word  wpAbortCnt;       // number of abort RSD packet
    Word  wpQuality;        // signal quality (0 - 100)
    Word  wpStrength;		// signal strength (0 - 100)
	Dword dwStatus;
	DTVStatistic StatisicEx;
	int chose, res;
	int count = 1, i = 0;
	FILE *pFile = NULL;

	printf("\n1. Display Statistics	      ");
	printf("\n2. Monitor Statistics        ");
	printf("\n3. Return to Main Menu       ");
	printf("\n=> Please Input Your Choice: ");
	res = scanf("%d", &chose);

	switch (chose)
	{
		case 1: 
			count = 1;
			break;

		case 2:
			pFile = fopen("Statistics.txt", "w");

			printf("\n=> Monitor Times: ");
			res = scanf("%d", &count);

			break;

		default:
			return 0;
	}

	do {
		dwStatus =  DTV_GetStatistic(10000,
										(Dword *)&dwpPostErrCnt,
										(Dword *)&dwpPostBitCnt,
										(Word *)&wpAbortCnt,
										(Word *)&wpQuality,
										(Word *)&wpStrength);

		if (dwStatus)
			printf("DTV_GetStatistic() return error!\n");

		dwStatus =  DTV_GetStatisticEx(&StatisicEx);
		if (dwStatus)
			printf("DTV_GetStatisticEx() return error!\n");
		
		printf("\n*******************************\n*** Channel Statistics: ***\n");

		printf("Demod locked: %d\n", StatisicEx.signalLocked);
		printf("TPS locked :%d\n", StatisicEx.signalPresented);
		printf("Quality: %d\n", wpQuality);
		printf("Strength: %d\n", wpStrength);
		if (dwpPostBitCnt != 0)
			printf("BER: %3.3e\n", ((float)dwpPostErrCnt)/dwpPostBitCnt);
		else
			printf("BER: %3.3e\n", (double) dwpPostBitCnt);
		printf("Abort Count: %d\n", wpAbortCnt);

		printf("*******************************\n");
		printf("\n");

		if (pFile) {
			fprintf(pFile, "\n*******************************\n*** Channel Statistics: ***\n");

			fprintf(pFile, "Demod locked: %d\n", StatisicEx.signalLocked);
			fprintf(pFile, "TPS locked :%d\n", StatisicEx.signalPresented);
			fprintf(pFile, "Quality: %d\n", wpQuality);
			fprintf(pFile, "Strength: %d\n", wpStrength);
			if (dwpPostBitCnt != 0)
				fprintf(pFile, "BER: %3.3e\n", ((float)dwpPostErrCnt)/dwpPostBitCnt);
			else
				fprintf(pFile, "BER: %3.3e\n", (double) dwpPostBitCnt);
			fprintf(pFile, "Abort Count: %d\n", wpAbortCnt);

			fprintf(pFile, "*******************************\n");
			fprintf(pFile, "\n");
		}

		usleep(1000000);
		i++;
	} while (i < count);

	if (pFile)
		fclose(pFile);

	return 0;
}


#define BSIZE 188

PIDINFO pidt[0x2001];

int dvbtraffic(int save, int count)
{
	int tempCount = 0;
	int packets = 0;
	struct timeval startt, startt2;
	char *search;
	unsigned char buffer[BSIZE];
	int i = 0, j = 0;
	uint8_t seq;
	uint8_t adaptationField;
	uint8_t checkBit;
	FILE *pFile = NULL;

	if (save) {
		pFile = fopen("DVB.ts", "wb");
	}

	for (j = 0; j < 0x2001; j++) {
		pidt[j].count = 0;
		pidt[j].ulLostCount = 0;
		pidt[j].pid = 0;
		pidt[j].sequence1 = 0xFF;
		pidt[j].sequence2 = 0xFF;
	}

	DTV_StartCapture();

	//usleep(2000000);

	gettimeofday(&startt, 0);
	startt2 = startt;

	//if (argc > 1)
	//	search = argv[1];
	//else
		search = 0;

	j = 0;
	if (count == -1)
		i = -2; // for Burn in test
	
	while (i < count) {
		int pid, ok;
		Dword r;
		r = 188;
		DTV_GetData(buffer, &r);
		//printf("DTV_GetData - %d\n", r);
		if (r <= 0) {
			//printf("read%d", j);
			usleep(100000);
			j++;
			if (j>100) // 10 sec
				break;
			continue;
		}
		j = 0;
		if (r != 188) {
			printf("only read %lu\n", r);
			break
			;
		}
		if (buffer[0] != 0x47) {
			//continue;
			printf("desync (%x, %x, %x, %x) - %lu\n", buffer[0], buffer[1], buffer[2], buffer[3], r);
			while (buffer[0] != 0x47) {
			r = 1;
				DTV_GetData(buffer, &r);
			}
			r = 187;
			DTV_GetData(buffer, &r);
			continue;
			tempCount++;
		}
			//printf("tempCount %d\n", tempCount);
		ok = 1;
		pid = ((((unsigned) buffer[1]) << 8) |
		       ((unsigned) buffer[2])) & 0x1FFF;

		checkBit = buffer[1] & 0x80;
		adaptationField = (buffer[3]&0x30) >> 4;
		seq = buffer[3] & 0xf;

		if (search) {
			int i, sl = strlen(search);
			ok = 0;
			if (pid != 0x1fff) {
				for (i = 0; i < (188 - sl); ++i) {
					if (!memcmp(buffer + i, search, sl))
						ok = 1;
				}
			}
		}

		if (pid == 0x1FFF) {
			//printf("\nSave %d KB...\n", packets*188/1024);
			continue;
		}

		if (ok) {
			pidt[pid].count++;

			if (pidt[pid].sequence1 == 0xFF) {
				//the first packet arrives, nothing to do
				pidt[pid].sequence1 = seq;
			}
			else {
				if (adaptationField==0 || adaptationField==2) {
					//No increment(lost) check for type 0 & 2 
				}
				else {
					//Increment(lost) check for type 1 & 3
					if ((pidt[pid].sequence1==seq) && (pidt[pid].dup_flag==0)) {
						// Duplicate packets may be sent as two, and only two, consecutive Transport Stream packets of the same PID
						pidt[pid].dup_flag = 1; // No more duplicate allowed
					}
					else {	
						if (pidt[pid].sequence1+1 == seq) {
							//no packet loss, expected packet arrives 
							if (pidt[pid].count == 100000)
								printf("%d", pidt[pid].ulLostCount);
						}
						else {
							if (pidt[pid].sequence1+1 < seq) {
								pidt[pid].ulLostCount += seq - (pidt[pid].sequence1+1);
							}
							else {
								pidt[pid].ulLostCount += (seq+0x10) - (pidt[pid].sequence1+1);
							}
						}
						pidt[pid].dup_flag = 0; //next duplicate allowed
						pidt[pid].sequence1 = seq;
					}
				}
			}
                    
			//Check if TEI error  
			if (checkBit != 0x00)
				pidt[pid].ulErrCount++;
		}

		packets++;

		if (!(packets & 0xFF)) {
			struct timeval now;
			uint32_t diff, diff2;
			gettimeofday(&now, 0);
			diff2 =
			    (now.tv_sec - startt2.tv_sec) * 1000 +
			    (now.tv_usec - startt2.tv_usec) / 1000;
			if (diff2 > 1000) {
				int n = 0;
				diff =
			   		(now.tv_sec - startt.tv_sec) * 1000 +
			   		(now.tv_usec - startt.tv_usec) / 1000;
				printf("\n-PID-------Total----Lost----Error---Mbps-\n");
				for (n = 0; n < 0x2001; n++) {
					if (pidt[n].count) {
						printf("%04x %10d %7d %7d %8d\n",
								n,
								pidt[n].count,
								pidt[n].ulLostCount,
								pidt[n].ulErrCount,
								pidt[n].count * 188 / 1000 * 8 * 1000 / diff);
					}
				}
				startt2 = now;
				if(i >= 0) i++;

				if (save) {
					printf("\nSave %d KB...\n", packets*188/1024);
				}
			}
		}

		if (save) {
			i = (packets*188) / 1024 / 1024; // MB
			fwrite(buffer, 1, sizeof(buffer), pFile);
		}
	}

	DTV_StopCapture();

	if (pFile) {
		fclose(pFile);
		printf("\n*** Save File to DVB.ts (%dM) ***\n", i);
	}

	return 0;
}

static int PacketTest()
{
	int chose, res;
	int count = 1;

	printf("\n1. Packet Error Testing      ");
	printf("\n2. Save Packet to File       ");
	printf("\n3. Infinite Testing      ");
	printf("\n4. Return to Main Menu       ");
	printf("\n=> Please Choice : ");
	res = scanf("%d", &chose);

	switch (chose)
	{
		case 1: 
			printf("\n=> Testing Times: ");
			res = scanf("%d", &count);

			dvbtraffic(0, count);

			break;

		case 2:
			printf("\n=> File Size (MB): ");
			res = scanf("%d", &count);

			dvbtraffic(1, count);

			break;
		
		case 3: 

			dvbtraffic(0, -1);

			break;
	}

	return 0;
}

static int RWRegister()
{
	int res = 0;
	int rwFlag = 0, processor = 0;
	uint32_t regAddr = 0, vaule = 0;
	Dword dwStatus;

	printf("\n=> Please Choose Read/Write Register (0: Read, 1: Write): ");
	res = scanf("%d", &rwFlag);

	printf("\n=> Please Choose LINK or OFDM (0: LINK, 1: OFDM): ");
	res = scanf("%d", &processor);

	printf("\n=> Please Enter Read/Write Register Address (Hex): ");
	res = scanf("%x", &regAddr);

	if (rwFlag == 1) { // Write register
		printf("\n=> Please Enter Write Value (Hex): ");
		res = scanf("%x", &vaule);

		if (processor == 0) {
			dwStatus = DTV_WriteRegLINK((Word)regAddr, (Byte)vaule);
			if (dwStatus)
				printf("DTV_WriteRegLINK() return error!\n");
			else
				printf("\nWrite LINK Address 0x%X Value 0x%X OK.\n", regAddr, vaule);
		}
		else {
			dwStatus = DTV_WriteRegOFDM((Word)regAddr, (Byte)vaule);
			if (dwStatus)
				printf("DTV_WriteRegOFDM() return error!\n");
			else
				printf("\nWrite OFDM Address 0x%X Value 0x%X OK.\n", regAddr, vaule);
		}
	}
	else { // Read register
		if (processor == 0) {
			dwStatus = DTV_ReadRegLINK((Word)regAddr, (Byte*) &vaule);
			if (dwStatus)
				printf("DTV_ReadRegLINK() return error 0x%lX!\n", dwStatus);
			else
				printf("\nRead LINK Address 0x%X Value 0x%X OK.\n", regAddr, vaule);
		}
		else {
			dwStatus = DTV_ReadRegOFDM((Word)regAddr, (Byte*) &vaule);
			if (dwStatus)
				printf("DTV_ReadRegOFDM() return error!\n");
			else
				printf("\nRead OFDM Address 0x%X Value 0x%X OK.\n", regAddr, vaule);
		}
	}
	return res;
}

int main(int argc, char **argv)
{
	int chose, res;
	int closeFlag = 0;
	struct timeval;
	Byte handleNum;
	Dword decryptKey;
	int decryptEnable;
		
	if(argv[1]==NULL){
		printf("\n\n================ Open default device handle ==================\n");
		printf("= To chose another driver handle. Please input handle number =\n");		
		printf("= Example: ./testkit_it913x_tx 1 -> for usb-it950x-rx1 handle ===\n");
		printf("==============================================================\n");		
		handleNum = 0;
	} else {
		handleNum = atoi(argv[1]);
		if(atoi(argv[1]) < 0) {
			printf("\n=============== The bad handle number! Please input again! =============\n\n\n");
			printf("\n===================== To chose driver handle sample ====================\n");
			printf("======= Example: ./testkit_it913x_tx   -> for usb-it950x-rx0 handle =======\n");			
			printf("======= Example: ./testkit_it913x_tx 1 -> for usb-it950x-rx1 handle =======\n");
			printf("========================================================================\n");
			return 0;	
		}
	}
	if(DTV_Initialize(handleNum) == ERR_INVALID_DEV_TYPE)
		return 0;	
	
	printf("\n========== ITEtech Linux DTV Testkit ==========\n");

	GetDriverInfo();

	printf("\n===============================================\n");

	while (!closeFlag)
	{

		printf("\n======= ITEtech Linux DTV Testkit =======\n");
		printf("\n1. Lock Channel              ");
		printf("\n2. Singal Quality Statistics ");
		printf("\n3. Record & Analyze Packets  ");
		printf("\n4. Mutil-Channel Lock Test   ");
		printf("\n5. Read/Write Register	   	");
		printf("\n6. Set TPS Decryption		   	");
		printf("\n0 Quit                      ");
		printf("\n=> Please Input Your Choice: ");
		res = scanf("%d", &chose);
    
		switch (chose)
		{
			case 1: 
				if (ChannelLock() < 0) {
					printf("Lock channel fail!\n");
				}

				break;

			case 2:
				ChannelStatisticTest();

				break;

			case 3:
				PacketTest();

				break;

			case 4:
				MultiChannelTest();
				break;

			case 5:
				RWRegister();
				break;
			case 6:
				decryptKey = 0;
				decryptEnable = 0;
				printf("Enable TPS Decryption Key(0:False, 1:True)?");
				res = scanf("%d", &decryptEnable);
				if(decryptEnable) {
					printf("Set TPS Decryption Key:");
					res = scanf("%lx", &decryptKey);
				}
				printf("DecryptEnable: %d\n", decryptEnable);
				printf("DecryptKey: %lx\n", decryptKey);
				if(DTV_SetDecrypt(decryptKey, (Byte) decryptEnable) == 0)
					printf("DTV_SetDecrypt ok\n");						
				else
					printf("DTV_SetDecrypt fail\n");					
				break;
			case 0:
				closeFlag = 1;
				break;
		}
	}
	DTV_Finalize();

	return 0;
}

