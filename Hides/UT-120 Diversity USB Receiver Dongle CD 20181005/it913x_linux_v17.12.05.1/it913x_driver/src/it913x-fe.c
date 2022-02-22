/*
 * Frontend part of the Linux driver for the IT913x
 */

#include "it913x.h"
#include "dvb_frontend.h"

int DCA_TO_PIP(Device_Context *DC)
{
	deb_data("- Enter %s Function -\n", __func__);

	switch (atomic_read(&DC->FilterCnt)) {
	case 1:
		/* Only one open, using DCA mode */
		DL_SetArchitecture(DC, Architecture_DCA);
		msleep(50);
		deb_data("\t TSMode = DCA\n");
		break;
	case 2:
		/* Two APP open, change to PIP mode */
		DL_SetArchitecture(DC, Architecture_PIP);
		msleep(50);
		deb_data("\t TSMode = PIP\n");
		break;
	default:
		deb_data("\t Error: FilterCnt not correct [%d]\n", atomic_read(&DC->FilterCnt));
		return -1;
		break;
	}

	return 0;
}

int PIP_TO_DCA(Device_Context *DC)
{
	deb_data("- Enter %s Function -\n", __func__);

	switch (atomic_read(&DC->FilterCnt)) {
	case 0:
	case 1:
		/* All or one app closed, using DCA mode */
		DL_SetArchitecture(DC, Architecture_DCA);
		msleep(50);
		deb_data("\t TSMode = DCA\n");
		break;
	default:
		deb_data("\t Error : FilterCnt not correct [%d]\n", atomic_read(&DC->FilterCnt));
		return -1;
		break;
	}

	return 0;
}

/* IT913x device tuner on init */
static int DVB_ops_init(struct dvb_frontend *dvb_fe)
{
	Device_Context *DC = (Device_Context *)dvb_fe->demodulator_priv;
	Byte chip_index = 0, chip_id = 0;
	DWORD result = Error_NO_ERROR;

	if (!DC) {
		deb_data("\t %s error: can't find Device_Context from dvb_frontend\n", __func__);
		return -1;
	}

	/* For 4-Rx */
	if (DC->Demodulator.chipNumber == 2) {
		if(DC->architecture == Architecture_PIP || DC->bDualTs == True) {
			chip_id = dvb_fe->dvb->num % 2;
		}
	}
	else
		chip_id = 0;

	deb_data("- Enter %s Function, chip[%d] -\n", __func__, chip_id);

	if (DC->disconnect == 1) {
		deb_data("\t Warning: device disconnect\n");
		return -1;
	}

	DC->AP_off = 0;
	atomic_add(1, &DC->FilterCnt);

	/* Power on, for performance, open all power */
	for (chip_index = 0; chip_index < DC->Demodulator.chipNumber; chip_index++) {
		if (DC->fc[chip_index].tunerinfo.bTunerInited == 0) {
			result = DL_ApCtrl(DC, chip_index, true);
			if (result) {
				deb_data("\t Error: chip[%d] POWER ON fail [0x%08lx]\n", chip_index, result);
				return -1;
			}
		}
		DC->fc[chip_index].bTimerOn = true;
		DC->fc[chip_index].tunerinfo.bTunerInited = true;
	}

	/* Change TS mode if supporting */
	if (DC->bDCAPIP)
		DCA_TO_PIP(DC);

	return 0;
}

