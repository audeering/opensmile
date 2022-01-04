%module(directors="1") OpenSMILE
%include various.i
%include "arrays_java.i"

%{
#include <android/log.h>
#include "smileapi/SMILEapi.h"
%}

%immutable;
%ignore smileopt_t; 
%clearnodefaultctor;   // Re-enable default constructors

%pragma(java) jniclasscode=%{
  static {
    try {
      System.loadLibrary("opensmile_jni");
    } catch (UnsatisfiedLinkError e) {
      System.err.println("Native code library failed to load. \n" + e);
      System.exit(1);
    }
  }
%}
%pragma(java) jniclassclassmodifiers="class"

%pragma(java) moduleimports=%{
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Set;
%}

/*******NOTE: THE PARAMETERS SHOULD BE EXACTLY THE SAME WITH THE FUNCTION 
********      PARAMETERS: (const float *data, int nFrames)     **********/
/******* THIS IS FOR smile_extsource_write_data(float[])       **********/
%typemap(jtype) (const float *data, int nFrames) "float[]"
%typemap(jstype) (const float *data, int nFrames) "float[]"
%typemap(jni) (const float *data, int nFrames) "jfloatArray"
%typemap(javain) (const float *data, int nFrames) "$javainput"

%typemap(in,numinputs=1) (const float *data, int nFrames) {
  $1 = (jfloat *) JCALL2(GetFloatArrayElements, jenv, $input, NULL);
  const size_t sz = JCALL1(GetArrayLength, jenv, $input);
  $2 = sz;
}

%typemap(freearg) (const float *data, int nFrames) {
  // Or use  0 instead of ABORT to keep changes if it was a copy
  JCALL3(ReleaseFloatArrayElements, jenv, $input, (jfloat *)$1, JNI_ABORT);
}

%apply (const float *data, int nFrames) {(float *data, int nFrames) }


/*******NOTE: THE PARAMETERS SHOULD BE EXACTLY THE SAME WITH THE FUNCTION 
********      PARAMETERS: (const void *data, int length)       **********/
/******* THIS IS FOR smile_extaudiosource_write_data(byte[])   **********/
%typemap(jtype) (const void *data, int length) "byte[]"
%typemap(jstype) (const void *data, int length) "byte[]"
%typemap(jni) (const void *data, int length) "jbyteArray"
%typemap(javain) (const void *data, int length) "$javainput"

%typemap(in,numinputs=1) (const void *data, int length) {
  $1 = (jbyte *) JCALL2(GetByteArrayElements, jenv, $input, NULL);
  const size_t sz = JCALL1(GetArrayLength, jenv, $input);
  $2 = sz;
}

%typemap(freearg) (const void *data, int length) {
  // Or use  0 instead of ABORT to keep changes if it was a copy
  JCALL3(ReleaseByteArrayElements, jenv, $input, (jbyte *)$1, JNI_ABORT);
}

%apply (const void *data, int length) {(void *data, int length) }

/*******NOTE: THE PARAMETERS SHOULD BE EXACTLY THE SAME WITH THE FUNCTION 
********      PARAMETERS: (const float *data, int nFrames)     **********/
/******* THIS IS FOR onCalledExternalSinkCallback(float[],obj) **********/
%typemap(jtype) (const float *data, long vectorSize) "float[]"
%typemap(jstype) (const float *data, long vectorSize) "float[]"
%typemap(jni) (const float *data, long vectorSize) "jfloatArray"
%typemap(javain) (const float *data, long vectorSize) "$javainput"

%typemap(in,numinputs=1) (const float *data, long vectorSize) {
  $1 = (float *) JCALL2(GetFloatArrayElements, jenv, $input, NULL);
  const size_t sz = JCALL1(GetArrayLength, jenv, $input);
  $2 = sz;
}

%typemap(freearg) (const float *data, long vectorSize) {
  JCALL3(ReleaseFloatArrayElements, jenv, $input, $1, JNI_ABORT);
}

