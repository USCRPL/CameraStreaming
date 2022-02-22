//
// ITEAPIEXTest_TX.cpp : Defines the entry point for the console application.
//
// This is a sample testkit for IT9500 device (modulator).
//

#include "stdafx.h"
#include "ITEAPIEXTest_TX.h"
#include <conio.h>


ULONG g_ChannelCapacity=0;
BYTE NullPacket[188]={0x47,0x1f,0xff,0x1c,0x00,0x00};
DWORD g_Frequency = 0;
UINT g_Bandwidth = 0;
MODULATION_PARAM g_ChannelModulation_Setting;

#include "crc.h"

//Test Periodical Custom Packets Insertion, (for SI/PSI table insertion)

//Sample SDT
BYTE CustomPacket_1[188]={
	0x47,0x40,0x11,0x10,0x00,0x42,0xF0,0x36,0x00,0x99,0xC1,0x00,0x00,0xFF,0x1A,0xFF,
	0x03,0xE8,0xFC,0x80,0x25,0x48,0x23,0x01,0x10,0x05,0x49,0x54,0x45,0x20,0X20,0X20,
	0X20,0X20,0X20,0X20,0X20,0X20,0X20,0X20,0X20,0x10,0x05,0x49,0x54,0x45,0x20,0x43,
	0x68,0x61,0x6E,0x6E,0x65,0x6C,0x20,0x31,0x20,0x20,0xFF,0xFF,0xFF,0xFF //LAST 4 BYTE=CRC32
	};

//Sample NIT
BYTE CustomPacket_2[188]={
	0x47,0x40,0x10,0x10,0x00,0x40,0xf0,0x38,0xff,0xaf,0xc1,0x00,0x00,0xf0,0x0d,0x40,/*0x00000000*/
	0x0b,0x05,0x49,0x54,0x45,0x20,0x4f,0x66,0x66,0x69,0x63,0x65,0xf0,0x1e,0x00,0x99,/*0x00000010*/
	0xff,0x1a,0xf0,0x18,0x83,0x04,0x03,0xe8,0xfc,0x5f,0x5a,0x0b,0x02,0xeb,0xae,0x40,/*0x00000020*/
	0x1f,0x42,0x52,0xff,0xff,0xff,0xff,0x41,0x03,0x03,0xe8,0x01,0x1a,0xe6,0x2c,0x3f,/*0x00000030*/
};
BYTE CustomPacket_3[188]={0x47,0x10,0x03,0x1c,0x00,0x00};
BYTE CustomPacket_4[188]={0x47,0x10,0x04,0x1c,0x00,0x00};
BYTE CustomPacket_5[188]={0x47,0x10,0x05,0x1c,0x00,0x00};


bool TxDevice_init(int Tx_DevNo)
{	
	// Initial Device
	if (g_ITEAPI_TxDeviceInit(Tx_DevNo))
	{
		printf("g_ITEAPI_TxDeviceInit(%d) ok\n", Tx_DevNo);
	}
	else
	{
		printf("g_ITEAPI_TxDeviceInit(%d) fail\n", Tx_DevNo);
		return false;
	}
	
	// Get Dll Version
	printf("DLL_VERSION = %s ok\n", g_ITEAPI_GetDllVersion());
	
	DRV_INFO _DriverInfo;
	if(g_ITEAPI_GetDrvInfo(&_DriverInfo, Tx_DevNo) == 0)
	{
		printf("g_ITEAPI_GetDrvInfo(PDRV_INFO) ok\n");
		printf("_DriverInfo.DriverPID = 0x%X\n", _DriverInfo.DriverPID);
		printf("_DriverInfo.DriverVersion = 0x%X\n", _DriverInfo.DriverVersion); 
		printf("_DriverInfo.FwVersion_LINK = 0x%X\n", _DriverInfo.FwVersion_LINK);
		printf("_DriverInfo.FwVersion_OFDM = 0x%X\n", _DriverInfo.FwVersion_OFDM); 
		printf("_DriverInfo.TunerID = 0x%X\n\n", _DriverInfo.TunerID);
	}
	else
		printf("g_ITEAPI_GetDrvInfo(PDRV_INFO) false\n");

	// Get Device Information
	DEVICE_INFO _device_info;	
	if(g_ITEAPI_GetDeviceInfo(&_device_info, Tx_DevNo))
	{
		printf("g_ITEAPI_GetDeviceInfo(PDEVICE_INFO) ok\n");
		printf("_device_info.ProductID = 0x%X\n", _device_info.ProductID);
		printf("_device_info.UsbMode = 0x%X\n", _device_info.UsbMode);
		printf("_device_info.VendorID = 0x%X\n\n", _device_info.VendorID);
	}
	else
		printf("g_ITEAPI_GetDeviceInfo(PDEVICE_INFO) fail\n");

	return true;	
}


long TX_SetChannelTransmissionParameters(int Tx_Dev_Num)
{
	UINT temp = 0;
	MODULATION_PARAM ChannelModulation_Setting;

	ULONG ChannelCapacity=0;

	printf("\nEnter frequency in KHz (ex. 666000Khz):  ");
	scanf_s("%d", &g_Frequency);


	printf("\nEnter bandwidth in KHz (ex. 8000Khz):  ");
	scanf_s("%d", &g_Bandwidth);

	if (g_ITEAPI_TxSetChannel(g_Frequency, (WORD)g_Bandwidth, Tx_Dev_Num) == 0)
		printf("g_ITEAPI_TxSetChannel ok\n");
	else
		printf("g_ITEAPI_TxSetChannel error\n");



	printf("\nEnter Constellation(0: QPSK, 1: 16QAM, 2: 64QAM):  ");
	scanf_s("%d", &temp);
	ChannelModulation_Setting.constellation = (BYTE)temp;

	printf("\nEnter Code Rate(0: 1/2, 1: 2/3, 2: 3/4, 3:5/6, 4: 7/8):  ");
	scanf_s("%d", &temp);
	ChannelModulation_Setting.highCodeRate = (BYTE)temp;

	printf("\nEnter Guard Interval(0: 1/32, 1: 1/16, 2: 1/8, 3: 1/4):  ");
	scanf_s("%d", &temp);
	ChannelModulation_Setting.interval = (BYTE)temp;

	printf("\nEnter Transmission Mode(0: 2K, 1: 8K):  ");
	scanf_s("%d", &temp);
	ChannelModulation_Setting.transmissionMode = (BYTE)temp;

	if(g_ITEAPI_TxSetChannelModulation(&ChannelModulation_Setting, Tx_Dev_Num) == 0)
		printf("g_ITEAPI_TxSetChannelModulation ok\n");
	else
		printf("g_ITEAPI_TxSetChannelModulation error\n");

	g_ChannelModulation_Setting=ChannelModulation_Setting;

/*
Tbandwidth = {6,000,000, 7,000,000, 8,000,000} in Hz for 6MHz, 7MHz, 8MHz
Tcode_rate = {1/2, 2/3, 3/4, 5/6, 7/8}
TConstellation = {2, 4, 6} <- QPSK = 2, 16QAM=4, 64QAM = 6
TGuardInterval = {4/5, 8/9, 16/17, 32/33}, 1/4 = 4/5, 1/8 = 8/9, 1/16 => 16/17, 1/32 => 32/33}
2K/8K mode does not matter
 
Maximum bit rate = 1512 / 2048 * 188 / 204 * 64 / 56 * TBandwidth * Tcode_rate * TConstellation * TGuardInterval (bps) 
= 423 / 544 * TBandwidth * Tcode_rate * TConstellation * TGuardInterval (bps)
*/

	ChannelCapacity=g_Bandwidth*1000;
	ChannelCapacity=ChannelCapacity*(ChannelModulation_Setting.constellation*2+2);
	
	switch (ChannelModulation_Setting.interval) {
		case 0: //1/32
			ChannelCapacity=ChannelCapacity*32/33;
			break;
		case 1: //1/16
			ChannelCapacity=ChannelCapacity*16/17;
			break;
		case 2: //1/8
			ChannelCapacity=ChannelCapacity*8/9;
			break;
		case 3: //1/4
			ChannelCapacity=ChannelCapacity*4/5;
			break;
	}
	switch (ChannelModulation_Setting.highCodeRate) {
		case 0: //1/2
			ChannelCapacity=ChannelCapacity*1/2;
			break;
		case 1: //2/3
			ChannelCapacity=ChannelCapacity*2/3;
			break;
		case 2: //3/4
			ChannelCapacity=ChannelCapacity*3/4;
			break;
		case 3: //5/6
			ChannelCapacity=ChannelCapacity*5/6;
			break;
		case 4: //7/8
			ChannelCapacity=ChannelCapacity*7/8;
			break;
	}

	ChannelCapacity=ChannelCapacity/544*423;
    printf("The Maximum Channel Capacity is %d bps(%d Kbps)\n",ChannelCapacity,ChannelCapacity/1000);
	return(ChannelCapacity);
}




