/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


#ifndef __CJNIMESSAGEINTERFACE_HPP
#define __CJNIMESSAGEINTERFACE_HPP

#include <core/smileCommon.hpp>
#ifdef __ANDROID__
#define BUILD_COMPONENT_JniMessageInterface
#ifdef BUILD_COMPONENT_JniMessageInterface
#include <list>
#include <jni.h>
#include <core/smileComponent.hpp>

#define COMPONENT_DESCRIPTION_CJNIMESSAGEINTERFACE "Component for transferring smile component messages from native C to Java via JNI interface.\n"
#define COMPONENT_NAME_CJNIMESSAGEINTERFACE "cJniMessageInterface"

#undef class
class DLLEXPORT cJniMessageInterface : public cSmileComponent {
private:

protected:
  SMILECOMPONENT_STATIC_DECL_PR

  int debugPrintJson_;
  int sendMessagesInTick_;
  jstring JNIcallbackClassJstring_;
  JNIEnv *env_;
  JavaVM *jvm_;
  jobject * gClassLoader_;
  jmethodID * gFindClassMethod_;
  const char * JNIcallbackClass_;
  const char * JNIstringReceiverMethod_;
  std::list<cComponentMessage> in_buffer_;

  virtual void myFetchConfig() override;

  JNIEnv * AttachToThreadAndGetEnv();
  void DetachFromThread(JNIEnv * env);
  int sendTextToJava(JNIEnv * env, const char * str);
  int sendMessageToJava(cComponentMessage &msg, JNIEnv * env);

  virtual int sendMessagesFromQueue();

public:
  SMILECOMPONENT_STATIC_DECL

  cJniMessageInterface(const char *_name);
  void setJavaVmPointer(JavaVM * jvm) {
    jvm_ = jvm;
  }
  virtual int processComponentMessage(cComponentMessage *_msg) override;
  virtual int myFinaliseInstance() override;
  virtual eTickResult myTick(long long t) override;
  virtual ~cJniMessageInterface();
};

#endif  // BUILD_COMPONENT
#endif  // __ANDROID__
#endif  // __CJNIMESSAGEINTERFACE_HPP
