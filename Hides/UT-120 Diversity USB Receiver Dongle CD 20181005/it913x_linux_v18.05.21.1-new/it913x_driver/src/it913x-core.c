/*
 * device driver for IT913x driver core
 * 
 * IT913x_module_init --> usb_register(&IT913x_Driver)
 * (The IT913x_module_init() macro defines which function is to be called at module insertion
 * time (if the file is compiled as a module), or at boot time:
 * if the file is not compiled as a module the IT913x_module_init() macro becomes equivalent
 * to __initcall(), which through linker magic ensures that the function is called on boot.)
 * 
 * IT913x_module_exit --> usb_deregister(&IT913x_Driver)
 * (This macro defines the function to be called at module removal time (or never, in the case
 * of the file compiled into the kernel). It will only be called if the module usage count has
 * reached zero. This function can also sleep, but cannot fail: everything must be cleaned up
 * by the time it returns. Note that this macro is optional: if it is not present, your module
 * will not be removable (except for 'rmmod -f').
 */

#include "it913x.h"

/********** For Dynamic setting URB buffer *************/
static int URB_BUFSIZE = 65424;
//let modinfo could get the URB_NUM information
module_param(URB_BUFSIZE, int, S_IRUGO);
MODULE_PARM_DESC(URB_BUFSIZE, "URB_BUFSIZE = 188*xxx");
/******** For Dynamic setting URB buffer end ***********/

static struct usb_driver IT913x_Driver;
DVB_DEFINE_MOD_OPT_ADAPTER_NR(adapter_nr);

/* 
 * Rx_fops_open is to be used to open up a device for use. usb_open must be called before
 * attempting to perform any operations to the device. Returns a handle used in future
 * communication with the device.
 */
static int Rx_fops_open(
	struct inode *inode,
	struct file *file)
{
	Device_Context *DC = NULL;
	struct usb_device *usb_dev = NULL;
	struct usb_interface *interface = NULL;
	int minor[2], ret = 0;

	deb_data("- Enter %s Function -\n", __func__);

	minor[0] = iminor(inode);
	minor[1] = minor[0] - 1;

get_interface:
	while (!interface) {
		minor[1]++;
		if (minor[1] > minor[0] + 16) {
			deb_data("\t Error: can't find interface from minor[%d] ~ [%d]\n", minor[0], minor[1]);
			ret = -ENODEV;
			goto exit;
		}

		interface = usb_find_interface(&IT913x_Driver, minor[1]);
	}

	usb_dev = interface_to_usbdev(interface);
	if (!usb_dev) {
		deb_data("\t Error: can't find usb_device from interface\n");
		ret = -ENODEV;
		goto exit;
	}

	DC = dev_get_drvdata(&usb_dev->dev);
	if (!DC) {
		deb_data("\t Error: can't find Device_Context from usb_device\n");
		ret = -ENODEV;
		goto exit;
	}

	if ((minor[0] != DC->minor[0]) & (minor[0] != DC->minor[1])) {
		interface = NULL;
		goto get_interface;
	}

	/* Save our object in the file's private structure */
	file->private_data = DC->usb_dev;

	deb_data("- %s success -\n", __func__);
	return ret;

exit:
	deb_data("- %s fail -\n", __func__);
	return ret;
}

static int Rx_fops_release(
	struct inode *inode,
	struct file *file)
{
	Device_Context *DC = NULL;
	struct usb_device *usb_dev = NULL;
	int ret = 0;
	Dword result = 0;

	deb_data("- Enter %s Function -\n", __func__);

	usb_dev = (struct usb_device *)file->private_data;
	if (!usb_dev) {
		deb_data("\t Error: can't find usb_device from interface\n");
		ret = -ENODEV;
		goto exit;
	}

	DC = (Device_Context *)dev_get_drvdata(&usb_dev->dev);
	if (!DC) {
		deb_data("\t Error: can't find Device_Context from usb_device\n");
		ret = -ENODEV;
		goto exit;
	}

	/* Release our object in the file's private structure */
	file->private_data = NULL;

	deb_data("- %s success -\n", __func__);
	return ret;

exit:
	deb_data("- %s fail -\n", __func__);
	return ret;
}