//Analyze PAT TSID and SID
long int Get_PAT_TSID_SID(HANDLE  TsFile,byte *TSid,byte *Sid)
{

    int mPID = 0;
	const int BUFSIZE = 512*188; 
	BYTE pbTSBuffData[BUFSIZE]; 
	DWORD   len,FileSize;

	len=FileSize=GetFileSize(TsFile,NULL); 
	if(len > BUFSIZE) 		len = BUFSIZE;


	DWORD dwReadSize = 0;
	DWORD FirstSyncBytePos=0;
	bool bPAT=false, bSDT=false,bNIT=false;




	SetFilePointer(TsFile, 0, NULL, FILE_BEGIN);
	BOOL bRet = ReadFile(TsFile,pbTSBuffData,len,&dwReadSize,NULL);

	//Find the first 0x47 sync byte
	for (FirstSyncBytePos=0;(FirstSyncBytePos<dwReadSize-188);FirstSyncBytePos++) 
	{
			if ((pbTSBuffData[FirstSyncBytePos] == 0x47)&&(pbTSBuffData[FirstSyncBytePos+188] == 0x47))
			{
				//Sync byte found
				//printf("TS Sync byte found in offset:%d\n",FirstSyncBytePos);
				break;
			}
	}

	if (FirstSyncBytePos>=dwReadSize-188) {
		printf("No sync byte found, it's not a valid 188-byte TS file!!!\n");

		return 0;
	}

	SetFilePointer(TsFile, FirstSyncBytePos, NULL, FILE_BEGIN);	

	ReadFile(TsFile,pbTSBuffData,len,&dwReadSize,NULL);
	while (dwReadSize>188&&!bPAT) 
	{		
			for (unsigned i=0;(i<dwReadSize);i+=188) 
			{
				unsigned PID, psi_offset, sec_len,numofserv;

				PID = ((pbTSBuffData[i+1] & 0x1f) << 8) |pbTSBuffData[i+2];	
				if (PID==0x11&&!bSDT)
				{
					bSDT=true;
					printf("Warning: SDT table already exists in this stream!\r\n");
				}

				if (PID==0&&!bPAT)
				{
					psi_offset=pbTSBuffData[i+4];

					if (pbTSBuffData[i+psi_offset+5]!=0) 
					{
						//it's not PAT Table ID
						continue;
					}

					sec_len=pbTSBuffData[i+psi_offset+7];
					memcpy(TSid, pbTSBuffData+i+psi_offset+8,2); //note it's big-endian
					numofserv=(sec_len-9)/4;
					for (unsigned int j=0;j<numofserv;j++) 
					{
						memcpy(Sid, pbTSBuffData+i+psi_offset+0xd+j*4,2); //note it's big-endian
						
						
						if (Sid[0] || Sid[1]) 
						{
							//We only want the first TV service 
							printf("PAT TS ID:0x%02x%02x and Service ID:0x%02x%02x found\r\n",TSid[0],TSid[1],Sid[0],Sid[1]);
							bPAT=true;
							break;
						}
						else 
						{
							//zero service id is NIT, instead of a service
							bNIT=true;
						}
					}
			 
				}
			}
			if (bPAT&&bSDT) break;
			ReadFile(TsFile,pbTSBuffData,len,&dwReadSize,NULL);
	}
	if (!bPAT) printf("No PAT or Service ID found!\r\n");
	return 0;

}

/*****************************************************************************/
//Get PCR (BASE  only, in 90KHz,  no extension)
ULONGLONG Get_PCR(BYTE *pbTSBuffData)
{   
	ULONGLONG pcr=0;
	ULONGLONG pcre=0;
	int adaptation_field_control ;

	unsigned PID;



	PID = ((pbTSBuffData[1] & 0x1f) << 8) |pbTSBuffData[2];	
	if (PID==0x1fff) return 0; //Null packet

	adaptation_field_control = ((pbTSBuffData[3] >> 4) & 0x3);

	if (adaptation_field_control == 3 || adaptation_field_control == 2)
	{
		//CHECK adaptation_field_length !=0 && PCR flag on
		if (pbTSBuffData[4] != 0 && (pbTSBuffData[5] & 0x10) != 0)
		{

		


				pcr=((ULONGLONG)pbTSBuffData[6]<<25) |				
					 (pbTSBuffData[7]<<17) |
					 (pbTSBuffData[8]<<9) |
					 (pbTSBuffData[9]<<1) |
					 (pbTSBuffData[10]>>7) ;

				pcre = ((pbTSBuffData[10] & 0x01) << 8) | (pbTSBuffData[11]);



		}
	}
	if (pcr) printf("PID:0x%x PCR:0x%lx-%lx\n",PID,pcr);
	//Return PCR base, no extension needed
	return pcr;
}

