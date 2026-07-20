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

#ifdef TARGET_OS_ANDROID

#include <jni.h>

class VxJni
{
public:
    static bool                 initJavaBindings( JNIEnv* env );
    static void                 shutdownJavaBindings( JNIEnv* env );

    static JNIEnv*              getJavaEnv( void );
    static jobject              getActivity( JNIEnv* env = nullptr );
    static jobject              getApplicationContext( JNIEnv* env = nullptr );

    static bool                 hasPermission( const char* permissionName );
    static bool                 requestPermissionWithCallback( const char* permissionName,
                                                               int requestCode,
                                                               const char* callbackType );
    static bool                 requestPermission( const char* permissionName, int requestCode );

    static bool                 callStaticVoidWithAppContext( const char* className,
                                                              const char* methodName,
                                                              const char* methodSignature );
};

#endif // TARGET_OS_ANDROID

