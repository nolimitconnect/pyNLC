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

#include "AssetBaseInfoDb.h"
#include "AssetBaseXferMgr.h"
#include "AssetBaseInfo.h"

#include <CoreLib/VxDebug.h>
#include <CoreLib/VxFileUtil.h>

namespace
{
	std::string 		TABLE_ASSETS	 				= "tblAssets";

	std::string 		CREATE_COLUMNS_ASSETS			= " (unique_id TEXT PRIMARY KEY, creatorId TEXT, historyId TEXT, adminId TEXT, thumbId TEXT, assetName TEXT, fileNameAndPath TEXT, length BIGINT, type INTEGER, hashId BLOB, locFlags INTEGER, attribFlags INTEGER, creationTime BIGINT, modifiedTime BIGINT, accessedTime BIGINT, assetTag TEXT, sendState INTEGER, pluginType INTEGER, sendToId TEXT) ";

	const int			COLUMN_ASSET_UNIQUE_ID			= 0;
	const int			COLUMN_ASSET_CREATOR_ID			= 1;
	const int			COLUMN_ASSET_HISTORY_ID			= 2;
	const int			COLUMN_ASSET_ADMIN_ID			= 3;
    const int			COLUMN_ASSET_THUMB_ID			= 4;
	const int			COLUMN_ASSET_NAME				= 5;
	const int			COLUMN_FILE_NAME_AND_PATH		= 6;
	const int			COLUMN_ASSET_LEN				= 7;
	const int			COLUMN_ASSET_TYPE				= 8;
	const int			COLUMN_ASSET_HASH_ID			= 9;
	const int			COLUMN_LOCATION_FLAGS			= 10;
    const int			COLUMN_ATTRIBUTE_FLAGS			= 11;
    const int			COLUMN_CREATION_TIME			= 12;
    const int			COLUMN_MODIFIED_TIME			= 13;
    const int			COLUMN_ACCESSED_TIME			= 14;
	const int			COLUMN_ASSET_TAG				= 15;
	const int			COLUMN_ASSET_SEND_STATE			= 16;
	const int			COLUMN_PLUGIN_TYPE				= 17;
	const int			COLUMN_SEND_TO_ID				= 18;
}

//============================================================================
AssetBaseInfoDb::AssetBaseInfoDb( AssetBaseMgr& hostListMgr, const char*dbName  )
: DbBase( dbName )
, m_AssetMgr( hostListMgr )
{
}

//============================================================================
//! create tables in database 
int32_t AssetBaseInfoDb::onCreateTables( int iDbVersion )
{
	lockAssetInfoDb();
	std::string strCmd = "CREATE TABLE " + TABLE_ASSETS + CREATE_COLUMNS_ASSETS;
	int32_t rc = sqlExec(strCmd);
	unlockAssetInfoDb();
	return rc;
}

//============================================================================
// delete tables in database
int32_t AssetBaseInfoDb::onDeleteTables( int iOldVersion ) 
{
	lockAssetInfoDb();
	std::string strCmd = "DROP TABLE IF EXISTS " + TABLE_ASSETS;
	int32_t rc = sqlExec(strCmd);
	unlockAssetInfoDb();
	return rc;
}

//============================================================================
void AssetBaseInfoDb::purgeAllAssets( void ) 
{
	lockAssetInfoDb();
	std::string strCmd = "DELETE FROM " + TABLE_ASSETS;
	int32_t rc = sqlExec( strCmd );
	unlockAssetInfoDb();
	if( rc )
	{
		LogMsg( LOG_ERROR, "AssetBaseInfoDb::purgeAllAssets error %d", rc );
	}
	else
	{
		LogMsg( LOG_INFO, "AssetBaseInfoDb::purgeAllAssets success" );
	}
}

//============================================================================
void AssetBaseInfoDb::removeAsset( const char* fileNameAndPath )
{
	DbBindList bindList( fileNameAndPath );
	lockAssetInfoDb();
	sqlExec( "DELETE FROM tblAssets WHERE fileNameAndPath=?", bindList );
	unlockAssetInfoDb();
}

//============================================================================
void AssetBaseInfoDb::removeAsset( VxGUID& assetId )
{
	std::string assetStr = assetId.toHexString();
	DbBindList bindList( assetStr.c_str() );
	lockAssetInfoDb();
	sqlExec( "DELETE FROM tblAssets WHERE unique_id=?", bindList );
	unlockAssetInfoDb();
}