/*****************************************************************************/
ULONGLONG Get_PTS(BYTE *pbTSBuffData)
{
	
	ULONGLONG ullPts = 0;
	ULONGLONG ullDts = 0;
	int adaptation_field_control,payload_unit_start_indicator ;
    int adaptation_field_length = 0;

	unsigned PID;


	PID = ((pbTSBuffData[1] & 0x1f) << 8) |pbTSBuffData[2];	
	if (PID==0x1fff) return 0; //Null packet

	adaptation_field_control = ((pbTSBuffData[3] >> 4) & 0x3);
	payload_unit_start_indicator = ((pbTSBuffData[1] >> 6) & 0x01);
	adaptation_field_length = pbTSBuffData[4];



                     if ((adaptation_field_control == 1 || adaptation_field_control == 3)) {

                           if (payload_unit_start_indicator == 1)

                           {

                                int nIndex = 4+1+adaptation_field_length;

                                DWORD packet_start_code_prefix = (pbTSBuffData[nIndex] << 16) | (pbTSBuffData[nIndex+1] << 8) | (pbTSBuffData[nIndex+2]);

                                BYTE bStreamID = pbTSBuffData[nIndex+3];

                                if (packet_start_code_prefix == 0x1) {

                                     DWORD PES_packet_length = ((pbTSBuffData[nIndex+4]) << 8 ) | pbTSBuffData[nIndex+5];

                                  

                                     //if ((PES_packet_length) > 14) 

                                     {    

                                           BYTE bTmpTSData = pbTSBuffData[nIndex+7];

 

                                           BYTE bPTS_DTS_flags = ((bTmpTSData >> 6) & 0x3);

                                           BYTE bESCR_flags = ((bTmpTSData >> 5) & 0x1);

                                           BYTE bES_rate_flags = ((bTmpTSData >> 4) & 0x1);

                                           BYTE bPES_CRC_flags = ((bTmpTSData >> 1) & 0x1);

 



                                           ULONG lAPTS_New = 0;

 

                                           if(bPTS_DTS_flags == 2 || bPTS_DTS_flags == 3)

                                           {

                                                if (((pbTSBuffData[nIndex+9] & 0xE1) != 0x21) || 

                                                     ((pbTSBuffData[nIndex+11] & 1) != 1) || 

                                                     ((pbTSBuffData[nIndex+13] & 1) != 1)) {

 

                                                     // Wrong PTS format

                                                     ullPts = 0;

                                                } else {

                                                     ullPts = (((ULONGLONG)(pbTSBuffData[nIndex+9] & 0x0E) << 29) |

                                                           ((pbTSBuffData[nIndex+10] & 0xFF) << 22) |

                                                           ((pbTSBuffData[nIndex+11] & 0xFE) << 14) |

                                                           ((pbTSBuffData[nIndex+12] & 0xFF) << 7) |

                                                           ((pbTSBuffData[nIndex+13] & 0xFE) >> 1));

                                                }

 

                                                if(bPTS_DTS_flags == 3)

                                                {

                                                     // DTS available 

                                                     if (((pbTSBuffData[nIndex+14] & 0xF1) != 0x11) || 

                                                           ((pbTSBuffData[nIndex+16] & 1) != 1) || 

                                                           ((pbTSBuffData[nIndex+18] & 1) != 1)) {

                                                           // Wrong DTS format

                                                           ullDts = 0;

                                                     } else {

                                                           ullDts = (((ULONGLONG)(pbTSBuffData[nIndex+14] & 0x0E) << 29) |

                                                                ((pbTSBuffData[nIndex+15] & 0xFF) << 22) |

                                                                ((pbTSBuffData[nIndex+16] & 0xFE) << 14) |

                                                                ((pbTSBuffData[nIndex+17] & 0xFF) << 7) |

                                                                ((pbTSBuffData[nIndex+18] & 0xFE) >> 1));                             

                                                     }

 

                                                }

 

                                           }

 

                                     } 

                                }

                           }

 

                     }

                     ///////////////////////////////

	if (ullPts)
		printf("PID:0x%x PTS:(0x%lx-%lx) DTS:(0x%lx-%lx)\n",PID,ullPts,ullDts);
	return ullPts;
}


/*****************************************************************************/
long int Get_DataBitRate_S(HANDLE  TsFile)
{

    int mPID = 0;
	const int BUFSIZE = 512*188; 
		BYTE pbTSBuffData[BUFSIZE]; 
	DWORD   len,FileSize;

	double min_Packet_Time=0xfffffff;   // in xxx  Hz per Packet, 90KHz PCR clock
	double max_Packet_Time=0x0;		 	// in xxx  Hz per Packet, 90KHz PCR clock
	double Packet_Time=0x0;

	ULONGLONG pcr_diff;
	ULONG     pkt_diff;


	len=FileSize=GetFileSize(TsFile,NULL); 
	if(len > BUFSIZE) 		len = BUFSIZE;


	DWORD dwReadSize = 0;

	DWORD FirstSyncBytePos=0;
	ULONG FileOffset=0;

	ULONGLONG pcr1=0,pcr2=0,prev_pcr=0;
	ULONGLONG pcr1_offset=0,pcr2_offset=0,prev_pcr_offset=0;
	long int lStreamBitRate = 0;

	SetFilePointer(TsFile, 0, NULL, FILE_BEGIN);
	BOOL bRet = ReadFile(TsFile,pbTSBuffData,len,&dwReadSize,NULL);

	//Find the first 0x47 sync byte
	for (FirstSyncBytePos=0;(FirstSyncBytePos<dwReadSize-188);FirstSyncBytePos++) 
	{
			if ((pbTSBuffData[FirstSyncBytePos] == 0x47)&&(pbTSBuffData[FirstSyncBytePos+188] == 0x47))
			{
				//Sync byte found
				printf("TS Sync byte found in offset:%d\n",FirstSyncBytePos);
				break;
			}
	}

	if (FirstSyncBytePos>=dwReadSize-188) {
		printf("No sync byte found, it's not a valid 188-byte TS file!!!\n");

		return 0;
	}

	SetFilePointer(TsFile, FirstSyncBytePos, NULL, FILE_BEGIN);	
	ReadFile(TsFile,pbTSBuffData,len,&dwReadSize,NULL);


	FileOffset=FirstSyncBytePos;
	while (dwReadSize>188) 
	{		
			for (ULONG i=0;(i<dwReadSize);i+=188) 
			{
				int adaptation_field_control ;
				unsigned PID;
				PID = ((pbTSBuffData[i+1] & 0x1f) << 8) |pbTSBuffData[i+2];	
				adaptation_field_control = ((pbTSBuffData[i+3] >> 4) & 0x3);
				if ( PID!=0x1fff &&(adaptation_field_control == 3 || adaptation_field_control == 2))
				{
					//CHECK adaptation_field_length !=0 && PCR flag on
					if (pbTSBuffData[i+4] != 0 && (pbTSBuffData[i+5] & 0x10) != 0)
					{

							
							ULONGLONG pcre;
							ULONGLONG pcr;


							
							
							pcr=((ULONGLONG)pbTSBuffData[i+6]<<25) |
								 (pbTSBuffData[i+7]<<17) |
								 (pbTSBuffData[i+8]<<9) |
								 (pbTSBuffData[i+9]<<1) |
								 (pbTSBuffData[i+10]>>7) ;

							pcre = ((pbTSBuffData[i+10] & 0x01) << 8) | (pbTSBuffData[i+11]);

							//printf("Offset:%d,PID:%d(0x%x),PCR:(0x%lx-%lx)\n",FileOffset+i,PID,PID,pcr);							

							if (!pcr1)
							{
								pcr1=pcr;	
								pcr1_offset=FileOffset+i;
								mPID=PID;
								printf("1'st PCR Offset:%d,PID:%d(0x%x),PCR:(0x%lx-%lx)\n",FileOffset+i,PID,PID,pcr);
								prev_pcr=pcr1;
								prev_pcr_offset=pcr1_offset;
							}
							else {
								if (mPID==PID) 
								{


									if (prev_pcr) {
										pkt_diff=(ULONG)((((ULONGLONG)FileOffset+i)-prev_pcr_offset)/188);
										
										if (pcr>prev_pcr)
											pcr_diff=pcr-prev_pcr;
										else 
											pcr_diff=pcr+0x1ffffffff-prev_pcr;


										Packet_Time=(double)pcr_diff/pkt_diff;

										if (min_Packet_Time>Packet_Time) {
											min_Packet_Time=Packet_Time;
											printf ("pcr diffb=%x-%x,pkt#=%d,pkt time=%f, min=%f, max=%f\n", pcr_diff,pkt_diff,Packet_Time,min_Packet_Time,max_Packet_Time);
										}
										if (max_Packet_Time<Packet_Time) {
											max_Packet_Time=Packet_Time;
											printf ("pcr diffb=%x-%x,pkt#=%d,pkt time=%f, min=%f, max=%f\n", pcr_diff,pkt_diff,Packet_Time,min_Packet_Time,max_Packet_Time);
										}
									}
									

									pcr2=pcr;	
									pcr2_offset=(ULONGLONG)FileOffset+i;

									prev_pcr=pcr2;
									prev_pcr_offset=pcr2_offset;
									//if (FileOffset>10000000) printf("2'nd PCR Offset:%u,PCRB:%u(0x%x),PCRE:%u(0x%x),PCR:%l(0x%lx)\n",FileOffset+i,pcrb,pcrb,pcre,pcre,pcr,pcr);
								}
							}


					
					}
				}

			}
			FileOffset+=dwReadSize;




			//Both PCR1 & PCR2 are found,
			//If it's a large file (>20MB) , then we skip the middle part of the file,
			// and try to locate PCR2 in the last 10 MB in the file end
			// As PCR2 gets farther away from PCR1, the bit rate calculated gets more precise

		    if (FileOffset>10000000 && FileOffset<FileSize-10000000 && pcr2) 
			{
	 				
				//Move to the last 10 MB position of file 
				//SetFilePointer(TsFile, FileSize-10000000, NULL, FILE_BEGIN);	

				SetFilePointer(TsFile, -10000000, NULL, FILE_END);	
				
				ReadFile(TsFile,pbTSBuffData,len,&dwReadSize,NULL);

				//Find the first 0x47 sync byte
				for (FirstSyncBytePos=0;(FirstSyncBytePos<dwReadSize-188);FirstSyncBytePos++) 
				{
					if ((pbTSBuffData[FirstSyncBytePos] == 0x47)&&(pbTSBuffData[FirstSyncBytePos+188] == 0x47))
					{
						//Sync byte found
						//printf("TS Sync byte found in offset:%d\n",FirstSyncBytePos+FileSize-10000000);
						break;
					}
				}

				if (FirstSyncBytePos>=dwReadSize-188)
				{
					printf("No sync byte found in the end 10 MB of file!!!\n");
					break;
				}

				//SetFilePointer(TsFile, FileSize-10000000+FirstSyncBytePos, NULL, FILE_BEGIN);	

				SetFilePointer(TsFile, -10000000+FirstSyncBytePos, NULL, FILE_END);	

				FileOffset=FileSize-10000000+FirstSyncBytePos;

				prev_pcr=0;
				prev_pcr_offset=0;
			}
			
			
			//if (FileOffset+len>FileSize) break; //EOF			

			ReadFile(TsFile,pbTSBuffData,len,&dwReadSize,NULL);
	}



	if (pcr2) 
	{
		double fTmp;
		if (pcr2>pcr1)
			fTmp =(double) ((pcr2_offset-pcr1_offset )* 8)/(double) ((pcr2 - pcr1));  //Bits per Hz
		else
			fTmp =(double) ((pcr2_offset-pcr1_offset )* 8)/(double) ((pcr2 +0x1ffffffff- pcr1));  //Bits per Hz
		

		lStreamBitRate = (long)(fTmp*90000);	 ////Bits per Seconds, 90KHz/Sec)
		
		//fTmp =(double) ((pcr2_offset-pcr1_offset ))/(double) ((pcr2 - pcr1))*8;  //Bits per Hz
		//float fTmp =(float) (pcr2_offset-pcr1_offset * 8) /(float) (pcr2 - pcr1);
		//lStreamBitRate = (long)(fTmp*90000);	 ////Bits per Seconds, 90KHz/Sec)
		
		LONG lStreamBitRate_Min=(LONG)(((double) 90000)/max_Packet_Time*188*8);
		LONG lStreamBitRate_Max=(LONG)(((double) 90000)/min_Packet_Time*188*8);


		printf ("min packet time=%f, max packet time=%f\n",min_Packet_Time,max_Packet_Time);
		printf ("Min Stream Data Rate=%d bps (%d Kbps)\n",lStreamBitRate_Min,lStreamBitRate_Min/1000);
		printf ("Max Stream Data Rate=%d bps (%d Kbps)\n",lStreamBitRate_Max,lStreamBitRate_Max/1000);

		printf ("Average Stream Data Rate=%d bps (%d Kbps)\n",lStreamBitRate,lStreamBitRate/1000);


	}
	return lStreamBitRate;
	
}

