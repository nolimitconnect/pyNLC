#pragma once
//============================================================================
// Copyright (C) 2026 Brett R. Jones 
//
// Code copyrighted by Brett R. Jones is under dual license similar to Ruby's license 
// See file COPYING and LEGAL in root of the No Limit Connect project
//
// bjones.engineer@gmail.com
// https://nolimitconnect.com
//============================================================================

#include "GuiInterface/IAppImplementation.h"

class AppImplementation: public IAppImplementation
{
    public:

    IToGui&                     getIToGui( void ) override;
    IAudioRequests&             getIAudioRequests( void ) override;
    INlcRender&                 getINlcRender( void ) override;
};

IAppImplementation& GetAppImplementation();