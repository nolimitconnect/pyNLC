//============================================================================
// Copyright (C) 2024 Brett R. Jones
//
// Code copyrighted by Brett R. Jones is under dual license similar to Ruby's license
// See file COPYING and LEGAL in root of the No Limit Connect project
//
// bjones.engineer@gmail.com
// https://nolimitconnect.com
//============================================================================

#include "VirtProviderFile.h"

#include <CoreLib/VxDebug.h>
#include <CoreLib/VxFileUtil.h>

#include <errno.h>
#include <stdio.h>
#include <utility>

#if defined(TARGET_OS_ANDROID)
#include <CoreLib/VxJni.h>
#include <jni.h>
#endif // defined(TARGET_OS_ANDROID)

#if defined(TARGET_OS_ANDROID)
namespace
{
bool HasAndClearPendingException( JNIEnv* env, const char* funcName )
{
	if( nullptr == env || !env->ExceptionCheck() )
	{
		return false;
	}

	env->ExceptionDescribe();
	env->ExceptionClear();
	LogMsg( LOG_ERROR, "%s JNI exception", funcName );
	return true;
}

jobject ParseUri( JNIEnv* env, const char* uriString )
{
	jclass uriClass = env->FindClass( "android/net/Uri" );
	if( nullptr == uriClass || HasAndClearPendingException( env, __func__ ) )
	{
		return nullptr;
	}

	jmethodID parseMethod = env->GetStaticMethodID( uriClass,
													 "parse",
													 "(Ljava/lang/String;)Landroid/net/Uri;" );
	if( nullptr == parseMethod || HasAndClearPendingException( env, __func__ ) )
	{
		env->DeleteLocalRef( uriClass );
		return nullptr;
	}

	jstring jUriString = env->NewStringUTF( uriString );
	if( nullptr == jUriString )
	{
		env->DeleteLocalRef( uriClass );
		return nullptr;
	}

	jobject uriObj = env->CallStaticObjectMethod( uriClass, parseMethod, jUriString );
	const bool hadException = HasAndClearPendingException( env, __func__ );
	env->DeleteLocalRef( jUriString );
	env->DeleteLocalRef( uriClass );
	return hadException ? nullptr : uriObj;
}

}
#endif // defined(TARGET_OS_ANDROID)

namespace
{
int64_t DetermineFileLengthFromOpenHandle( FILE* fileHandle )
{
	if( nullptr == fileHandle )
	{
		return 0;
	}

#if defined(TARGET_OS_WINDOWS)
	const __int64 originalPos = _ftelli64( fileHandle );
	if( originalPos < 0 )
	{
		return 0;
	}

	if( 0 != _fseeki64( fileHandle, 0, SEEK_END ) )
	{
		return 0;
	}

	const __int64 endPos = _ftelli64( fileHandle );
	(void)_fseeki64( fileHandle, originalPos, SEEK_SET );
	return endPos > 0 ? static_cast<int64_t>( endPos ) : 0;
#else
	const off_t originalPos = ftello( fileHandle );
	if( originalPos < 0 )
	{
		return 0;
	}

	if( 0 != fseeko( fileHandle, 0, SEEK_END ) )
	{
		return 0;
	}

	const off_t endPos = ftello( fileHandle );
	(void)fseeko( fileHandle, originalPos, SEEK_SET );
	return endPos > 0 ? static_cast<int64_t>( endPos ) : 0;
#endif
}
}

//============================================================================
VirtProviderFile::VirtProviderFile( std::string fileName )
	: m_FileName( std::move( fileName ) )
{
}

//============================================================================
VirtProviderFile::~VirtProviderFile()
{
	closeFile();
}

