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

#include "IGlobalDb.h"
#include "VxSettings.h"

class GlobalDb : public IGlobalDb, public VxSettings
{
public:
    GlobalDb();
    ~GlobalDb() = default;

    static GlobalDb&			getInstance( void );

    uint32_t                    initGlobalDb( std::string dbPath ) override;

	void						setCamEnable( bool camEnable ) override;
	bool						getCamEnable( void ) override;

	void						setCamSourceId( std::string camId ) override;
	std::string					getCamSourceId( void ) override;

	void						setCamRotation( std::string camId, uint32_t camRotation ) override;
	uint32_t					getCamRotation( std::string camId ) override;

	void						setVidFeedRotation( uint32_t feedRotation ) override;
	uint32_t					getVidFeedRotation( void ) override;

protected:
	bool						m_GlobalDbInitialized = false;
    std::string                 m_DbFilePathAndName;
};