/* IT913x device tuner off sleep */
static int DVB_ops_sleep(struct dvb_frontend *dvb_fe)
{
	DWORD result = Error_NO_ERROR;
	Byte chip_index = 0, chip_id = 0;
	Device_Context *DC = (Device_Context *)dvb_fe->demodulator_priv;

	if (!DC) {
		deb_data("\t %s error: can't find Device_Context from dvb_frontend\n", __func__);
		return -1;
	}

	/* For 4-Rx */
	if (DC->Demodulator.chipNumber == 2) {
		if(DC->architecture == Architecture_PIP || DC->bDualTs == True) {
			chip_id = dvb_fe->dvb->num % 2;
		}
	}
	else
		chip_id = 0;

	deb_data("- Enter %s Function, chip[%d] -\n", __func__, chip_id);

	atomic_sub(1, &DC->FilterCnt);

	if (atomic_read(&DC->FilterCnt) == 0)
		DC->AP_off = 1;

	if (DC->disconnect == 1) {
		#if HOT_PLUGGING
			deb_data("\t Warning: device disconnect, wait all AP closed\n");
			wake_up_interruptible(&(DC->disconnect_wait));
		#endif
		return -ENODEV;
	}

	/* Change TS mode if supporting */
	if (DC->bDCAPIP)
		PIP_TO_DCA(DC);

	switch (atomic_read(&DC->FilterCnt)) {
	case 0:
		for (chip_index = DC->Demodulator.chipNumber; chip_index > 0; chip_index--) {
			DC->fc[chip_index - 1].bTimerOn = false;
			DC->fc[chip_index - 1].tunerinfo.bTunerInited = false;
			DC->fc[chip_index - 1].ulDesiredFrequency = 0;
			DC->fc[chip_index - 1].ucDesiredBandWidth = 0;
			DL_ApCtrl(DC, (chip_index - 1), false);
			if (result) {
				deb_data("\t Error: chip[%d] POWER DOWN fail [0x%08lx]\n", chip_index, result);
				return -1;
			}
		}
		break;
	case 1:
		/* For performance, not only close one power */
		DC->fc[chip_id].bTimerOn = false;
		DC->fc[chip_id].ulDesiredFrequency = 0;
		DC->fc[chip_id].ucDesiredBandWidth = 0;
		DL_Tuner_SetFreqBw(DC, chip_id, DC->fc[!chip_id].ulCurrentFrequency, (DC->fc[!chip_id].ucCurrentBandWidth / 1000));
		break;
	default:
		deb_data("\t Error: FilterCnt not correct [%d]\n", atomic_read(&DC->FilterCnt));
		break;
	}

	return 0;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 3, 0)
static int DVB_ops_get_frontend(
	struct dvb_frontend *dvb_fe,
	struct dvb_frontend_parameters *dvb_fe_prm)
{
	Byte chip_id = 0;
	Device_Context *DC = (Device_Context *)dvb_fe->demodulator_priv;

	if (!DC) {
		deb_data("\t %s error: can't find Device_Context from dvb_frontend\n", __func__);
		return -1;
	}

	/* For 4-Rx */
	if (DC->Demodulator.chipNumber == 2) {
		if(DC->architecture == Architecture_PIP || DC->bDualTs == True) {
			chip_id = dvb_fe->dvb->num % 2;
		}
	}
	else
		chip_id = 0;

	deb_data("- Enter %s Function, chip[%d] -\n", __func__, chip_id);

	if (DC->disconnect == 1) {
		deb_data("\t Warning: device disconnect\n");
		return -ENODEV;
	}

	dvb_fe_prm->inversion = INVERSION_AUTO;

	return 0;
}

/* Set OFDM bandwidth and tuner frequency */
static int DVB_ops_set_frontend(
	struct dvb_frontend *dvb_fe,
	struct dvb_frontend_parameters *dvb_fe_prm)
{
	Device_Context *DC = (Device_Context *)dvb_fe->demodulator_priv;
	DWORD result = Error_NO_ERROR;
	Dword tmpLoFreq = 1800000;
	long long frequency_diff = 0;
	u32 frequency_kHz = 0, tmpfreq = 0;
	u8 bandwidth = 0;
	Byte chip_id = 0;

	if (!DC) {
		deb_data("\t %s error: can't find Device_Context from dvb_frontend\n", __func__);
		return -1;
	}

