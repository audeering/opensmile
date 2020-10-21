/*F***************************************************************************
 * This file is part of openSMILE.
 *
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


/*  openSMILE component:

    iosRecorder : Recording audio in iOS platform.

    This uses Audio Queue api for recording. This sends the event and the data
    to the delegate.

*/

#if defined(__IOS__) && !defined(__IOS_WATCH__)

#include <ios/iosRecorder.hpp>
#include <core/smileCommon.hpp>

#define MODULE "cIosRecorder"

/**
   Callback function from audio queue

   @remark Passing the data to the delegate.
*/
static void HandleInputBuffer(
                              void                                 *pRecorder,
                              AudioQueueRef                        inAQ,
                              AudioQueueBufferRef                  inBuffer,
                              const AudioTimeStamp                 *inStartTime,
                              UInt32                               inNumPackets,
                              const AudioStreamPacketDescription   *inPacketDesc) {

    cIosRecorder *recorder = (cIosRecorder*)pRecorder;
    if (NULL != recorder) {
        if (inNumPackets == 0 &&
            recorder->mDataFormat.mBytesPerPacket != 0) {
            inNumPackets = inBuffer->mAudioDataByteSize / recorder->mDataFormat.mBytesPerPacket;
        }

        if (NULL != recorder->delegate) {
            recorder->delegate->onIosExternalSourceReceived(recorder, inBuffer->mAudioData, inBuffer->mAudioDataByteSize);
        }

        recorder->mCurrentPacket += inNumPackets;

        if (!recorder->mIsRunning) {
            return;
        }

        AudioQueueEnqueueBuffer(recorder->mQueue, inBuffer, 0, NULL);
    }
}


cIosRecorder::cIosRecorder(UInt32 rate, UInt32 chn, UInt32 size, Float64 bufsec) :
    mQueue(NULL),
    bufferByteSize(0),
    mCurrentPacket(0),
    mIsRunning(false),
    sampleRate(0),
    channels(0),
    sampleSize(0),
    bufferSec(0.0),
    delegate(NULL)
{
    sampleRate = rate;
    channels = chn;
    sampleSize = size;
    bufferSec = bufsec;
}


cIosRecorder::~cIosRecorder(){
    if(true == mIsRunning) {
        stop();
    }
}


void cIosRecorder::start() {
    prepareRecording();
    startAudioQueue();
}

void cIosRecorder::pause() {
    AudioQueuePause(mQueue);
}


void cIosRecorder::resume() {
    AudioQueueStart(mQueue, NULL);
}

void cIosRecorder::stop() {
    stopAudioQueue();
    cleaningUpRecording();
}


void cIosRecorder::prepareRecording() {
    //
    // 1. mDataFormat
    // AudioStreamBasicDescription mDataFormat
    //
    mDataFormat.mFormatID         = kAudioFormatLinearPCM;
    mDataFormat.mSampleRate       = sampleRate; // 16000.0;
    mDataFormat.mChannelsPerFrame	= channels; // 1;
    mDataFormat.mBitsPerChannel		= sampleSize * 8; // 16;
    mDataFormat.mBytesPerPacket		=
        mDataFormat.mBytesPerFrame = mDataFormat.mChannelsPerFrame * sampleSize;
    mDataFormat.mFramesPerPacket	= 1;
    mDataFormat.mFormatFlags      =
        kAudioFormatFlagIsSignedInteger | kAudioFormatFlagIsPacked;
    AudioFileTypeID fileType        = kAudioFileWAVEType;

    //
    // 2. mQueue
    // AudioQueueRef mQueue
    //
    AudioQueueNewInput (
                        &mDataFormat,
                        HandleInputBuffer,
                        this,
                        NULL,
                        kCFRunLoopCommonModes,
                        0,
                        &mQueue
                        );

    //
    // 3. Fill out the mDataFormat(AudioStreamBasicDescription struceture) more completely.
    //
    UInt32 dataFormatSize = sizeof (mDataFormat);
    AudioQueueGetProperty (
                           mQueue,
                           kAudioQueueProperty_StreamDescription,
                           &mDataFormat,
                           &dataFormatSize
                           );

    //
    // 4. Get bufferByteSize more precisely.
    //
    bufferByteSize = DeriveBufferSizeForSeconds(bufferSec);

    //
    // 5. Prepare a set of audio queue buffers.
    //
    for (int i = 0; i < kNumberBuffers; ++i) {
        AudioQueueAllocateBuffer (
                                  mQueue,
                                  bufferByteSize,
                                  &mBuffers[i]
                                  );

        AudioQueueEnqueueBuffer (
                                 mQueue,
                                 mBuffers[i],
                                 0,
                                 NULL
                                 );
    }

}

void cIosRecorder::startAudioQueue() {

    OSStatus status = AudioQueueStart (mQueue, NULL);
    if(kAudioServicesNoError == status) {
        mIsRunning = true;
        mCurrentPacket = 0;

        SMILE_MSG(1, "AudioQueueStart succeeded.\n");
        if (NULL != delegate) {
            delegate->onIosExternalSourceStarted(this);
        }
    }
    else {
        SMILE_MSG(1, "AudioQueueStart faild, OSStatus code: %d\n", status);
        if (NULL != delegate) {
            delegate->onIosExternalSourceFailed(this);
        }
    }
}

/**
   Deriving a recording audio queue buffer size for given seconds

   @param seconds Buffer length for the second.
   @result Buffer size
*/
UInt32 cIosRecorder::DeriveBufferSizeForSeconds(Float64 seconds)
{
    AudioQueueRef audioQueue = mQueue;
    AudioStreamBasicDescription ASBDescription = mDataFormat;

    static const int maxBufferSize = 0x50000; // Size in bytes

    int maxPacketSize = ASBDescription.mBytesPerPacket;
    if (maxPacketSize == 0) {
        UInt32 maxVBRPacketSize = sizeof(maxPacketSize);
        AudioQueueGetProperty (
                               audioQueue,
                               kAudioQueueProperty_MaximumOutputPacketSize,	// in Mac OS X v10.5, instead use kAudioConverterPropertyMaximumOutputPacketSize
                               &maxPacketSize,
                               &maxVBRPacketSize
                               );
    }

    Float64 numBytesForTime = ASBDescription.mSampleRate * maxPacketSize * seconds;
    UInt32 size = (UInt32)((numBytesForTime < maxBufferSize) ? numBytesForTime : maxBufferSize);
    return size;
}


void cIosRecorder::stopAudioQueue() {
    AudioQueueStop (mQueue, true);
    mIsRunning = false;

    if (NULL != delegate) {
        delegate->onIosExternalSourceStopped(this);
    }
}


void cIosRecorder::cleaningUpRecording() {
    AudioQueueDispose (mQueue, true);
}

#endif  // #if defined(__IOS__) && !defined(__IOS_WATCH__)
