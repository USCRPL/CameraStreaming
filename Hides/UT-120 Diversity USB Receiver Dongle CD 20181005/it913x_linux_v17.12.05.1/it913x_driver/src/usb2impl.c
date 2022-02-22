//all for Linux
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/kref.h>
#include <linux/usb.h>
#include <asm/uaccess.h>
#include <linux/device.h>
#include <linux/string.h>
#include <linux/firmware.h>
#include <linux/delay.h>
#include <linux/vmalloc.h>

#include "usb2impl.h"
#include "it913x.h"

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 33)
#include <linux/autoconf.h>
#else
#include <generated/autoconf.h>
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 39)
#include <linux/smp_lock.h>
#endif


#ifdef UNDER_CE


Dword Usb2_getDriver (
    IN  Demodulator*    demodulator,
    OUT Handle*         handle
) {
    return (Error_NO_ERROR);
}


Dword Usb2_exitDriver (
    IN  Demodulator*    demodulator
) {
    return (Error_NO_ERROR);
}


Dword Usb2_writeControlBus (
    IN  Demodulator*    demodulator,
    IN  Dword           bufferLength,
    IN  Byte*           buffer
) {
    return (Error_NO_ERROR);
}


Dword Usb2_readControlBus (
    IN  Demodulator*    demodulator,
    IN  Dword           bufferLength,
    OUT Byte*           buffer
) {
    return (Error_NO_ERROR);
}


Dword Usb2_readDataBus (
    IN  Demodulator*    demodulator,
    IN  Dword           bufferLength,
    OUT Byte*           buffer
) {
    return (Error_NO_ERROR);
}

#else

Dword Usb2_getDriver(
	IN  Demodulator*	demodulator,
	OUT Handle*			handle)
{
	Dword error = Error_NO_ERROR;

	return (error);
}

Dword Usb2_exitDriver(IN  Demodulator*	demodulator)
{
    Dword error = Error_NO_ERROR;

    return (error);
}

Dword Usb2_writeControlBus(
	IN  Demodulator*	demodulator,
	IN  Dword			bufferLength,
	IN  Byte*			buffer)
{
    Dword result = Error_NO_ERROR;
    int act_len;
	Device_Context *DC = (Device_Context *)demodulator->userData;

	memcpy(DC->Temp_writebuffer, buffer, bufferLength);

	//deb_data("- Enter %s Function -\n", __func__);

	result = usb_bulk_msg(usb_get_dev(demodulator->driver),
			usb_sndbulkpipe(usb_get_dev(demodulator->driver), 0x02),
			DC->Temp_writebuffer,
			bufferLength,
			&act_len,
			1000000);

	if (result) {
		deb_data("\t Error: Usb2_writeControlBus fail [0x%08lx]\n", result);
		return result;
	}

	return result;
}

Dword Usb2_readControlBus (
	IN  Demodulator*	demodulator,
	IN  Dword			bufferLength,
	OUT Byte*			buffer)
{
	Dword result = Error_NO_ERROR;
	int nBytesRead;
	Device_Context *DC = (Device_Context *)demodulator->userData;

	//deb_data("- Enter %s Function -\n", __func__);

	result = usb_bulk_msg(usb_get_dev(demodulator->driver),
				usb_rcvbulkpipe(usb_get_dev(demodulator->driver),129),
				DC->Temp_readbuffer,
				bufferLength,
				&nBytesRead,
				1000000);

	if (DC->Temp_readbuffer)
		memcpy(buffer, DC->Temp_readbuffer, bufferLength);

	if (result) {
		deb_data("\t Error: Usb2_readControlBus fail [0x%08lx]\n", result);
		return result;
	}


	return result;
}

Dword Usb2_readDataBus (
	IN  Demodulator*	demodulator,
	IN  Dword			bufferLength,
	OUT Byte*			buffer)
{
	return (Error_NO_ERROR);
}
#endif