	/* For 4-Rx */
	if (DC->Demodulator.chipNumber == 2) {
		if(DC->architecture == Architecture_PIP || DC->bDualTs == True) {
			chip_id = dvb_fe->dvb->num % 2;
		}
	}
	else
		chip_id = 0;

	//deb_data("- Enter %s Function, chip[%d] -\n", __func__, chip_id);

	if (DC->disconnect == 1) {
		//deb_data("\t Warning: device disconnect\n");
		return -ENODEV;
	}

	switch (dvb_fe_prm->u.ofdm.bandwidth) {
		case BANDWIDTH_8_MHZ:
			bandwidth = 8;
			break;
		case BANDWIDTH_7_MHZ:
			bandwidth = 7;
			break;
		case BANDWIDTH_6_MHZ:
			bandwidth = 6;
			break;
#if LINUX_VERSION_CODE > KERNEL_VERSION(3, 0, 0)
		case BANDWIDTH_5_MHZ:
			bandwidth = 5;
			break;
		case BANDWIDTH_1_712_MHZ:
			bandwidth = 2;
			break;
#endif
		default:
			deb_data("\t Warning: bandwidth not correct [%d]\n", dvb_fe_prm->u.ofdm.bandwidth);
	}

/*This is for v4l only, prevent v4l would slightly modulate frequency during set frequency*/	
	if(DC->board_id == DownConverter_ADRF6655)
		tmpLoFreq = 1800000;
	else if(DC->board_id == DownConverter_RFFC2072)
		tmpLoFreq = LO_Frequency;
	else if(DC->board_id == DownConverter_RFFC2071)
		tmpLoFreq = LO_Frequency;
	frequency_kHz = dvb_fe_prm->frequency / 1000;

	if(frequency_kHz > 1000000)
	{
		if(frequency_kHz > tmpLoFreq)
			tmpfreq = frequency_kHz - tmpLoFreq;
		else
			tmpfreq =  tmpLoFreq - frequency_kHz;
	}
	else
		tmpfreq = frequency_kHz;
/*This is for v4l only, prevent v4l would slightly modulate frequency during set frequency  end*/	

	frequency_diff = DC->fc[chip_id].ulCurrentFrequency - tmpfreq;
	if ((bandwidth != DC->fc[chip_id].ucCurrentBandWidth))
	{
		if ((frequency_diff > 500) || (frequency_diff < -500) )
			result = DL_Tuner_SetFreqBw(DC, chip_id, frequency_kHz, bandwidth);

		if (result) {
			deb_data("\t Error: DL_Tuner_SetFreqBw fail [0x%08lx]\n", result);
			return -1;
		}
	}
	return 0;
}

#else
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 6, 0)
static int DVB_ops_get_frontend(struct dvb_frontend *dvb_fe, struct dtv_frontend_properties *props)
#else
static int DVB_ops_get_frontend(struct dvb_frontend *dvb_fe)
#endif	
{
	Device_Context *DC = (Device_Context *)dvb_fe->demodulator_priv;
	Byte chip_id = 0;

	if (!DC) {
		deb_data("\t %s error: can't find Device_Context from dvb_frontend\n", __func__);
		return -1;
	}

	/* For 4-Rx */
	if (DC->Demodulator.chipNumber == 2) {
		if(DC->architecture == Architecture_PIP || DC->bDualTs == True) {
			chip_id = dvb_fe->dvb->num % 2;
		}
	}
	else
		chip_id = 0;

	deb_data("- Enter %s Function, chip[%d] -\n", __func__, chip_id);

	if (DC->disconnect == 1) {
		deb_data("\t Warning: device disconnect: DVB_ops_get_frontend\n");
		return -ENODEV;
	}

	dvb_fe->dtv_property_cache.inversion = INVERSION_AUTO;

	return 0;
}

