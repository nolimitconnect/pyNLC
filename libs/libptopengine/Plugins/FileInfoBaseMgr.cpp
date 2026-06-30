//============================================================================
// Copyright (C) 2022 Brett R. Jones
//
// Code copyrighted by Brett R. Jones is under dual license similar to Ruby's license
// See file COPYING and LEGAL in root of the No Limit Connect project
//
// bjones.engineer@gmail.com
// https://nolimitconnect.com
//============================================================================

#include "FileInfoBaseMgr.h"

#include "PluginBase.h"
#include <GuiInterface/IToGui.h>

#include <AssetBase/AssetBaseInfo.h>
#include <AssetMgr/AssetMgr.h>

#include <Plugins/FileInfo.h>
#include <Plugins/PluginBase.h>

#include <P2PEngine/P2PEngine.h>

#include <CoreLib/Sha1GeneratorMgr.h>
#include <CoreLib/VxDebug.h>
#include <CoreLib/VxFileUtil.h>
#include <CoreLib/VxFileIsTypeFunctions.h>
#include <CoreLib/VxGlobals.h>
#include <CoreLib/VxSha1Hash.h>
#include <CoreLib/VxFileShredder.h>
#include <CoreLib/VxElapseTimer.h>

#include <PktLib/PktAnnounce.h>
#include <PktLib/PktsFileList.h>
#include <PktLib/PktsFileInfo.h>

//============================================================================
FileInfoBaseMgr::FileInfoBaseMgr( P2PEngine& engine, PluginBase& plugin, FileInfoDb& fileInfoDb )
: m_Plugin( plugin )
, m_FileInfoDb( fileInfoDb )
, m_FileInfoXferMgr( engine, plugin, *this )
, m_PrivateEngine( engine )
{
	if(LogEnabled(eLogStartup))LogMsg( LOG_VERBOSE, "FileInfoBaseMgr::FileInfoBaseMgr %s %p", DescribePluginType( plugin.getPluginType() ), this );
}

//============================================================================
FileInfoBaseMgr::~FileInfoBaseMgr()
{
	fileInfoMgrShutdown();
}

//============================================================================
std::string FileInfoBaseMgr::getIncompleteFileXferDirectory( VxGUID& onlineId )
{
	std::string incompleteDir = m_Plugin.getIncompleteFileXferDirectory( onlineId );
	if( incompleteDir.empty() )
	{
		incompleteDir = VxGetIncompleteDirectory();
	}

	return incompleteDir;
}

//============================================================================
void FileInfoBaseMgr::onAfterUserLogOnThreaded( void )
{
	if( ePluginTypeFileShareServer == m_Plugin.getPluginType() )
	{
		// code has changed and now uses asset manager to manage shared files
		m_FilesInitialized = true;
		return;
	}

	int timeStart = GetApplicationAliveMs();	// user specific directory should be set
	std::string dbName = VxGetUserSpecificDataDirectory() + "settings/";
	dbName += m_FileInfoDb.getFileInfoDbName();
	std::vector<std::string> toDeleteFiles;
	std::map<VxGUID, FileInfo>	dbFileList;

	lockFileList();
	m_FileInfoDb.dbShutdown();
	m_FileInfoDb.dbStartup( 1, dbName );

	m_FileInfoDb.getAllFiles( dbFileList );

	for( auto iter = dbFileList.begin(); iter != dbFileList.end(); ++iter )
	{
		FileInfo& fileInfo = iter->second;
        int64_t curFileLen = VxFileUtil::fileExists( fileInfo.getFileNameAndPath().c_str() );
		if( curFileLen && fileInfo.isValid( false ) )
		{
			if( curFileLen == fileInfo.getFileLength() && fileInfo.isValid( true ) )
			{
				// because file length is same and sha1 is valid assume is ready for use
				m_FileInfoList[fileInfo.getAssetId()] = fileInfo;
			}
			else
			{
				fileInfo.setFileLength( curFileLen );
				
                m_FileInfoNeedHashAndSaveList.emplace_back( fileInfo );
				
				addFileToGenHashQue( fileInfo.getAssetId(), fileInfo.getFileName(), fileInfo.getFileNameAndPath() );
			}
		}
		else
		{
			// file no longer exists
			toDeleteFiles.emplace_back( fileInfo.getFileNameAndPath() );
		}

		if( VxIsAppShuttingDown() )
		{
			unlockFileList();
			return;
		}
	}

	unlockFileList();
	updateFileTypes();

	// delete from db files that no longer exist
	for( auto assetId : toDeleteFiles )
	{
		lockFileList();
		m_FileInfoDb.removeFile( assetId );
		unlockFileList();
	}

	checkForInitializeCompleted();
}

//============================================================================
void FileInfoBaseMgr::fileInfoMgrShutdown( void )
{
	lockFileList();
	m_FileInfoDb.dbShutdown();
	unlockFileList();
}

//============================================================================
void FileInfoBaseMgr::addFileToGenHashQue( VxGUID& fileId, std::string fileName, std::string fileNameAndPath )
{
	GetSha1GeneratorMgr().generateSha1( fileId, fileName, fileNameAndPath, this );
}

//============================================================================
void FileInfoBaseMgr::removeFileFromGenHashQue( VxGUID& fileId, std::string fileNameAndPath )
{
	GetSha1GeneratorMgr().cancelGenerateSha1( fileId, fileNameAndPath, this );
}

