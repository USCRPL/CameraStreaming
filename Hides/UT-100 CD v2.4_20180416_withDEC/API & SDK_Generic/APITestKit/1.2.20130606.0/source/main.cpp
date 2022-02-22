#include <stdio.h>
#include <conio.h>
#include "api\IT9507.h"
#include "api\modulatorFirmware.h"
//#include "api\IT9133\IT9133.h"



Modulator eagle;

Byte numberOfChips = 1;
Word bandwidth = 6000;
Word sawBandwidth = 8000;
TsInterface tsin_streamType = PARALLEL_TS_INPUT;
IQtable IQ_tableEx [65536];

extern Byte InterfaceIndex_af9035u2i;
extern Word I2c_IO_PORT;



Dword Example_LoadIQCalibrationTable () {
    Dword error = ModulatorError_NO_ERROR;
	FILE *pfile = NULL;
	char buf[16]={0};
	char inputFile[255]={0};
	int dAmp = 0;			  
	int dPhi = 0;
	int row;
	Byte groups;
	//IQtable IQ_tableEx [91];
	Dword freq = 0;
	

	printf("input File name ?\n");
	scanf ("%s", inputFile);
	pfile = fopen(inputFile,"rb");
	if(pfile != NULL){
		fread(buf,16,1,pfile);
		groups = buf[14]<<8 | buf[15];	//frequency groups ex:(950000-50000)/10000 + 1 = 91
		for(row = 0; row < groups; row++){
			fread(&freq,4,1,pfile);
			fread(&dAmp,1,2,pfile);
			fread(&dPhi,1,2,pfile);
			
			IQ_tableEx[row].frequency = freq;
			IQ_tableEx[row].dAmp = (short)dAmp;
			IQ_tableEx[row].dPhi = (short)dPhi;

		}
		error = IT9507_setIQtable(&eagle, &IQ_tableEx[0],groups);
		if(error)
			printf("ModulatorError_NULL_PTR \n");
		else
			printf("Load IQ Calibration Table successful\n");
		//eagle.ptrIQtableEx = &IQ_tableEx[0];
		fclose(pfile);
		pfile = NULL;
	}else{
		error = ModulatorError_OPEN_FILE_FAIL;
	}

	return error;
}
Dword Example_initialize () {
    Dword error = ModulatorError_NO_ERROR;
	Dword start,end; 
	Byte Bus_id = Bus_USB;
	//Byte temp;
	
	Word wtemp;

	printf ("Interface Bus? 1:I2C 2:USB 3:9035U2I_1 4:9035U2I_2 \n");
    scanf_s ("%d", &wtemp);
	Bus_id  = (Byte)wtemp;
	if(Bus_id == 1){
		printf ("set print port address(HEX)? default:0x378  \n");
        scanf_s ("%08X", &wtemp);
		I2c_IO_PORT = wtemp;
	}else if(Bus_id == 3){
		InterfaceIndex_af9035u2i = 1;
	}else if(Bus_id == 4){
		InterfaceIndex_af9035u2i = 2;
		Bus_id = 3;
	}
    printf ("TS in Stream Type? 0:SERIAL TS in 1:PARALLEL TS in 2:USB in \n");
    scanf_s ("%d", &wtemp);

	if(wtemp == 0)
		tsin_streamType = SERIAL_TS_INPUT;
	else if(wtemp == 1)
		tsin_streamType = PARALLEL_TS_INPUT;
	else
		tsin_streamType = SERIAL_TS_INPUT;

	start= GetTickCount();
	
    error = IT9507_initialize (&eagle, tsin_streamType, Bus_id, EagleUser_IIC_ADDRESS);
    if (error) goto exit;
	end= GetTickCount();
   
	printf("IT9507_TXinitialize successful Time =%d\n",end-start);
	

exit:
    if (error) printf("Error = 0x%08x\n", error);

	return error;
}

