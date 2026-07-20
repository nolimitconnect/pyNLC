//============================================================================
// Copyright (C) 2015 Brett R. Jones
//
// Code copyrighted by Brett R. Jones is under dual license similar to Ruby's license
// See file COPYING and LEGAL in root of the No Limit Connect project
//
// bjones.engineer@gmail.com
// https://nolimitconnect.com
//============================================================================

#include <config_appcorelibs.h>

#include "AssetInfoDb.h"
#include "AssetMgr.h"
#include "AssetInfo.h"

//============================================================================
AssetInfoDb::AssetInfoDb( AssetBaseMgr& assetInfoMgr, const char* dbName )
: AssetBaseInfoDb( assetInfoMgr, dbName )
{
}

//============================================================================
AssetBaseInfo* AssetInfoDb::createAssetInfo( EAssetType assetType, const char* assetName, const char* fileNameAndPath, uint64_t assetLen )
{
    return new AssetInfo( assetType, assetName, fileNameAndPath, assetLen );
}

//============================================================================
AssetBaseInfo* AssetInfoDb::createAssetInfo( EAssetType assetType, const char* assetName, const char* fileNameAndPath, uint64_t assetLen, VxGUID& assetId )
{
    return new AssetInfo( assetType, assetName, fileNameAndPath, assetLen, assetId );
}

//============================================================================
AssetBaseInfo* AssetInfoDb::createAssetInfo( AssetBaseInfo& assetInfo )
{
    return new AssetInfo( assetInfo );
}