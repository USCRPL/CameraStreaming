#include "modulatorUser.h"
#include "IT9507.h"
#include ".\ADF4351\ADF4351.h"
/**
 * Variable of critical section
 */

Dword EagleUser_memoryCopy (
    IN  Modulator*    modulator,
    IN  void*           dest,
    IN  void*           src,
    IN  Dword           count
) {
    /*
     *  ToDo:  Add code here
     *
     *  //Pseudo code
     *  memcpy(dest, src, (size_t)count);
     *  return (0);
     */
    return (ModulatorError_NO_ERROR);
}

Dword EagleUser_delay (
    IN  Modulator*    modulator,
    IN  Dword           dwMs
) {
    /*
     *  ToDo:  Add code here
     *
     *  //Pseudo code
     *  delay(dwMs);
     *  return (0);
     */
	Sleep(dwMs);
    return (ModulatorError_NO_ERROR);
}


Dword EagleUser_enterCriticalSection (
    IN  Modulator*    modulator
) {
    /*
     *  ToDo:  Add code here
     *
     *  //Pseudo code
     *  return (0);
     */
    return (ModulatorError_NO_ERROR);
}


Dword EagleUser_leaveCriticalSection (
    IN  Modulator*    modulator
) {
    /*
     *  ToDo:  Add code here
     *
     *  //Pseudo code
     *  return (0);
     */
    return (ModulatorError_NO_ERROR);
}


Dword EagleUser_mpegConfig (
    IN  Modulator*    modulator
) {
    /*
     *  ToDo:  Add code here
     *
     */
    return (ModulatorError_NO_ERROR);
}


Dword EagleUser_busTx (
    IN  Modulator*    modulator,
    IN  Dword           bufferLength,
    IN  Byte*           buffer
) {
    /*
     *  ToDo:  Add code here
     *
     *  //Pseudo code
     *  short i;
     *
     *  start();
     *  write_i2c(uc2WireAddr);
     *  ack();
     *  for (i = 0; i < bufferLength; i++) {
     *      write_i2c(*(ucpBuffer + i));
     *      ack();
     *  }
     *  stop();
     *
     *  // If no error happened return 0, else return error code.
     *  return (0);
     */
	Dword error = 0;
	if(modulator->busId == Bus_I2C)
		error = I2c_writeControlBus(modulator,bufferLength,buffer);
	else if(modulator->busId == Bus_USB)
		error = Usb2_writeControlBus(modulator,bufferLength,buffer);
	else if(modulator->busId == 3)
		error = Af9035u2i_writeControlBus(modulator,bufferLength,buffer);
	else
		error = ModulatorError_INVALID_BUS_TYPE;

    return (error);
}


Dword EagleUser_busRx (
    IN  Modulator*    modulator,
    IN  Dword           bufferLength,
    OUT Byte*           buffer
) {
    /*
     *  ToDo:  Add code here
     *
     *  //Pseudo code
     *  short i;
     *
     *  start();
     *  write_i2c(uc2WireAddr | 0x01);
     *  ack();
     *  for (i = 0; i < bufferLength - 1; i++) {
     *      read_i2c(*(ucpBuffer + i));
     *      ack();
     *  }
     *  read_i2c(*(ucpBuffer + bufferLength - 1));
     *  nack();
     *  stop();
     *
     *  // If no error happened return 0, else return error code.
     *  return (0);
     */
    //return (Af9035u2i_readControlBus(modulator,bufferLength,buffer));

	Dword error = 0;
	if(modulator->busId == Bus_I2C)
		error = I2c_readControlBus(modulator,bufferLength,buffer);
	else if(modulator->busId == Bus_USB)
		error = Usb2_readControlBus(modulator,bufferLength,buffer);
	else if(modulator->busId == 3)
		error = Af9035u2i_readControlBus(modulator,bufferLength,buffer);
	else
		error = ModulatorError_INVALID_BUS_TYPE;
	return (error);
}


