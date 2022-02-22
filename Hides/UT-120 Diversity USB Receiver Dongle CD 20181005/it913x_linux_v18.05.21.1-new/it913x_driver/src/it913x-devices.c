/* 
 * it913x-device.c is used to "it913x-core.c --> IT913x_probe --> dvb_usb_device_init func"
 * init it913x properties
 */

#include "it913x.h"

/* Table of devices that work with this driver */
struct usb_device_id IT913x_usb_id_table[] = {
	{USB_DEVICE(USB_VID_ITETECH, USB_PID_ITETECH_IT913X_9005)},
	{USB_DEVICE(USB_VID_ITETECH, USB_PID_ITETECH_IT913X_9006)},
	{USB_DEVICE(USB_VID_ITETECH, USB_PID_ITETECH_IT913X)},
	{0}, /* Terminating entry */
};
MODULE_DEVICE_TABLE(usb, IT913x_usb_id_table);

#if LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 37)
static int DVB_remote;
module_param_named(remote, DVB_remote, int, 0644);

static struct Remote_config remote_config;
char rc_map_type[4];

static char *RemoteConfig_setup_match(
	unsigned int id,
	const struct RC_Setup *table)
{
	for (; table->rc_codes; table++)
		if (table->id == id)
			return table->rc_codes;

	return NULL;
}

static const struct RC_Setup rc_setup_modparam[] = {
	{IT913X_REMOTE_A_LINK_DTU_M, RC_MAP_ALINK_DTU_M},
	{IT913X_REMOTE_MSI_DIGIVOX_MINI_II_V3, RC_MAP_MSI_DIGIVOX_II},
	{IT913X_REMOTE_MYGICTV_U718, RC_MAP_TOTAL_MEDIA_IN_HAND},
	{IT913X_REMOTE_DIGITTRADE_DVB_T, RC_MAP_DIGITTRADE},
	{IT913X_REMOTE_AVERMEDIA_KS, RC_MAP_AVERMEDIA_RM_KS},
	{ }
};

static const struct RC_Setup rc_setup_hashes[] = {
	{0xb8feb708, RC_MAP_MSI_DIGIVOX_II},
	{0xa3703d00, RC_MAP_ALINK_DTU_M},
	{0x9b7dc64e, RC_MAP_TOTAL_MEDIA_IN_HAND}, /* MYGICTV U718 */
	{ }
};

static const struct RC_Setup rc_setup_usbids[] = {
//	{(USB_VID_TERRATEC << 16) + USB_PID_TERRATEC_CINERGY_T_STICK_RC, RC_MAP_TERRATEC_SLIM_2 },
	{(USB_VID_TERRATEC << 16) + USB_PID_TERRATEC_CINERGY_T_STICK_DUAL_RC, RC_MAP_TERRATEC_SLIM},
	{(USB_VID_VISIONPLUS << 16) + USB_PID_AZUREWAVE_AD_TU700, RC_MAP_AZUREWAVE_AD_TU700},
	{(USB_VID_VISIONPLUS << 16) + USB_PID_TINYTWIN, RC_MAP_AZUREWAVE_AD_TU700},
	{(USB_VID_MSI_2 << 16) + USB_PID_MSI_DIGI_VOX_MINI_III, RC_MAP_MSI_DIGIVOX_III},
	{(USB_VID_LEADTEK << 16) + USB_PID_WINFAST_DTV_DONGLE_GOLD, RC_MAP_LEADTEK_Y04G0051},
	{(USB_VID_AVERMEDIA << 16) + USB_PID_AVERMEDIA_VOLAR_X, RC_MAP_AVERMEDIA_M135A},
	{(USB_VID_AFATECH << 16) + USB_PID_TREKSTOR_DVBT, RC_MAP_TREKSTOR},
	{(USB_VID_KWORLD_2 << 16) + USB_PID_TINYTWIN_2, RC_MAP_DIGITALNOW_TINYTWIN},
	{(USB_VID_GTEK << 16) + USB_PID_TINYTWIN_3, RC_MAP_DIGITALNOW_TINYTWIN},
	{(USB_VID_ITETECH << 16) + USB_PID_ITETECH_IT913X, rc_map_type},
	{(USB_VID_ITETECH << 16) + USB_PID_ITETECH_IT913X_9005, rc_map_type},
	{(USB_VID_ITETECH << 16) + USB_PID_ITETECH_IT913X_9006, rc_map_type},
	{ }
};

void RemoteConfig_set(
	struct usb_device *udev,
	struct dvb_usb_device_properties *props)
{
	u16 vid = le16_to_cpu(udev->descriptor.idVendor);
	u16 pid = le16_to_cpu(udev->descriptor.idProduct);
	char manufacturer[10];
	Device_Context *DC = NULL;