/************************************************************************************/
void PCR_Adaptation(BYTE *fileBuf,ULONG bytesRead,DWORD PCRAdaptationOffset)
{
	ULONG pcrb1=0,pcrb1_a=0,pcrb1_b=0,pcrb2=0,pcrb2_a=0,pcrb2_b=0;	

			//PCR adaptation only is done. TODO, PTS/DTS adaptation 

			for (unsigned i=0;(i<bytesRead);i+=188) 
			{
				int adaptation_field_control ;

				adaptation_field_control = ((fileBuf[i+3] >> 4) & 0x3);
				if (adaptation_field_control == 3 || adaptation_field_control == 2)
				{
					//CHECK adaptation_field_length !=0 && PCR flag on
					if (fileBuf[i+4] != 0 && (fileBuf[i+5] & 0x10) != 0)
					{
							ULONG pcrb;
							//we take only the MSB 32 bit PCR, ignore the LSB.
							//because PCRAdaptationOffset is calculated in 45KHz, instead of 90KHz
							pcrb=(fileBuf[i+6]<<24) |
								 (fileBuf[i+7]<<16) |
								 (fileBuf[i+8]<<8) |
								 (fileBuf[i+9]) ;

							if (!pcrb1) pcrb1=pcrb;
							else pcrb2=pcrb;

							pcrb=pcrb+PCRAdaptationOffset;

											
							if (!pcrb1_a) pcrb1_a=pcrb;
							else pcrb2_a=pcrb;
							
							fileBuf[i+6]=(BYTE)((pcrb&0xff000000)>>24);
							fileBuf[i+7]=(BYTE)((pcrb&0xff0000)>>16);
							fileBuf[i+8]=(BYTE)((pcrb&0xff00)>>8);
							fileBuf[i+9]=(BYTE)(pcrb&0xff);		
				
					}
				}

			}

	//printf("PCR Adaptation:0x%x->0x%x,0x%x->0x%x\n",pcrb1,pcrb1_a,pcrb2,pcrb2_a);
	


}
//***************************************************************************************