/*******NOTE: THE PARAMETERS SHOULD BE EXACTLY THE SAME WITH THE FUNCTION 
********      PARAMETERS: (int nOptions, const smileopt_t *options) *****/
/******* THIS IS FOR smile_initialize                               *****/
%typemap(jtype) (int nOptions, const smileopt_t *options) "String[]" 			            // THIS IS FOR THE JAVA class: OpenSMILEJNI
%typemap(jstype) (int nOptions, const smileopt_t *options) "HashMap<String,String>" 	// THIS IS FOR THE JAVA class: OpenSMILE
%typemap(jni) (int nOptions, const smileopt_t *options) "jobjectArray" 		            // THIS IS FOR THE JAVA class: OpenSMILE
%typemap(javain, pre="
    Set<String> keyset = nOptions.keySet();
    List<String> params = new ArrayList<> ();
    for (String key : keyset) {
      params.add(\"-\" + key);
      if (nOptions.get(key) != null)
        params.add(nOptions.get(key));
    }
    String[] paramsarray=params.toArray(new String[0]);"
    ) (int nOptions, const smileopt_t *options) "paramsarray"

%typemap(in,numinputs=1) (int nOptions, const smileopt_t *options) {			// THIS IS FOR THE C++ FUNC: smile_initialize
  int stringCount = JCALL1(GetArrayLength, jenv, $input);

  auto *opts = new smileopt_t[stringCount + 1];
  int nOpts = 0;
  int filled = 0;
  while (nOpts < stringCount) {
    auto str = (jstring) JCALL2(GetObjectArrayElement, jenv, $input, nOpts);
    char *paramName =  (char *) JCALL2(GetStringUTFChars, jenv, str, nullptr);
    if (paramName[0] == '-')
      opts[filled].name = paramName + 1;
    str = (jstring) JCALL2(GetObjectArrayElement, jenv, $input, nOpts + 1);
    char *paramValueCandidate = (char *) JCALL2(GetStringUTFChars, jenv, str, nullptr);
    if (paramValueCandidate[0] != '-') {
      opts[filled].value = paramValueCandidate;
      nOpts += 1;
    }
    nOpts += 1;
    filled += 1;
  }

  for (int iOpts = 0; iOpts < filled; iOpts++)
    __android_log_print(ANDROID_LOG_INFO, "opensmile params:", "%s -> %s", opts[iOpts].name,
                        opts[iOpts].value);

  $1=filled;
  $2=opts;
}

/*******NOTE: THE PARAMETERS SHOULD BE EXACTLY THE SAME WITH THE FUNCTION 
********      PARAMETERS: (void *)                             **********/
/******* THIS IS FOR void* to long                             **********/
%apply jlong { void * };
%typemap(in) void * {
  $1 = (void*)($input);
}
%typemap(out) void * {
  $result = (jlong)($1);
}

%pragma(java) jniclassclassmodifiers="class"
%pragma(java) moduleclassmodifiers="class"

%include "../../include/smileapi/SMILEapi.h"

/**************************************************************/
//CALLBACKS: CallbackExternalMessageInterfaceJson
%feature("director") CallbackExternalMessageInterfaceJson;
%inline %{

static bool callbackfunjson(const char*msg,void*param);

class CallbackExternalMessageInterfaceJson {
public:
  CallbackExternalMessageInterfaceJson() = default;

  virtual ~CallbackExternalMessageInterfaceJson() = default;

  virtual bool onCalledExternalMessageInterfaceJsonCallback(const char *msg) = 0;

  ExternalMessageInterfaceJsonCallback createExternalMessageInterfaceJsonCallback() {
    return callbackfunjson;
  }
};

static bool callbackfunjson(const char*msg, void*param){
  return ((CallbackExternalMessageInterfaceJson*)param)->onCalledExternalMessageInterfaceJsonCallback(msg);
}
%}

//CALLBACKS: CallbackExternalSink
%typemap(directorin, descriptor="[F") (const float *data, long vectorSize) {
  jfloatArray jb = (jenv)->NewFloatArray($2);
  (jenv)->SetFloatArrayRegion(jb, 0, $2, (jfloat *)$1);
  $input = jb;
}
%typemap(directorargout) (const float *data, long vectorSize) 
%{(jenv)->GetFloatArrayRegion($input, 0, $2, (jfloat *)$1); %} 

%feature("director") CallbackExternalSink;
%inline %{
static bool callbackfunextsing(const float *data, long vectorSize, void *param);
class CallbackExternalSink {
public:
  CallbackExternalSink() = default;

  virtual ~CallbackExternalSink() = default;

  /* This method will be called to in Java World by the overridden Java implementation */
  virtual bool onCalledExternalSinkCallback(const float *data, long vectorSize) = 0;

  /* This method will be used from Java World to create a std::function<> object to the
     callback in our native API */
  ExternalSinkCallback createExternalSinkCallback() {
    return callbackfunextsing;
  }
};

static bool callbackfunextsing(const float *data, long vectorSize, void *param){
  return ((CallbackExternalSink*)param)->onCalledExternalSinkCallback(data,vectorSize);
}
%}

// ONE MORE THING TO DO IS TO REPLACE THE FOLLOWING IN OpenSMILEJNI.java
// (data == 0) ? null : new SWIGTYPE_p_float(data, false)
// WITH
// data