void Example_monitorVersion () {
    Dword error = ModulatorError_NO_ERROR;
	Dword version = 0;

	printf("API Version = %04x.%08x.%02x\n", Eagle_Version_NUMBER, Eagle_Version_DATE, Eagle_Version_BUILD);

	error = IT9507_getFirmwareVersion ((Modulator*) &eagle, Processor_LINK, &version);
	if (error) {
		printf("Demodulator_getFirmwareVersion(LINK) failed! Error = 0x%08x\n", error);
		return;
	} else {
		printf("LINK FW Version = 0x%08x\n", version);
	}
	error = IT9507_getFirmwareVersion ((Modulator*) &eagle, Processor_OFDM, &version);
	if (error) {
		printf("Demodulator_getFirmwareVersion(OFDM) failed! Error = 0x%08x\n", error);
		return;
	} else {
		printf("OFDM FW Version = 0x%08x\n", version);
	}
}

int main (int argc, char* argv[]) {

    char c = 0;
    Byte i = 0;
	Byte temp;
	Byte temp2;
	Byte temp3;
	Word w_temp;
    Dword val = 0;
    Dword reg = 0;
    Dword processor = 0;
	Dword freq;
    Dword error = 0;
	ChannelModulation      channelModulation;
	Word bw;
	Pid tempPID;
	Byte PSIbuffer[188];
	Byte run = 1;
	int gain;	
	Byte index;
	Byte timer;

	
    while (run) {
		printf("\n");
		printf("=============================================\n");
		printf("===     API Version : %04x.%08x.%02x    ===\n",Eagle_Version_NUMBER,Eagle_Version_DATE,Eagle_Version_BUILD);
		printf("=== FW LINK Version : %02x.%02x.%02x.%02x         ===\n",DVB_LL_VERSION1,DVB_LL_VERSION2,DVB_LL_VERSION3,DVB_LL_VERSION4);
		printf("=== FW OFDM Version : %02x.%02x.%02x.%02x         ===\n",DVB_OFDM_VERSION1,DVB_OFDM_VERSION2,DVB_OFDM_VERSION3,DVB_OFDM_VERSION4);
		printf("=============================================\n");
			
        printf ("i : initialize chips\n");
        printf ("w : write register\n");
        printf ("r : read register\n");
		printf ("s : Set Channel Modulation Parameters\n");
		printf ("e : enable/disable Transmission\n");
        printf ("b : back to boot code\n");		
		printf ("L : Load IQ Calibration Table\n");

		printf ("0 : enable PidFilter\n");
		printf ("1 : IT9507_resetPidFilter()\n");
        printf ("2 : IT9507_addPidToFilter()\n");
		printf ("3 : Send custom SI/PSI packet-One-shot IT9507_sendHwPSITable()\n");
		printf ("4 : acquire TX channels(set IT9500 channel frequency)\n");
		printf ("5 : IT9507_AdjustOutputGain()\n");
		printf ("8 : Periodical custom packet transmission-Set packet IT9507_accessFwPSITable()\n");
		printf ("9 : Periodical custom packet transmission-Set timer IT9507_setFwPSITableTimer()\n");

        printf ("x : exit\n");
        printf ("Please enter your command:\n");
        c = (char)_getch();
		printf("\n");
        switch (c) {

            case 'i' :
            case 'I' :
                /**
                 * DVB-T: Initialize chip.
                 */
                Example_initialize ();                
                break;
            case 'w' :
            case 'W' :
				                             
				printf ("which processor to write? 0:LINK / 1:OFDM\n");
                scanf_s ("%d", &processor);

                printf ("which register to write?(HEX)\n");
                scanf_s ("%08X", &reg);

                printf ("value?(HEX)\n");
                scanf_s ("%02X", &val);
				

				if (processor == 0)
				{
					error = IT9507_writeRegister((Modulator*) &eagle, Processor_LINK, reg, (Byte)val);
					if (error) 
						printf("IT9507 write register(link) failed! Error = %08x\n", error);
					else
						printf("IT9507 write register(link) successful\n");
				}
				else
				{
					error = IT9507_writeRegister((Modulator*) &eagle, Processor_OFDM, reg, (Byte)val);
					if (error) 
						printf("IT9507 write register(ofdm) failed! Error = %08x\n", error);
					else
						printf("IT9507 write register(ofdm) successful\n");
				}
				
                break;
            case 'r' :
            case 'R' :

	            printf ("which processor to read? 0:LINK / 1:OFDM\n");
                scanf_s ("%d", &processor);

                printf ("which register to read?(HEX)\n");
                scanf_s ("%08X", &reg);

				
				if (processor == 0)
				{
					error = IT9507_readRegister((Modulator*) &eagle, Processor_LINK, reg, (Byte *)&val);
					if (error) printf("IT9507 read register failed! Error = %08x\n", error);
					else printf ("IT9507 value = 0x%02x\n", (val & 0x000000FF));
				}
				else
				{
					error = IT9507_readRegister((Modulator*) &eagle, Processor_OFDM, reg, (Byte *)&val);
					if (error) printf("IT9507 read register failed! Error = %08x\n", error);
					else printf ("IT9507 value = 0x%02x\n", (val & 0x000000FF));
				}
				
                break;
 

			case 'b' :
            case 'B' :
                /**
                 * back to boot code               
                 */
                error = IT9507_finalize((Modulator*) &eagle);
				if (error) 
					printf("back to boot code failed! Error = %08x\n", error);
				else
					printf("back to boot code successful\n");
                break;


			case 'v' :
            case 'V' :
                /**
                 * DVB-T: Monitor the Version of chip.               
                 */
                Example_monitorVersion ();
                break;

			case 's' :
            case 'S' :
               
				printf ("constellation type? 0:QPSK 1:16QAM 2:64QAM\n");
                scanf_s ("%d", &temp);
				channelModulation.constellation=(Constellation)temp;
				printf ("coding rate? 0:1/2, 1:2/3, 2:3/4, 3:5/6, 4:7/8\n");
                scanf_s ("%d", &temp);
				channelModulation.highCodeRate=(CodeRate)temp;
				printf ("guard interval? 0:1/32, 1:1/16, 2:1/8, 3:1/4\n");
                scanf_s ("%d", &temp);
				channelModulation.interval=(Interval)temp;
				printf ("transmission Mode? 0:2k, 1:8k\n");
                scanf_s ("%d", &temp);
				channelModulation.transmissionMode=(TransmissionModes)temp;
				
				error = IT9507_setTXChannelModulation((Modulator*) &eagle, &channelModulation);
				if (error) 
					printf("IT9507_setChannelModulation failed! Error = %08x\n", error);
				else
					printf("set Channel Modulation successful\n");

                break;
			case 'e' :
            case 'E' :
				printf ("Transmission mode enable/disable 0:disable 1:enable\n");
                scanf_s ("%d", &temp);
				error = IT9507_setTxModeEnable((Modulator*) &eagle, temp);
				if (error) 
					printf("IT9507_setTxModeEnable failed! Error = %08x\n", error);
				else
					printf("IT9507_setTxModeEnable successful\n");
                break;
  
            case 'x' :
            case 'X' :
				run = 0;
               break;
 			
			case 'l' :
            case 'L' :
				Example_LoadIQCalibrationTable();
				break;

			case '0':
				printf ("control PidFilter 0:pass the specified PID's(Pass mode) 1:filter the specified PID's(Block mode)\n");
                scanf_s ("%d", &temp);
				temp2 = temp;
				printf ("enable PidFilter 0:disable 1:enable\n");
                scanf_s ("%d", &temp);
				temp3 = temp;
				error = IT9507_controlPidFilter((Modulator*) &eagle, temp2,temp3);
				if (error) 
					printf("IT9507_controlPidFilter failed! Error = %08x\n", error);
				else
					if(temp)
						printf("enable PidFilter successful\n");
					else
						printf("disable PidFilter successful\n");
				break;

			case '1': 
				error = IT9507_resetPidFilter((Modulator*) &eagle);
				if (error) 
					printf("IT9507_resetPidFilter failed! Error = %08x\n", error);
				else
					printf("IT9507_resetPidFilter successful\n");
					
				break;
			case '2':
				printf ("IT9507_addPidToFilter index (1~31)?\n");
                scanf_s ("%d", &temp);
				printf ("IT9507_addPidToFilter PID (Decimal:0~8191)\n");
                scanf_s ("%d", &w_temp);
				tempPID.value = w_temp;
				error = IT9507_addPidToFilter((Modulator*) &eagle, temp, tempPID);
				if (error) 
					printf("IT9507_controlPidFilter failed! Error = %08x\n", error);
				else
					printf("add Pid successful\n");
									
				break;

			case '3':
				for(i=0;i<188;i++){
					if((i==0) || (i==187))
						temp = 0x47;
					else
						temp = i+5;

					PSIbuffer[i] = temp;
				}
								
				error = IT9507_sendHwPSITable((Modulator*) &eagle, PSIbuffer);				
				if (error) 
					printf("%d:IT9507_sendHwPSITable failed! Error = %08x\n",i ,error);
				else
					printf("%d:IT9507_sendHwPSITable successful (fake data is 0x47 0x06 0x07 0x08 ... )\n",i);
					
				break;

			case '4' :
            
                /**
                 * DVB-H: Use IT9507_acquireChannel to tune channel to specified
                 *        frequency and use IT9507_acquirePlatform to get all IP
                 *        platforms in this frequency.
                 */
				
				printf ("Input channel frequency in KHz?\n");
				scanf_s ("%d", &freq);
				printf ("Bandwidth in KHz?\n");
				scanf_s ("%d", &bw);
				error = IT9507_acquireTxChannel ((Modulator*) &eagle,bw,freq);
				if (error) 
					printf("IT9507_acquireTxChannel failed! Error = %08x\n", error);
				else
					printf("IT9507_acquireTxChannel successful\n");
				break;
			case '5' :
				printf ("Gain +/- dBm ?\n");
				scanf_s ("%d", &gain);

				error = IT9507_adjustOutputGain((Modulator*) &eagle,&gain);
				if(error)
					printf("IT9507_AdjustOutputGain failed! Error = %08x\n", error);
				else
					printf("IT9507_AdjustOutputGain successful gain = %d\n",gain);
				
				break;

			case '8' :

				printf ("Periodical Custom SI/PSI packet index (1~5)?\n");
				scanf_s ("%d", &temp);


				PSIbuffer[0] = 0x47;
				for(i=1;i<188;i++){
					PSIbuffer[i] = temp+i-1;
				}

				error = IT9507_accessFwPSITable((Modulator*) &eagle,temp ,PSIbuffer);
			
				if(error){
					printf("IT9507_accessFwPSITable failed! Error = %08x\n", error);
				}else{
					printf("IT9507_accessFwPSITable successful index = %d\n",temp);
					printf("fake data is 0x47, index, index+1, index+2... \n");
				}
				
				break;
			case '9' :

				printf ("Periodical Custom SI/PSI packet index (1~5)?\n");
				scanf_s ("%d", &temp);
				index = temp;
				printf ("Timer = ? (ms)\n");
				scanf_s ("%d", &temp);
				timer = temp;

				error = IT9507_setFwPSITableTimer((Modulator*) &eagle,index ,timer);
			
				if(error)
					printf("IT9507_getDeviceType failed! Error = %08x\n", error);
				else
					printf("IT9507_getDeviceType successful index = %d Timer = %d(ms)\n",index ,timer);
				
				break;

        }

       
    }


	system("pause");
    return (0);
}

