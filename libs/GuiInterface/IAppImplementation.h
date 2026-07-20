#pragma once
//============================================================================
// Copyright (C) 2019 Brett R. Jones
//
// Code copyrighted by Brett R. Jones is under dual license similar to Ruby's license
// See file COPYING and LEGAL in root of the No Limit Connect project
//
// bjones.engineer@gmail.com
// https://nolimitconnect.com
//============================================================================

class INlcRender;
class IToGui;
class IAudioRequests;

class IAppImplementation
{
public:
    virtual IToGui&             getIToGui( void ) = 0;
    virtual IAudioRequests&     getIAudioRequests( void ) = 0;
    virtual INlcRender&         getINlcRender( void ) = 0;
};

extern IAppImplementation& GetAppImplementation();