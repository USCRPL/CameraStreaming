#include "usb2impl.h"

#ifdef UNDER_CE

Handle Usb2_handle = NULL;


Dword Usb2_getDriver (
    IN  Modulator*    modulator,
    OUT Handle*         handle
) {
    return (Error_NO_ERROR);
}


Dword Usb2_exitDriver (
    IN  Modulator*    modulator
) {
    return (Error_NO_ERROR);
}


Dword Usb2_writeControlBus (
    IN  Modulator*    modulator,
    IN  Dword           bufferLength,
    IN  Byte*           buffer
) {
    return (Error_NO_ERROR);
}


Dword Usb2_readControlBus (
    IN  Modulator*    modulator,
    IN  Dword           bufferLength,
    OUT Byte*           buffer
) {
    return (Error_NO_ERROR);
}


Dword Usb2_readDataBus (
    IN  Modulator*    modulator,
    IN  Dword           bufferLength,
    OUT Byte*           buffer
) {
    return (Error_NO_ERROR);
}

#else
#ifdef WIN32
Handle Usb2_handle = NULL;
Bool bCoInitial = False;
bool (__cdecl *Usb2_initialize) (
	int Tx_DevNo
);
void (__cdecl *Usb2_finalize) (
    int Tx_DevNo
);
bool (__cdecl *Usb2_writeControl) (
    BYTE*       poutBuf,
    ULONG       WriteLen,
    ULONG*      pnBytesWrite,
	int			Tx_DevNo 
);
bool (__cdecl *Usb2_readControl) (
    BYTE*       pinBuf,
    ULONG       ReadLen,
    ULONG*      pnBytesRead,
	int			Tx_DevNo
);
bool (__cdecl *Usb2_readData) (
    BYTE*       pinBuf,
    ULONG       ReadLen
);
#endif

Dword Usb2_getDriver (
    IN  Modulator*    modulator,
    OUT Handle*         handle
) {
    Dword error = ModulatorError_NO_ERROR;
#ifdef WIN32
    HINSTANCE instance = NULL;
	HRESULT hr;

	if(modulator == NULL)
		return (ModulatorError_NULL_HANDLE_PTR);

    instance = LoadLibrary ("ITEAPIEX.dll");
    Usb2_initialize = (bool (__cdecl *) (
					int Tx_DevNo
            )) GetProcAddress (instance, "ITE_TxDeviceInit");
    Usb2_finalize = (void (__cdecl *) (
					int Tx_DevNo
            )) GetProcAddress (instance, "ITE_TxDeviceExit");
    Usb2_writeControl = (bool (__cdecl *) (
                    BYTE*       poutBuf,
                    ULONG       WriteLen,
                    ULONG*      pnBytesWrite,
					int			Tx_DevNo
            )) GetProcAddress (instance, "ITE_TxWriteBulkData");
    Usb2_readControl = (bool (__cdecl *) (
                    BYTE*       pinBuf,
                    ULONG       ReadLen,
                    ULONG*      pnBytesRead,
					int			Tx_DevNo
            )) GetProcAddress (instance, "ITE_TxReadBulkData");
    Usb2_readData = (bool (__cdecl *) (
                    BYTE*       pinBuf,
                    ULONG       ReadLen
            )) GetProcAddress (instance, "af35_GetTsData");
	hr = CoInitialize(NULL);
    if (FAILED (hr)){
		//printf("CoInitializeEx error!!!");
        return 1;
    }

    if (!Usb2_initialize (0))
        error = ModulatorError_DRIVER_INVALID;
    else
        *handle = (Handle) instance;
#endif
    return (error);
}


Dword Usb2_exitDriver (
    IN  Modulator*    modulator
) {
    Dword error = ModulatorError_NO_ERROR;
#ifdef WIN32
	if(modulator == NULL)
		return (ModulatorError_NULL_HANDLE_PTR);
    if (Usb2_finalize != NULL)
        Usb2_finalize (0);

	if(bCoInitial)
		CoUninitialize();

	bCoInitial = False; //flag
    
    Usb2_handle = NULL;
#endif
    return (error);
}


Dword Usb2_writeControlBus (
    IN  Modulator*    modulator,
    IN  Dword           bufferLength,
    IN  Byte*           buffer
) {
    Dword error = ModulatorError_NO_ERROR;
#ifdef WIN32
    ULONG writeLength;

    if (Usb2_handle == NULL) {
        error = Usb2_getDriver (modulator, &Usb2_handle);
        if (error) goto exit;
    }
	
    if (!Usb2_writeControl (buffer, bufferLength, &writeLength, 0))
        error = ModulatorError_INTERFACE_FAIL;

exit :
#endif
    return (error);
}


Dword Usb2_readControlBus (
    IN  Modulator*    modulator,
    IN  Dword           bufferLength,
    OUT Byte*           buffer
) {
    Dword error = ModulatorError_NO_ERROR;
#ifdef WIN32
    ULONG readLength;

    if (Usb2_handle == NULL) {
        error = Usb2_getDriver (modulator, &Usb2_handle);
        if (error) goto exit;
    }
    if (!Usb2_readControl (buffer, bufferLength, &readLength, 0))
        error = ModulatorError_INTERFACE_FAIL;

exit :
#endif
    return (error);
}


Dword Usb2_readDataBus (
    IN  Modulator*    modulator,
    IN  Dword           bufferLength,
    OUT Byte*           buffer
) {
    Dword error = ModulatorError_NO_ERROR;
#ifdef WIN32

    if (Usb2_handle == NULL) {
        error = Usb2_getDriver (modulator, &Usb2_handle);
        if (error) goto exit;
    }
    if (!Usb2_readData (buffer, bufferLength)) {
        EagleUser_delay (modulator, 1000);
        if (!Usb2_readData (buffer, bufferLength))
            error = ModulatorError_INTERFACE_FAIL;
    }

exit :
#endif
    return (error);
}

#endif
