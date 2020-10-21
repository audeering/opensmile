/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/



#include <android/jniMessageInterface.hpp>

#ifdef __ANDROID__
#ifdef BUILD_COMPONENT_JniMessageInterface

#define MODULE "cJniMessageInterface"

SMILECOMPONENT_STATICS(cJniMessageInterface)

SMILECOMPONENT_REGCOMP(cJniMessageInterface)
{
  SMILECOMPONENT_REGCOMP_INIT
  scname = COMPONENT_NAME_CJNIMESSAGEINTERFACE;
  sdescription = COMPONENT_DESCRIPTION_CJNIMESSAGEINTERFACE;
  SMILECOMPONENT_CREATE_CONFIGTYPE
  SMILECOMPONENT_IFNOTREGAGAIN(
    ct->setField("sendMessagesInTick", "1/0 enable/disable sending of messages synchronized in tick(). "
        "If set to 0, a background thread will be started which will send messages from the queue in the "
        "background (NOT YET IMPLEMENTED - only in tick is supported now).", 1);
    ct->setField("JNIcallbackClass", "Fully qualified Java name of class in APP which handles callbacks. "
        "Must be changed to the app namespace and domain.", "java/com/audeering/<yourappname>/SmileJNIcallbacks");
    ct->setField("JNIstringReceiverMethod", "Name of method which receives string messages in JNIcallbackClass. "
        "Default should not need to be changed, if class wasn't changed.", "receiveText");
    ct->setField("debugPrintJson", "1 = debug print to smile log the formatted json before sending.", 0);
  )
  SMILECOMPONENT_MAKEINFO(cJniMessageInterface);
}


SMILECOMPONENT_CREATE(cJniMessageInterface)

//-----
cJniMessageInterface::cJniMessageInterface(const char *_name):
  cSmileComponent(_name), jvm_(NULL), sendMessagesInTick_(1),
  env_(NULL) { }

void cJniMessageInterface::myFetchConfig() {
  sendMessagesInTick_ = getInt("sendMessagesInTick");
  JNIstringReceiverMethod_ = getStr("JNIstringReceiverMethod");
  JNIcallbackClass_ = getStr("JNIcallbackClass");
  jvm_ = (JavaVM *)getExternalPointer("JavaVM");
  gFindClassMethod_ = (jmethodID *)getExternalPointer("FindClassMethod");
  gClassLoader_ = (jobject *)getExternalPointer("ClassLoader");
  debugPrintJson_ = getInt("debugPrintJson");
}

int cJniMessageInterface::myFinaliseInstance() {
  if (!sendMessagesInTick_) {
    // TODO: create background thread!
    SMILE_IERR(1, "background message thread not yet supported! Message sending through JNI disabled.");
  }
  return 1;
}

// JNI calls for exchange of smile messages

JNIEnv * cJniMessageInterface::AttachToThreadAndGetEnv() {
 JNIEnv * env = NULL;
 if (jvm_ == NULL) {
   SMILE_IERR(2, "no JVM pointer! Cannot access Java VM to get environment and exchange JNI data.");
   return env;
 }
 // double check it's all ok
 int getEnvStat = jvm_->GetEnv((void **)&env, JNI_VERSION_1_6);
 if (getEnvStat == JNI_EDETACHED) {
   SMILE_IMSG(4, "GetEnv: not attached");
   if (jvm_->AttachCurrentThread((JNIEnv **)&env, NULL) != 0) {
     SMILE_IERR(2, "GetEnv: failed to attach to current thread!");
   }
 } else if (getEnvStat == JNI_OK) {
   //
 } else if (getEnvStat == JNI_EVERSION) {
   SMILE_IERR(2, "GetEnv: version not supported");
 }
 return env;
}

void cJniMessageInterface::DetachFromThread(JNIEnv * env) {
  if (env == NULL || jvm_ == NULL)
   return;
  if (env->ExceptionCheck()) {
    env->ExceptionDescribe();
  }
  jvm_->DetachCurrentThread();
}

/*
 *
 * JNIEnv * env = getEnv();
    //replace with one of your classes in the line below
    jclass randomClass = env->FindClass("com/audeering/testapp01/SmileJNI");
    jclass classClass = env->GetObjectClass(randomClass);
    jclass classLoaderClass = env->FindClass("java/lang/ClassLoader");
    jmethodID getClassLoaderMethod = env->GetMethodID(classClass, "getClassLoader",
                                             "()Ljava/lang/ClassLoader;");
    gClassLoader = env->CallObjectMethod(randomClass, getClassLoaderMethod);
    gFindClassMethod = env->GetMethodID(classLoaderClass, "findClass",
                                    "(Ljava/lang/String;)Ljava/lang/Class;");
    return JNI_VERSION_1_6;
}

 *
 */

