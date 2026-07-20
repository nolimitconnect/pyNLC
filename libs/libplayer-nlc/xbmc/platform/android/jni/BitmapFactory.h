#pragma once
/*
 * Minimal Android JNI wrapper for android.graphics.BitmapFactory
 */

#include "JNIBase.h"
#include "Bitmap.h"

class CJNIBitmapFactory
{
public:
  static CJNIBitmap decodeFile(const std::string& path)
  {
    return jni::call_static_method<jni::jhobject>(
        jni::find_class("android/graphics/BitmapFactory"),
        "decodeFile",
        "(Ljava/lang/String;)Landroid/graphics/Bitmap;",
        jni::jcast<jni::jhstring>(path));
  }
};