/* Set OFDM bandwidth and tuner frequency */
static int DVB_ops_set_frontend(struct dvb_frontend *dvb_fe)
{
	Device_Context *DC = (Device_Context *)dvb_fe->demodulator_priv;
	struct dtv_frontend_properties *dtv_fe_prop = &dvb_fe->dtv_property_cache;
	long long frequency_diff = 0;
	DWORD result = Error_NO_ERROR;
	Dword tmpLoFreq = 1800000;
	u32 frequency_kHz = 0, tmpfreq = 0;
	u8 bandwidth = 0;
	Byte chip_id = 0;

	if (!DC) {
		deb_data("\t %s error: can't find Device_Context from dvb_frontend\n", __func__);
		return -1;
	}

	/* For 4-Rx */
	if (DC->Demodulator.chipNumber == 2) {
		if(DC->architecture == Architecture_PIP || DC->bDualTs == True) {
			chip_id = dvb_fe->dvb->num % 2;
		}
	}
	else
		chip_id = 0;

	deb_data("- Enter %s Function, chip[%d] -\n", __func__, chip_id);

	if (DC->disconnect == 1) {
		//deb_data("\t Warning: device disconnect: DVB_ops_set_frontend 1\n");	
		return -ENODEV;
	}

	/* 
	 * The bandwidth value is changed by kernel. The operation is working at
	 * "dvb-core/dvb_frontend.c"->dtv_property_cache_sync().
	 */
	switch (dtv_fe_prop->bandwidth_hz) {
	default:
		deb_data("\t Warning: bandwidth not correct [%d]\n", dtv_fe_prop->bandwidth_hz);
	case 10000000:
	case 8000000:
	case 7000000:
	case 6000000:
	case 5000000:
		bandwidth = dtv_fe_prop->bandwidth_hz / 1000000;
		break;
	case 1712000:
		bandwidth = 2;
		break;
	}
/*This is for v4l only, prevent v4l would slightly modulate frequency during set frequency*/	
	if(DC->board_id == DownConverter_ADRF6655)
		tmpLoFreq = 1800000;
	else if(DC->board_id == DownConverter_RFFC2072)
		tmpLoFreq = LO_Frequency;
	else if(DC->board_id == DownConverter_RFFC2071)
		tmpLoFreq = LO_Frequency;

	frequency_kHz = dtv_fe_prop->frequency / 1000;

	if(frequency_kHz > 1000000)
	{
		if(frequency_kHz > tmpLoFreq)
			tmpfreq = frequency_kHz - tmpLoFreq;
		else
			tmpfreq =  tmpLoFreq - frequency_kHz;
	}
	else
		tmpfreq = frequency_kHz;
/*This is for v4l only, prevent v4l would slightly modulate frequency during set frequency  end*/	

	frequency_diff = DC->fc[chip_id].ulCurrentFrequency - tmpfreq;
	if ((bandwidth != DC->fc[chip_id].ucCurrentBandWidth))
	{
		if ((frequency_diff > 500) || (frequency_diff < -500) )
			result = DL_Tuner_SetFreqBw(DC, chip_id, frequency_kHz, bandwidth);

		if (result) {
			deb_data("\t Error: DL_Tuner_SetFreqBw fail [0x%08lx]\n", result);
			return -1;
		}
	}

	return 0;
}
#endif

/* Check tuner signal lock */

#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 2, 0)
static int DVB_ops_read_status(
	struct dvb_frontend *dvb_fe,
	fe_status_t *stat)
#else
static int DVB_ops_read_status(
	struct dvb_frontend *dvb_fe,
	enum fe_status *stat)
