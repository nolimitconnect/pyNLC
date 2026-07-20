#pragma once
//============================================================================
// Copyright (C) 2015 Brett R. Jones
//
// Code copyrighted by Brett R. Jones is under dual license similar to Ruby's license
// See file COPYING and LEGAL in root of the No Limit Connect project
//
// bjones.engineer@gmail.com
// https://nolimitconnect.com
//============================================================================

#include <AssetBase/AssetBaseInfoDb.h>

class BlobInfo;
class BlobMgr;

class BlobInfoDb : public AssetBaseInfoDb
{
public:
	BlobInfoDb( AssetBaseMgr& mgr, const char* dbName );
	virtual ~BlobInfoDb() = default;

protected:
    AssetBaseInfo*     createAssetInfo( enum EAssetType assetType, const char* fileName, const char* fileNameAndPath, uint64_t fileLen ) override;
    AssetBaseInfo*     createAssetInfo( enum EAssetType assetType, const char* fileName, const char* fileNameAndPath, uint64_t fileLen, VxGUID& assetId ) override;
    AssetBaseInfo*     createAssetInfo( AssetBaseInfo& assetInfo ) override;

};