ssize_t Rx_fops_read(
	struct file *file,
	char __user *buf,
	size_t count,
	loff_t *f_pos)
{
	Device_Context *DC = NULL;
	struct usb_device *usb_dev = NULL;
	DWORD result = Error_NO_ERROR;
	DWORD ulRawIr = 0;
	ssize_t ret = 0;

	usb_dev = (struct usb_device *)file->private_data;
	if (!usb_dev) {
		deb_data("\t Error: can't find usb_device from interface\n");
		ret = -ENODEV;
		goto exit;
	}

	DC = (Device_Context *)dev_get_drvdata(&usb_dev->dev);
	if (!DC) {
		deb_data("\t Error: can't find Device_Context from usb_device\n");
		ret = -ENODEV;
		goto exit;
	}

	if (DC->disconnect == 1) {
		deb_data("\t Error: device disconnect\n");
		ret = -ENODEV;
		goto exit;
	}

	result = DL_ReadRawIR(DC, &ulRawIr);
	if (result) {
		deb_data("\t DL_ReadRawIR fail [0x%08lx]\n", result);
		ret = -ENOTTY;
		goto exit;
	}
	else {
		copy_to_user(buf, &ulRawIr, 4);
		ret = count;
	}

exit:
	return ret;
}

/* I/O operation to usb device */
int Rx_fops_ioctl(
	struct inode *inode,
	struct file *file,
	unsigned int cmd,
	unsigned long parg)
{
	Device_Context *DC = NULL;
	struct usb_device *usb_dev = NULL;
	Byte chip_id = 0;
	int minor[2];

	minor[0] = iminor(inode);
	minor[1] = minor[0] -1;

	//deb_data("- Enter %s Function -\n", __func__);

	usb_dev = (struct usb_device *)file->private_data;
	if (!usb_dev) {
		deb_data("\t Error: can't find usb_device from interface\n");
		return -ENODEV;
	}

	DC = (Device_Context *)dev_get_drvdata(&usb_dev->dev);
	if (!DC) {
		deb_data("\t Error: can't find Device_Context from usb_device\n");
		return -ENODEV;
	}

	if (DC->disconnect == 1) {
		deb_data("\t Error: device disconnect\n");
		return -ENODEV;
	}

	if (DC->Demodulator.chipNumber == 2 && DC->minor[1] == minor[0])
		chip_id = 1;
	else
		chip_id = 0;

	return DL_DemodIOCTLFun(DC, (DWORD)cmd, parg, chip_id);
}

/* I/O operation to usb device if LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 35) */
long Rx_fops_unlocked_ioctl(
	struct file *file,
	unsigned int cmd,
	unsigned long parg)
{
	Device_Context *DC = NULL;
	struct usb_device *usb_dev = NULL;
	Byte chip_id= 0;
	int minor[2];
	//using file->f_mapping_host to get inode information
	struct inode *inode = file->f_mapping->host;

	minor[0] = iminor(inode);
	minor[1] = minor[0] -1;

	//deb_data("- Enter %s Function -\n", __func__);

	usb_dev = (struct usb_device *)file->private_data;
	if (!usb_dev) {
		deb_data("\t Error: can't find usb_device from interface\n");
		return -ENODEV;
	}

	DC = (Device_Context *)dev_get_drvdata(&usb_dev->dev);
	if (!DC) {
		deb_data("\t Error: can't find Device_Context from usb_device\n");
		return -ENODEV;
	}

	if (DC->disconnect == 1) {
		deb_data("\t Error: device disconnect\n");
		return -ENODEV;
	}

	if (DC->Demodulator.chipNumber == 2 && DC->minor[1] == minor[0])
		chip_id = 1;
	else
		chip_id = 0;

	return DL_DemodIOCTLFun(DC, (DWORD)cmd, parg, chip_id);
}

#ifdef CONFIG_COMPAT
static long Rx_fops_compact_ioctl(
		struct file *file,
		unsigned int cmd,
		unsigned long parg)
{
	unsigned long compat_parg = 0;

	//deb_data("- Enter %s Function -\n", __func__);

	compat_parg = (unsigned long)compat_ptr(parg);

	return Rx_fops_unlocked_ioctl(file, cmd, compat_parg);
}
#endif

