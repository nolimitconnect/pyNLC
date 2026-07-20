//============================================================================
// Copyright (C) 2026 Brett R. Jones 
//
// Code copyrighted by Brett R. Jones is under dual license similar to Ruby's license 
// See file COPYING and LEGAL in root of the No Limit Connect project
//
// bjones.engineer@gmail.com
// https://nolimitconnect.com
//============================================================================

#include "GlobalDb.h"

#include "VxParse.h"

namespace
{
	#define GLOBAL_DB_DBVERSION		1
}

//============================================================================
GlobalDb& GlobalDb::getInstance( void )
{ 
    static GlobalDb instance; 
    return instance; 
};

//============================================================================
GlobalDb::GlobalDb()
: VxSettings("GlobalDb.db3")
{
}

//============================================================================
uint32_t GlobalDb::initGlobalDb( std::string dbPath )
{
    m_DbFilePathAndName = dbPath;
    m_DbFilePathAndName += "GlobalDb.db3";
	uint32_t rc = dbStartup( GLOBAL_DB_DBVERSION, m_DbFilePathAndName.c_str() );
	if( 0 == rc )
	{
		m_GlobalDbInitialized = true;
	}

	return rc;
}

//============================================================================
void GlobalDb::setCamRotation( std::string camId, uint32_t camRotation )
{
	std::string camKey;
	StdStringFormat( camKey, "CamRotation%s", camId.c_str() );
	setIniValue( camKey.c_str(), camRotation );
}

//============================================================================
uint32_t GlobalDb::getCamRotation( std::string camId )
{
	uint32_t camRotation = 0;
	std::string camKey;
	StdStringFormat( camKey, "CamRotation%s", camId.c_str() );
	getIniValue( camKey.c_str(), camRotation, 0 );
	return camRotation;
}

//============================================================================
void GlobalDb::setCamEnable( bool camEnableIn )
{
	uint32_t camEnable = camEnableIn ? 1 : 0;
	setIniValue( "CamEnable", camEnable );
}

//============================================================================
bool GlobalDb::getCamEnable( void )
{
	uint32_t camEnable = 0;
	getIniValue( "CamEnable", camEnable, 1 );
	return camEnable ? true : false;
}

//============================================================================
void GlobalDb::setCamSourceId( std::string camId )
{
	setIniValue( "CamSourceId", camId );
}

//============================================================================
std::string GlobalDb::getCamSourceId( void )
{
	std::string camSourceId;
	getIniValue( "CamSourceId", camSourceId, "" );
	return camSourceId;
}

//============================================================================
void GlobalDb::setVidFeedRotation( uint32_t feedRotation )
{
	setIniValue( "VidFeedRotation", feedRotation );
}

//============================================================================
uint32_t GlobalDb::getVidFeedRotation( void )
{
	uint32_t feedRotation = 0;
	getIniValue( "VidFeedRotation", feedRotation, 0 );
	return feedRotation;
}