//============================================================================
bool FileInfoBaseMgr::isAllowedFileOrDir( std::string strFileName )
{
	if( VxIsExecutableFile( strFileName ) 
		|| VxIsShortcutFile( strFileName ) )
	{
		return false;
	}

	return true;
}

//============================================================================
// returns -1 if unknown else percent downloaded
int FileInfoBaseMgr::fromGuiGetFileDownloadState( uint8_t * fileHashId )
{
	int result = -1;
	lockFileList();
	for( auto iter = m_FileInfoList.begin(); iter != m_FileInfoList.end(); ++iter )
	{
		FileInfo& fileInfo = iter->second;
		if( fileInfo.getFileHashId().isEqualTo( fileHashId ) )
		{
			result = 100;
			break;
		}
	}

	unlockFileList();
	return result;
}

//============================================================================
bool FileInfoBaseMgr::addFileToDbAndList( VxGUID& onlineId, std::string& fileName, std::string& fileNameAndPath, VxGUID& assetId )
{
	if( fileName.empty() )
	{
		LogMsg( LOG_ERROR, "FileInfoBaseMgr::addFileToDbAndList invalid param empty fileName" );
		return false;
	}


	// file is not currently in library and should be
	uint64_t fileLen = VxFileUtil::fileExists( fileNameAndPath.c_str() );
	uint8_t fileType = VxFileExtensionToFileTypeFlag( fileName.c_str() );
	if( ( false == isAllowedFileOrDir( fileName ) )
		|| ( 0 == fileLen ) )
	{
		return false;
	}

	FileInfo fileInfo( onlineId, fileName, fileNameAndPath, fileLen, fileType, assetId );
	if( isLibraryServer() )
	{
		fileInfo.setIsInLibrary( true );
	}
	
	if( isFileShareServer() )
	{
		fileInfo.setIsSharedFile( true );
	}

	AssetBaseInfo* assetBaseInfo = getEngine().getAssetMgr().findAsset( assetId );
	if( !assetBaseInfo )
	{
		getEngine().getAssetMgr().updateAsset( fileInfo );
	}

	if( !addFileToDbAndList( fileInfo ) )
	{
		return false;
	}

	return true;
}

//============================================================================
bool FileInfoBaseMgr::addFileToDbAndList( FileInfo& fileInfoIn )
{
	fileInfoIn.assureValidAssetId();
	lockFileList();

	for( auto iter = m_FileInfoList.begin(); iter != m_FileInfoList.end(); ++iter )
	{
		if( fileInfoIn.getFileNameAndPath() == iter->second.getFileNameAndPath() )
		{
			m_FileInfoDb.removeFile( fileInfoIn.getFileNameAndPath() );

			m_FileInfoList.erase( iter );
			unlockFileList();
			updateFileTypes();
			break;
		}
	}

	unlockFileList();
	bool result{ false };

	if( fileInfoIn.isValid( true ) )
	{
		lockFileList();
		m_FileInfoList[fileInfoIn.getAssetId()] = fileInfoIn;
		m_FileInfoDb.addFile( fileInfoIn );
		unlockFileList();
		result = true;
	}
	else if( fileInfoIn.isValid( false ) )
	{
		// needs file hash then save
		lockFileList();
		m_FileInfoNeedHashAndSaveList.emplace_back( fileInfoIn );
		unlockFileList();
		addFileToGenHashQue( fileInfoIn.getAssetId(), fileInfoIn.getFileName(), fileInfoIn.getFileNameAndPath() );
		result = true;
	}
	else
	{
		LogMsg( LOG_ERROR, "%s invalid %s", __func__, fileInfoIn.getFileName().c_str() );
	}

	return result;
}

//============================================================================
bool FileInfoBaseMgr::addFileToDbAndList(   VxGUID&			onlineId,
											VxGUID&			assetId, 
											std::string		fileName,
											std::string		fileNameAndPath,
											uint64_t		fileLen, 
											uint8_t			fileType,
											VxSha1Hash&		fileHashId )
{
	FileInfo fileInfo( onlineId, fileName, fileNameAndPath, fileLen, fileType, assetId, fileHashId );
	return addFileToDbAndList( fileInfo );
}

//============================================================================
bool FileInfoBaseMgr::cancelAndDelete( VxGUID& assetId )
{
	bool result{ false };

	lockFileList();
	auto iter = m_FileInfoList.find( assetId );
	if( iter != m_FileInfoList.end() )
	{
		m_FileInfoXferMgr.fileAboutToBeDeleted( iter->second.getFileNameAndPath() );
		m_FileInfoDb.removeFile( iter->second.getFileNameAndPath() );
		VxFileUtil::deleteFile( iter->second.getFileNameAndPath().c_str() );
		m_FileInfoList.erase( iter );
	}

	unlockFileList();
	updateFileTypes();
	return result;
}

//============================================================================
bool FileInfoBaseMgr::isFileShared( std::string& fileNameAndPath, bool listIsLocked )
{
	bool isInLib = false;
	if( !listIsLocked )
	{
		lockFileList();
	}

	for( auto iter = m_FileInfoList.begin(); iter != m_FileInfoList.end(); ++iter )
	{
		if( fileNameAndPath == iter->second.getFileNameAndPath() )
		{
			isInLib = true;
			break;
		}
	}

	if( !listIsLocked )
	{
		unlockFileList();
	}

	if( isInLib )
	{
		if( 0 == VxFileUtil::fileExists( fileNameAndPath.c_str() ) )
		{
			removeFromDbAndList( fileNameAndPath );
			isInLib = false;
		}
	}

	return isInLib;
}