void SetSectionCRC(byte *TS_Packet)
{
	//Set Section CRC
	
    unsigned  CRC_32 = 0,section_length;

	section_length=((TS_Packet[6]&0xf)<<8)+TS_Packet[7];
	CRC_32 = GetCRC32(TS_Packet+5, section_length-1);
    TS_Packet[section_length-1+5] = ((CRC_32 >> 24) & 0xFF);
    TS_Packet[section_length-1+6] = ((CRC_32 >> 16) & 0xFF);
    TS_Packet[section_length-1+7] = ((CRC_32 >> 8) & 0xFF);
    TS_Packet[section_length-1+8] = ((CRC_32) & 0xFF);
}
//***************************************************************************************
void SetPeriodicCustomPacket(HANDLE  TsFile,int Tx_Dev_Num)
{
	int timer_interval;


	byte TSid[2]={0x00, 0x00}; //Note: it's big-endian
	byte ONid[2]={0x01, 0x01};
	byte NETid[2]={0x02, 0x02};
	byte Sid[2]={0x00, 0x01};
	byte ProviderName[16]={0x05,'i','T','E',0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20};
						//In sample SDT table, the name lenther is fixed as 16 bytes
						//The first byte is Selection of character table, 0x5 is ISO/IEC 8859-9 [33] Latin alphabet No. 5
	byte ServiceName[16]={0x05,'i','T','E',' ','C','H','1',0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20};
						//In sample SDT table, the name lenther is fixed as 16 bytes
						//The first byte is Selection of character table, 0x5 is ISO/IEC 8859-9 [33] Latin alphabet No. 5
	byte NetworkName[11]={0x05,'i','T','E',' ','T','a','i','p','e','i'};
						//In sample NIT table, the name lenther is fixed as 11 bytes
						//The first byte is Selection of character table, 0x5 is ISO/IEC 8859-9 [33] Latin alphabet No. 5
	byte LCN=0x50;   //Logical Channel Number
	
	byte DeliveryParameters[3]={0x00, 0x00,0x00};


    unsigned  CRC_32 = 0;


	//GET TSID & SID from stream file
	Get_PAT_TSID_SID(TsFile,TSid,Sid);

/***************************************************************************************************/
//	Custom SDT Table Insertion
/***************************************************************************************************/
    //Set Sample SDT IN CustomPacket_1
	memcpy(CustomPacket_1+8 ,TSid,2);
	memcpy(CustomPacket_1+13,ONid,2);
	memcpy(CustomPacket_1+0x10,Sid,2);
	memcpy(CustomPacket_1+0x19,ProviderName,16);
	memcpy(CustomPacket_1+0x2A,ServiceName,16);

	//Set CRC
	SetSectionCRC(CustomPacket_1);



	// Set & Copy SDT packets to internal buffer 1

	if (g_ITEAPI_TxSetPeridicCustomPacket(188, CustomPacket_1, 1, Tx_Dev_Num))
		printf("g_ITEAPI_TxAccessFwPSITable 1 fail\n");

	//printf("Enter the timer interval for custom table packet buffer 1, SDT (in ms, 0 means disabled): ");
	//scanf_s("%d", &timer_interval);

	timer_interval=20; //Set SDT repetition rate to 20 ms, 
		
	if (g_ITEAPI_TxSetPeridicCustomPacketTimer(1, timer_interval, Tx_Dev_Num))		
		printf("g_ITEAPI_TxSetFwPSITableTimer  %d fail\n",1);

	return ;

	//The following codes are for programmer's reference to customize other SI tables in table packet buffer 2~5

/***************************************************************************************************/
//	Custom NIT Table Insertion (set LCN and Delivery descriptor)
/***************************************************************************************************/

    //Set Sample NIT IN CustomPacket_2
	memcpy(CustomPacket_2+8   ,NETid,2);
	memcpy(CustomPacket_2+0x11,NetworkName,11);
	memcpy(CustomPacket_2+0x1E,TSid,2);
	memcpy(CustomPacket_2+0x20,ONid,2);


	//Set LCN Descriptor:0x83
	memcpy(CustomPacket_2+0x26,Sid,2);
	memcpy(CustomPacket_2+0x29,&LCN,1);


	//Set Delievery Descriptor:0x5a, NOTE:it's in 10Hz, instead of KHz or Hz
    CustomPacket_2[0x2c] = (byte)((g_Frequency*100 >> 24) & 0xFF);
    CustomPacket_2[0x2d] = (byte)((g_Frequency*100 >> 16) & 0xFF);
    CustomPacket_2[0x2e] = (byte)((g_Frequency*100 >> 8) & 0xFF);
    CustomPacket_2[0x2f] = (byte)((g_Frequency*100) & 0xFF);


	//Bandwidth 000:8M, 001:7,010:6,011:5
	DeliveryParameters[0]|=(abs((int)g_Bandwidth/1000-8)<<5);
	//Priority,timeslice, MPEFEC, reserved bits
	DeliveryParameters[0]|=0x1f;



	//Constellation:0: QPSK, 1: 16QAM, 2: 64QAM
	DeliveryParameters[1]|=(g_ChannelModulation_Setting.constellation<<6);
	//CR:0: 1/2, 1: 2/3, 2: 3/4, 3:5/6, 4: 7/8
	DeliveryParameters[1]|=(g_ChannelModulation_Setting.highCodeRate);


	//GI:0: 1/32, 1: 1/16, 2: 1/8, 3: 1/4
	DeliveryParameters[2]|=(g_ChannelModulation_Setting.interval<<3);

	//Transmission mode: 0: 2K, 1: 8K  2:4K
	DeliveryParameters[2]|=(g_ChannelModulation_Setting.transmissionMode<<1);

	memcpy(CustomPacket_2+0x30,DeliveryParameters,3);


	//Set Service list descriptror:0x41
	memcpy(CustomPacket_2+0x39,Sid,2);

	
	//Set CRC
	SetSectionCRC(CustomPacket_2);


	

	if (g_ITEAPI_TxSetPeridicCustomPacket(188, CustomPacket_2, 2, Tx_Dev_Num))
		printf("g_ITEAPI_TxAccessFwPSITable 2 fail\n");


	printf("Enter the timer interval for custom packet table 2, NIT (in ms, 0 means disabled): ");
	scanf_s("%d", &timer_interval);
		
		
	if (g_ITEAPI_TxSetPeridicCustomPacketTimer(2, timer_interval, Tx_Dev_Num) )		
		printf("g_ITEAPI_TxSetFwPSITableTimer  %d fail\n",2);



/***************************************************************************************************/
//	Other Custom Table Insertion
/***************************************************************************************************/


	if (g_ITEAPI_TxSetPeridicCustomPacket(188, CustomPacket_3, 3, Tx_Dev_Num) == 0)
		printf("g_ITEAPI_TxAccessFwPSITable 3 ok\n");
	else
		printf("g_ITEAPI_TxAccessFwPSITable 3 fail\n");
	if (g_ITEAPI_TxSetPeridicCustomPacket(188, CustomPacket_4, 4, Tx_Dev_Num) == 0)
		printf("g_ITEAPI_TxAccessFwPSITable 4 ok\n");
	else
		printf("g_ITEAPI_TxAccessFwPSITable 4 fail\n");
	if (g_ITEAPI_TxSetPeridicCustomPacket(188, CustomPacket_5, 5, Tx_Dev_Num) == 0)
		printf("g_ITEAPI_TxAccessFwPSITable 5 ok\n");
	else
		printf("g_ITEAPI_TxAccessFwPSITable 5 fail\n");


			

	for (int i=3;i<=5;i++) 
		{

				printf("Enter the timer interval for custom packet %d (in ms, 0 means disabled): ", i);
				scanf_s("%d", &timer_interval);
		
		
				if (g_ITEAPI_TxSetPeridicCustomPacketTimer(i, timer_interval, Tx_Dev_Num))		
					printf("g_ITEAPI_TxSetFwPSITableTimer  %d fail\n",i);
		}
			

	
}

