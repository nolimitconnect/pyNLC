//============================================================================
// Copyright (C) 2024 Brett R. Jones
//
// Code copyrighted by Brett R. Jones is under dual license similar to Ruby's license
// See file COPYING and LEGAL in root of the No Limit Connect project
//
// bjones.engineer@gmail.com
// https://nolimitconnect.com
//============================================================================

#include "VirtStreamCache.h"

#include <CoreLib/VxDebug.h>

#include <memory.h>

//============================================================================
int64_t VirtCache::hasData( int64_t offset, int64_t len ) // return true if has any of the data
{
	int64_t lenInCache{ 0 };
	int64_t endPoint = offset + len;
	if( m_CacheStartOffs <= offset && m_CacheEndOffs > offset )
	{
		lenInCache = std::min( len, m_CacheEndOffs - offset );
	}
	else if( m_CacheStartOffs <= endPoint && m_CacheEndOffs > endPoint )
	{
		lenInCache = std::min( len, endPoint - m_CacheStartOffs );
	}

	//if( lenInCache && lenInCache != len )
	//{
	//	LogMsg( LOG_VERBOSE, "VirtCache::%s has 0x%" PRIx64 " of len 0x%" PRIx64 " at offs 0x%" PRIx64 "", __func__, lenInCache, len, offset);
	//}

	return lenInCache;
}

//============================================================================
int64_t VirtCache::writeData( int64_t offset, char* data, int64_t dataLen )
{
	m_CacheStartOffs = offset;
	memcpy( m_Buf, data, dataLen );
	m_CacheEndOffs = m_CacheStartOffs + dataLen;
	return dataLen;
}

//============================================================================
int64_t VirtCache::readData( int64_t offset, char* data, int64_t len )
{
	int64_t lenInCache{ 0 };
	int64_t endPoint = offset + len;
	if( m_CacheStartOffs <= offset && m_CacheEndOffs > offset )
	{
		lenInCache = std::min( len, m_CacheEndOffs - offset );
		int64_t buffOffs = offset - m_CacheStartOffs;
		memcpy( data, &m_Buf[buffOffs], lenInCache );
	}
	else if( m_CacheStartOffs <= endPoint && m_CacheEndOffs > endPoint )
	{
		lenInCache = std::min( len, endPoint - m_CacheStartOffs );
		int64_t buffOffs = endPoint - m_CacheStartOffs;
		memcpy( data, &m_Buf[buffOffs], lenInCache );
	}

	//if( lenInCache != len )
	//{
	//	LogMsg( LOG_VERBOSE, "VirtCache::%s read 0x%" PRIx64 " of len 0x%" PRIx64 " at offs 0x%" PRIx64 "", __func__, lenInCache, len, offset);
	//}

	return lenInCache;
}

//============================================================================
//============================================================================

//============================================================================
VirtStreamCache::~VirtStreamCache()
{
	clearCache( true );
}

//============================================================================
void VirtStreamCache::clearCache( bool isLocked )
{
	if( !isLocked )
	{
		lockCache();
	}
;
	for( auto iter = m_VirtCache.begin(); iter != m_VirtCache.end(); ++iter )
	{
		delete* iter;
	}

	m_VirtCache.clear();
	m_CacheStartOffs = 0;
	m_CacheEndOffs = 0;
	m_LastReadOffs = 0;

	if( !isLocked )
	{
		unlockCache();
	}
}

//============================================================================
int64_t VirtStreamCache::hasData( int64_t offset, int64_t len )
{
	int64_t lenInCache{ 0 };
	int64_t endPoint = offset + len;
	lockCache();
	if( m_CacheStartOffs <= offset && m_CacheEndOffs > offset )
	{
		lenInCache = std::min( len, m_CacheEndOffs - offset );
	}
	else if( m_CacheStartOffs <= endPoint && m_CacheEndOffs > endPoint )
	{
		lenInCache = std::min( len, endPoint - m_CacheStartOffs );
	}

	unlockCache();

	//if( lenInCache && lenInCache != len )
	//{
	//	LogMsg( LOG_VERBOSE, "VirtStreamCache::%s has 0x%" PRIx64 " of len 0x%" PRIx64 " at offset 0x%" PRIx64 "", __func__, lenInCache, len, offset);
	//}

	return lenInCache;
}