	deb_data("- Enter %s Function -\n", __func__);

	DC = (Device_Context *)dev_get_drvdata(&udev->dev);
	if (!DC) {
		deb_data("\t Error: can't find Device_Context from usb_device\n");
		return ;
	}

	switch (DC->bIrType) {
	case 0:
		strcpy(rc_map_type, "NEC");
		break;
	case 1:
		strcpy(rc_map_type, "RC6");
		break;
	default:
		deb_data("\t Warning: exception bIrType [%d], using default [RC5]\n", DC->bIrType);
	case 2:
		strcpy(rc_map_type, "RC5");
		break;
	}

	/* Try to load remote based module param */
	props->rc.core.rc_codes = RemoteConfig_setup_match(DVB_remote, rc_setup_modparam);

	/* Try to load remote based eeprom hash */
	if (!props->rc.core.rc_codes)
		props->rc.core.rc_codes = RemoteConfig_setup_match(remote_config.eeprom_sum, rc_setup_hashes);

	/* Try to load remote based USB ID */
	if (!props->rc.core.rc_codes)
		props->rc.core.rc_codes = RemoteConfig_setup_match((vid << 16) + pid, rc_setup_usbids);

	/* Try to load remote based USB iManufacturer string */
	if (!props->rc.core.rc_codes && vid == USB_VID_AFATECH){
		/* Check USB manufacturer and product strings and try to determine correct remote in case of chip vendor
		   reference IDs are used. DO NOT ADD ANYTHING NEW HERE. Use hashes instead. */

		memset(manufacturer, 0, sizeof(manufacturer));
		usb_string(udev, udev->descriptor.iManufacturer, manufacturer, sizeof(manufacturer));

		if (!strcmp("MSI", manufacturer)) {
			/* iManufacturer 1 MSI
			   iProduct      2 MSI K-VOX */
			props->rc.core.rc_codes = RemoteConfig_setup_match(IT913X_REMOTE_MSI_DIGIVOX_MINI_II_V3, rc_setup_modparam);
		}
	}

	/* Finally load "empty" just for leaving IR receiver enabled */
	if (!props->rc.core.rc_codes) {
		deb_data("\t Warning: props->rc.core.rc_codes = NULL, using default [RC_MAP_EMPTY]\n");
		props->rc.core.rc_codes = RC_MAP_EMPTY;
	}
}

static int RemoteConfig_query(struct dvb_usb_device *dvb_usb_dev)
{
	struct it913x_state *priv = dvb_usb_dev->priv;
	Device_Context *DC = NULL;
	DWORD result = Error_NO_ERROR;
	DWORD ulRawIr = 0;

	//deb_data("- Enter %s Function -\n", __func__);

	DC = (Device_Context *)dev_get_drvdata(&dvb_usb_dev->adapter->dev->udev->dev);
	if (!DC) {
		deb_data("\t %s error: can't find Device_Context from usb_device\n", __func__);
		return -ENODEV;
	}

	result = DL_ReadRawIR(DC, &ulRawIr);
	if (result) {
		deb_data("\t Waring: DL_ReadRawIR fail [0x%08lx] (No signal)\n", result);
		return -1;
	}

	priv->rc_keycode = ulRawIr;
	#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 17, 0)
		rc_keydown(dvb_usb_dev->rc_dev, 0, priv->rc_keycode, 0);
	#else
		rc_keydown(dvb_usb_dev->rc_dev, priv->rc_keycode, 0);
	#endif

	return 0;
}
#endif

static int PID_filter_ctrl(
	struct dvb_usb_adapter *adap,
	int onoff)
{
	#ifdef DVB_USB_ADAP_NEED_PID_FILTER
		#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 2, 0)
			Device_Context *DC = (Device_Context *)adap->fe_adap[0].fe->demodulator_priv;
		#else
			Device_Context *DC = (Device_Context *)adap->fe->demodulator_priv;
		#endif

		#if LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 18)
			DWORD result = Error_NO_ERROR;

			result = DL_PIDOnOff(DC, (BYTE)adap->id, onoff);
			if (result) {
				deb_data("\t Error: DL_PIDOnOff fail [0x%08lx]\n", result);
				return -1;
			}
		#endif
	#endif

	return 0;
}

