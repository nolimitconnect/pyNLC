//============================================================================
// Copyright (C) 2026 Brett R. Jones 
//
// Code copyrighted by Brett R. Jones is under dual license similar to Ruby's license 
// See file COPYING and LEGAL in root of the No Limit Connect project
//
// bjones.engineer@gmail.com
// https://nolimitconnect.com
//============================================================================

#include "AppImplementation.h"
#include "AppCommon.h"

//============================================================================
IAppImplementation& GetAppImplementation()
{
    static AppImplementation appImpl;
    return appImpl;
}

//============================================================================
IToGui& AppImplementation::getIToGui( void )
{
    return GetAppInstance().getIToGui();
}

//============================================================================
IAudioRequests& AppImplementation::getIAudioRequests( void )
{
    return GetAppInstance().getIAudioRequests();
}

//============================================================================
INlcRender& AppImplementation::getINlcRender( void )
{
    return GetAppInstance().getINlcRender();
}