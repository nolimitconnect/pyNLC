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

#include <config_appcorelibs.h>
#include <GuiInterface/IDefs.h>

#include <CoreLib/DbBase.h>
#include <CoreLib/VxGUID.h>
#include <CoreLib/AssetDefs.h>

class AssetBaseInfo;
class AssetBaseMgr;
class VxGUID;
class VxSha1Hash;

class AssetBaseInfoDb : public DbBase
{
public:
	AssetBaseInfoDb( AssetBaseMgr& mgr, const char*dbName );
	virtual ~AssetBaseInfoDb() = default;

	void						lockAssetInfoDb( void )					{ m_AssetBaseInfoDbMutex.lock(); }
	void						unlockAssetInfoDb( void )				{ m_AssetBaseInfoDbMutex.unlock(); }

    bool						addAsset(	VxGUID&			assetId, 
											VxGUID&			creatorId, 
											VxGUID&			historyId, 
											VxGUID&			adminId, 
											VxGUID&			sendToId,
                                            VxGUID&			thumbId, 
											const char*		assetName, 
										    const char*		fileNameAndPath,
											int64_t			assetLen, 
											uint32_t		assetType, 							
											VxSha1Hash&		hashId, 
											uint32_t		locationFlags, 
                                            uint32_t		attributedFlags, 
                                            EPluginType     pluginType,
                                            int64_t			createdTimestamp = 0,
                                            int64_t			modifiedTimestamp = 0,     
                                            int64_t			accessedTimestamp = 0,          
											const char*	assetTag = "", 
											EAssetSendState sendState = eAssetSendStateNone);

	bool 						addAsset( AssetBaseInfo* assetInfo );

	void						removeAsset( const char* fileNameAndPath );
	void						removeAsset( VxGUID& assetId );
	void						removeAsset( AssetBaseInfo* assetInfo );

	void						getAllAssets( std::vector<AssetBaseInfo*>& AssetBaseAssetList );
	void						purgeAllAssets( void ); 
	void						updateAssetSendState( VxGUID& assetId, EAssetSendState sendState );

protected:
    virtual AssetBaseInfo*     createAssetInfo( EAssetType assetType, const char* assetName, const char* fileNameAndPath, uint64_t assetLen ) = 0;
    virtual AssetBaseInfo*     createAssetInfo( EAssetType assetType, const char* assetName, const char* fileNameAndPath, uint64_t assetLen, VxGUID& assetGUID ) = 0;
    virtual AssetBaseInfo*     createAssetInfo( AssetBaseInfo& assetInfo ) = 0;

	virtual int32_t				onCreateTables( int iDbVersion );
	virtual int32_t				onDeleteTables( int iOldVersion );
	void						insertAssetInTimeOrder( AssetBaseInfo*assetInfo, std::vector<AssetBaseInfo*>& assetList );

	AssetBaseMgr&				m_AssetMgr;
	VxMutex						m_AssetBaseInfoDbMutex;
};