//============================================================================
bool FileInfoBaseMgr::isFileShared( VxSha1Hash& fileHashId, bool listIsLocked )
{
	bool isInLib = false;
	std::string fileName;
	if( !listIsLocked )
	{
		lockFileList();
	}

	for( auto iter = m_FileInfoList.begin(); iter != m_FileInfoList.end(); ++iter )
	{
		if( fileHashId == iter->second.getFileHashId() )
		{
			isInLib = true;
			fileName = iter->second.getFileNameAndPath();
			break;
		}
	}

	if( !listIsLocked )
	{
		unlockFileList();
	}

	if( isInLib && fileName.size() )
	{
		if( 0 == VxFileUtil::fileExists( fileName.c_str() ) )
		{
			removeFromDbAndList( fileName );
			isInLib = false;
		}
	}

	return isInLib;
}

//============================================================================
bool FileInfoBaseMgr::isFileShared( VxGUID& assetId, bool listIsLocked )
{
	bool isInLib = false;
	std::string fileName( "" );
	if( !listIsLocked )
	{
		lockFileList();
	}

	for( auto iter = m_FileInfoList.begin(); iter != m_FileInfoList.end(); ++iter )
	{
		if( assetId == iter->second.getAssetId() )
		{
			isInLib = true;
			fileName = iter->second.getFileNameAndPath();
			break;
		}
	}

	if( !listIsLocked )
	{
		unlockFileList();
	}

	if( isInLib && fileName.size() )
	{
		if( 0 == VxFileUtil::fileExists( fileName.c_str() ) )
		{
			removeFromDbAndList( fileName, listIsLocked );
			isInLib = false;
		}
	}

	return isInLib;
}

//============================================================================
bool FileInfoBaseMgr::isFileShared( VxGUID& assetId, VxSha1Hash& fileHashId, bool listIsLocked )
{
	bool isShared = isFileShared( assetId, listIsLocked );
	if( isShared && fileHashId.isHashValid() )
	{
		isShared &= isFileShared( fileHashId, listIsLocked );
	}

	return isShared;
}

//============================================================================
bool FileInfoBaseMgr::removeFromDbAndList( std::string& fileNameAndPath, bool listIsLocked )
{
	bool wasRemoved{ false };
	if( !listIsLocked )
	{
		lockFileList();
	}

	for( auto iter = m_FileInfoList.begin(); iter != m_FileInfoList.end(); ++iter )
	{
		if( fileNameAndPath == iter->second.getFileNameAndPath() )
		{
			if( ePluginTypeFileShareServer != m_Plugin.getPluginType() )
			{
				m_FileInfoDb.removeFile( fileNameAndPath );
			}

			m_FileInfoList.erase( iter );
			wasRemoved = true;
			break;
		}
	}

	// also remove from waiting for hash generation
	for( auto iter = m_FileInfoNeedHashAndSaveList.begin(); iter != m_FileInfoNeedHashAndSaveList.end(); )
	{
		FileInfo& fileInfo = *iter;
		if( fileNameAndPath == fileInfo.getFileNameAndPath() )
		{
			iter = m_FileInfoNeedHashAndSaveList.erase( iter );
			
			break;
		}
		else
		{
			iter++;
		}
	}


	if( !listIsLocked )
	{
		unlockFileList();
		updateFileTypes();
	}

	return wasRemoved;
}

//============================================================================
bool FileInfoBaseMgr::getFileFullName( VxGUID& assetId, VxSha1Hash& fileHashId, std::string& retFileFullName )
{
	bool isShared = false;
	lockFileList();
	for( auto &fileInfo : m_FileInfoList )
	{
		if( assetId == fileInfo.second.getAssetId() && fileHashId == fileInfo.second.getFileHashId() )
		{
			isShared = true;
			retFileFullName = fileInfo.second.getFileNameAndPath();
			break;
		}
	}

	unlockFileList();
	return isShared;
}

//============================================================================
bool FileInfoBaseMgr::getFileHashId( std::string& fileFullName, VxSha1Hash& retFileHashId )
{
	bool foundHash = false;
	lockFileList();
	for( auto iter = m_FileInfoList.begin(); iter != m_FileInfoList.end(); ++iter )
	{
		if( fileFullName == iter->second.getFileNameAndPath() )
		{
			retFileHashId = iter->second.getFileHashId();
			foundHash = retFileHashId.isHashValid();
			break;
		}
	}

	unlockFileList();
	return foundHash;
}