//============================================================================
int64_t VirtStreamCache::writeData( int64_t offset, char* data, int64_t len )
{
	if( hasData( offset, len ) )
	{
		LogModule( eLogStreams, LOG_ERROR, "VirtStreamCache::%s data already exist", __func__ );
		vx_assert( false );
		return false;
	}

	int64_t lenWritten{ 0 };
	lockCache();
	if( offset != 0 && offset != m_CacheEndOffs )
	{
		LogModule( eLogStreams, LOG_WARN, "VirtStreamCache::%s data is not consistent.. flushing cache", __func__ );
		clearCache( true );
		m_CacheStartOffs = offset;
	}
	else
	{
		cleanUpCacheToSizeLimit( true );
	}

	while( lenWritten < len )
	{
		VirtCache* cache = new VirtCache();
		int64_t lenThisCache = cache->writeData( offset + lenWritten, &data[lenWritten], len );
		lenWritten += lenThisCache;
		m_VirtCache.emplace_back( cache );
		m_CacheEndOffs += lenThisCache;
	}

	unlockCache();
	return lenWritten;
}

//============================================================================
int64_t VirtStreamCache::readData( int64_t offset, char* data, int64_t len )
{
	int64_t lenRead = 0;
	bool foundData{ false };

	int64_t lenInBuf = hasData( offset, len );
	//if( len != lenInBuf )
	//{
	//	LogMsg( LOG_VERBOSE, "VirtStreamCache::%s has 0x%" PRIx64 " of len 0x%" PRIx64 " at offset 0x%" PRIx64, __func__, lenInBuf, len, offset );
	//}

	lockCache();
	int startIdx = (offset - m_CacheStartOffs) / PKT_TYPE_FILE_MAX_DATA_LEN;
	auto iter = m_VirtCache.begin();
	std::advance( iter, startIdx );
	if( iter == m_VirtCache.end() )
	{
		unlockCache();
		return 0;
	}

	for( ; iter != m_VirtCache.end(); ++iter )
	{
		VirtCache* virtCache = *iter;
		int64_t maxRead = len - lenRead;
		if( virtCache->hasData( offset + lenRead, len - lenRead ) )
		{
			foundData = true;
			int64_t dataReadThisCache = virtCache->readData( offset + lenRead, &data[lenRead], maxRead );
			if( !dataReadThisCache )
			{
				LogMsg( LOG_ERROR, "VirtStreamCache::%s failed to read from cache that has data", __func__ );
				vx_assert( false );
				unlockCache();
				return 0;
			}
			else
			{
				lenRead += dataReadThisCache;
			}
		}
		else if( foundData )
		{
			LogModule( eLogStreams, LOG_WARN, "VirtStreamCache::%s only 0x%" PRIx64 " of data in the cache", __func__, lenRead );
			break;
		}

		if( lenRead > len )
		{
			LogMsg( LOG_ERROR, "VirtStreamCache::%s read more data than specified", __func__ );
			vx_assert( false );
			unlockCache();
			return 0;
		}

		if( lenRead == len )
		{
			break;
		}
	}

	if( lenRead )
	{
		m_LastReadOffs = offset;
	}

	unlockCache();
	//if( len != lenInBuf )
	//{
	//	LogMsg( LOG_VERBOSE, "VirtStreamCache::%s read 0x%" PRIx64 " of len 0x%" PRIx64, __func__, lenRead, len );
	//}

	return lenRead;
}

//============================================================================
void VirtStreamCache::cleanUpCacheToSizeLimit( bool cacheIsLocked )
{
	const int64_t maxCacheLen = 100000000;
	if( !cacheIsLocked )
	{
		lockCache();
	}

	if( getCachedSize() > maxCacheLen && m_LastReadOffs > m_CacheStartOffs + PKT_TYPE_FILE_MAX_DATA_LEN )
	{
		for( auto iter = m_VirtCache.begin(); iter != m_VirtCache.end(); )
		{
			VirtCache* virtCache = *iter;
			if( virtCache->m_CacheStartOffs + PKT_TYPE_FILE_MAX_DATA_LEN >= m_LastReadOffs )
			{
				break;
			}

			if( getCachedSize() <= maxCacheLen )
			{
				break;
			}

			m_CacheStartOffs += virtCache->getCachedSize();
			delete virtCache;
			iter = m_VirtCache.erase( iter );
		}
	}

	if( !cacheIsLocked )
	{
		unlockCache();
	}
}
