/*F***************************************************************************
 * This file is part of openSMILE.
 *
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


/*  openSMILE component:

    Source for external data injection. Creates
    a dataMemory level, and provides an externally
    callable function which can write data matrices
    to the datamemory asynchronously.

    Provides another function which converts audio data
    from several PCM formats to float -1 to +1.

*/

#if defined(__IOS__) && !defined(__IOS_WATCH__)

#include <ios/coreAudioSource.hpp>
#include <core/componentManager.hpp>
#define MODULE "cCoreAudioSource"

SMILECOMPONENT_STATICS(cCoreAudioSource)

SMILECOMPONENT_REGCOMP(cCoreAudioSource)
{
    SMILECOMPONENT_REGCOMP_INIT

        scname = COMPONENT_NAME_CCOREAUDIOSOURCE;
    sdescription = COMPONENT_DESCRIPTION_CCOREAUDIOSOURCE;

    // we inherit cDataSource configType and extend it:
    SMILECOMPONENT_INHERIT_CONFIGTYPE("cExternalAudioSource")

        SMILECOMPONENT_IFNOTREGAGAIN(
                                     ct->setField("period",(const char*)NULL,0,0,0);
                                     ct->setField("sampleRate","The sampling rate of the external audio input",16000);
                                     ct->setField("channels","The number of channels of the external audio input",1);
                                     ct->setField("nBits","The number of bits per sample and channel of the external audio input",16);
                                     ct->setField("nBPS","The number of bytes per sample and channel of the external audio input (0=determine automatically from nBits)",0,0,0);
                                     ct->setField("blocksize","The maximum size of audio sample buffers that can be passed to this component at once in samples (per channel, overwrites blocksize_sec, if set)", 0, 0, 0);
                                     ct->setField("blocksize_sec","The maximum size of sample buffers that can be passed to this component at once in seconds.", 0.05);
                                     ct->setField("fieldName", "Name of dataMemory field data is written to.", "pcm");
                                     ct->setField("recordBufferSec", "Set the size of buffur of recording in seconds", 0.01);
                                     )
        SMILECOMPONENT_MAKEINFO(cCoreAudioSource);
}

SMILECOMPONENT_CREATE(cCoreAudioSource)

//-----

cCoreAudioSource::cCoreAudioSource(const char *_name) :
cExternalAudioSource(_name),
    recorder(NULL)
{
}

void cCoreAudioSource::myFetchConfig()
{
    cExternalAudioSource::myFetchConfig();
    recordBufferSec = getDouble("recordBufferSec");
    if(recordBufferSec <= 0.0) recordBufferSec = 0.01;
}

eTickResult cCoreAudioSource::myTick(long long t)
{
    if(NULL == recorder) {
        recorder = new cIosRecorder(getSampleRate(), getChannels(), getNBPS(), recordBufferSec);
        recorder->delegate = this;
        recorder->start();
    }

    return cExternalAudioSource::myTick(t);
}

int cCoreAudioSource::pauseEvent()
{
    if(NULL != recorder) {
        recorder->pause();
    }
    return 1;
}


void cCoreAudioSource::resumeEvent()
{
    if(NULL != recorder) {
        recorder->resume();
    }
}


void cCoreAudioSource::setExternalEOI()
{
    if(NULL != recorder) {
        recorder->stop();
    }
    cExternalAudioSource::setExternalEOI();
}


cCoreAudioSource::~cCoreAudioSource()
{
    if(NULL != recorder) {
        recorder->stop();
        delete recorder;
        recorder = NULL;
    }
}

/**
   Recorder started

   @remark cIosRecorder call this right after audio queue started succefully.
   @param externalSource cIosRecorder instance..
*/
void cCoreAudioSource::onIosExternalSourceStarted(void* externalSource)
{
}

/**
   Recorder stopped

   @remark cIosRecorder call this right after audio queue stop succefully.
   @param externalSource cIosRecorder instance..
*/
void cCoreAudioSource::onIosExternalSourceStopped(void* externalSource)
{
}

/**
   Error in recorder

   @remark There is an error in audio queue.
   @param externalSource cIosRecorder instance..
*/
void cCoreAudioSource::onIosExternalSourceFailed(void* externalSource)
{
    setExternalEOI();
}

/**
   Received recording data from cIosRecorder

   @remark cIosRecorder call this when audio queue fill a buffer.
   @param externalSource cIosRecorder instance..
   @param buf Recording data from audio queue.
   @param size Size of the data in bytes.
*/
void cCoreAudioSource::onIosExternalSourceReceived(void* externalSource, void* buf, UInt32 size)
{
    if(NULL != buf && 0 < size) {
        writeData((uint8_t*)buf, (int)size);
    }
}

#endif  // #if defined(__IOS__) && !defined(__IOS_WATCH__)
