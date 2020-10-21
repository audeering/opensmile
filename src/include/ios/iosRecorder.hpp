/*F***************************************************************************
 * This file is part of openSMILE.
 *
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/

#ifndef __IOSRECORDER_HPP
#define __IOSRECORDER_HPP

#if defined(__IOS__) && !defined(__IOS_WATCH__)

#include <AVFoundation/AVFoundation.h>
#include <ios/iosExternalSourceDelegate.hpp>

/**
   Audio Queue Services Programming Guide:
   https://developer.apple.com/library/content/documentation/MusicAudio/Conceptual/AudioQueueProgrammingGuide/AQRecord/RecordingAudio.html#//apple_ref/doc/uid/TP40005343-CH4-SW4
*/
static const int kNumberBuffers  = 3;			// Number of buffers used in Audio Queue.

class cIosRecorder {
private:
    void prepareRecording();
    void startAudioQueue();
    void stopAudioQueue();
    void cleaningUpRecording();
    UInt32 DeriveBufferSizeForSeconds(Float64 seconds);

public:
    AudioStreamBasicDescription  mDataFormat;
    AudioQueueRef                mQueue;
    AudioQueueBufferRef          mBuffers[kNumberBuffers];
    UInt32                       bufferByteSize;
    SInt64                       mCurrentPacket;
    bool                         mIsRunning;
    UInt32 sampleRate;
    UInt32 channels;
    UInt32 sampleSize;
    Float64 bufferSec; // buffer size of the audio queue in seconds

    cIosExternalSourceDelegate* delegate;

    void start();
    void pause();
    void resume();
    void stop();

    cIosRecorder(UInt32 rate, UInt32 chn, UInt32 size, Float64 buftime);
    virtual ~cIosRecorder();
};


#endif // #if defined(__IOS__) && !defined(__IOS_WATCH__)
#endif // __IOSRECORDER_HPP