//============================================================================
bool VirtProviderFile::openReadOnly( void )
{
	closeFile();

#if defined(TARGET_OS_ANDROID)
	if( VxFileUtil::fileIsProviderFile( m_FileName.c_str() ) )
	{
		JNIEnv* env = VxJni::getJavaEnv();
		if( nullptr == env )
		{
			m_Error = EACCES;
			return false;
		}

		jobject appContext = VxJni::getApplicationContext( env );
		if( nullptr == appContext )
		{
			m_Error = EACCES;
			return false;
		}

		jclass contextClass = env->GetObjectClass( appContext );
		if( nullptr == contextClass || HasAndClearPendingException( env, __func__ ) )
		{
			env->DeleteLocalRef( appContext );
			m_Error = EACCES;
			return false;
		}

		jmethodID getResolverMethod = env->GetMethodID( contextClass,
														 "getContentResolver",
														 "()Landroid/content/ContentResolver;" );
		if( nullptr == getResolverMethod || HasAndClearPendingException( env, __func__ ) )
		{
			env->DeleteLocalRef( contextClass );
			env->DeleteLocalRef( appContext );
			m_Error = EACCES;
			return false;
		}

		jobject resolverObj = env->CallObjectMethod( appContext, getResolverMethod );
		if( nullptr == resolverObj || HasAndClearPendingException( env, __func__ ) )
		{
			env->DeleteLocalRef( contextClass );
			env->DeleteLocalRef( appContext );
			m_Error = EACCES;
			return false;
		}

		jobject uriObj = ParseUri( env, m_FileName.c_str() );
		if( nullptr == uriObj )
		{
			env->DeleteLocalRef( resolverObj );
			env->DeleteLocalRef( contextClass );
			env->DeleteLocalRef( appContext );
			m_Error = EACCES;
			return false;
		}

		jclass resolverClass = env->GetObjectClass( resolverObj );
		if( nullptr == resolverClass || HasAndClearPendingException( env, __func__ ) )
		{
			env->DeleteLocalRef( uriObj );
			env->DeleteLocalRef( resolverObj );
			env->DeleteLocalRef( contextClass );
			env->DeleteLocalRef( appContext );
			m_Error = EACCES;
			return false;
		}

		jmethodID openFdMethod = env->GetMethodID( resolverClass,
												   "openFileDescriptor",
												   "(Landroid/net/Uri;Ljava/lang/String;)Landroid/os/ParcelFileDescriptor;" );
		if( nullptr == openFdMethod || HasAndClearPendingException( env, __func__ ) )
		{
			env->DeleteLocalRef( resolverClass );
			env->DeleteLocalRef( uriObj );
			env->DeleteLocalRef( resolverObj );
			env->DeleteLocalRef( contextClass );
			env->DeleteLocalRef( appContext );
			m_Error = EACCES;
			return false;
		}

		jstring jReadMode = env->NewStringUTF( "r" );
		if( nullptr == jReadMode || HasAndClearPendingException( env, __func__ ) )
		{
			env->DeleteLocalRef( resolverClass );
			env->DeleteLocalRef( uriObj );
			env->DeleteLocalRef( resolverObj );
			env->DeleteLocalRef( contextClass );
			env->DeleteLocalRef( appContext );
			m_Error = EACCES;
			return false;
		}

		jobject pfdObj = env->CallObjectMethod( resolverObj, openFdMethod, uriObj, jReadMode );
		const bool openHadException = HasAndClearPendingException( env, __func__ );
		env->DeleteLocalRef( jReadMode );
		if( nullptr == pfdObj || openHadException )
		{
			env->DeleteLocalRef( resolverClass );
			env->DeleteLocalRef( uriObj );
			env->DeleteLocalRef( resolverObj );
			env->DeleteLocalRef( contextClass );
			env->DeleteLocalRef( appContext );
			m_Error = EACCES;
			return false;
		}

		jclass pfdClass = env->GetObjectClass( pfdObj );
		if( nullptr == pfdClass || HasAndClearPendingException( env, __func__ ) )
		{
			env->DeleteLocalRef( pfdObj );
			env->DeleteLocalRef( resolverClass );
			env->DeleteLocalRef( uriObj );
			env->DeleteLocalRef( resolverObj );
			env->DeleteLocalRef( contextClass );
			env->DeleteLocalRef( appContext );
			m_Error = EACCES;
			return false;
		}

		jmethodID detachFdMethod = env->GetMethodID( pfdClass, "detachFd", "()I" );
		if( nullptr == detachFdMethod || HasAndClearPendingException( env, __func__ ) )
		{
			env->DeleteLocalRef( pfdClass );
			env->DeleteLocalRef( pfdObj );
			env->DeleteLocalRef( resolverClass );
			env->DeleteLocalRef( uriObj );
			env->DeleteLocalRef( resolverObj );
			env->DeleteLocalRef( contextClass );
			env->DeleteLocalRef( appContext );
			m_Error = EACCES;
			return false;
		}

		jint fd = env->CallIntMethod( pfdObj, detachFdMethod );
		const bool fdHadException = HasAndClearPendingException( env, __func__ );

		env->DeleteLocalRef( pfdClass );
		env->DeleteLocalRef( pfdObj );
		env->DeleteLocalRef( resolverClass );
		env->DeleteLocalRef( uriObj );
		env->DeleteLocalRef( resolverObj );
		env->DeleteLocalRef( contextClass );
		env->DeleteLocalRef( appContext );

		if( fdHadException || fd < 0 )
		{
			m_Error = EACCES;
			return false;
		}

		m_File = fdopen( fd, "rb" );
		if( nullptr == m_File )
		{
			m_Error = errno;
			return false;
		}

		m_FileLen = DetermineFileLengthFromOpenHandle( m_File );
		return true;
	}
#endif // defined(TARGET_OS_ANDROID)

	m_File = fopen( m_FileName.c_str(), "rb" );
	if( nullptr == m_File )
	{
		m_Error = errno;
		return false;
	}

	m_FileLen = DetermineFileLengthFromOpenHandle( m_File );
	return true;
}

//============================================================================
bool VirtProviderFile::isOpen( void ) const
{
	return nullptr != m_File;
}

//============================================================================
int64_t VirtProviderFile::size( void ) const
{
	return m_FileLen;
}

//============================================================================
int64_t VirtProviderFile::read( char* buf, int64_t readLen )
{
	if( nullptr == m_File || nullptr == buf || readLen <= 0 )
	{
		return -1;
	}

	return static_cast<int64_t>( fread( buf, 1, static_cast<size_t>( readLen ), m_File ) );
}

//============================================================================
bool VirtProviderFile::seek( int64_t pos )
{
	if( nullptr == m_File )
	{
		return false;
	}

#if defined(TARGET_OS_WINDOWS)
	return 0 == _fseeki64( m_File, pos, SEEK_SET );
#else
	return 0 == fseeko( m_File, pos, SEEK_SET );
#endif
}

//============================================================================
void VirtProviderFile::closeFile( void )
{
	if( m_File )
	{
		fclose( m_File );
		m_File = nullptr;
	}

	delete m_VFile;
	m_VFile = nullptr;
	m_FileLen = 0;
}

