/*F***************************************************************************
 * This file is part of openSMILE.
 *
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/

#ifndef __CCOREAUDIOSOURCE_HPP
#define __CCOREAUDIOSOURCE_HPP

#if defined(__IOS__) && !defined(__IOS_WATCH__)

#include <core/smileCommon.hpp>
#include <core/dataSource.hpp>
#include <iocore/externalAudioSource.hpp>
#include <ios/iosRecorder.hpp>
#include <ios/iosExternalSourceDelegate.hpp>

#define COMPONENT_DESCRIPTION_CCOREAUDIOSOURCE "This component receives recording data from cIosRecorder class."
#define COMPONENT_NAME_CCOREAUDIOSOURCE "cCoreAudioSource"

class cCoreAudioSource : public cExternalAudioSource, public cIosExternalSourceDelegate
{
 private:
    cIosRecorder* recorder;
    float recordBufferSec;

 protected:
    SMILECOMPONENT_STATIC_DECL_PR

    // from cExternalAudioSource
    virtual void myFetchConfig() override;
    virtual eTickResult myTick(long long t) override;
    virtual int pauseEvent() override;
    virtual void resumeEvent() override;

    // from cIosExternalSourceDelegate
    virtual void onIosExternalSourceStarted(void* externalSource) override;
    virtual void onIosExternalSourceStopped(void* externalSource) override;
    virtual void onIosExternalSourceFailed(void* externalSource) override;
    virtual void onIosExternalSourceReceived(void* externalSource, void* buf, UInt32 size) override;

 public:
    SMILECOMPONENT_STATIC_DECL

    cCoreAudioSource(const char *name);
    void setExternalEOI() ;
    virtual ~cCoreAudioSource();
};


#endif // #if defined(__IOS__) && !defined(__IOS_WATCH__)
#endif // __CCOREAUDIOSOURCE_HPP