void TX_DataOutputTest(int Tx_Dev_Num)
{
	UINT input = 0;
	HANDLE TsFile = NULL;
	DWORD dwFileSize = 0;
	char FilePath[128] = "C:\\test.ts";
	BYTE *fileBuf = NULL;
	DWORD Tx_datalength = TRANSMITTER_BLOCK_SIZE;
	DWORD Txbytes_PerMs = 0;
	
	int i = 0;
	
	DWORD LoopStartTime = 0;
	DWORD StartTime = 0;
	ULONG FirstSyncBytePos=0;
	ULONG bytesRead,Total_bytesSent;

	bool Play_InfiniteLoop = false;
	ULONG TSFileDataRate;
	int LoopCnt=0;

	DWORD TxStartTime = 0;
	DWORD PCRAdaptationOffset = 0;
	char key;



	printf("Input the TS file path name:  ");
	
	scanf_s("%64s", FilePath, _countof(FilePath)); // buffer size is 10, width specification is 9

		

	TsFile = CreateFile(FilePath,
						GENERIC_READ,
						0,
						NULL,
						OPEN_EXISTING,
						FILE_ATTRIBUTE_NORMAL,
						NULL);

	if (TsFile == INVALID_HANDLE_VALUE)
	{
		printf("Open Ts File Error!!!\n");
		return ;
	}


	dwFileSize = GetFileSize(TsFile, NULL);
	printf("%s size = %u\n", FilePath, dwFileSize);


	printf("Insert Periodical Custom Packets for SI SDT Table (y:yes) ?");	 
	key=_getch();
	printf("\n");
	if (key=='y'||key=='Y')
	{
		SetPeriodicCustomPacket(TsFile, Tx_Dev_Num);
	}
	else
	{
		//Disable Custom table packet insertion
		for (int i=1;i<=5;i++) {
			if (g_ITEAPI_TxSetPeridicCustomPacketTimer(i, 0, Tx_Dev_Num))		
			printf("g_ITEAPI_TxSetFwPSITableTimer  %d fail\n",i);
		}
	}

	
	//Check input file data bit rate here, slow but safe
	TSFileDataRate=Get_DataBitRate_S(TsFile);
	printf("The recommended input file data rate for %s is = %u KBps\n",FilePath ,TSFileDataRate/1000);

	
	//Check input file data bit rate here, quick but not so safe
	//TSFileDataRate=Get_DataBitRate(TsFile);
	//printf("The recommended input file data rate for %s is = %u KBps\n",FilePath ,TSFileDataRate/1000);

	


	printf("Enter Data Bit rate in Kbps(ex. 10000 for 10 Mbps):  ");
	scanf_s("%u", &input);
	
	TSFileDataRate=input*1000;  //convert to bps
	

	if (TSFileDataRate>g_ChannelCapacity)
	{
		printf("Warning the input file data rate (%d bps) is larger than channel data rate(%d bps)",TSFileDataRate,g_ChannelCapacity);
	}




	printf("Repeat Loop: 1.Repeat, 2.Once:  ");
	scanf_s("%u", &input);
	if(input == 1)
		Play_InfiniteLoop = true;


	
	printf("Press 'A' or 'a' to abort...\n");

	fileBuf = (BYTE *)malloc(Tx_datalength);
	
	


	//Find the first 0x47 sync byte
	SetFilePointer(TsFile, 0, NULL, FILE_BEGIN);
	ReadFile(TsFile, fileBuf, Tx_datalength, &bytesRead, NULL);


    for (FirstSyncBytePos=0;(FirstSyncBytePos<bytesRead-188);FirstSyncBytePos++) 
	{
			if ((fileBuf[FirstSyncBytePos] == 0x47)&&(fileBuf[FirstSyncBytePos+188] == 0x47))
			{
				//Sync byte found
				printf("TS Sync byte found in offset:%d\n",FirstSyncBytePos);
				break;
			}
	}

	if (FirstSyncBytePos>=bytesRead-188) {
		printf("No sync byte found, it's not a valid 188-byte TS file!!!\n");

		goto TX_DataOutputTest_exit;
	}


	//FirstSyncBytePos=0; //TEST
	

	Total_bytesSent=0;
	LoopCnt=0;

	SetFilePointer(TsFile, FirstSyncBytePos, NULL, FILE_BEGIN);
	LoopStartTime =TxStartTime= GetTickCount();

	

	while(1)
	{

	
		if ( _kbhit() !=0 )
        {
            char stopChar=(char)_getch();
			if (stopChar=='a' || stopChar=='A') break;
        }

		memset(fileBuf, 0, Tx_datalength);

		ReadFile(TsFile, fileBuf, Tx_datalength, &bytesRead, NULL);


		/*
		//Check each packet PCR/PTS
		for (int i=0; i<bytesRead;i+=188)
		{
			if (i==0x4218) 				
			{
				i=i;
			}

			Get_PCR(fileBuf+i);
			Get_PTS(fileBuf+i);
		}
		*/

/*
		
		if (LoopCnt)
		{
			// PCR, PTS, DTS adaptaion can be implemented here
			PCR_Adaptation(fileBuf,bytesRead,PCRAdaptationOffset);
		}

*/



		//It may be necessary that we need to fill the remaining empty buffer with null packets
		if(bytesRead < Tx_datalength)
		{
/*			
			for (ULONG i=((bytesRead/188)*188);i<Tx_datalength;i+=188) 
			{
				memcpy(fileBuf+i, NullPacket,188);
			}
*/
			printf("Last data block:%d, bytes\n",bytesRead);

		}


		if(g_ITEAPI_TxSendTSData(&bytesRead, fileBuf, Tx_Dev_Num) != 0) 
		{
			printf("g_ITEAPI_TxSendTSData fail !!!\n");
			break;
		}

		Total_bytesSent+=bytesRead;

		//it's the end of file ?
		if(bytesRead < Tx_datalength)
		{
			
			printf("End of file reached..Total loop time: %d ms\n", GetTickCount()-LoopStartTime);

			if(Play_InfiniteLoop)
			{
				//Repeat Loop 
				printf("Press 'A' or 'a' to abort...\n");
				//SetFilePointer(TsFile, 0, NULL, FILE_BEGIN);
				SetFilePointer(TsFile, FirstSyncBytePos, NULL, FILE_BEGIN);
				LoopStartTime= GetTickCount();
				PCRAdaptationOffset=(LoopStartTime-TxStartTime)*90/2; //Change system time offset(in ms) to PCR's tick count offset (in 45KHz)
				Total_bytesSent=0;
				
				LoopCnt++;
			}
			else
				break;
		}

		//Data Rate Control, IF the TS data rate is less than channel modulation data rate
		//and excluding the first time check (divide by zero check) 
		while ((TSFileDataRate<g_ChannelCapacity-1024)&&Total_bytesSent)
		{
			ULONG Output_Data_Rate;

			if (GetTickCount()==LoopStartTime) continue;
			
			Output_Data_Rate= Total_bytesSent*8/(GetTickCount()-LoopStartTime)*1000 ;			
			if (Output_Data_Rate<=TSFileDataRate)
			{
				break;
			}
			else Sleep(1);
		}
	}
	

TX_DataOutputTest_exit:
	if(fileBuf)
		free (fileBuf);

	if(TsFile)
		CloseHandle(TsFile);
}





void TX_SendCustomPacketOnce(int Tx_Dev_Num)
{

			if (g_ITEAPI_TxSendCustomPacketOnce(188, CustomPacket_1, Tx_Dev_Num) == 0)
				printf("g_ITEAPI_TxSendHwPSITable ok\n");		
			else
				printf("g_ITEAPI_TxSendHwPSITable fail\n");


}



