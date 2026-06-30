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

#include "TestFileWav.h"

#include <vector>   

class TestFileWavMgr
{
public:
    TestFileWavMgr() = default;
    ~TestFileWavMgr() = default;

    static TestFileWavMgr&      getInstance( void )                         { static TestFileWavMgr instance; return instance; }

    void                        addTestFile( std::string testFileWav );

    std::vector<std::string>    getTestFileList( void ) const { return m_TestFilesList; }
    const std::vector<TestFileWav>& getTestFileWavList( void ) const { return m_TestFileWavList; }
    bool                        indexIsValid( int index ) const { return index >= 0 && index < m_TestFileWavList.size(); }
    TestFileWav&                getTestFileWav( int index ) { return m_TestFileWavList[index]; }


protected:
    std::vector<std::string>    m_TestFilesList;
    std::vector<TestFileWav>    m_TestFileWavList;
};