#endif
{	
	Device_Context *DC = (Device_Context *)dvb_fe->demodulator_priv;
	DWORD result = Error_NO_ERROR;
	Bool bLock = true;
	Byte chip_id = 0;
	int i = 0;

	if (!DC) {
		deb_data("\t %s error: can't find Device_Context from dvb_frontend\n", __func__);
		return -1;
	}

	/* For 4-Rx */
	if (DC->Demodulator.chipNumber == 2) {
		if(DC->architecture == Architecture_PIP || DC->bDualTs == True) {
			chip_id = dvb_fe->dvb->num % 2;
		}
	}
	else
		chip_id = 0;

	//deb_data("- Enter %s Function, chip[%d], state = %d -\n", __func__, chip_id, *stat);

	*stat = 0;

	if (DC->disconnect == 1) {
		//deb_data("\t Warning: device disconnect: DVB_ops_read_status\n");
		return -ENODEV;
	}

/*****************************************************
 *  This modification--
 *  Due to some customer said the origin check procedure
 *  would influence the get packet performance.
 *
 *  New method -- only read Mpeg2 lock status
 *
*****************************************************/
#if 0 
	result = DL_isLocked(DC, chip_id, &bLock);
	if (result) {
		deb_data("\t Error: DL_isLocked fail [0x%08lx]\n", result);
		return -1;
	}
#endif
# if 1 

	mutex_lock(&DC->dev_mutex);
	for (i = 0; i < 2; i++) {
		DC->Demodulator.statistic[i].signalPresented = False;
		DC->Demodulator.statistic[i].signalLocked = False;
		DC->Demodulator.statistic[i].signalQuality = 0;
		DC->Demodulator.statistic[i].signalStrength = 0;
	}

	if (DC->architecture == Architecture_DCA) {
		result = Standard_isMpeg2Locked ((Demodulator *)&DC->Demodulator, 0, &DC->Demodulator.statistic[0].signalLocked);
		if (result) {
			mutex_unlock(&DC->dev_mutex);
			return -ENODEV;
		}
		if (DC->Demodulator.statistic[0].signalLocked == True) {
			for (i = 0; i < 2; i++) {
				DC->Demodulator.statistic[i].signalQuality = 80;
				DC->Demodulator.statistic[i].signalStrength = 80;
			}
			bLock = True;
		}
		else
			bLock = False;
	}
	else {
		result = Standard_isMpeg2Locked ((Demodulator *)&DC->Demodulator, chip_id, &DC->Demodulator.statistic[chip_id].signalLocked);
		if (result) {
			mutex_unlock(&DC->dev_mutex);
			return -ENODEV;
		}
		if (DC->Demodulator.statistic[chip_id].signalLocked == True) {
			DC->Demodulator.statistic[chip_id].signalQuality = 80;
			DC->Demodulator.statistic[chip_id].signalStrength = 80;
			bLock = True;
		}
		else
			bLock = False;
	}
	mutex_unlock(&DC->dev_mutex);
#endif
	if (bLock) {
		*stat |= FE_HAS_SIGNAL;
		*stat |= FE_HAS_CARRIER;
		*stat |= FE_HAS_LOCK;
		*stat |= FE_HAS_VITERBI;
		*stat |= FE_HAS_SYNC;
	}
	else {
		*stat |= FE_TIMEDOUT;
	}

	return 0;
}

/* Get ChannelStatistic and calculate ber value */
static int DVB_ops_read_ber(
	struct dvb_frontend *dvb_fe,
	u32 *ber)
{
	Device_Context *DC = (Device_Context *)dvb_fe->demodulator_priv;
	DWORD result = Error_NO_ERROR;
	ChannelStatistic channelStatistic;
	Byte chip_id = 0;

	if (!DC) {
		deb_data("\t %s error: can't find Device_Context from dvb_frontend\n", __func__);
		return -1;
	}

	/* For 4-Rx */
	if (DC->Demodulator.chipNumber == 2) {
		if(DC->architecture == Architecture_PIP || DC->bDualTs == True) {
			chip_id = dvb_fe->dvb->num % 2;
		}
	}
	else
		chip_id = 0;

	//deb_data("- Enter %s Function, chip[%d] -\n", __func__, chip_id);

	if (DC->disconnect == 1) {
		deb_data("\t Warning: device disconnect: DVB_ops_read_ber\n");
		return -ENODEV;
	}

	result = DL_getChannelStatistic(DC, chip_id, &channelStatistic);	
	if (result) {
		deb_data("\t Error: DL_getChannelStatistic fail [0x%08lx]\n", result);
		return -1;
	}

	*ber = channelStatistic.postVitErrorCount * (0xFFFFFFFF / channelStatistic.postVitBitCount);

	return 0;
}