static int PID_filter(
	struct dvb_usb_adapter *adap,
	int index,
	u16 pidnum,
	int onoff)
{
	#ifdef DVB_USB_ADAP_NEED_PID_FILTER 
		#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 2, 0)
			Device_Context *DC = (Device_Context *)adap->fe_adap[0].fe->demodulator_priv;
		#else
			Device_Context *DC = (Device_Context *)adap->fe->demodulator_priv;
		#endif

		#if LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 18)
			int ret = 0;
			Pid pid;
			DWORD result = Error_NO_ERROR;

			//deb_data("\t Set pid filter[%d] = %s, feedcount = %d\n", index, onoff ? "ON" : "OFF", adap->feedcount);
			pid.value = (Word)pidnum;

			if (pidnum > 0x1fff)
				return 0;

			if (onoff) {
				result = DL_AddPID(DC, (BYTE)adap->id, index, pid);
				if (result) {
					deb_data("\t Error: DL_AddPID fail [0x%08lx]\n", result);
					return -1;
				}
			}
			else {
				result = DL_RemovePID(DC, (BYTE)adap->id, index, pid);
				if (result) {
					deb_data("\t Error: DL_RemovePID fail [0x%08lx]\n", result);
					return -1;
				}
			}
		#endif
	#endif

	return 0;
}

static int DVB_download_firmware(
	struct usb_device *usb_dev,
	const struct firmware *fw)
{
	//deb_data("- Enter %s Function -\n", __func__);

	return 0;
}

static int DVB_power_ctrl(
	struct dvb_usb_device *dvb_usb_dev,
	int on)
{
	//deb_data("- Enter %s Function, POWER = %s -\n", __func__, on ? "ON" : "DOWN");

	return 0;
}

static int DVB_identify_state(
	struct usb_device *udev,
	struct dvb_usb_device_properties *props,
	struct dvb_usb_device_description **desc,
	int *cold)
{
	//deb_data("- Enter %s Function, state = %s -\n", __func__, *cold ? "cold" : "warm");

	*cold = 0;

	return 0;
}

static int I2C_xfer(
	struct i2c_adapter *adap,
	struct i2c_msg msg[],
	int num)
{
	//deb_data("- Enter %s Function -\n", __func__);

	return 0;
}

static u32 I2C_func(struct i2c_adapter *adapter)
{
	//deb_data("- Enter %s Function -\n", __func__);

	return I2C_FUNC_I2C;
}

static struct i2c_algorithm I2C_algo = {
	.master_xfer = I2C_xfer,
	.functionality = I2C_func,
};

/* called to attach the possible frontends */
static int DVB_frontend_attach(struct dvb_usb_adapter *adap)
{
	Device_Context *DC =NULL;

	//deb_data("- Enter %s Function -\n", __func__);

	DC = dev_get_drvdata(&adap->dev->udev->dev);
	if (!DC) {
		deb_data("\t %s error: can't find Device_Context from usb_device\n", __func__);
		return -ENODEV;
	}

	#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 2, 0)
		adap->fe_adap[0].fe = DVB_frontend(adap);
		return adap->fe_adap[0].fe == NULL ? -ENODEV : 0;
	#else
		adap->fe = DVB_frontend(adap);
		return adap->fe == NULL ? -ENODEV : 0;
	#endif
}

static int DVB_streaming_ctrl(
	struct dvb_usb_adapter *adap,
	int onoff)
{
	#if LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 18)
		deb_data("- Enter %s Function, chip[%d] streaming = %s -\n", __func__, adap->id, onoff ? "ON" : "OFF");
	#else
		deb_data("- Enter %s Function, streaming = %s -\n", __func__, onoff ? "ON" : "OFF");
	#endif

	return 0;
}

struct dvb_usb_device_properties DVB_properties[] = {
	{
		.caps = DVB_USB_IS_AN_I2C_ADAPTER,
		.size_of_priv = sizeof(struct it913x_state),
		.i2c_algo = &I2C_algo,