//============================================================================
void AssetBaseInfoDb::removeAsset( AssetBaseInfo* assetInfo )
{
	// the bind string is not copied so must be in memory until sql is executed
	//DbBindList bindList( assetInfo->getAssetUniqueId().toHexString().c_str() );
	std::string hexId = assetInfo->getAssetUniqueId().toHexString();
	DbBindList bindList( hexId.c_str() );
	lockAssetInfoDb();
	sqlExec( "DELETE FROM tblAssets WHERE unique_id=?", bindList );
	unlockAssetInfoDb();
}

//============================================================================
bool AssetBaseInfoDb::addAsset( VxGUID&			assetId,
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
                                uint32_t		attibuteFlags,
								EPluginType     pluginType,
                                int64_t			creationTimeStamp,
                                int64_t			modifiedTimeStamp,
                                int64_t			accessedTimeStamp,
                                const char*		assetTag,
                                EAssetSendState sendState)
{
    removeAsset( assetId );

	if( ePluginTypeInvalid == pluginType && eAssetTypeThumbnail == assetType )
	{
		pluginType = ePluginTypeThumbnail;
	}

    std::string assetIdStr = assetId.toHexString();
    std::string creatorIdStr = creatorId.toHexString();
    std::string historyIdStr = historyId.toHexString();
	std::string adminIdStr = adminId.toHexString();
    std::string thumbIdStr = thumbId.toHexString();
	std::string sendToIdStr = sendToId.toHexString();

    DbBindList bindList( assetIdStr.c_str() );
    bindList.add( creatorIdStr.c_str() );
    bindList.add( historyIdStr.c_str() );
	bindList.add( adminIdStr.c_str() );
    bindList.add( thumbIdStr.c_str() );
    bindList.add( assetName );
	bindList.add( fileNameAndPath );
    bindList.add( assetLen );
    bindList.add( (int)assetType );
    bindList.add( (void *)hashId.getHashData(), 20 );
    bindList.add( locationFlags );
    bindList.add( attibuteFlags );
    bindList.add( (uint64_t)creationTimeStamp );
    bindList.add( (uint64_t)modifiedTimeStamp );
    bindList.add( (uint64_t)accessedTimeStamp );
    bindList.add( assetTag );
    bindList.add( (int)sendState );
    bindList.add( (int)pluginType );
	bindList.add( sendToIdStr.c_str() );

	if( ePluginTypeInvalid == pluginType )
	{
		LogMsg( LOG_VERBOSE, "AssetBaseInfoDb::addAsset no plugin type" );
	}

	lockAssetInfoDb();
    int32_t rc = sqlExec( "INSERT INTO tblAssets (unique_id,creatorId,historyId,adminId,thumbId,assetName,fileNameAndPath,length,type,hashId,locFlags,attribFlags,creationTime,modifiedTime,accessedTime,assetTag,sendState,pluginType,sendToId) values(?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)",
        bindList );
	unlockAssetInfoDb();

    if( rc )
    {
        LogMsg( LOG_ERROR, "AssetBaseInfoDb::addAsset error %d", rc );
    }

    vx_assert( 0 == rc );
    return 0 == rc;
}

//============================================================================
void AssetBaseInfoDb::updateAssetSendState( VxGUID& assetId, EAssetSendState sendState )
{
	std::string assetIdStr		= assetId.toHexString();
	DbBindList bindList(  (int)sendState );
	bindList.add( assetIdStr.c_str() );

	lockAssetInfoDb();
	int32_t rc  = sqlExec( "UPDATE tblAssets SET sendState=? WHERE unique_id=?", bindList );
	unlockAssetInfoDb();
    vx_assert( 0 == rc );
	if( rc )
	{
		LogMsg( LOG_ERROR, "AssetBaseInfoDb::addAsset error %d", rc );
	}
}

//============================================================================
bool AssetBaseInfoDb::addAsset( AssetBaseInfo* assetInfo )
{
    if( assetInfo->isDeleted() )
    {
        LogMsg( LOG_WARN, "AssetBaseInfoDb::addAsset removing deleted asset %s", assetInfo->getAssetName().c_str() );
        removeAsset( assetInfo );
        return true;
    }

	if( !assetInfo->isValid() )
	{
		LogMsg( LOG_ERROR, "AssetBaseInfoDb::addAsset is INVALID %s", assetInfo->getAssetName().c_str() );
		return false;
	}

	return addAsset(	assetInfo->getAssetUniqueId(),
				        assetInfo->getCreatorId(),
				        assetInfo->getHistoryId(),
						assetInfo->getAdminId(),
						assetInfo->getSendToId(),
                        assetInfo->getThumbId(),
				        assetInfo->getAssetName().c_str(),
						assetInfo->getFileNameAndPath().c_str(),
				        assetInfo->getAssetLength(),
				        (uint32_t)assetInfo->getAssetType(),				
				        assetInfo->getAssetHashId(),
				        assetInfo->getLocationFlags(),
                        (uint32_t)assetInfo->getAttributeFlags(),
                        assetInfo->getPluginType(),
                        assetInfo->getCreationTime(),
                        assetInfo->getModifiedTime(),
                        assetInfo->getAccessedTime(),
				        assetInfo->getAssetTag().length() ? assetInfo->getAssetTag().c_str() : "",
				        assetInfo->getAssetSendState()
                        );
}

