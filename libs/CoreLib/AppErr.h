#pragma once
//============================================================================
// Copyright (C) 2014 Brett R. Jones 
//
// Code copyrighted by Brett R. Jones is under dual license similar to Ruby's license 
// See file COPYING and LEGAL in root of the No Limit Connect project
//
// bjones.engineer@gmail.com
// https://nolimitconnect.com
//============================================================================
#include <CoreLib/config_corelib.h>

enum EAppErr
{
	eAppErrUnknown							= -1,
	eAppErrNone								= 0,

	eAppErrFailedToConnect					= 1000000,
	eAppErrTxError							= 1000001,
	eAppErrRxError							= 1000002,
	eAppErrParseError						= 1000003,
	eAppErrPortIsClosed						= 1000004,
	eAppErrFailedToResolveAddr				= 1000005,
	eAppErrThreadCreateFailed				= 1000006,
	eAppErrFailedConnectNetServices			= 1000007,
	eAppErrNetServicesFailedToRespond		= 1000008,
	eAppErrFailedConnectHost				= 1000009,
	eAppErrAccessDenied						= 1000010,
	eAppErrBadParameter						= 1000011,
	eAppErrNoFileExist						= 1000012,
	eAppErrFileRead							= 1000013,
	eAppErrFileWrite						= 1000014,
	eAppErrFileSeek							= 1000015,
	eAppErrBusy								= 1000016,
    eAppErrSeviceError                      = 1000017,

	eAppPopupErrNetworkHostConnectFail		= 1100100,
	eAppPopupErrConnectTestHostConnectFail  = 1100101,

	eAppPopupErrVideoPlayFail  				= 1100102,
	eAppPopupErrAudioPlayFail  				= 1100103,

    eAppPopupErrNoMicDevices  				= 1100104,
	eAppPopupErrMicDeviceOutOfRange  		= 1100105,
    eAppPopupErrMicDeviceFailedToInitialize  		= 1100106,

    eAppPopupErrNoSpeakerDevices  			= 1100107,
	eAppPopupErrSpeakerDeviceOutOfRange  	= 1100108,
    eAppPopupErrSpeakerDeviceFailedToInitialize  	= 1100109,
    eAppPopupErrSpeakerDeviceInvaidFormat  	= 1100110,

    eAppPopupErrEchoDelayTestFail  	        = 1100111,
    eAppPopupErrEchoDelayTestPass  	        = 1100112,

	eMaxAppErr
};

typedef void (*APP_ERR_FUNCTION )( void *, EAppErr, char * );

void VxSetAppErrHandler( APP_ERR_FUNCTION pfuncAppErrHandler, void * userData );

void AppErr( EAppErr eAppErr, const char* errMsg, ...);

