/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


/*  openSMILE component:

Simple message sender for debugging and testing purposes. 
Reads data from an input level,
sends a message based on a threshold event in a selectable input channel, 
or sends all read data periodically as a message.

*/


#ifndef __SIMPLE_MESSAGE_SENDER_HPP
#define __SIMPLE_MESSAGE_SENDER_HPP

#include <core/smileCommon.hpp>
#include <core/dataSink.hpp>

#define COMPONENT_DESCRIPTION_CSIMPLEMESSAGESENDER "This is an example of a cDataSink descendant. It reads data from the data memory and prints it to the console. This component is intended as a template for developers."
#define COMPONENT_NAME_CSIMPLEMESSAGESENDER "cSimpleMessageSender"
#define BUILD_COMPONENT_SimpleMessageSender


enum cSimpleMessageSender_Conditions {
  COND_UNDEF=0,
  COND_GTEQ=1, // trigger event, always when input greater or equal to the reference (threshold parameter)
  COND_GT=2,   // trigger event, always when greater
  COND_EQ=3,   // trigger event, always when equal
  COND_LE=4,   // trigger event, always when lesser
  COND_LEEQ=5, // trigger event, always when lesser or equal
  COND_GTEQ_S=101, // trigger event, only the first time the value is greater or equal
  COND_GT_S=102,   // trigger event, only the first time the value is greater
  COND_EQ_S=103,   // trigger event, only the first time the value is equal
  COND_LE_S=104,   // trigger event, only the first time the value is lesser
  COND_LEEQ_S=105, // trigger event, only the first time the value is lesser or equal
};

class cSimpleMessageSender : public cDataSink {
  private:
    const char *messageRecp;  // the comma separated list of message receipients (component names)
    const char *messageName;
    const char *messageType;
    bool showCustDataAsText, showCustData2AsText;
    bool sendPeriodically;        // Flag to enable/disable sending of periodic messages (sending of every value read from the input)
    bool useJsonFormat;           // Flag to control whether data in messages is stored in the float values field of cComponentMessage (up to 8 values), or formatted as JSON
    bool enableDebugReceiver;     // Flag to enable/disable the debug print functionality for received(!) messages.
    bool enableDebugSender;       // Flag to enable/disable the debug print functionality for sent messages (before sending).
    const char *dataElementName;  // the name of the input element to send periodically or base event decisions on
    long dataElementIndex;
    FLOAT_DMEM threshold;         // the threshold for triggering an event
    cSimpleMessageSender_Conditions condition;  // the operation to apply to the element with dataElementName to trigger event based messages

    bool condFlag;   // used as state variable for the _S event conditions

  protected:
    SMILECOMPONENT_STATIC_DECL_PR
    
    virtual void myFetchConfig() override;
    virtual int myFinaliseInstance() override;
    virtual eTickResult myTick(long long t) override;

    // Implements a debug message receiver here. It prints all received messages to the log.
    virtual int processComponentMessage(cComponentMessage *_msg) override;

    void sendMessage(cComponentMessage *msg);
    void sendJsonMessage(const rapidjson::Document &doc);
    void printMessage(cComponentMessage *msg);
    void printJsonMessage(const rapidjson::Document &doc);
    void sendPeriodicMessage(FLOAT_DMEM *v, int firstElementIndex, int Nv, long vi, double tm);
    void sendEventMessage(FLOAT_DMEM *v, int Nv, const char * text, FLOAT_DMEM ref, long vi, double tm);
    void eventMessage(FLOAT_DMEM v, long vi, double tm);

  public:
    SMILECOMPONENT_STATIC_DECL
    
    cSimpleMessageSender (const char *_name);

    virtual ~cSimpleMessageSender();
};




#endif // __SIMPLE_MESSAGE_SENDER_HPP