//============================================================================
void AssetBaseInfoDb::getAllAssets( std::vector<AssetBaseInfo*>& AssetAssetList )
{
	std::string assetName;
	std::string fileNameAndPath;
	EAssetType assetType;
	uint64_t assetLen;
	std::string destasset;
	std::string consoleId;

	std::vector<AssetBaseInfo*> toRemoveList;

	lockAssetInfoDb();
	DbCursor * cursor = startQuery( "SELECT * FROM tblAssets" ); // ORDER BY unique_id DESC  // BRJ don't know why ORDER BY quit working on android.. do in code
	if( cursor )
	{
		while( cursor->getNextRow() )
		{
			assetName = cursor->getString( COLUMN_ASSET_NAME );
			fileNameAndPath = cursor->getString( COLUMN_FILE_NAME_AND_PATH );
			assetLen =  (uint64_t)cursor->getS64( COLUMN_ASSET_LEN );
			assetType = (EAssetType)cursor->getS32( COLUMN_ASSET_TYPE );

            VxGUID assetGUID(cursor->getString( COLUMN_ASSET_UNIQUE_ID ));
            vx_assert(assetGUID.isValid());
            AssetBaseInfo* assetInfo = createAssetInfo( assetType, assetName.c_str(), fileNameAndPath.c_str(), assetLen, assetGUID );

			assetInfo->setCreatorId( cursor->getString( COLUMN_ASSET_CREATOR_ID ) );
			assetInfo->setHistoryId( cursor->getString( COLUMN_ASSET_HISTORY_ID ) );
			assetInfo->setAdminId( cursor->getString( COLUMN_ASSET_ADMIN_ID ) );
            assetInfo->setThumbId( cursor->getString( COLUMN_ASSET_THUMB_ID ) );
			assetInfo->setAssetHashId( (uint8_t *)cursor->getBlob( COLUMN_ASSET_HASH_ID ) );
			assetInfo->setLocationFlags( cursor->getS32( COLUMN_LOCATION_FLAGS ) );
            assetInfo->setAttributeFlags( cursor->getS32( COLUMN_ATTRIBUTE_FLAGS ) );
            assetInfo->setCreationTime(  (int64_t)cursor->getS64( COLUMN_CREATION_TIME ) );
            assetInfo->setModifiedTime(  (int64_t)cursor->getS64( COLUMN_MODIFIED_TIME ) );
            assetInfo->setAccessedTime(  (int64_t)cursor->getS64( COLUMN_ACCESSED_TIME ) );
			assetInfo->setAssetTag( cursor->getString( COLUMN_ASSET_TAG ) );		
			assetInfo->setAssetSendState( ( EAssetSendState )cursor->getS32( COLUMN_ASSET_SEND_STATE ) );
			assetInfo->setPluginType( (EPluginType)cursor->getS32( COLUMN_PLUGIN_TYPE ) );
			assetInfo->setDestUserId( cursor->getString( COLUMN_SEND_TO_ID ) );

			if( assetInfo->isValid( false ) && assetInfo->validateAssetExist() )
			{
				if( assetInfo->isThumbAsset() && assetInfo->isTemporary() )
				{
					// remove temporary thumbs so does not fill up drive
					VxFileUtil::deleteFile( fileNameAndPath.c_str() );
					toRemoveList.emplace_back( assetInfo );
				}
				else
				{
					insertAssetInTimeOrder( assetInfo, AssetAssetList );
				}	
			}
			else
			{
				toRemoveList.emplace_back( assetInfo );
			}
		}

		cursor->close();
	}

	unlockAssetInfoDb();
	// clear out any that do not exist anymore
	for( auto asset : toRemoveList )
	{
		removeAsset( asset );
		delete asset;
	}
} 

//============================================================================
void AssetBaseInfoDb::insertAssetInTimeOrder( AssetBaseInfo*assetInfo, std::vector<AssetBaseInfo*>& assetList )
{
    vx_assert( assetInfo->isValid() );

	std::vector<AssetBaseInfo*>::iterator iter;
	for( iter = assetList.begin(); iter != assetList.end(); ++iter )
	{
		if( (*iter)->getCreationTime() > assetInfo->getCreationTime() )
		{
			assetList.insert( iter, assetInfo );
			return;
		}
	}

	assetList.emplace_back( assetInfo );
}