static struct file_operations Rx_File_Operations = {
	.owner = THIS_MODULE,
	.read = Rx_fops_read,
	.open = Rx_fops_open,
	.release = Rx_fops_release,

#ifdef CONFIG_COMPAT
	.compat_ioctl = Rx_fops_compact_ioctl,
#endif

#if LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 35)
	.unlocked_ioctl = Rx_fops_unlocked_ioctl,
#else
	.ioctl = Rx_fops_ioctl,
#endif
};

/*
 * Usb class driver info in order to get a minor number from the usb core,
 * and to have the device registered with devfs and the driver core
 */
static struct usb_class_driver IT913x_Class_Driver = {
	.name = "usb-it913x%d",
	.fops = &Rx_File_Operations,
	.minor_base = USB_ITE_MINOR_BASE
};

/* Register a DVB_USB device and a USB device node */
static int IT913x_probe(
	struct usb_interface *intf,
	const struct usb_device_id *id)
{
	Device_Context *DC = NULL;
	struct dvb_usb_device *dvb_usb_dev = NULL;
	Byte chip_index = 0, device_index = 0;
	int ret = -ENOMEM;
	DWORD result = Error_NO_ERROR;
	int kernel_version[3];

	deb_data("===== it913x usb device pluged in =====\n");
	deb_data("- Enter %s Function -\n", __func__);

	kernel_version[0] = LINUX_VERSION_CODE >> 16;
	kernel_version[1] = (LINUX_VERSION_CODE >> 8) - (kernel_version[0] << 8);
	kernel_version[2] = LINUX_VERSION_CODE - (kernel_version[0] << 16) - (kernel_version[1] << 8);
	deb_data("\t KERNEL_VERSION(%d, %d, %d)\n", kernel_version[0], kernel_version[1], kernel_version[2]);

	DC = kzalloc(sizeof(Device_Context), GFP_KERNEL);
	if (!DC) {
		deb_data("\t Error: Device_Context out of memory\n");
		ret = -ENOMEM;
		goto exit;
	}

	DC->Temp_readbuffer = kzalloc(sizeof(int) * 1024, GFP_KERNEL);
	if (!DC->Temp_readbuffer) {
		deb_data("\t Error: Temp_readbuffer out of memory\n");
		ret = -ENOMEM;
		goto exit;
	}

	DC->Temp_writebuffer = kzalloc(sizeof(int) * 1024, GFP_KERNEL);
	if (DC->Temp_writebuffer == NULL) {
		deb_data("\t Error: Temp_writebuffer out of memory\n");
		ret = -ENOMEM;
		goto exit;
	}

	init_waitqueue_head(&DC->disconnect_wait);

	DC->minor[0] = 0;
	DC->minor[1] = 0;
	mutex_init(&DC->dev_mutex);
	DC->usb_dev = interface_to_usbdev(intf);

	/*** Dynamic urb buffer size ***/
	if((URB_BUFSIZE <= 0) )
		URB_BUFSIZE = 65424;
	else if(((URB_BUFSIZE % 188)!= 0))
		URB_BUFSIZE=((URB_BUFSIZE/188)+1)*188;
	DC->Demodulator.user_usb20_frame_size_dw = URB_BUFSIZE / 4;

	deb_data("===== URB_BUFSIZE:%d \n", URB_BUFSIZE);
	/*** Dynamic urb buffer size  end ***/

	result = Device_init(DC->usb_dev, DC, true);
	if (result) {
		deb_data("\t Error: Device_init fail [0x%08lx]\n", result);
		ret = -ENOMEM;
		goto exit;
	}


	/* We can register the usb_device now, as it is ready */
	for (chip_index = 0; chip_index < DC->Demodulator.chipNumber; chip_index++) {
		if ((chip_index > 0) && (DC->bDualTs == False)) // For DCA mode
			break;

		intf->minor = -1;
		ret = usb_register_dev(intf, &IT913x_Class_Driver);
		if (ret) {
			deb_data("\t Error: not able to get a minor for chip[%d].\n", chip_index);
			usb_set_intfdata(intf, NULL);
			ret = -ENOTTY;
			goto exit;
		}

		DC->minor[chip_index] = intf->minor;
		deb_data("USB IT913x now attached to USBSkel-%d \n", DC->minor[chip_index]);
	}
	usb_set_intfdata(intf, DC);

	/* We can register the dvb_usb_device now, as it is read */
	if (it913x_device_count > 2) {
		deb_data("\t Warning: it913x_device_count = %d.\n", it913x_device_count);
		it913x_device_count = 2;
	}
	
	deb_data("\t Warning: it913x_device_count = %d.\n", it913x_device_count);

	for (device_index = 0; device_index < it913x_device_count; device_index++) {
		#if LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 37)
			if ((DC->bIrTblDownload == 0) || (DC->bProprietaryIr == 0))
				DVB_properties[device_index].rc.core.rc_codes = NULL;
			else
				RemoteConfig_set(DC->usb_dev, &DVB_properties[device_index]);
		#endif

		#if LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 18)
			if ((DC->Demodulator.chipNumber == 2) && (DC->bDualTs == True)) {
				DVB_properties[device_index].num_adapters = 2;
				DVB_properties[device_index].caps = DVB_USB_IS_AN_I2C_ADAPTER;
			}
			else {
				DVB_properties[device_index].num_adapters = 1;
				DVB_properties[device_index].caps = 0;
			}
		#endif

		/*** Dynamic urb buffer size ***/
		#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 2, 0)
			DVB_properties[device_index].adapter[0].fe[0].stream.u.bulk.buffersize = URB_BUFSIZE;//User_USB20_FRAME_SIZE ;
		#else
			DVB_properties[device_index].adapter[0].stream.u.bulk.buffersize = URB_BUFSIZE;//User_USB20_FRAME_SIZE ;
		#endif
		/*** Dynamic urb buffer size  end ***/

		#if LINUX_VERSION_CODE <= KERNEL_VERSION(2, 6, 25)
			if (dvb_usb_device_init(intf, &DVB_properties[device_index], THIS_MODULE, &dvb_usb_dev)) {
		#else
			if (dvb_usb_device_init(intf, &DVB_properties[device_index], THIS_MODULE, &dvb_usb_dev, adapter_nr)) {
		#endif
				ret = -ENOTTY;
				goto exit;
			}
	}

	deb_data("- %s success -\n", __func__);
	return ret;

exit:
	deb_data("- %s fail -\n", __func__);
	return ret;
}

static int IT913x_suspend(
	struct usb_interface *intf,
	pm_message_t state)
{
	Device_Context *DC = NULL;
	struct usb_device *usb_dev = NULL;
	Byte chip_index = 0;
	DWORD result = Error_NO_ERROR;

	deb_data("- Enter %s Function -\n", __func__);

	usb_dev = interface_to_usbdev(intf);
	if (!usb_dev) {
		deb_data("\t Error: can't find usb_device from interface\n");
		goto exit;
	}

	DC = (Device_Context *)dev_get_drvdata(&usb_dev->dev);
	if (!DC) {
		deb_data("\t Error: can't find Device_Context from usb_device\n");
		goto exit;
	}

	if (DC->disconnect == 1) {
		deb_data("\t Error: device disconnect\n");
		goto exit;
	}

	/* Power down if on and save the power state*/
	if (DC->fc[0].bTimerOn || DC->fc[1].bTimerOn) {
		for (chip_index = DC->Demodulator.chipNumber; chip_index > 0; chip_index--) {
			DC->power_state[chip_index - 1] = DC->fc[chip_index - 1].bTimerOn;
			DC->fc[chip_index - 1].bTimerOn = false;
			DC->fc[chip_index - 1].tunerinfo.bTunerInited = false;
			result = DL_ApCtrl(DC, (chip_index - 1), false);
			if (result) {
				deb_data("\t Error: chip[%d] POWER DOWN fail [0x%08lx]\n", chip_index - 1, result);
				goto exit;
			}
		}
	}

	deb_data("- %s : success -\n", __func__);
	return 0;

exit:
	deb_data("- %s : fail -\n", __func__);
	return -1;
}

static int IT913x_resume(struct usb_interface *intf)
{
	Device_Context *DC = NULL;
	struct usb_device *usb_dev = NULL;
	Byte chip_index = 0;
	DWORD result = Error_NO_ERROR;

	deb_data("- Enter %s Function -\n", __func__);

	usb_dev = interface_to_usbdev(intf);
	if (!usb_dev) {
		deb_data("\t Error: can't find usb_device from interface\n");
		goto exit;
	}

	DC = (Device_Context *)dev_get_drvdata(&usb_dev->dev);
	if (!DC) {
		deb_data("\t Error: can't find Device_Context from usb_device\n");
		goto exit;
	}

	if (DC->disconnect == 1) {
		deb_data("\t Error: device disconnect\n");
		goto exit;
	}

	/* Power on for resume */
	for (chip_index = 0; chip_index < DC->Demodulator.chipNumber; chip_index++) {
		DC->fc[chip_index].bTimerOn = true;
		DC->fc[chip_index].tunerinfo.bTunerInited = true;

		result = DL_ApCtrl(DC, chip_index, true);
		if (result) {
			deb_data("\t Error: chip[%d] POWER ON fail [0x%08lx]\n", chip_index, result);
			goto exit;
		}
	}

	result = Device_init(DC->usb_dev, DC, false);
	if (result) {
		deb_data("\t Error: Device_init fail [0x%08lx]\n", result);
		goto exit;
	}

	/* Resume power state */
	if (DC->power_state[0] || DC->power_state[1]) {
		for (chip_index = 0; chip_index < DC->Demodulator.chipNumber; chip_index++) {
			DC->fc[chip_index].bTimerOn = DC->power_state[chip_index];
			DC->fc[chip_index].tunerinfo.bTunerInited = true;

			result = DL_ApCtrl(DC, chip_index, true);
			if (result) {
				deb_data("\t Error: chip[%d] POWER ON fail [0x%08lx]\n", chip_index, result);
				goto exit;
			}
		}
	}

	deb_data("- %s : success -\n", __func__);
	return 0;

exit:
	deb_data("- %s : fail -\n", __func__);
	return -1;
}

static int IT913x_reset_resume(struct usb_interface *intf)
{
	Device_Context *DC = NULL;
	struct usb_device *usb_dev = NULL;
	Byte chip_index = 0;
	DWORD result = Error_NO_ERROR;

	deb_data("- Enter %s Function -\n", __func__);

	usb_dev = interface_to_usbdev(intf);
	if (!usb_dev) {
		deb_data("\t Error: can't find usb_device from interface\n");
		goto exit;
	}

	DC = (Device_Context *)dev_get_drvdata(&usb_dev->dev);
	if (!DC) {
		deb_data("\t Error: can't find Device_Context from usb_device\n");
		goto exit;
	}

	if (DC->disconnect == 1) {
		deb_data("\t Error: device disconnect\n");
		goto exit;
	}

	/* Power on for resume */
	for (chip_index = 0; chip_index < DC->Demodulator.chipNumber; chip_index++) {
		DC->fc[chip_index].bTimerOn = true;
		DC->fc[chip_index].tunerinfo.bTunerInited = true;

		result = DL_ApCtrl(DC, chip_index, true);
		if (result) {
			deb_data("\t Error: chip[%d] POWER ON fail [0x%08lx]\n", chip_index, result);
			goto exit;
		}
	}

	result = Device_init(DC->usb_dev, DC, false);
	if (result) {
		deb_data("\t Error: Device_init fail [0x%08lx]\n", result);
		goto exit;
	}

	/* Resume power state */
	if (DC->power_state[0] || DC->power_state[1]) {
		for (chip_index = 0; chip_index < DC->Demodulator.chipNumber; chip_index++) {
			DC->fc[chip_index].bTimerOn = DC->power_state[chip_index];
			DC->fc[chip_index].tunerinfo.bTunerInited = true;

			result = DL_ApCtrl(DC, chip_index, true);
			if (result) {
				deb_data("\t Error: chip[%d] POWER ON fail [0x%08lx]\n", chip_index, result);
				goto exit;
			}
		}
	}

	deb_data("- %s : success -\n", __func__);
	return 0;

exit:
	deb_data("- %s : fail -\n", __func__);
	return -1;
}

static void I2C_exit(struct dvb_usb_device *dvb_usb_dev)
{
	struct it913x_state *state = dvb_usb_dev->priv;

	deb_data("- Enter %s Function -\n", __func__);

	/* Remove 2nd I2C adapter */
	if (dvb_usb_dev->state & DVB_USB_STATE_I2C) {
		try_module_get(THIS_MODULE);
		i2c_del_adapter(&state->i2c_adap);
	}
}

static void IT913x_disconnect(struct usb_interface *intf)
{
	Device_Context *DC = NULL;
	struct usb_device *usb_dev = NULL;
	struct dvb_usb_device *dvb_usb_dev = NULL;
	Byte chip_index = 0;

	deb_data("- Enter %s Function -\n", __func__);

	dvb_usb_dev = usb_get_intfdata(intf);
	if (!dvb_usb_dev) {
		deb_data("IT913x_disconnect - Error: device not exist\n");
		return;
	}

	usb_dev = interface_to_usbdev(intf);
	if (!usb_dev) {
		deb_data("\t Error: can't find usb_device from interface\n");
		return ;
	}

	DC = (Device_Context *)dev_get_drvdata(&usb_dev->dev);
	if (!DC) {
		deb_data("\t Error: can't find Device_Context from usb_device\n");
		return ;
	}

	DC->disconnect = 1;

	#if HOT_PLUGGING
		/* Patch for supporting hot plugging */
		while (DC->AP_off == 0) {
			deb_data("\t Warning: device disconnect, wait AP close\n");
			wait_event_interruptible(DC->disconnect_wait, (DC->AP_off == 1)); //sleep until app close
		}
	#endif

	for (chip_index = 0; chip_index < DC->Demodulator.chipNumber; chip_index++) {
		if ((chip_index > 0) && (DC->bDualTs == False))
			break;
		
		intf->minor = DC->minor[chip_index];
		usb_deregister_dev(intf, &IT913x_Class_Driver);
		deb_data("USB IT913x (USBSkel-%d) now disconnected\n", DC->minor[chip_index]);
	}

	if (DC->Temp_readbuffer)
		kfree(DC->Temp_readbuffer);

	if (DC->Temp_writebuffer)
		kfree(DC->Temp_writebuffer);

	kfree(DC);

	/* Remove 2nd I2C adapter */
	if (dvb_usb_dev != NULL && dvb_usb_dev->desc != NULL)
		I2C_exit(dvb_usb_dev);

	/* Deregister dvb usb device, call DVB_ops_release */
	try_module_get(THIS_MODULE);
	dvb_usb_device_exit(intf);
}

static struct usb_driver IT913x_Driver = {
#if LINUX_VERSION_CODE <= KERNEL_VERSION(2, 6, 15)
	.owner = THIS_MODULE,
#endif

	.name = "dvb_usb_it913x",
	.probe = IT913x_probe,
	.disconnect = IT913x_disconnect,
	.id_table = IT913x_usb_id_table,
	.suspend = IT913x_suspend,
	.resume = IT913x_resume,

#if LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 22)
	.reset_resume = IT913x_reset_resume,
#endif
};

/* Insert it913x module and prepare register it913x driver */
static int __init IT913x_module_init(void)
{
	int result = 0;

	deb_data("- Enter %s Function -\n", __func__);

	if ((result = usb_register(&IT913x_Driver))) {
		deb_data("\t Error: usb_register fail [%d]\n", result);
		return result;
	}

	return 0;
}

/* Deregister this driver with the USB subsystem */
static void __exit IT913x_module_exit(void)
{
	deb_data("- Enter %s Function -\n", __func__);
	usb_deregister(&IT913x_Driver);
}

module_init(IT913x_module_init);
module_exit(IT913x_module_exit);

MODULE_AUTHOR("Figo Chang <Figo.Chang@ite.com.tw>");
MODULE_DESCRIPTION("Driver for devices based on ITEtech IT913x");
MODULE_VERSION(DRIVER_RELEASE_VERSION);
MODULE_LICENSE("GPL");