Dword EagleUser_setBus (
    IN  Modulator*    modulator
) {
	/*
     *  ToDo:  Add code here
     *
     *  // If no error happened return 0, else return error code.
     *  return (0);
     */
    Dword error = ModulatorError_NO_ERROR;

    return(error);
}


 Dword EagleUser_Initialization  (
    IN  Modulator*    modulator
) {
	/*
     *  ToDo:  Add code here
     *
     *  // If no error happened return 0, else return error code.
     *  return (0);
     */
	Dword error = 0;

	error = IT9507_writeRegister (modulator, Processor_LINK, p_eagle_reg_top_gpioh8_en, 1);	//U/V filter control pin
	if (error) goto exit;
	error = IT9507_writeRegister (modulator, Processor_LINK, p_eagle_reg_top_gpioh8_on, 1);
	if (error) goto exit;

	if((modulator->tsInterfaceType == SERIAL_TS_INPUT) || (modulator->tsInterfaceType == PARALLEL_TS_INPUT)){
		error = IT9507_writeRegister (modulator, Processor_LINK, p_eagle_reg_top_gpioh1_o, 1); //RX(IT9133) rest
		if (error) goto exit;

		error = IT9507_writeRegister (modulator, Processor_LINK, p_eagle_reg_top_gpioh1_en, 1);
		if (error) goto exit;
		error = IT9507_writeRegister (modulator, Processor_LINK, p_eagle_reg_top_gpioh1_on, 1);
		if (error) goto exit;
		EagleUser_delay(modulator, 10);

		error = IT9507_writeRegister (modulator, Processor_LINK, p_eagle_reg_top_gpioh5_o, 0); //RX(IT9133) power up
		if (error) goto exit;
		error = IT9507_writeRegister (modulator, Processor_LINK, p_eagle_reg_top_gpioh5_en, 1);
		if (error) goto exit;
		error = IT9507_writeRegister (modulator, Processor_LINK, p_eagle_reg_top_gpioh5_on, 1);
		if (error) goto exit;

		error = IT9507_writeRegister (modulator, Processor_LINK, p_eagle_reg_top_gpioh4_en, 1);
		if (error) goto exit;
		error = IT9507_writeRegister (modulator, Processor_LINK, p_eagle_reg_top_gpioh4_on, 1);
		if (error) goto exit;
		error = IT9507_writeRegister (modulator, Processor_LINK, p_eagle_reg_top_gpioh6_en, 1);
		if (error) goto exit;
		error = IT9507_writeRegister (modulator, Processor_LINK, p_eagle_reg_top_gpioh6_on, 1);
		if (error) goto exit;
		error = IT9507_writeRegister (modulator, Processor_LINK, p_eagle_reg_top_gpioh7_en, 1);
		if (error) goto exit;
		error = IT9507_writeRegister (modulator, Processor_LINK, p_eagle_reg_top_gpioh7_on, 1);
		if (error) goto exit;
	}

	error = IT9507_writeRegister (modulator, Processor_LINK, p_eagle_reg_top_gpioh2_o, 0); //RF out power down
	if (error) goto exit;
	error = IT9507_writeRegister (modulator, Processor_LINK, p_eagle_reg_top_gpioh2_en, 1);
	if (error) goto exit;
	error = IT9507_writeRegister (modulator, Processor_LINK, p_eagle_reg_top_gpioh2_on, 1);
	if (error) goto exit;

	//ADF4351 init
	ADF4351_busInit (modulator);
exit:

    return (error);

 }


Dword EagleUser_Finalize  (
    IN  Modulator*    modulator
) {
	/*
     *  ToDo:  Add code here
     *
     *  // If no error happened return 0, else return error code.
     *  return (0);
     */
	Dword error = 0;
	if(modulator->busId == Bus_USB)
		error = Usb2_exitDriver(modulator);
	else if(modulator->busId == 3)
		error = Af9035u2i_exitDriver(modulator);
	else
		error = ModulatorError_INVALID_BUS_TYPE;
    return (ModulatorError_NO_ERROR);

 }
 

