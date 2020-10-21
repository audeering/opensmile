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

#include <other/externalMessageInterface.hpp>

#define MODULE "cExternalMessageInterface"

SMILECOMPONENT_STATICS(cExternalMessageInterface)

SMILECOMPONENT_REGCOMP(cExternalMessageInterface)
{
    SMILECOMPONENT_REGCOMP_INIT
    scname = COMPONENT_NAME_CEXTERNALMESSAGEINTERFACE;
    sdescription = COMPONENT_DESCRIPTION_CEXTERNALMESSAGEINTERFACE;
    
    // configure your component's configType:
    SMILECOMPONENT_CREATE_CONFIGTYPE
    SMILECOMPONENT_IFNOTREGAGAIN( {
    } )
    
    SMILECOMPONENT_MAKEINFO(cExternalMessageInterface);
}

SMILECOMPONENT_CREATE(cExternalMessageInterface)


//-----

cExternalMessageInterface::cExternalMessageInterface(const char *_name) :
cSmileComponent(_name), messageCallback(NULL), jsonMessageCallback(NULL),
callbackParam(NULL)
{
}

int cExternalMessageInterface::processComponentMessage(cComponentMessage *msg)
{
    int ret = 0;

    if (messageCallback != NULL) {
        ret = messageCallback(msg, callbackParam);
    }

    if (jsonMessageCallback != NULL) {
        char *json = msg->serializeToJson();
        if (json == NULL) 
            return 0;	
        
        ret = jsonMessageCallback(json, callbackParam);
        free(json);
    }

    return ret;
}

void cExternalMessageInterface::setMessageCallback(ExternalMessageInterfaceCallback callback, void* param)
{
    messageCallback = callback;
    callbackParam = param;
}

void cExternalMessageInterface::setJsonMessageCallback(ExternalMessageInterfaceJsonCallback callback, void* param)
{
    jsonMessageCallback = callback;
    callbackParam = param;
}

cExternalMessageInterface::~cExternalMessageInterface()
{
}

