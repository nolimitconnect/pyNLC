//============================================================================
// Copyright (C) 2015 Brett R. Jones
//
// Code copyrighted by Brett R. Jones is under dual license similar to Ruby's license
// See file COPYING and LEGAL in root of the No Limit Connect project
//
// bjones.engineer@gmail.com
// https://nolimitconnect.com
//============================================================================

#include "BlobInfo.h"
#include "BlobInfoDb.h"
#include "BlobMgr.h"

//============================================================================
BlobInfoDb::BlobInfoDb( AssetBaseMgr& hostListMgr, const char* dbName )
: AssetBaseInfoDb( hostListMgr, dbName )
{
}

//============================================================================
AssetBaseInfo* BlobInfoDb::createAssetInfo( EAssetType assetType, const char* assetName, const char* fileNameAndPath, uint64_t assetLen )
{
    return new BlobInfo( assetType, assetName, fileNameAndPath, assetLen );
}

//============================================================================
AssetBaseInfo* BlobInfoDb::createAssetInfo( EAssetType assetType, const char* assetName, const char* fileNameAndPath, uint64_t assetLen, VxGUID& assetId )
{
    return new BlobInfo( assetType, assetName, fileNameAndPath, assetLen, assetId );
}

//============================================================================
AssetBaseInfo* BlobInfoDb::createAssetInfo( AssetBaseInfo& assetInfo )
{
    return new BlobInfo( assetInfo );
}