          ITEtech IT913x DVB-T USB Linux TV TestKit User Guide
                            2013/05/17
             Copyright (C) 2007-2013 ITE Technologies, INC.

1. Feature descriptions for Testkit Multiple Devices:
    a. cd DTVTestKit_MultiDevice
    b. The setting properties are in "info.txt":
       <DeviceNum>      => Total device number.
       <SettingTypeNum> => Total setting type of functions( ChannelLock, Multiple Channel scan, Read Write Register... 
    		     	      functions)
       '*' symbol       => the '*' symbol is use to identify one type of functions.
	'[' or ']' symbol=> Using to identify setting number of devices in the type.
	'<' or '>' symbol=> type or setting strings include at this.
	
	Example:
	<DeviceNum> 2             <=  2 devices. 
	<SettingTypeNum> 3        <=  3 setting type.
	*ChannelLock
	[1]
	<frequency> 545000        <=  set device1 for 545MHz.       [Frequency (KHz)]
	<bandwidth> 6000          <=  set device1 for 6MHz.         
	[2]
	<frequency> 593000        <=  set device2 for 593MHz. 
	<bandwidth> 7000          <=  set device2 for 7MHz. 
	--------------------
	*MultiChannel
	[1]
	<startfreq> 533000        <=  set device1 for 533MHz.       [Start Frequency (KHz)]
	<endfreq>   570000  	     <=  set device1 for 570MHz.       [Stop Frequency (KHz)]
	<bandwidth> 6000          <=  set device1 for 6MHz.
	[2]
	<startfreq> 533000        <=  set device2 for 533MHz.
	<endfreq>   598000  	     <=  set device2 for 598MHz. 
	<bandwidth> 6000          <=  set device1 for 6MHz.
	--------------------
	*RWRegister
	[1]
	<0:Read, 1:Write> 1       <=  set device1 for write mode.   [Read/Write Register(0: Read, 1: Write)]
	<0:LINK, 1:OFDM> 1        <=  set device1 for write OFDM.   [LINK or OFDM (0: LINK, 1: OFDM)]
	<Address (Hex)> 0x13      <=  write address 0x13.           [Read/Write Register Address]
	<Write Value (Hex)> 0x46  <=  write value 0x46.             
	[2]
	<0:Read, 1:Write> 0       <=  set device1 for read mode.
	<0:LINK, 1:OFDM> 0        <=  set device1 for read LINK.
	<Address (Hex)> 0xdd8d    <=  write address 0xdd8d.
     
       Note: In read register mode, the "<Write Value (Hex)>" tag must be remove.

2. Make and run testkit
    a. cd it913x_testkit_multi_device
    b. make
    c. ./it913x_testkit_multi_device
    d.  The output messages are saved in "./Output Msg" file.

3. Revision history
	-----------------------
	     v13.05.17.1  
	-----------------------
	- Change input bandwidth parameter.
	- Change process of open handle.
	- Fixed Statistics of BER not correct.

	-----------------------
	     v12.08.15.1  
	-----------------------
	- The first version of DTVTestKit_MultiDevice.

