#ifndef __ADRF6755_H_
#define __ADRF6755_H_

#include "demodulator.h"

Dword ADRF6655_init (Demodulator*	demodulator);
Dword ADRF6655_writeData (
		Demodulator*	demodulator,
		Dword		data	
);

Dword ADRF6655_setLoFrequency (
		Demodulator*	demodulator,
		Dword		freq_KHz	//KHz
);

Dword ADRF6655_setCpRefSource (
		Demodulator*	demodulator,
		Bool		isExternal
);

Dword ADRF6655_setPhaseOffseMultiplier (
		Demodulator*	demodulator,
		Byte		multiplier
);

Dword ADRF6655_setCpCurrentMultiplier (
		Demodulator*	demodulator,
		Byte		multiplier
);


#endif