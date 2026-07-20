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

#include <list>

#include <CoreLib/VxMutex.h>

#include <PktLib/PktsFileShare.h>

class VirtCache
{
public:
	void						clear( void ) { m_CacheStartOffs = 0; m_CacheEndOffs = 0; }

	int64_t						getCachedSize( void ) { return m_CacheEndOffs - m_CacheStartOffs; }

	int64_t						hasData( int64_t offset, int64_t len ); // return length of the data
	int64_t						readData( int64_t offset, char* data, int64_t len );
	int64_t						writeData( int64_t offset, char* data, int64_t len );

	char						m_Buf[ PKT_TYPE_FILE_MAX_DATA_LEN ]; 
	int64_t						m_CacheStartOffs{ 0 };
	int64_t						m_CacheEndOffs{ 0 };
};

class VirtStreamCache
{
public:
	VirtStreamCache() = default;
	virtual ~VirtStreamCache();

	void						clearCache( bool isLocked );
	int64_t						getCachedSize( void ) { return m_CacheEndOffs - m_CacheStartOffs; }

	int64_t						hasData( int64_t offset, int64_t len );
	int64_t						writeData( int64_t offset, char* data, int64_t len );
	int64_t						readData( int64_t offset, char* data, int64_t len );

	void						lockCache( void ) { m_CacheMutex.lock(); }
	void						unlockCache( void ) { m_CacheMutex.unlock(); }

	void						cleanUpCacheToSizeLimit( bool cacheIsLocked );

	//=== vars ===//
	std::list<VirtCache*>		m_VirtCache;

	int64_t						m_CacheStartOffs{ 0 };
	int64_t						m_CacheEndOffs{ 0 };
	int64_t						m_LastReadOffs{ 0 };

	VxMutex						m_CacheMutex;
};