//============================================================================
void FileInfoBaseMgr::callbackSha1GenerateResult( ESha1GenResult sha1GenResult, VxGUID& assetId, Sha1Info& sha1Info )
{
	lockFileList();
	for( auto iter = m_FileInfoNeedHashAndSaveList.begin(); iter != m_FileInfoNeedHashAndSaveList.end(); )
	{
		bool wasErased{ false };
		FileInfo& fileInfo = *iter;
		if( assetId == fileInfo.getAssetId() )
		{
			if( eSha1GenResultNoError == sha1GenResult )
			{
				fileInfo.setFileHashId( sha1Info.getSha1Hash() );
				fileInfo.setFileTime( GetGmtTimeMs() );
				m_FileInfoDb.addFile( fileInfo );
				m_FileInfoList[ fileInfo.getAssetId() ] = fileInfo;

				iter = m_FileInfoNeedHashAndSaveList.erase( iter );
				wasErased = true;
			}
			else
			{
				LogMsg( LOG_VERBOSE, "FileInfoBaseMgr::callbackSha1GenerateResult failed %s", DescribeSha1GenResult( sha1GenResult ) );
			}

			break;
		}

		if( !wasErased )
		{
			iter++;
		}
	}

	unlockFileList();

    if( isFileShareServer() && eSha1GenResultNoError == sha1GenResult )
    {
        // file share server adds file directly to list even if not valid hash
        lockFileList();
        for( auto& fileInfoPair : m_FileInfoList )
        {
            FileInfo& fileInfo = fileInfoPair.second;
            if( assetId == fileInfo.getAssetId() )
            {
                fileInfo.setFileHashId( sha1Info.getSha1Hash() );
                fileInfo.setFileTime( GetGmtTimeMs() );
                break;
            }
        }

        unlockFileList();
    }

	updateFileTypes();
	checkForInitializeCompleted();
}

//============================================================================
bool FileInfoBaseMgr::fromGuiAddSharedFile( FileInfo& fileInfo, bool isShared )
{
	if( isLibraryServer() )
	{
		fileInfo.setIsInLibrary( isShared );
	}
	else if( isFileShareServer() )
	{
		fileInfo.setIsSharedFile( isShared );
	}

	if( isShared )
	{
		return addFileToDbAndList( fileInfo );
	}
	else
	{
		return removeFromDbAndList( fileInfo.getFileNameAndPath() );
	}
}

//============================================================================
bool FileInfoBaseMgr::fromGuiQueryFileHash( FileInfo& fileInfo )
{
	bool foundHash = false;
	lockFileList();
	for( auto iter = m_FileInfoList.begin(); iter != m_FileInfoList.end(); ++iter )
	{
		if( fileInfo.getFileLength() == iter->second.getFileLength() && fileInfo.getFileNameAndPath() == iter->second.getFileNameAndPath() )
		{
			if( iter->second.getFileHashId().isHashValid() )
			{
				fileInfo.setFileHashId( iter->second.getFileHashId() );
				foundHash = true;
			}

			break;
		}
	}

	unlockFileList();
	return foundHash;
}

//============================================================================
void FileInfoBaseMgr::fromGuiFileHashGenerated( std::string& fileNameAndPath, int64_t fileLen, VxSha1Hash& fileHash )
{
	lockFileList();
	for( auto iter = m_FileInfoList.begin(); iter != m_FileInfoList.end(); ++iter )
	{
		if( fileLen == iter->second.getFileLength() && fileNameAndPath == iter->second.getFileNameAndPath() )
		{
			if( !iter->second.getFileHashId().isHashValid() || !iter->second.getFileHashId().isEqualTo( fileHash.getHashData() ) )
			{
				iter->second.getFileHashId().setHashData( fileHash.getHashData() );
				if( ePluginTypeFileShareServer != m_Plugin.getPluginType() )
				{
					m_FileInfoDb.addFile( iter->second );
				}
			}

			break;
		}
	}

	unlockFileList();
}

//============================================================================
bool FileInfoBaseMgr::loadAboutMePageStaticAssets( void )
{
	static std::vector<std::string>	g_AboutMeNameList{
		{ "favicon.ico" },			
		{ "aboutme_thumb.png" },	
		{ "index.htm" },			
		{ "me.png" },					
	};

	if( m_RootFileFolder.empty() )
	{
		LogMsg( LOG_ERROR, "getAboutMePageStaticAssets No Root File Folder" );
		return false;
	}

	int fileCount{ 0 };
	bool result{ true };
	for( auto &fileShortName : g_AboutMeNameList )
	{
		std::string fullFileName = m_RootFileFolder + fileShortName;
		int64_t fileLen = VxFileUtil::fileExists( fullFileName.c_str() );
		VxGUID assetId;
		if( fileLen )
		{
			assetId.initializeWithNewVxGUID();
			// we can generate the sha1Hash now instead of in thread because there is only a few small files
			VxSha1Hash sha1Hash;
			if( sha1Hash.generateHashFromFile( fullFileName.c_str() ) )
			{
				FileInfo fileInfo( getEngine().getMyOnlineId(), fileShortName, fullFileName, fileLen, VxFileExtensionToFileTypeFlag( fullFileName.c_str() ), assetId, sha1Hash );
				if( fileInfo.isValid( true ) )
				{
					lockFileList();
					m_FileInfoList[assetId] = fileInfo;
					unlockFileList();
					fileCount++;
				}
				else
				{
					LogMsg( LOG_ERROR, "getAboutMePageStaticAssets Invalid File %s", fullFileName.c_str() );
					result = false;
					break;
				}
			}
			else
			{
				LogMsg( LOG_ERROR, "getAboutMePageStaticAssets Generate Sha1Hash Failed File %s", fullFileName.c_str() );
				result = false;
				break;
			}			
		}
		else
		{
			LogMsg( LOG_ERROR, "getAboutMePageStaticAssets 0 file Len File %s", fullFileName.c_str() );
			result = false;
			break;
		}
	}

    return result && fileCount == (int)g_AboutMeNameList.size();
}