/*
jclass cJniMessageInterface::findClass(JNIEnv *env, const char* name) {
    return static_cast<jclass>(env->CallObjectMethod(gClassLoader, gFindClassMethod, getEnv()->NewStringUTF(name)));
}*/

int cJniMessageInterface::sendTextToJava(JNIEnv *env, const char * str) {
  if (str == NULL || env == NULL)
    return 0;
  SMILE_IDBG(2, "resolving:0 JNIcallbackClass_ %s", JNIcallbackClass_);
  jclass app = static_cast<jclass>(env->CallObjectMethod(*gClassLoader_, *gFindClassMethod_, JNIcallbackClassJstring_));
  jmethodID sendDataToJava = env->GetStaticMethodID(app, JNIstringReceiverMethod_, "(Ljava/lang/String;)V");  // "([B)V"
  jstring jstr = env->NewStringUTF(str);
  env->CallStaticVoidMethod(app, sendDataToJava, jstr);
  env->DeleteLocalRef(jstr);
  env->DeleteLocalRef(app);
  return 1;
}

int cJniMessageInterface::sendMessageToJava(cComponentMessage &msg, JNIEnv *env) {
  char * json = msg.serializeToJson();
  if (debugPrintJson_) {
    SMILE_IMSG(1, "JSON message:\n %s\n", json);
  }
  return sendTextToJava(env, json);
}

int cJniMessageInterface::sendMessagesFromQueue() {
  // Sends out all messages in the queue, and remove from queue.
  int ret = 0;
  for (std::list<cComponentMessage>::iterator it = in_buffer_.begin(); it != in_buffer_.end(); ) {
    if (env_ != NULL) {
      sendMessageToJava(*it, env_);
      ret = 1;
    }
    if (it->custData != NULL)
      free(it->custData);
    if (it->custData2 != NULL)
      free(it->custData2);
    it = in_buffer_.erase(it);
  }
  return ret;
}

eTickResult cJniMessageInterface::myTick(long long t) {
  if (isEOI()) {
    SMILE_IMSG(2, "detaching from thread due to EOI");
    if (env_ != NULL) {
      DetachFromThread(env_);
      env_ = NULL;
    }
  } else {
    if (env_ == NULL) {
      SMILE_IMSG(2, "attaching to thread (env is NULL)");
      env_ = AttachToThreadAndGetEnv();
      if (env_ != NULL) {
        JNIcallbackClassJstring_ = env_->NewStringUTF(JNIcallbackClass_);
      }
    }
    if (sendMessagesInTick_) {
      return sendMessagesFromQueue() ? TICK_SUCCESS : TICK_INACTIVE;
    }
  }
  return TICK_INACTIVE;
}

int cJniMessageInterface::processComponentMessage(cComponentMessage *msg) {
  // We copy the component message, as all memory referenced in the
  // message will be invalid after we call "return" here. 
  cComponentMessage c;
  memcpy(&c, msg, sizeof(cComponentMessage));
  if (msg->custData != NULL) {
    if (c.custDataType == CUSTDATA_TEXT) {
      c.custData = malloc(sizeof(char) * (strlen((char*)msg->custData) + 1));
      strcpy((char *)c.custData, (char *)msg->custData);
    } else {
      c.custData = malloc(msg->custDataSize + 1);
      memcpy(c.custData, msg->custData, msg->custDataSize);
    }
  }
  if (msg->custData2 != NULL) {
    if (c.custDataType == CUSTDATA_TEXT) {
      c.custData2 = malloc(sizeof(char) * (strlen((char*)msg->custData2) + 1));
      strcpy((char *)c.custData2, (char *)msg->custData2);
    } else {
      c.custData2 = malloc(msg->custData2Size + 1);
      memcpy(c.custData2, msg->custData2, msg->custData2Size);
    }
  }
  // Now we save the message in the FIFO queue.
  in_buffer_.push_back(c);
  return 1;
}

cJniMessageInterface::~cJniMessageInterface() {
  // Free the message queue:
  for (std::list<cComponentMessage>::iterator it = in_buffer_.begin(); it != in_buffer_.end(); ) {
    if (it->custData != NULL)
      free(it->custData);
    if (it->custData2 != NULL)
      free(it->custData2);
    it = in_buffer_.erase(it);
  }
  // JNIEnv *env = AttachToThreadAndGetEnv();
  if (env_ != NULL) {
    //env_->DeleteLocalRef(JNIcallbackClassJstring_);
    //DetachFromThread(env_);
  }
}

#endif  // BUILD_COMPONENT
#endif  // __ANDROID__

