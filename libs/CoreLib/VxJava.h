#pragma once
//============================================================================
// Copyright (C) 2018 Brett R. Jones
//
// Code copyrighted by Brett R. Jones is under dual license similar to Ruby's license
// See file COPYING and LEGAL in root of the No Limit Connect project
//
// bjones.engineer@gmail.com
// https://nolimitconnect.com
//============================================================================
#ifdef TARGET_OS_ANDROID

# include <CoreLib/VxMutex.h>
# include <jni.h>

class VxJava
{
public:
    VxJava() = default;

    static JavaVM *             getJavaVM( void )                       { return m_JavaVM; }
    static JNIEnv *             getJavaEnv( void );


    static JavaVM *             m_JavaVM;
};


VxJava& GetJavaEnvCache();

JavaVM* GetJavaVM();

#endif // TARGET_OS_ANDROID