//============================================================================
bool FileInfoBaseMgr::loadStoryboardPageFileAssets( void )
{
	static std::vector<std::string>	g_StoryboardNameList{
		{ "favicon.ico" },				// !0C42AEB1E8072DBE12B699D027EB2393! no limit icon
		{ "storyboard_thumb.png" },		// !F841B8C2C7953211B11ED11DEF45D19A! story_board thumb
		{ "story_board.htm" },			// !F6460B0A160B362D30B11737E4CB018B! story_board index
		{ "storyboard_background.png" }, // !124F588DFCD993003BC21C1AA5C785A3! story_board background
		{ "me.png" },					// !52452C124D2D3D7288114D7EC151A6BC! story_board me.png
	};


	if( m_RootFileFolder.empty() )
	{
		LogMsg( LOG_ERROR, "%s No Root File Folder", __func__ );
		return false;
	}

	std::vector<std::string> fileList;
	VxFileUtil::listFilesInDirectory( m_RootFileFolder.c_str(), fileList );
	if( fileList.size() < g_StoryboardNameList.size() )
	{
		LogMsg( LOG_ERROR, "%s Missing Files only %zu found", __func__, fileList.size() );
		return false;
	}

	// TODO loading and generate has should probably be put in a unique thread for people how create a very long storyboard web page
	int fileCount{ 0 };
	bool result{ true };
	for( auto& fullFileName : fileList )
	{
		int64_t fileLen = VxFileUtil::fileExists( fullFileName.c_str() );
		VxGUID assetId;
		if( fileLen )
		{
			assetId.initializeWithNewVxGUID();
			VxSha1Hash sha1Hash;
			if( sha1Hash.generateHashFromFile( fullFileName.c_str() ) )
			{
				VxFileInfoBase fileInfoBase;
				if( VxFileUtil::getFileInfo( fullFileName.c_str(), fileInfoBase ) )
				{
					FileInfo fileInfo( fileInfoBase );
					fileInfo.setFileHashId( sha1Hash );
					fileInfo.setAssetId( assetId );
					fileInfo.setOnlineId( getEngine().getMyOnlineId() );
					if( fileInfo.isValid( true ) )
					{
						lockFileList();
						m_FileInfoList[assetId] = fileInfo;
						unlockFileList();
						fileCount++;
					}
					else
					{
						LogMsg( LOG_ERROR, "%s Invalid File %s", __func__, fullFileName.c_str() );
						result = false;
						break;
					}
				}
				else
				{
					LogMsg( LOG_ERROR, "%s Invalid File Info %s", __func__, fullFileName.c_str() );
					result = false;
					break;
				}

			}
			else
			{
				LogMsg( LOG_ERROR, "%s Generate Sha1Hash Failed File %s", __func__, fullFileName.c_str() );
				result = false;
				break;
			}
		}
		else
		{
			LogMsg( LOG_ERROR, "%s 0 file Len File %s", __func__, fullFileName.c_str() );
			result = false;
			break;
		}
	}

    return result && fileCount == (int)fileList.size();
}

//============================================================================
void FileInfoBaseMgr::updateFileTypes( void )
{
	int64_t	newUpdateTime{ 0 };
	int64_t	newTotalBytes{ 0 };
	uint16_t newFileTypes{ 0 };

	lockFileList();
	int64_t	oldUpdateTime = m_LastUpdateTime;
	int64_t	oldTotalBytes =	m_s64TotalByteCnt;
	uint16_t oldFileTypes = m_u16FileTypes;

	for( auto iter = m_FileInfoList.begin(); iter != m_FileInfoList.end(); ++iter )
	{
		newTotalBytes += iter->second.getFileLength();
		newFileTypes |= iter->second.getFileType();
		if( iter->second.getFileTime() > newUpdateTime )
		{
			newUpdateTime = iter->second.getFileTime();
		}
	}

	unlockFileList();
	if( oldTotalBytes != newTotalBytes || oldFileTypes != newFileTypes || oldUpdateTime != newUpdateTime )
	{
		lockFileList();
		m_s64TotalByteCnt = newTotalBytes;
		m_u16FileTypes = newFileTypes;
		unlockFileList();

		if( m_FilesInitialized )
		{
			m_Plugin.onFilesChanged( newUpdateTime, newTotalBytes, newFileTypes );
		}
	}

	if( ePluginTypeFileShareServer == m_Plugin.getPluginType() )
	{
		// update pktann with shared file types
		// ignore extended types
		uint16_t u16FileTypes = m_u16FileTypes & 0xff;

		m_PrivateEngine.lockAnnouncePktAccess();
		PktAnnounce& pktAnn = m_PrivateEngine.getMyPktAnnounce();
		if( pktAnn.getSharedFileTypes() != u16FileTypes )
		{
			pktAnn.setSharedFileTypes( (uint8_t)u16FileTypes );
		}

		m_PrivateEngine.unlockAnnouncePktAccess();
	}
}

//============================================================================
void FileInfoBaseMgr::checkForInitializeCompleted( void )
{
	if( !m_FilesInitialized && m_FileInfoNeedHashAndSaveList.empty() )
	{	
		lockFileList();
		m_FilesInitialized = true;
		int64_t	lastUpdateTime = m_LastUpdateTime;
		int64_t	totalByteCnt = m_s64TotalByteCnt;
		uint16_t FileTypes = m_u16FileTypes;
		unlockFileList();

		m_Plugin.onLoadedFilesReady( lastUpdateTime, totalByteCnt, FileTypes );
	}
}

