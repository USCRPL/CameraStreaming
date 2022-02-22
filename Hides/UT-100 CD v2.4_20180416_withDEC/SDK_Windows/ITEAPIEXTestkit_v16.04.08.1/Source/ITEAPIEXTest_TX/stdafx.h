// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers

#include <stdio.h>



// TODO: reference additional headers your program requires here
#define   _WIN32_WINNT   0x0500 //for SetWaitableTimer 
#include <windows.h>

#include <streams.h>
#include <mmreg.h>
#include <msacm.h>
#include <fcntl.h>
#include <io.h>
#include <stdio.h> 
#include <ks.h>
#include <ksmedia.h>
#include <bdatypes.h>
#include <bdamedia.h>
#include <bdaiface.h>
#include <uuids.h>
#include <tuner.h>
#include <commctrl.h>
#include <atlbase.h>
#include <ks.h>
#include <ksproxy.h>
#include <signal.h>