void Set_IQtable(int Dev_num)
{
	HANDLE IQTable_File = NULL;
	char FilePath[128] = {0};
	LONGLONG fileSize = 0;
	int LoadingLength = 8;
	DWORD dwReadSize = 0;
	BYTE  Readbuf[20] = {0};
	unsigned int num_of_entries=0;
	//IQtable IQ_table[IQ_TABLE_NROW] = {0};
	IQtable *pIQ_table = NULL;

	printf("Input the IQtable file path name:  ");
	
	scanf_s("%64s", FilePath, _countof(FilePath));

	IQTable_File = CreateFile(FilePath,
						GENERIC_READ,
						0,
						NULL,
						OPEN_EXISTING,
						FILE_ATTRIBUTE_NORMAL,
						NULL);

	if (IQTable_File == INVALID_HANDLE_VALUE)
	{
		printf("Open IQtable File Error!!!\n");
		return ;
	}

	fileSize = (LONGLONG)GetFileSize(IQTable_File, NULL);
	printf("%s size = %u\n", FilePath, fileSize);


	
	ReadFile(IQTable_File, Readbuf, LoadingLength*2, &dwReadSize, NULL);

	num_of_entries=Readbuf[14];
	num_of_entries = ((num_of_entries << 8) & 0xFF00) + Readbuf[15];	
	printf("num_of_entries = %u\n", num_of_entries);
	
	pIQ_table=(IQtable *)malloc(sizeof(IQtable)*num_of_entries);


	fileSize -= LoadingLength*2; //skip filename & index



	for(unsigned int i=0;i<num_of_entries ;i++)
	{

		ReadFile(IQTable_File, Readbuf, LoadingLength, &dwReadSize, NULL);		

		pIQ_table[i].frequency = (Readbuf[0] + Readbuf[1]*256 
			+ Readbuf[2]*256*256 + Readbuf[3]*256*256*256);

		pIQ_table[i].dAmp = (signed short)(Readbuf[4] + Readbuf[5]*256);
		pIQ_table[i].dPhi = (signed short)(Readbuf[6] + Readbuf[7]*256);		

		fileSize -= LoadingLength;

		//printf("IQ_table[%d].frequency = %d\n", i, IQ_table[i].frequency);
		//printf("IQ_table[%d].dAmp = %d\n", i, IQ_table[i].dAmp);
		//printf("IQ_table[%d].dPhi = %d\n", i, IQ_table[i].dPhi);
		//printf("==================================");

		ZeroMemory(Readbuf, sizeof(Readbuf));
	}

	if(IQTable_File)
		CloseHandle(IQTable_File);

	if (g_ITE_TxSetIQtable(pIQ_table, (WORD)num_of_entries, Dev_num) == 0)
		printf("g_ITE_TxSetIQtable ok\n");		
	else
		printf("g_ITE_TxSetIQtable fail\n");

	free(pIQ_table);
}