//============================================================================
bool FileInfoBaseMgr::findFileAsset( VxGUID& fileAssetId, FileInfo& fileInfo)
{
	auto iter = m_FileInfoList.find( fileAssetId );
	if( iter != m_FileInfoList.end() )
	{
		fileInfo = iter->second;
		return true;
	}

	return false;
}

//============================================================================
ECommErr FileInfoBaseMgr::searchRequest( PktFileInfoSearchReply& pktReply, VxGUID& specificAssetId, std::string& searchStr, uint8_t searchFileTypes, std::shared_ptr<VxSktBase>& sktBase, VxGUID onlineId )
{
	ECommErr searchErr = m_FilesInitialized ? eCommErrNone : eCommErrPluginNotEnabled;
	if( eCommErrNone == searchErr )
	{
		pktReply.setCommError( eCommErrNotFound );
		lockFileList();

		if( specificAssetId.isValid() )
		{
			FileInfo fileInfo;
			if( findFileAsset( specificAssetId, fileInfo ) && fileInfo.isValid() )
			{
				if( fileInfo.addToBlob( pktReply.getBlobEntry() ) )
				{
					pktReply.setCommError( eCommErrNone );
				}
			}
		}
		else if( searchStr.length() >= FileInfo::FILE_INFO_SHORTEST_SEARCH_TEXT_LEN && searchStr.length() <= FileInfo::FILE_INFO_LONGEST_SEARCH_TEXT_LEN )
		{
			pktReply.setCommError( eCommErrSearchNoMatch );
			bool foundMatch = false;
			for( auto iter = m_FileInfoList.begin(); iter != m_FileInfoList.end(); ++iter  )
			{
				FileInfo& fileInfo = iter->second;
				if( fileInfo.isValid() && fileInfo.matchTextAndType( searchStr, searchFileTypes ) )
				{
					if( pktReply.getBlobEntry().haveRoom( fileInfo.calcBlobLen() ) )
					{
						if( fileInfo.addToBlob( pktReply.getBlobEntry() ) )
						{
							pktReply.incrementFileInfoCount();
							foundMatch = true;
						}
					}
					else
					{
						// there are more to match
						pktReply.setMoreFileInfosExist( true );
						pktReply.setNextSearchAssetId( fileInfo.getAssetId() );
						break;
					}
				}
			}

			if( foundMatch )
			{
				pktReply.setCommError( eCommErrNone );
			}
		}
		else
		{
			if( !searchStr.empty() )
			{
				LogMsg( LOG_VERBOSE, "%s Warning search text was to short.. sending all files", __func__ );
			}

			// all files list
			bool foundMatch = false;
			for( auto iter = m_FileInfoList.begin(); iter != m_FileInfoList.end(); ++iter )
			{
				FileInfo& fileInfo = iter->second;
				if( fileInfo.isValid() )
				{
					if( pktReply.getBlobEntry().haveRoom( fileInfo.calcBlobLen() ) )
					{
						if( fileInfo.addToBlob( pktReply.getBlobEntry() ) )
						{
							pktReply.incrementFileInfoCount();
							foundMatch = true;
						}
					}
					else
					{
						// there are more to match
						pktReply.setMoreFileInfosExist( true );
						pktReply.setNextSearchAssetId( fileInfo.getAssetId() );
						break;
					}
				}
			}

			if( foundMatch )
			{
				pktReply.setCommError( eCommErrNone );
			}
		}

		unlockFileList();
	}

	pktReply.calcPktLen();
	return pktReply.getCommError();
}

//============================================================================
ECommErr FileInfoBaseMgr::searchMoreRequest( PktFileInfoMoreReply& pktReply, VxGUID& nextFileAssetId, std::string& searchStr, uint8_t searchFileTypes, std::shared_ptr<VxSktBase>& sktBase, VxGUID onlineId )
{
	ECommErr searchErr = m_FilesInitialized ? eCommErrNone : eCommErrPluginNotEnabled;
	if( eCommErrNone == searchErr && !nextFileAssetId.isValid() )
	{
		searchErr = eCommErrInvalidParam;
	}

	if( eCommErrNone == searchErr )
	{
		pktReply.setCommError( eCommErrNotFound );
		bool foundMatch = false;		
		lockFileList();
		for( auto iter = m_FileInfoList.find( nextFileAssetId ); iter != m_FileInfoList.end(); ++iter )
		{
			FileInfo& fileInfo = iter->second;
			if( fileInfo.isValid() )
			{
				if( searchStr.length() >= FileInfo::FILE_INFO_SHORTEST_SEARCH_TEXT_LEN && searchStr.length() <= FileInfo::FILE_INFO_LONGEST_SEARCH_TEXT_LEN )
				{
					if( fileInfo.matchTextAndType( searchStr, searchFileTypes ) )
					{
						if( pktReply.getBlobEntry().haveRoom( fileInfo.calcBlobLen() ) )
						{
							if( fileInfo.addToBlob( pktReply.getBlobEntry() ) )
							{
								pktReply.incrementFileInfoCount();
								foundMatch = true;
							}
						}
						else
						{
							// there are more to match
							pktReply.setMoreFileInfosExist( true );
							pktReply.setNextSearchAssetId( fileInfo.getAssetId() );
							break;
						}
					}
				}
				else
				{
					if( pktReply.getBlobEntry().haveRoom( fileInfo.calcBlobLen() ) )
					{
						if( fileInfo.addToBlob( pktReply.getBlobEntry() ) )
						{
							pktReply.incrementFileInfoCount();
							foundMatch = true;
						}
					}
					else
					{
						// there are more to match
						pktReply.setMoreFileInfosExist( true );
						pktReply.setNextSearchAssetId( fileInfo.getAssetId() );
						break;
					}
				}
			}
		}

		unlockFileList();
		if( foundMatch )
		{
			pktReply.setCommError( eCommErrNone );
		}
	}

	pktReply.calcPktLen();
	return pktReply.getCommError();
}

