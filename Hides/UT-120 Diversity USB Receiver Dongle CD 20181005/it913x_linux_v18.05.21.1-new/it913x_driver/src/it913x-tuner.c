#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/delay.h>
#include <linux/dvb/frontend.h>
#include "dvb_frontend.h"
#include "it913x.h"

#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 10, 0)
static int tuner_release(struct dvb_frontend *fe)
{
	kfree(fe->tuner_priv);
	fe->tuner_priv = NULL;
	return 0;
}
#else
static void tuner_release(struct dvb_frontend *fe)
{
	kfree(fe->tuner_priv);
	fe->tuner_priv = NULL;
}
#endif

static int tuner_init(struct dvb_frontend *fe)
{
	return 0;
}

static int tuner_sleep(struct dvb_frontend *fe)
{
	return 0;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 3, 0)
static int tuner_set_params(
	struct dvb_frontend *fe,
	struct dvb_frontend_parameters *params)
{
	struct tuner_priv *priv = NULL;
	Device_Context *DC = (Device_Context *)fe->demodulator_priv;
	DWORD result = Error_NO_ERROR;
	DWORD freq = params->frequency; // 1000; // Hz -> kHz

	priv->bandwidth = (fe->ops.info.type == FE_OFDM) ? params->u.ofdm.bandwidth : 0;
	deb_data("- Enter %s Function, set freq = %ld, bandwith = %d -\n", __func__, freq, priv->bandwidth);

	result = DL_Tuner_SetFreqBw(DC, fe->dvb->num, freq,priv->bandwidth);
	if (result) {
		deb_data("\t Error: DL_Tuner_SetFreqBw fail [0x%08lx]\n", result);
		return -1;
	}

	return 0;
}
#else
static int tuner_set_params(struct dvb_frontend *fe)
{
	struct tuner_priv *priv = NULL;
	Device_Context *DC = (Device_Context *)fe->demodulator_priv;
	DWORD result = Error_NO_ERROR;
	struct dtv_frontend_properties *c = &fe->dtv_property_cache;

	DWORD freq = c->frequency; // 1000; // Hz -> kHz
	priv->bandwidth = c->bandwidth_hz;

	deb_data("- Enter %s Function, set freq = %ld, bandwith = %d -\n", __func__, freq, priv->bandwidth);

	result = DL_Tuner_SetFreqBw(DC, fe->dvb->num, freq,priv->bandwidth);
	if (result) {
		deb_data("\t Error: DL_Tuner_SetFreqBw fail [0x%08lx]\n", result);
		return -1;
	}

	return 0;
}
#endif

static int tuner_get_frequency(
	struct dvb_frontend *fe,
	u32 *frequency)
{
	struct tuner_priv *priv = fe->tuner_priv;

	*frequency = priv->frequency;

	return 0;
}

static int tuner_get_bandwidth(
	struct dvb_frontend *fe,
	u32 *bandwidth)
{
	struct tuner_priv *priv = fe->tuner_priv;

	*bandwidth = priv->bandwidth;

	return 0;
}

static const struct dvb_tuner_ops tuner_tuner_ops = {
	.info = {
		.name = "dvb_usb_tuner",
		.frequency_min = 48000000,
		.frequency_max = 950000000,
		.frequency_step = 50000,
	},

	.release = tuner_release,
	.init = tuner_init,
	.sleep = tuner_sleep,
	.set_params = tuner_set_params,
	.get_frequency = tuner_get_frequency,
	.get_bandwidth = tuner_get_bandwidth
};

/* This functions tries to identify a MT2060 tuner by reading the PART/REV register. This is hasty. */
struct dvb_frontend *tuner_attach(struct dvb_frontend *fe)
{
	struct tuner_priv *priv = NULL;

	priv = kzalloc(sizeof(struct tuner_priv), GFP_KERNEL);
	if (priv == NULL)
		goto error;

	memcpy(&fe->ops.tuner_ops, &tuner_tuner_ops, sizeof(struct dvb_tuner_ops));
	fe->tuner_priv = priv;

	return fe;

error:
	kfree(priv);
	return NULL;
}
