#pragma once
/*
 * Minimal Android JNI wrapper for android.media.session.PlaybackState
 */

#include "JNIBase.h"

class CJNIPlaybackState : public CJNIBase
{
public:
  CJNIPlaybackState() : CJNIBase("android/media/session/PlaybackState") {}
  CJNIPlaybackState(const jni::jhobject& object) : CJNIBase(object) {}

  static constexpr int STATE_NONE = 0;
  static constexpr int STATE_STOPPED = 1;
  static constexpr int STATE_PAUSED = 2;
  static constexpr int STATE_PLAYING = 3;
};

class CJNIPlaybackStateBuilder : public CJNIBase
{
public:
  CJNIPlaybackStateBuilder() : CJNIBase("android/media/session/PlaybackState$Builder")
  {
    m_object = jni::new_object(GetClassName());
    m_object.setGlobal();
  }

  CJNIPlaybackStateBuilder(const jni::jhobject& object) : CJNIBase(object) {}

  CJNIPlaybackStateBuilder setState(int state, int64_t position, float speed, int64_t updateTime)
  {
    return jni::call_method<jni::jhobject>(
        m_object,
        "setState",
        "(IJFJ)Landroid/media/session/PlaybackState$Builder;",
        static_cast<jint>(state), static_cast<jlong>(position), static_cast<jfloat>(speed), static_cast<jlong>(updateTime));
  }

  CJNIPlaybackStateBuilder setActions(int64_t actions)
  {
    return jni::call_method<jni::jhobject>(
        m_object,
        "setActions",
        "(J)Landroid/media/session/PlaybackState$Builder;",
        static_cast<jlong>(actions));
  }

  CJNIPlaybackState build() const
  {
    return jni::call_method<jni::jhobject>(
        m_object,
        "build",
        "()Landroid/media/session/PlaybackState;");
  }
};