//============================================================================
bool FileInfoBaseMgr::startDownload( FileInfo& fileInfo, VxGUID& searchSessionId, std::shared_ptr<VxSktBase>& sktBase, VxGUID onlineId )
{
	bool downStarted = m_FileInfoXferMgr.startDownload( fileInfo, searchSessionId, sktBase, onlineId );
    m_Plugin.onFileDownloadStart( downStarted, onlineId, sktBase, searchSessionId, fileInfo.getRemoteFileName(), fileInfo.getAssetId() );
	return downStarted;
}

//============================================================================
bool FileInfoBaseMgr::onFileDownloadComplete( VxGUID& onlineId, std::shared_ptr<VxSktBase>& sktBase, VxGUID& lclSessionId, std::string& fileName, VxGUID& assetId, VxSha1Hash& sha11Hash )
{
	LogMsg( LOG_VERBOSE, "%s %s", __func__, fileName.c_str() );
	return m_Plugin.onFileDownloadComplete( onlineId, sktBase, lclSessionId, fileName, assetId, sha11Hash );
}

//============================================================================
void FileInfoBaseMgr::toGuiRxedPluginOffer( VxGUID onlineId, EPluginType pluginType, OfferBaseInfo& offerInfo, VxGUID& lclSessionId )
{
	m_Plugin.toGuiRxedPluginOffer( onlineId, pluginType, offerInfo, lclSessionId );
}

//============================================================================
void FileInfoBaseMgr::toGuiRxedOfferReply( VxGUID onlineId, EPluginType pluginType, OfferBaseInfo& offerInfo, VxGUID& lclSessionId, EOfferResponse offerResponse )
{
	m_Plugin.toGuiRxedOfferReply( onlineId, pluginType, offerInfo, lclSessionId, offerResponse );
}

//============================================================================
void FileInfoBaseMgr::toGuiFileUploadStart( VxGUID& onlineId, VxGUID& lclSessionId, FileInfo& fileInfo )
{
	m_Plugin.toGuiFileUploadStart( onlineId, lclSessionId, fileInfo );
}

//============================================================================
void FileInfoBaseMgr::toGuiFileDownloadStart( VxGUID& onlineId, VxGUID& lclSessionId, FileInfo& fileInfo )
{
	m_Plugin.toGuiFileDownloadStart( onlineId, lclSessionId, fileInfo );
}

//============================================================================
void FileInfoBaseMgr::updateToGuiFileXferState( VxGUID& lclSessionId, EXferDirection xferDir, EXferState xferState, EXferError xferErr, int param )
{
	if(LogEnabled(eLogFileXfer)) LogModule( eLogFileXfer, LOG_VERBOSE, "FileInfoBaseMgr::%s session id %s %s %s param %d", __func__, 
			lclSessionId.toHexString().c_str(), DescribeXferState( xferState ), DescribeXferError( xferErr ), param );
	m_Plugin.toGuiFileXferState( lclSessionId, xferDir, xferState, xferErr, param );
}

//============================================================================
void FileInfoBaseMgr::toGuiFileDownloadComplete( VxGUID& lclSessionId, std::string fileName, EXferError xferError )
{
	if(LogEnabled(eLogFileXfer)) LogModule( eLogFileXfer, LOG_VERBOSE, "FileInfoBaseMgr::%s session id %s file name %s xferError %s ", __func__, 
			lclSessionId.toHexString().c_str(), fileName.c_str(), DescribeXferError( xferError ) );
	m_Plugin.toGuiFileDownloadComplete( lclSessionId, fileName, xferError );
}

//============================================================================
void FileInfoBaseMgr::toGuiFileUploadComplete( VxGUID& lclSessionId, std::string fileName, EXferError xferError )
{
	if(LogEnabled(eLogFileXfer)) LogModule( eLogFileXfer, LOG_VERBOSE, "FileInfoBaseMgr::%s session id %s file name %s xferError %s ", __func__, 
			lclSessionId.toHexString().c_str(), fileName.c_str(), DescribeXferError( xferError ) );
	m_Plugin.toGuiFileUploadComplete( lclSessionId, fileName, xferError );
}

//============================================================================
bool FileInfoBaseMgr::fromGuiSetFileIsShared( FileInfo& fileInfoIn, bool isShared )
{
	std::string fileName = fileInfoIn.getFileNameAndPath();
	return fromGuiSetFileIsShared( fileName, isShared );
}