/* Calculate SNR value */
static int DVB_ops_read_snr(
	struct dvb_frontend *dvb_fe,
	u16 *snr)
{
	Device_Context *DC = (Device_Context *)dvb_fe->demodulator_priv;
	DWORD result = Error_NO_ERROR;
	Constellation constellation;
	BYTE SignalSnr = 0;
	Byte chip_id = 0;

	if (!DC) {
		deb_data("\t %s error: can't find Device_Context from dvb_frontend\n", __func__);
		return -1;
	}

	/* For 4-Rx */
	if (DC->Demodulator.chipNumber == 2) {
		if(DC->architecture == Architecture_PIP || DC->bDualTs == True) {
			chip_id = dvb_fe->dvb->num % 2;
		}
	}
	else
		chip_id = 0;

	//deb_data("- Enter %s Function, chip[%d] -\n", __func__, chip_id);

	if (DC->disconnect == 1) {
		deb_data("\t Warning: device disconnect: DVB_ops_read_snr\n");
		return -ENODEV;
	}

	result = DL_getSNR(DC, chip_id, &constellation, &SignalSnr);
	if (result) {
		deb_data("\t Error: DL_getSNR fail [0x%08lx]\n", result);
		return -1;
	}

	switch (constellation) {
	case 0:
		*snr = (u16)SignalSnr * (0xFFFF / 23);
		break;
	case 1:
		*snr = (u16)SignalSnr * (0xFFFF / 26);
		break;
	case 2:
		*snr = (u16)SignalSnr * (0xFFFF / 32);
		break;
	default:
		deb_data("\t Error: get constellation is failed\n");
		break;
	}

	return 0;
}

static int DVB_ops_read_unc_blocks(
	struct dvb_frontend *dvb_fe,
	u32 *unc)
{
	Device_Context *DC = (Device_Context *)dvb_fe->demodulator_priv;
	Byte chip_id = 0;

	if (!DC) {
		deb_data("\t %s error: can't find Device_Context from dvb_frontend\n", __func__);
		return -1;
	}

	/* For 4-Rx */
	if (DC->Demodulator.chipNumber == 2) {
		if(DC->architecture == Architecture_PIP || DC->bDualTs == True) {
			chip_id = dvb_fe->dvb->num % 2;
		}
	}
	else
		chip_id = 0;

	if (DC->disconnect == 1) {
		deb_data("\t Warning: device disconnect: DVB_ops_read_unc_blocks\n");
		return -ENODEV;
	}

	//deb_data("- Enter %s Function, chip[%d] -\n", __func__, chip_id);

	return 0;
}

/* Read signal strength and calculate strength value */
static int DVB_ops_read_signal_strength(
	struct dvb_frontend *dvb_fe,
	u16 *strength)
{
	Device_Context *DC = (Device_Context *)dvb_fe->demodulator_priv;
	DWORD result = Error_NO_ERROR;
	BYTE SignalStrength = 0, SignalStrength2 = 0;
	Byte chip_id = 0;

	if (!DC) {
		deb_data("\t %s error: can't find Device_Context from dvb_frontend\n", __func__);
		return -1;
	}

	if (DC->disconnect == 1) {
		deb_data("\t Warning: device disconnect: DVB_ops_read_signal_strength\n");
		return -ENODEV;
	}

	/* For 4-Rx */
	if (DC->Demodulator.chipNumber == 2) {
		if(DC->architecture == Architecture_PIP || DC->bDualTs == True) {
			chip_id = dvb_fe->dvb->num % 2;
		}
	}
	else
		chip_id = 0;

	//deb_data("- Enter %s Function, chip[%d] -\n", __func__, chip_id);

	result = DL_getSignalStrengthIndication(DC, chip_id, &SignalStrength);
	if (result) {
		deb_data("\t Error: DL_getSignalStrengthIndication fail [0x%08lx]\n", result);
		return -1;
	}
	
	if ((DC->Demodulator.chipNumber == 2) && (DC->architecture == Architecture_DCA)) {
		result = DL_getSignalStrengthIndication(DC, 1, &SignalStrength2);
		if (result) {
			deb_data("\t Error: DL_getSignalStrengthIndication 2 fail [0x%08lx]\n", result);
			return -1;
		}
		else {
			if (SignalStrength2 > SignalStrength)
				SignalStrength = SignalStrength2;
		}
	}

	*strength = (u16)SignalStrength * (0xFFFF/100);

	return 0;
}

