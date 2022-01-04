/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


/*  openSMILE component:
 
 Component that calls a callback function in external code whenever
 a component message is received.
 
 */

#ifndef __CEXTERNALMESSAGEINTERFACE_HPP
#define __CEXTERNALMESSAGEINTERFACE_HPP

#include <core/smileCommon.hpp>
#include <core/smileComponent.hpp>

#define COMPONENT_DESCRIPTION_CEXTERNALMESSAGEINTERFACE "This component forwards component messages to external code via callbacks."
#define COMPONENT_NAME_CEXTERNALMESSAGEINTERFACE "cExternalMessageInterface"

// we currently support two types of callback interfaces; one receiving a pointer to a cComponentMessage object,
// the other receiving a JSON representation. The latter may be somewhat easier to use from non-C languages but
// it contains only a subset of all information in the message.
typedef bool (*ExternalMessageInterfaceCallback)(const cComponentMessage *msg, void *param);
typedef bool (*ExternalMessageInterfaceJsonCallback)(const char *msg, void *param);

class cExternalMessageInterface : public cSmileComponent {
private:
    ExternalMessageInterfaceCallback messageCallback;
    ExternalMessageInterfaceJsonCallback jsonMessageCallback;
	void *callbackParam; // optional pointer to a custom object that will be passed to the callback function as a second parameter

protected:
    SMILECOMPONENT_STATIC_DECL_PR
    
    virtual int processComponentMessage(cComponentMessage *_msg) override;
    
public:
    SMILECOMPONENT_STATIC_DECL
    
    cExternalMessageInterface(const char *_name);
    virtual ~cExternalMessageInterface();

    void setMessageCallback(ExternalMessageInterfaceCallback callback, void *param);
    void setJsonMessageCallback(ExternalMessageInterfaceJsonCallback callback, void *param);
};


#endif // __CEXTERNALMESSAGEINTERFACE_HPP