//============================================================================
bool FileInfoBaseMgr::fromGuiSetFileIsShared( std::string& fileNameAndPath, bool isShared )
{
	if( fileNameAndPath.empty() )
	{
		LogMsg( LOG_ERROR, "%s empty file name", __func__ );
		return false;
	}

	lockFileList();

	for( auto iter = m_FileInfoList.begin(); iter != m_FileInfoList.end(); ++iter )
	{
		FileInfo& fileInfo = iter->second;
		if( fileNameAndPath == fileInfo.getFileNameAndPath() )
		{
			if( false == isShared )
			{
				m_FileInfoDb.removeFile( fileNameAndPath );
				m_FileInfoList.erase( iter );
				unlockFileList();
				updateFileTypes();
				return true;
			}
			else
			{
				if( isLibraryServer() )
				{
					fileInfo.setIsInLibrary( isShared );
				}
				else if( isFileShareServer() )
				{
					fileInfo.setIsSharedFile( isShared );
				}

				unlockFileList();
				return true;
			}
		}
	}

	unlockFileList();

	if( isShared )
	{
		// file is not currently shared and should be
		uint64_t fileLen = VxFileUtil::fileExists( fileNameAndPath.c_str() );
		if( (false == isAllowedFileOrDir( fileNameAndPath ))
			|| (0 == fileLen) )
		{
			LogMsg( LOG_ERROR, "%s file does not exist or not allowed %s", __func__, fileNameAndPath.c_str() );
			return false;
		}

		VxGUID assetId;
		std::string justFileName;
		getEngine().getAssetMgr().lockResources();
		AssetBaseInfo* assetInfo = getEngine().getAssetMgr().findAsset( fileNameAndPath );
		if( assetInfo )
		{
			justFileName = assetInfo->getFileName();
			// if already exists as asset be sure to use the same asset id
			if( isLibraryServer() )
			{
				assetInfo->setIsInLibrary( isShared );
			}
			else if( isFileShareServer() )
			{
				assetInfo->setIsSharedFileAsset( isShared );
			}

			assetId = assetInfo->getAssetUniqueId();
			getEngine().getAssetMgr().unlockResources();
		}
		else
		{
			getEngine().getAssetMgr().unlockResources();
			assetId.initializeWithNewVxGUID();
			VxFileInfoBase fileInfo;
			if( VxFileUtil::getFileInfo( fileNameAndPath.c_str(), fileInfo) )
			{
				justFileName = fileInfo.getFileName();
			}
		}

		if( !justFileName.empty() )
		{
			return addFileToDbAndList( getEngine().getMyOnlineId(), justFileName, fileNameAndPath, assetId );
		}	
	}

	return false;
}

//============================================================================
void FileInfoBaseMgr::fileShareEnable( AssetBaseInfo* assetInfo, bool isShared )
{
	assetInfo->setIsSharedFileAsset( isShared );
	FileInfo fileInfoIn = assetInfo->getFileInfo();
	std::string fileNameAndPath = fileInfoIn.getFileNameAndPath();

	// add to file list but not db 
	lockFileList();

	for( auto iter = m_FileInfoList.begin(); iter != m_FileInfoList.end(); ++iter )
	{
		FileInfo& fileInfo = iter->second;
		if( fileNameAndPath == fileInfo.getFileNameAndPath() )
		{
			if( !isShared )
			{
				//m_FileInfoDb.removeFile( fileNameAndPath );
				m_FileInfoList.erase( iter );
				unlockFileList();
				updateFileTypes();
				return;
			}
			else
			{
				iter->second = fileInfoIn;

				unlockFileList();
                generateHashIfNeeded( fileInfoIn );
				return;
			}
		}
	}

	unlockFileList();

	if( isShared )
	{
		// file is not currently shared and should be
		uint64_t fileLen = VxFileUtil::fileExists( fileNameAndPath.c_str() );
		if( (false == isAllowedFileOrDir( fileNameAndPath ))
			|| (0 == fileLen) )
		{
			LogMsg( LOG_ERROR, "%s file does not exist or not allowed %s", __func__, fileNameAndPath.c_str() );
			return;
		}

		VxGUID assetId = assetInfo->assureAssetUniqueId();

		lockFileList();
		m_FileInfoList[ assetId ] = fileInfoIn;
		unlockFileList();
        generateHashIfNeeded( fileInfoIn );
		updateFileTypes();
	}
}

//============================================================================
bool FileInfoBaseMgr::fromGuiGetFileIsShared( FileInfo& fileInfo )
{
	return isFileShared( fileInfo.getFileNameAndPath() );
}

//============================================================================
bool FileInfoBaseMgr::fromGuiGetFileIsShared( std::string& fileName )
{
	return isFileShared( fileName );
}

//============================================================================
bool FileInfoBaseMgr::isFileShareServer( void )
{
	return m_Plugin.getPluginType() == ePluginTypeFileShareServer;
}

//============================================================================
bool FileInfoBaseMgr::isLibraryServer( void )
{
	return m_Plugin.getPluginType() == ePluginTypeLibraryServer;
}

//============================================================================
void FileInfoBaseMgr::sendFileSearchResultToGui( VxGUID& searchSessionId, VxGUID& onlineId, FileInfo& fileInfo )
{
	m_PrivateEngine.getToGui().toGuiSearchResultFileSearch( onlineId, m_Plugin.getPluginType(), searchSessionId, fileInfo );
}

//============================================================================
void FileInfoBaseMgr::generateHashIfNeeded( FileInfo& fileInfoIn )
{
    if( fileInfoIn.getFileHashId().isHashValid() )
    {
        addFileToGenHashQue( fileInfoIn.getAssetId(), fileInfoIn.getFileName(), fileInfoIn.getFileNameAndPath() );
    }
}
