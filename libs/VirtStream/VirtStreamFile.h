//============================================================================
// Copyright (C) 2024 Brett R. Jones
//
// Code copyrighted by Brett R. Jones is under dual license similar to Ruby's license
// See file COPYING and LEGAL in root of the No Limit Connect project
//
// bjones.engineer@gmail.com
// https://nolimitconnect.com
//============================================================================
#pragma once

#include "VirtStreamCache.h"

#include <CoreLib/VFile.h>
#include <NetLib/VxSktBase.h>

#include <AssetBase/AssetBaseInfo.h>

#include <errno.h>

class VirtStreamFile
{
public:
	void						clear( void );

	bool						setConnection( std::shared_ptr<VxSktBase> sktBase );
	std::shared_ptr<VxSktBase>&	getConnection( void )		{ return m_SktBase; }

	void						removeConnection( void );
	bool						isConnected( void );

	void						setError( int err )			{ m_Error = err; }
	int							getError( void )			{ return m_Error; }

	//=== vars ===//
	VFile*						m_VFile{ nullptr };
	VirtStreamCache				m_StreamCache;
	std::string					m_FileName;
	std::string					m_FileMode;
	AssetBaseInfo				m_StreamAssetInfo;
	VxGUID						m_StreamSessionId;
	VxGUID						m_ServerSessionId;
	std::shared_ptr<VxSktBase>  m_SktBase;
	int							m_Error{ 0 };
	VirtCache					m_FileTail;
};
