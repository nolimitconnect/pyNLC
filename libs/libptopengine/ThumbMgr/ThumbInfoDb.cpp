//============================================================================
// Copyright (C) 2015 Brett R. Jones
//
// Code copyrighted by Brett R. Jones is under dual license similar to Ruby's license
// See file COPYING and LEGAL in root of the No Limit Connect project
//
// bjones.engineer@gmail.com
// https://nolimitconnect.com
//============================================================================

#include "ThumbInfoDb.h"
#include "ThumbMgr.h"
#include "ThumbInfo.h"

//============================================================================
ThumbInfoDb::ThumbInfoDb( AssetBaseMgr& hostListMgr, const char* dbname )
: AssetBaseInfoDb( hostListMgr, dbname )
{
}

//============================================================================
AssetBaseInfo* ThumbInfoDb::createAssetInfo( EAssetType assetType, const char* assetName, const char* fileNameAndPath, uint64_t assetLen )
{
    return new ThumbInfo( assetName, fileNameAndPath, assetLen );
}

//============================================================================
AssetBaseInfo* ThumbInfoDb::createAssetInfo( EAssetType assetType, const char* assetName, const char* fileNameAndPath, uint64_t assetLen, VxGUID& assetId )
{
    return new ThumbInfo( assetName, fileNameAndPath, assetLen, assetId );
}

//============================================================================
AssetBaseInfo* ThumbInfoDb::createAssetInfo( AssetBaseInfo& assetInfo )
{
    return new ThumbInfo( assetInfo );
}

//============================================================================
bool ThumbInfoDb::saveToDatabase( ThumbInfo& thumbInfo )
{
    return AssetBaseInfoDb::addAsset( &thumbInfo );
}