#pragma once
//============================================================================
// Copyright (C) 2026 Brett R. Jones
//
// Code copyrighted by Brett R. Jones is under dual license similar to Ruby's license
// See file COPYING and LEGAL in root of the No Limit Connect project
//
// bjones.engineer@gmail.com
// https://nolimitconnect.com
//============================================================================

#include <string>
#include <vector>
#include <stdint.h>

struct VxAndroidPathInfo
{
    std::string                 m_FileName;
    std::string                 m_FileNameAndPath;
    int64_t                     m_FileLength{ 0 };
    bool                        m_IsDirectory{ false };
    bool                        m_IsReadable{ false };
    bool                        m_IsExecutable{ false };
};

namespace VxAndroid
{
    bool                        requestPermission( const char* permissionName );

    bool                        directoryExists( const char* dirPath );
    bool                        getPathInfo( const char* fileNameAndPath, VxAndroidPathInfo& pathInfo );

    int                         listDirectory( const char* srcDir, std::vector<VxAndroidPathInfo>& fileList );
}
