#pragma once
/*
 * Minimal Android JNI wrapper for android.view.SurfaceHolder
 */

#include "JNIBase.h"
#include "Surface.h"

class CJNISurfaceHolder : public CJNIBase
{
public:
  CJNISurfaceHolder() : CJNIBase("android/view/SurfaceHolder") {}
  CJNISurfaceHolder(const jni::jhobject& object) : CJNIBase(object) {}

  CJNISurface getSurface() const
  {
    return jni::call_method<jni::jhobject>(
        m_object,
        "getSurface",
        "()Landroid/view/Surface;");
  }
};

class CJNISurfaceHolderCallback
{
public:
  virtual ~CJNISurfaceHolderCallback() = default;
  virtual void surfaceChanged(CJNISurfaceHolder holder, int format, int width, int height) = 0;
  virtual void surfaceCreated(CJNISurfaceHolder holder) = 0;
  virtual void surfaceDestroyed(CJNISurfaceHolder holder) = 0;
};