Dword EagleUser_acquireChannel (
	IN  Modulator*    modulator,
	IN  Word          bandwidth,
	IN  Dword         frequency
){

	Dword error = 0;
	if(frequency <= 230000){ // <=230000KHz v-filter gpio set to Lo
		error = IT9507_writeRegister (modulator, Processor_LINK, p_eagle_reg_top_gpioh8_o, 0); 
		if (error) goto exit;
		
	}else{
		error = IT9507_writeRegister (modulator, Processor_LINK, p_eagle_reg_top_gpioh8_o, 1); 
		if (error) goto exit;		 
	}	

	//ADF4351 control
	ADF4351_setFrequency(modulator,frequency);
exit:
	return (error);
}

Dword EagleUser_setTxModeEnable (
	IN  Modulator*            modulator,
	IN  Byte                    enable	
) {
	Dword error = ModulatorError_NO_ERROR;

	if(enable){
		error = IT9507_writeRegister (modulator, Processor_LINK, p_eagle_reg_top_gpioh2_o, 1); //RF power up 
		if (error) goto exit;
	}else{
		error = IT9507_writeRegister (modulator, Processor_LINK, p_eagle_reg_top_gpioh2_o, 0); //RF power down 
		if (error) goto exit;
		
	}
exit :

	return (error);
}


Dword EagleUser_getChannelIndex (
	IN  Modulator*            modulator,
	IN  Byte*                    index	
) {
	Dword error = ModulatorError_NO_ERROR;
	Byte Freqindex = 0;
	Byte temp = 0;
	// get HW setting
	error = IT9507_writeRegister (modulator, Processor_LINK, p_eagle_reg_top_gpioh3_o, 1); 
	if (error) goto exit;
	error = IT9507_writeRegister (modulator, Processor_LINK, p_eagle_reg_top_gpioh3_en, 1);
	if (error) goto exit;
	error = IT9507_writeRegister (modulator, Processor_LINK, p_eagle_reg_top_gpioh3_on, 1);
	if (error) goto exit;
	error = IT9507_writeRegister (modulator, Processor_LINK, p_eagle_reg_top_gpioh4_en, 0);
	if (error) goto exit;
	error = IT9507_writeRegister (modulator, Processor_LINK, p_eagle_reg_top_gpioh5_en, 0);
	if (error) goto exit;
	error = IT9507_writeRegister (modulator, Processor_LINK, p_eagle_reg_top_gpioh6_en, 0);
	if (error) goto exit;
	error = IT9507_writeRegister (modulator, Processor_LINK, p_eagle_reg_top_gpioh7_en, 0);
	if (error) goto exit;
	//--- get HW freq setting
	error = IT9507_readRegister (modulator, Processor_LINK, r_eagle_reg_top_gpioh4_i, &temp);
	if (error) goto exit;
	Freqindex = Freqindex | (temp);
	error = IT9507_readRegister (modulator, Processor_LINK, r_eagle_reg_top_gpioh5_i, &temp);
	if (error) goto exit;
	Freqindex = Freqindex | (temp<<1);
	error = IT9507_readRegister (modulator, Processor_LINK, r_eagle_reg_top_gpioh6_i, &temp);
	if (error) goto exit;
	Freqindex = Freqindex | (temp<<2);
	error = IT9507_readRegister (modulator, Processor_LINK, r_eagle_reg_top_gpioh7_i, &temp);
	if (error) goto exit;
	Freqindex = Freqindex | (temp<<3);

	error = IT9507_readRegister (modulator, Processor_LINK, 0x49E5, &temp);
	if (error) goto exit;
	Freqindex = Freqindex | (temp<<4);
	//--------------------

	error = IT9507_writeRegister (modulator, Processor_LINK, p_eagle_reg_top_gpioh4_en, 1);
	if (error) goto exit;
	error = IT9507_writeRegister (modulator, Processor_LINK, p_eagle_reg_top_gpioh5_en, 1);
	if (error) goto exit;
	error = IT9507_writeRegister (modulator, Processor_LINK, p_eagle_reg_top_gpioh6_en, 1);
	if (error) goto exit;
	error = IT9507_writeRegister (modulator, Processor_LINK, p_eagle_reg_top_gpioh7_en, 1);
	if (error) goto exit;
	error = IT9507_writeRegister (modulator, Processor_LINK, p_eagle_reg_top_gpioh3_o, 0);
	if (error) goto exit;

	*index = Freqindex;
exit :

	return (error);
}