		.usb_ctrl = DEVICE_SPECIFIC,
		.download_firmware = DVB_download_firmware,
		.no_reconnect = 1,
		.power_ctrl = DVB_power_ctrl,
		.identify_state = DVB_identify_state,
		.num_adapters = 1,
		#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 2, 0)
			.adapter = {
				{
					.num_frontends = 1,
					.fe = {{
						#ifdef DVB_USB_ADAP_NEED_PID_FILTER
							.caps = DVB_USB_ADAP_HAS_PID_FILTER | DVB_USB_ADAP_PID_FILTER_CAN_BE_TURNED_OFF | DVB_USB_ADAP_NEED_PID_FILTERING,
						#else
							.caps = DVB_USB_ADAP_HAS_PID_FILTER | DVB_USB_ADAP_PID_FILTER_CAN_BE_TURNED_OFF,
						#endif
						.pid_filter_count = 32,
						.pid_filter = PID_filter,
						.pid_filter_ctrl = PID_filter_ctrl,
						.frontend_attach = DVB_frontend_attach,
						.streaming_ctrl = DVB_streaming_ctrl,
						.stream = {
							.type = USB_BULK,
							.count = 4,
							.endpoint = 0x84,
							.u = {
								.bulk = {
									//.buffersize = User_USB20_FRAME_SIZE,
									.buffersize = (188 * 348),
								}
							}
						}
					}}, //fe
				},
				{
					.num_frontends = 1,
					.fe = {{
						#ifdef DVB_USB_ADAP_NEED_PID_FILTER
							.caps = DVB_USB_ADAP_HAS_PID_FILTER | DVB_USB_ADAP_PID_FILTER_CAN_BE_TURNED_OFF | DVB_USB_ADAP_NEED_PID_FILTERING,
						#else
							.caps = DVB_USB_ADAP_HAS_PID_FILTER | DVB_USB_ADAP_PID_FILTER_CAN_BE_TURNED_OFF,
						#endif
						.pid_filter_count = 32,
						.pid_filter = PID_filter,
						.pid_filter_ctrl = PID_filter_ctrl,
						.frontend_attach = DVB_frontend_attach,
						.streaming_ctrl = DVB_streaming_ctrl,
						.stream = {
							.type = USB_BULK,
							.count = 4,
							.endpoint = 0x85,
							.u = {
								.bulk = {
									//.buffersize = User_USB20_FRAME_SIZE,
									.buffersize = (188 * 348),
								}
							}
						}
					}}, //fe
				},
			}, //adapter
		#else
			.adapter = {
				{
					#ifdef DVB_USB_ADAP_NEED_PID_FILTER
						.caps = DVB_USB_ADAP_HAS_PID_FILTER | DVB_USB_ADAP_PID_FILTER_CAN_BE_TURNED_OFF | DVB_USB_ADAP_NEED_PID_FILTERING,
					#else
						.caps = DVB_USB_ADAP_HAS_PID_FILTER | DVB_USB_ADAP_PID_FILTER_CAN_BE_TURNED_OFF,
					#endif
					.pid_filter_count = 32,
					.pid_filter = PID_filter,
					.pid_filter_ctrl = PID_filter_ctrl,
					.frontend_attach = DVB_frontend_attach,
					.streaming_ctrl = DVB_streaming_ctrl,
					.stream = {
						.type = USB_BULK,
						.count = 4,
						.endpoint = 0x84,
						.u = {
							.bulk = {
								//.buffersize = User_USB20_FRAME_SIZE,
								.buffersize = (188 * 348),
							}
						}
					}
				},
				{
					#ifdef DVB_USB_ADAP_NEED_PID_FILTER
						.caps = DVB_USB_ADAP_HAS_PID_FILTER | DVB_USB_ADAP_PID_FILTER_CAN_BE_TURNED_OFF | DVB_USB_ADAP_NEED_PID_FILTERING,
					#else
						.caps = DVB_USB_ADAP_HAS_PID_FILTER | DVB_USB_ADAP_PID_FILTER_CAN_BE_TURNED_OFF,
					#endif
					.pid_filter_count = 32,
					.pid_filter = PID_filter,
					.pid_filter_ctrl = PID_filter_ctrl,
					.frontend_attach = DVB_frontend_attach,
					.streaming_ctrl = DVB_streaming_ctrl,
					.stream = {
						.type = USB_BULK,
						.count = 4,
						.endpoint = 0x85,
						.u = {
							.bulk = {
								//.buffersize = User_USB20_FRAME_SIZE,
								.buffersize = (188 * 348),
							}
						}
					}
				},
			}, //adapter
		#endif

		#if LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 37)
			.rc.core = {
				.module_name		= "it913x",
				.rc_query			= RemoteConfig_query,
				.rc_interval		= IT913X_RC_INTERVAL,
				#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 14, 0)
					.protocol			= RC_PROTO_RC6_0,
					.allowed_protos	= RC_PROTO_RC6_0,
				#elif LINUX_VERSION_CODE >= KERNEL_VERSION(3, 8, 0)
					.protocol			= RC_TYPE_RC6_0,
					.allowed_protos	= RC_TYPE_RC6_0,
				#else
					.protocol			= RC_TYPE_RC6,
					.allowed_protos	= RC_TYPE_RC6,
				#endif
			},
		#endif
		.num_device_descs = 1,
		.devices = {
			{
				"ITEtech USB2.0 DVB-T Recevier",
				{
					&IT913x_usb_id_table[0], &IT913x_usb_id_table[1],&IT913x_usb_id_table[2],
					&IT913x_usb_id_table[3], &IT913x_usb_id_table[4],&IT913x_usb_id_table[5],
					&IT913x_usb_id_table[6], &IT913x_usb_id_table[7], NULL
				},
				{
					NULL
				},
			},
			{
				NULL
			},

		}
	}
};
int it913x_device_count = ARRAY_SIZE(DVB_properties);
