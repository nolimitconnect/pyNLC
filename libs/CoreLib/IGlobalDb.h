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

#include <string>
#include <cstdint>

class IGlobalDb
{
public:
    static IGlobalDb&			getIGlobalDb( void );

    virtual uint32_t            initGlobalDb( std::string dbPath ) = 0;

	virtual void			    setCamEnable( bool camEnable ) = 0;
	virtual bool				getCamEnable( void ) = 0;

	virtual void				setCamSourceId( std::string camId ) = 0;
	virtual std::string			getCamSourceId( void ) = 0;

	virtual void				setCamRotation( std::string camId, uint32_t camRotation ) = 0;
	virtual uint32_t			getCamRotation( std::string camId ) = 0;

	virtual void				setVidFeedRotation( uint32_t feedRotation ) = 0;
	virtual uint32_t			getVidFeedRotation( void ) = 0;

};
