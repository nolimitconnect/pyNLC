#pragma once
//============================================================================
// Copyright (C) 2013 Brett R. Jones
//
// Code copyrighted by Brett R. Jones is under dual license similar to Ruby's license
// See file COPYING and LEGAL in root of the No Limit Connect project
//
// bjones.engineer@gmail.com
// https://nolimitconnect.com
//============================================================================

#include <QWidget>

#include <Plugins/FileInfo.h>

#define FILE_STATUS_DOWNLOADING			0x0001
#define FILE_STATUS_UPLOADING			0x0002

class VxNetIdent;
class AppCommon;
class AssetBaseInfo;

class FileItemInfo
{
public:
	FileItemInfo( FileInfo& fileInfo );

	void						setFileInfo( FileInfo& fileInfo )		{ m_FileInfo = fileInfo; }

	void						setIsSharedFile( bool isShared )		{ m_FileInfo.setIsSharedFile( isShared ); }
	bool						getIsSharedFile( void )					{ return m_FileInfo.getIsSharedFile(); }

	void						setIsInLibrary( bool isInLibrary )		{ m_FileInfo.setIsInLibrary( isInLibrary ); }
	bool						getIsInLibrary( void )					{ return m_FileInfo.getIsInLibrary(); }

	bool						getIsInUse( void )						{ return m_FileInfo.getIsInUse(); }

	QString					    getFileNameAndPath( void )				{ return m_FileInfo.getFileNameAndPath().c_str(); }
	QString						getFileName( void )						{ return m_FileInfo.getFileName().c_str(); }

    uint8_t						getFileType( void )						{ return m_FileInfo.getFileType(); }
    uint64_t					getFileLength( void )					{ return m_FileInfo.getFileLength(); }
	VxSha1Hash&					getFileHashId( void )					{ return m_FileInfo.getFileHashId(); }

	bool						getIsMediaFile( void )					{ return m_FileInfo.getIsMediaFile(); }
	bool						getIsVideoFile( void )					{ return m_FileInfo.getIsVideoFile(); }
	bool						getIsAudioFile( void )					{ return m_FileInfo.getIsAudioFile(); }
	bool						getIsImageFile( void )					{ return m_FileInfo.getIsImageFile(); }

	void 						setAssetId( VxGUID& assetId )			{ m_FileInfo.setAssetId( assetId ); }
	VxGUID						getAssetId( void )						{ return m_FileInfo.getAssetId(); }

	void 						setThumbId( VxGUID& assetId )			{ m_FileInfo.setThumbId( assetId ); }
	VxGUID&						getThumbId( void )						{ return m_FileInfo.getThumbId(); }

	bool						toggleIsShared( void );
	bool						toggleIsInLibrary( void );
	void						updateWidget( void );
	bool						shouldOpenFile( void );

	bool						isDirectory( void )						{ return m_FileInfo.isDirectory(); }

    QString						describeFileType();
    QString						describeFileLength();

	FileInfo&					getFileInfo( void )						{ return m_FileInfo; }
	bool						toAsssetInfo( AppCommon& myApp, AssetBaseInfo& assetInfo, bool* retIsNewAsset = nullptr );

protected:
	//=== vars ===//
    FileInfo					m_FileInfo;
    QWidget*					m_Widget{nullptr};
};
