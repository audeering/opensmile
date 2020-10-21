/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/

#ifndef __IOSEXTERNALSOURCEDELEGATE_HPP
#define __IOSEXTERNALSOURCEDELEGATE_HPP
#ifdef __IOS__

class cIosExternalSourceDelegate
{
public:
    cIosExternalSourceDelegate() {};
    virtual ~cIosExternalSourceDelegate() {};
    virtual void onIosExternalSourceStarted(void* externalSource){};
    virtual void onIosExternalSourceStopped(void* externalSource){};
    virtual void onIosExternalSourceFailed(void* externalSource){};
    virtual void onIosExternalSourceReceived(void* externalSource, void* buf, UInt32 size){};
};

#endif // __IOS__
#endif // __IOSEXTERNALSOURCEDELEGATE_HPP
