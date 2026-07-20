//============================================================================
// Copyright (C) 2024 Brett R. Jones
//
// Code copyrighted by Brett R. Jones is under dual license similar to Ruby's license
// See file COPYING and LEGAL in root of the No Limit Connect project
//
// bjones.engineer@gmail.com
// https://nolimitconnect.com
//============================================================================
#pragma once

#include <CoreLib/VFile.h>

#include <string>

class VirtProviderFile
{
public:
	VirtProviderFile() = default;
	explicit VirtProviderFile( std::string fileName );
	~VirtProviderFile();

	bool                        openReadOnly( void );
	bool                        isOpen( void ) const;
	int64_t                     size( void ) const;
	int64_t                     read( char* buf, int64_t readLen );
	bool                        seek( int64_t pos );

    void                        closeFile( void );

	void						setError( int err )			{ m_Error = err; }
	int							getError( void )			{ return m_Error; }

	//=== vars ===//
	FILE*                       m_File{ nullptr };
	VFile*						m_VFile{ nullptr };
	std::string					m_FileName;
	std::string					m_FileMode;
	int64_t                     m_FileLen{ 0 };
	int							m_Error{ 0 };
};