int main(int argc, char* argv[]) 
{
	HINSTANCE hGetIp;
	HRESULT hr;	
	int Choice = 0;
	int Dev_num = 0;
	int Device_type = 0;
	int result = 0;
	
	hGetIp = ::LoadLibrary(_T("ITEAPIEX.dll"));
	if (hGetIp == NULL)
	{
		printf("LoadLibrary error!!!\n");
		return 0;
	}
	
	// Get API handle from ITEAPIEX.dll
	g_ITEAPI_GetDllVersion =			(ITE_GetDllVersion)::GetProcAddress(hGetIp, _T("ITE_GetDllVersion"));
	g_ITEAPI_GetDrvInfo =				(ITE_TxGetDrvInfo)::GetProcAddress(hGetIp, _T("ITE_TxGetDrvInfo"));
	g_ITEAPI_GetDeviceInfo =			(ITE_TxGetDeviceInfo)::GetProcAddress(hGetIp, _T("ITE_TxGetDeviceInfo"));
	g_ITEAPI_TxDeviceInit =				(ITE_TxDeviceInit)::GetProcAddress(hGetIp, _T("ITE_TxDeviceInit"));
	g_ITEAPI_TxDeviceExit =				(ITE_TxDeviceExit)::GetProcAddress(hGetIp, _T("ITE_TxDeviceExit"));
	g_ITEAPI_PowerCtl =					(ITE_TxPowerCtl)::GetProcAddress(hGetIp, _T("ITE_TxPowerCtl"));
	g_ITEAPI_TxSetChannel =				(ITE_TxSetChannel)::GetProcAddress(hGetIp, _T("ITE_TxSetChannel"));
	g_ITEAPI_TxSetChannelModulation =	(ITE_TxSetChannelModulation)::GetProcAddress(hGetIp, _T("ITE_TxSetChannelModulation"));
	g_ITEAPI_TxSetModeEnable =			(ITE_TxSetModeEnable)::GetProcAddress(hGetIp, _T("ITE_TxSetModeEnable"));
	g_ITEAPI_TxSetDeviceType =			(ITE_TxSetDeviceType)::GetProcAddress(hGetIp, _T("ITE_TxSetDeviceType"));
	g_ITEAPI_TxAdjustOutputGain =		(ITE_TxAdjustOutputGain)::GetProcAddress(hGetIp, _T("ITE_TxAdjustOutputGain"));
	g_ITEAPI_TxSendTSData =				(ITE_TxSendTSData)::GetProcAddress(hGetIp, _T("ITE_TxSendTSData"));

	g_ITEAPI_TxWriteLinkReg =			(ITE_TxWriteLinkReg)::GetProcAddress(hGetIp, _T("ITE_TxWriteLinkReg"));
	g_ITEAPI_TxReadLinkReg =			(ITE_TxReadLinkReg)::GetProcAddress(hGetIp, _T("ITE_TxReadLinkReg"));
	g_ITEAPI_TxWriteOFDMReg =			(ITE_TxWriteOFDMReg)::GetProcAddress(hGetIp, _T("ITE_TxWriteOFDMReg"));
	g_ITEAPI_TxReadOFDMReg =			(ITE_TxReadOFDMReg)::GetProcAddress(hGetIp, _T("ITE_TxReadOFDMReg"));

	g_ITEAPI_TxGetDeviceType =			(ITE_TxGetDeviceType)::GetProcAddress(hGetIp, _T("ITE_TxGetDeviceType"));


	g_ITEAPI_TxSendCustomPacketOnce =		 (ITE_TxSendCustomPacketOnce)::GetProcAddress(hGetIp, _T("ITE_TxSendCustomPacketOnce"));
	g_ITEAPI_TxSetPeridicCustomPacket =		 (ITE_TxSetPeridicCustomPacket)::GetProcAddress(hGetIp, _T("ITE_TxSetPeridicCustomPacket"));
	g_ITEAPI_TxSetPeridicCustomPacketTimer = (ITE_TxSetPeridicCustomPacketTimer)::GetProcAddress(hGetIp, _T("ITE_TxSetPeridicCustomPacketTimer"));


	g_ITEAPI_TxGetNumOfDevice =			(ITE_TxGetNumOfDevice)::GetProcAddress(hGetIp, _T("ITE_TxGetNumOfDevice"));
	g_ITEAPI_TxGetOutputGain =			(ITE_TxGetOutputGain)::GetProcAddress(hGetIp, _T("ITE_TxGetOutputGain"));
	g_ITEAPI_TxGetChipType =			(ITE_TxGetChipType)::GetProcAddress(hGetIp, _T("ITE_TxGetChipType"));
	g_ITEAPI_TxGetTPS =					(ITE_TxGetTPS)::GetProcAddress(hGetIp, _T("ITE_TxGetTPS"));
	g_ITEAPI_TxSetTPS =					(ITE_TxSetTPS)::GetProcAddress(hGetIp, _T("ITE_TxSetTPS"));
	g_ITEAPI_TxGetGainRange =			(ITE_TxGetGainRange)::GetProcAddress(hGetIp, _T("ITE_TxGetGainRange"));

	g_ITE_TxSetIQtable =				(ITE_TxSetIQtable)::GetProcAddress(hGetIp, _T("ITE_TxSetIQtable"));
	g_ITE_TxSetDCCalibrationValue =		(ITE_TxSetDCCalibrationValue)::GetProcAddress(hGetIp, _T("ITE_TxSetDCCalibrationValue"));

	// Initialize COM library
    hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if (FAILED (hr)){
		printf("CoInitializeEx error!!!");
        return 1;
    }
	
	// Start to test API	


	//Check the number of IT9500 devices
	{
		int Tx_NumOfDev = 0;
		if(g_ITEAPI_TxGetNumOfDevice(&Tx_NumOfDev) == 0) {
			printf("%d devices\n", Tx_NumOfDev);
		}
		else {
			printf("g_ITEAPI_TxGetNumOfDevice error\n");
			CoUninitialize();
			return 0;
		}
	}

	
	// ------------------init------------------

	//Pick up the first IT9500 device 
	Dev_num = 0;

	if(!TxDevice_init(Dev_num))
		return 0;


	//Show the IT9500 chip type
	{
		int ChipType = 0;	
		if(g_ITEAPI_TxGetChipType(&ChipType, Dev_num) == 0) {
			printf("Dev_num=%d ChipType = 0x%x\n",Dev_num, ChipType);
		}
		else
			printf("g_ITEAPI_TxGetChipType error\n");
	}
	


	//Turn on device power
	g_ITEAPI_PowerCtl(true, Dev_num);

	Choice=0xff;
	while (Choice)
	{

		printf("\n====== IT9500 API Test ======");
		printf("\n1.  Set Modulation Transmission Parameters");
		printf("\n2.  Get Device/Board Type");
		printf("\n3.  Set RF output Gain/Attenuation");
		printf("\n4.  Transmission Parameter Signalling Cell-id Setting");
		printf("\n5.  Load IQ calibration table");
		printf("\n6.  Set DC Calibration Value");
		printf("\n9.  Output Test (Streaming a TS File)");
		printf("\n0.  Quit                      ");
		printf("\nEnter Number:  ");
		result = scanf_s("%d", &Choice);			
		if(result <= 0)
		{
			printf("input ERROR !!!\n");			
			goto exit;
		}

		switch (Choice)
		{			
			case 1:
				g_ChannelCapacity=TX_SetChannelTransmissionParameters(Dev_num);					
				break;
			case 2:
				{
					UINT deviceType = 0;

					g_ITEAPI_TxGetDeviceType((BYTE*)&deviceType, Dev_num);

					printf("Device Type =%d\n ",deviceType);
					/*
					printf("Enter Device Type (0: EVB, 1:DB-01-01 v01, 2:DB-01-02 v01, 3:DB-01-01 v03):  ");
					scanf_s("%d", &deviceType);
					if(g_ITEAPI_TxSetDeviceType((BYTE)deviceType, Dev_num) == 0)
						printf("g_ITEAPI_TxSetDeviceType ok\n");
					else
						printf("g_ITEAPI_TxSetDeviceType error\n");
					*/
				}
				break;
			case 3:
				{
					int gain = 0;

					int MaxGain = 0;
					int MinGain = 0;
					

					//Get valid gain range
					g_ITEAPI_TxGetGainRange(g_Frequency, g_Bandwidth, &MaxGain, &MinGain, Dev_num);
					printf("Frequency=%d KHz, Bandwidth=%d KHz, MinGain = %d, MaxGain = %d\n",g_Frequency, g_Bandwidth, MinGain, MaxGain);


					//Get current gain setting
					if(g_ITEAPI_TxGetOutputGain(&gain, Dev_num) != 0)
						printf("g_ITEAPI_TxGetOutputGain error\n");


					printf(" Enter Gain/Attenuation (current setting:%d):  ",gain);
					result = scanf_s("%d", &gain);
					if(g_ITEAPI_TxAdjustOutputGain(&gain, Dev_num) == 0) {
						printf("g_ITEAPI_TxAdjustOutputGain ok:%d dB\n", gain);
					}
					else
						printf("g_ITEAPI_TxAdjustOutputGain error\n");
				}
				break;
			case 4:
				{
					TPS tps;		
					//WORD tempValue = 0;
					UINT tempValue = 0;

					//Get current TPS setting
					if(g_ITEAPI_TxGetTPS(&tps, Dev_num) == 0) {
						printf("=====================\n");
						printf("Current TPS:\n");
						//Convert big-endian to little-endian
						tempValue=(tps.cellid<<8)&0xff00 | (tps.cellid>>8);
						printf("cellid = %d(0x%x)\n", tempValue, tempValue);
						/*
						printf("constellation = %d\n", tps.constellation);
						printf("highCodeRate = %d\n", tps.highCodeRate);
						printf("interval = %d\n", tps.interval);
						printf("lowCodeRate = %d\n", tps.lowCodeRate);
						printf("transmissionMode = %d\n", tps.transmissionMode);
						*/
						printf("=====================\n");
					}
					else
						printf("g_ITEAPI_TxGetTPS error\n");
					

					printf("Set TPS cellid = ");
					result = scanf_s("%u", &tempValue);

					//Convert little-endian to big-endian						
					printf("Set cellid = %d(0x%x)\n", tempValue, tempValue);
					tps.cellid = (tempValue<<8)&0xff00 |(tempValue>>8) ;


					/*
					printf("constellation = ");
					result = scanf_s("%u", &tempValue);					
					tps.constellation = (BYTE)tempValue;
					printf("highCodeRate = ");
					result = scanf_s("%u", &tempValue);
					tps.highCodeRate = (BYTE)tempValue;
					printf("interval = ");
					result = scanf_s("%u", &tempValue);
					tps.interval = (BYTE)tempValue;
					printf("lowCodeRate = ");
					result = scanf_s("%u", &tempValue);
					tps.lowCodeRate = (BYTE)tempValue;
					printf("transmissionMode = ");
					result = scanf_s("%u", &tempValue);
					tps.transmissionMode = (BYTE)tempValue;
					*/


					if(g_ITEAPI_TxSetTPS(tps, Dev_num) == 0) {
						printf("g_ITEAPI_TxSetTPS ok\n");						
					}
					else
						printf("g_ITEAPI_TxSetTPS error\n");

					break;
				}
			case 5:
				{
					Set_IQtable(Dev_num);
					break;
				}
			case 6:
				{
					int dc_i = 0;
					int dc_q = 0;
					printf("\nEnter dc_i:  ");
					result = scanf_s("%d", &dc_i);

					printf("\nEnter dc_q:  ");
					result = scanf_s("%d", &dc_q);

					if(g_ITE_TxSetDCCalibrationValue(dc_i, dc_q, Dev_num) == 0) {
						printf("g_ITE_TxSetDCCalibrationValue ok\n");						
					}
					else
						printf("g_ITE_TxSetDCCalibrationValue error\n");

					break;
				}
			case 9:
				//Turn Transmitter, RF transmission started
				g_ITEAPI_TxSetModeEnable(true, Dev_num); 
				TX_DataOutputTest(Dev_num);
				g_ITEAPI_TxSetModeEnable(false, Dev_num);

				break;

			case 0:
				break;
		}
	}

exit:
	g_ITEAPI_PowerCtl(false, Dev_num);
	g_ITEAPI_TxDeviceExit(Dev_num);
	printf("g_ITEAPI_TxDeviceExit()\n");	

	CoUninitialize();

	return 0;
}