static void DVB_ops_release(struct dvb_frontend *dvb_fe)
{
	deb_data("- Enter %s Function -\n", __func__);

	kfree(dvb_fe);
}

static struct dvb_frontend_ops DVB_fe_ops = {
	#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 3, 0)
		.delsys = { SYS_DVBT },
	#endif

	.info = {
		.name = "IT913x USB DVB-T",
		#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 3, 0)
			.type = FE_OFDM,
		#endif
		.frequency_min = 44250000,
		.frequency_max = 2600000000ul,
		.frequency_stepsize = 62500,
		.caps = FE_CAN_INVERSION_AUTO |
			FE_CAN_FEC_1_2 | FE_CAN_FEC_2_3 | FE_CAN_FEC_3_4 |
			FE_CAN_FEC_5_6 | FE_CAN_FEC_7_8 | FE_CAN_FEC_AUTO |
			FE_CAN_QPSK | FE_CAN_QAM_16 | FE_CAN_QAM_64 | FE_CAN_QAM_AUTO |
			FE_CAN_TRANSMISSION_MODE_AUTO |
			FE_CAN_GUARD_INTERVAL_AUTO |
			FE_CAN_RECOVER |
			FE_CAN_HIERARCHY_AUTO,
	},

	.release = DVB_ops_release,
	.init = DVB_ops_init,
	.sleep = DVB_ops_sleep,

	.set_frontend = DVB_ops_set_frontend,
	.get_frontend = DVB_ops_get_frontend,

	.read_status = DVB_ops_read_status,
	.read_ber = DVB_ops_read_ber,
	.read_signal_strength = DVB_ops_read_signal_strength,
	.read_snr = DVB_ops_read_snr,
	.read_ucblocks = DVB_ops_read_unc_blocks,
};

struct dvb_frontend *DVB_frontend(struct dvb_usb_adapter *adapter)
{
	struct dvb_frontend *dvb_fe = NULL;
	Device_Context *DC = NULL;

	//deb_data("- Create %s struct -\n", __func__);

	DC = dev_get_drvdata(&adapter->dev->udev->dev);
	if (!DC) {
		deb_data("\t Error: can't find Device_Context from dvb_usb_adapter\n");
		goto exit;
	}

	dvb_fe = kzalloc(sizeof(struct dvb_frontend), GFP_KERNEL);
	if (dvb_fe == NULL) {
		deb_data("\t Error: dvb_frontend out of memory\n");
		goto exit;
	}

	dvb_fe->demodulator_priv = DC; //Add Device_Context to private data

	memcpy(&dvb_fe->ops, &DVB_fe_ops, sizeof(struct dvb_frontend_ops));

	deb_data("- Create %s success -\n", __func__);
	return dvb_fe;

exit:
	deb_data("- Create %s fail -\n", __func__);
	return NULL;
}

MODULE_AUTHOR("Figo Chang <Figo.Chang@ite.com.tw>");
MODULE_DESCRIPTION("Driver for the IT913x demodulator");
MODULE_LICENSE("GPL");
