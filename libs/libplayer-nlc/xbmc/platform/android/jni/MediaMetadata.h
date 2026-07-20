#pragma once
/*
 * Minimal Android JNI wrapper for android.media.MediaMetadata
 */

#include "JNIBase.h"
#include "Bitmap.h"

class CJNIMediaMetadata : public CJNIBase
{
public:
  CJNIMediaMetadata() : CJNIBase("android/media/MediaMetadata") {}
  CJNIMediaMetadata(const jni::jhobject& object) : CJNIBase(object) {}

  static constexpr const char* METADATA_KEY_DISPLAY_TITLE = "android.media.metadata.DISPLAY_TITLE";
  static constexpr const char* METADATA_KEY_TITLE = "android.media.metadata.TITLE";
  static constexpr const char* METADATA_KEY_DURATION = "android.media.metadata.DURATION";
  static constexpr const char* METADATA_KEY_DISPLAY_SUBTITLE = "android.media.metadata.DISPLAY_SUBTITLE";
  static constexpr const char* METADATA_KEY_ARTIST = "android.media.metadata.ARTIST";
  static constexpr const char* METADATA_KEY_ART = "android.media.metadata.ART";
};

class CJNIMediaMetadataBuilder : public CJNIBase
{
public:
  CJNIMediaMetadataBuilder() : CJNIBase("android/media/MediaMetadata$Builder")
  {
    m_object = jni::new_object(GetClassName());
    m_object.setGlobal();
  }

  CJNIMediaMetadataBuilder(const jni::jhobject& object) : CJNIBase(object) {}

  CJNIMediaMetadataBuilder putString(const std::string& key, const std::string& value)
  {
    return jni::call_method<jni::jhobject>(
        m_object,
        "putString",
        "(Ljava/lang/String;Ljava/lang/String;)Landroid/media/MediaMetadata$Builder;",
        jni::jcast<jni::jhstring>(key), jni::jcast<jni::jhstring>(value));
  }

  CJNIMediaMetadataBuilder putLong(const std::string& key, int64_t value)
  {
    return jni::call_method<jni::jhobject>(
        m_object,
        "putLong",
        "(Ljava/lang/String;J)Landroid/media/MediaMetadata$Builder;",
        jni::jcast<jni::jhstring>(key), static_cast<jlong>(value));
  }

  CJNIMediaMetadataBuilder putBitmap(const std::string& key, const CJNIBitmap& bitmap)
  {
    return jni::call_method<jni::jhobject>(
        m_object,
        "putBitmap",
        "(Ljava/lang/String;Landroid/graphics/Bitmap;)Landroid/media/MediaMetadata$Builder;",
        jni::jcast<jni::jhstring>(key), bitmap.get_raw());
  }

  CJNIMediaMetadata build() const
  {
    return jni::call_method<jni::jhobject>(
        m_object,
        "build",
        "()Landroid/media/MediaMetadata;");
  }
};
