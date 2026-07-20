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

class ThumbMgr;
class ThumbInfo;

class ThumbInfoDb : public AssetBaseInfoDb
{
public:
	ThumbInfoDb( AssetBaseMgr& mgr, const char* dbname );
	virtual ~ThumbInfoDb() = default;

    bool                        saveToDatabase( ThumbInfo& thumbInfo );

protected:
    AssetBaseInfo*     createAssetInfo( EAssetType assetType, const char* fileName, const char* fileNameAndPath, uint64_t fileLen ) override;
    AssetBaseInfo*     createAssetInfo( EAssetType assetType, const char* fileName, const char* fileNameAndPath, uint64_t fileLen, VxGUID& assetId ) override;
    AssetBaseInfo*     createAssetInfo( AssetBaseInfo& assetInfo ) override;
};

