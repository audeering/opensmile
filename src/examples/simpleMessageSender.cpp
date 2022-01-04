/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


/*  openSMILE component:


*/


#include <examples/simpleMessageSender.hpp>

#define MODULE "cSimpleMessageSender"


SMILECOMPONENT_STATICS(cSimpleMessageSender)

SMILECOMPONENT_REGCOMP(cSimpleMessageSender)
{
  SMILECOMPONENT_REGCOMP_INIT

  scname = COMPONENT_NAME_CSIMPLEMESSAGESENDER;
  sdescription = COMPONENT_DESCRIPTION_CSIMPLEMESSAGESENDER;

  // we inherit cDataSink configType and extend it:
  SMILECOMPONENT_INHERIT_CONFIGTYPE("cDataSink")
  
  SMILECOMPONENT_IFNOTREGAGAIN(
    ct->setField("messageRecp", "A comma separated list of message receipients (component names). If you leave this empty, no messages will be sent.", (const char *)NULL);
    ct->setField("messageName", "The name of the message that will be sent.", "demo_message");
    ct->setField("messageType", "The type of the message that will be sent.", "simpleMessage");
    ct->setField("dataElementName", "The name of the input level data element to send periodically or base the event decisions on. If left empty, the first element will be used.", (const char *)NULL);
    ct->setField("sendPeriodically", "1 = Enable sending of periodic messages for each sample of the input data element. 2 = ignore dataElementName and send all elements in the input data (up to the first 8 elements if useJsonFormat is not set).", 0);
    ct->setField("useJsonFormat", "1 = Send messages in JSON format. This allows sending arbitrary large inputs if sendPeriodically is set to 2.", 0);
    ct->setField("enableDebugReceiver", "1/0 = enable/disable the debug print functionality for received(!) messages.", 1);
    ct->setField("enableDebugSender", "1/0 = enable/disable the debug print functionality for sent messages (before sending).", 1);
    ct->setField("showCustDataAsText", "1/0 = enable/disable printing of (non NULL) custData field as text string in debug message logs.", 0);
    ct->setField("showCustData2AsText", "1/0 = enable/disable printing of (non NULL) custData2 field as text string in debug message logs.", 0);
    ct->setField("threshold", "The threshold for triggering an event on the input data element.", 0.0);
    ct->setField("condition", "The condition to apply to the element with dataElementName to trigger event based messages. Choose one of the following: eq, gteq, gt, leeq, le for the conditions =, >=, >, <=, <. The event will be triggered always when the condition is met. Add the suffix _s to the condition name, to trigger the event only the first time the condition changes from false to true. The condition is applied as: <input_value> <cond> <threshold>.", "eq");
  )

  SMILECOMPONENT_MAKEINFO(cSimpleMessageSender);
}

SMILECOMPONENT_CREATE(cSimpleMessageSender)

//-----

cSimpleMessageSender::cSimpleMessageSender(const char *_name) :
  cDataSink(_name),
  condition(COND_UNDEF), sendPeriodically(false), useJsonFormat(false),
  enableDebugReceiver(false), enableDebugSender(false),
  dataElementIndex(0), dataElementName(NULL),
  messageRecp(NULL), condFlag(false),
  showCustDataAsText(false), showCustData2AsText(false)
{
  // ...
}

void cSimpleMessageSender::myFetchConfig()
{
  cDataSink::myFetchConfig();
  messageRecp = getStr("messageRecp");
  messageName = getStr("messageName");
  messageType = getStr("messageType");
  dataElementName = getStr("dataElementName");
  int tmp = getInt("sendPeriodically");
  if (tmp) {
    sendPeriodically = true;
  }
  if (tmp == 2) {
    dataElementIndex = -1;
  }
  useJsonFormat = getInt("useJsonFormat") != 0;
  enableDebugReceiver = getInt("enableDebugReceiver") != 0;
  enableDebugSender = getInt("enableDebugSender") != 0;
  showCustDataAsText = getInt("showCustDataAsText") != 0;
  showCustDataAsText = getInt("showCustData2AsText") != 0;
  threshold = (FLOAT_DMEM)getDouble("threshold") != 0;

  const char *cond = getStr("condition");
  if (!strncasecmp(cond, "eq", 2)) {
    if (!strncasecmp(cond, "eq_s", 4)) {
      condition = COND_EQ_S;
    } else {
      condition = COND_EQ;
    }
  } else if (!strncasecmp(cond, "gteq", 4)) {
    if (!strncasecmp(cond, "gteq_s", 6)) {
      condition = COND_GTEQ_S;
    } else {
      condition = COND_GTEQ;
    }
  } else if (!strncasecmp(cond, "leeq", 4)) {
    if (!strncasecmp(cond, "leeq_s", 6)) {
      condition = COND_LEEQ_S;
    } else {
      condition = COND_LEEQ;
    }
  } else if (!strncasecmp(cond, "gt", 2)) {
    if (!strncasecmp(cond, "gt_s", 4)) {
      condition = COND_GT_S;
    } else {
      condition = COND_GT;
    }
  } else if (!strncasecmp(cond, "le", 2)) {
    if (!strncasecmp(cond, "le_s", 4)) {
      condition = COND_LE_S;
    } else {
      condition = COND_LE;
    }
  }

  if (!sendPeriodically && useJsonFormat) {
    SMILE_IERR(1, "useJsonFormat is currently only supported for periodic sending (sendPeriodically != 0)");
  }
}

int cSimpleMessageSender::myFinaliseInstance()
{
  // FIRST call cDataSink myFinaliseInstance, this will finalise our dataWriter...
  int ret = cDataSink::myFinaliseInstance();

  /* if that was SUCCESSFUL (i.e. ret == 1), then do some stuff like
     loading models or opening output files: */

  // We find the index of dataElementName here. 
  // The lookup of this index has computational overhead, so we do not
  // want to do it for every tick in the myTick() function.
  
  if (dataElementName != NULL && dataElementIndex != -1) {
    const FrameMetaInfo * fmeta = reader_->getFrameMetaInfo();
    dataElementIndex = fmeta->findField(dataElementName);
  }
  return ret;
}

int cSimpleMessageSender::processComponentMessage(cComponentMessage *msg)
{
  if (enableDebugReceiver && msg != NULL) {
    SMILE_IMSG(2, "Got component message from '%s'", msg->sender);
    printMessage(msg);
    return 1;
  }
  return 0;
}

void cSimpleMessageSender::printMessage(cComponentMessage *msg)
{
  if (msg->sender != NULL) { 
    SMILE_PRINT("  Sender: %s", msg->sender);
  }
  SMILE_PRINT("  MsgType: %s", msg->msgtype);
  SMILE_PRINT("  MsgName: %s", msg->msgname);
  SMILE_PRINT("  MsgId: %i", msg->msgid);
  SMILE_PRINT("  ReaderTime: %f", msg->readerTime);
  SMILE_PRINT("  SmileTime: %f", msg->smileTime);
  if (msg->userTime1 != 0.0) {
    SMILE_PRINT("  UserTime1: %f", msg->userTime1);
  }
  if (msg->userTime2 != 0.0) {
    SMILE_PRINT("  UserTime2: %f", msg->userTime2);
  }
  if (msg->userflag1 != 0) {
    SMILE_PRINT("  UserFlag1: %i", msg->userflag1);
  }
  if (msg->userflag2 != 0) {
    SMILE_PRINT("  UserFlag2: %i", msg->userflag2);
  }
  if (msg->userflag3 != 0) {
    SMILE_PRINT("  UserFlag3: %i", msg->userflag3);
  }
  for (int i = 0; i < CMSG_nUserData; ++i) { 
    SMILE_PRINT("  Float[%i]: %f", i, msg->floatData[i]);
  }
  for (int i = 0; i < CMSG_nUserData; ++i) { 
    SMILE_PRINT("  Int[%i]: %i", i, msg->intData[i]);
  }
  SMILE_PRINT("  MsgText: %s", msg->msgtext);
  SMILE_PRINT("  CustDataSize: %i, CustData2Size: %i", msg->custDataSize, msg->custData2Size);
  if (showCustDataAsText && msg->custData != NULL) {
    SMILE_PRINT("  CustData : '%s'", msg->custData);
  }
  if (showCustData2AsText && msg->custData2 != NULL) {
    SMILE_PRINT("  CustData2: '%s'", msg->custData2);
  }
  SMILE_PRINT("--- end of message ---\n");
}

void cSimpleMessageSender::printJsonMessage(const rapidjson::Document &doc)
{
  rapidjson::StringBuffer str;
  rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(str);
  writer.SetIndent(' ', 2);
  doc.Accept(writer);
  const char *jsonString = str.GetString();
  if (jsonString != NULL) {
    SMILE_PRINT("%s", jsonString);
  }
  SMILE_PRINT("--- end of message ---\n");
}

void cSimpleMessageSender::sendMessage(cComponentMessage *msg)
{
  if (enableDebugSender) {
    SMILE_IMSG(2, "Printing message that will be sent to '%s':", messageRecp);
    printMessage(msg);
  }
  sendComponentMessage(messageRecp, msg);
}

void cSimpleMessageSender::sendJsonMessage(const rapidjson::Document &doc)
{
  if (enableDebugSender) {
    SMILE_IMSG(2, "Printing message that will be sent to '%s':", messageRecp);
    printJsonMessage(doc);
  }
  sendJsonComponentMessage(messageRecp, doc);
}

void cSimpleMessageSender::sendPeriodicMessage(FLOAT_DMEM *v, int firstElementIndex, int Nv, long vi, double tm)
{
  if (!useJsonFormat) {
    cComponentMessage msg;
    memset(&msg, 0, sizeof(msg));
    strncpy(msg.msgtype, messageType, CMSG_typenameLen);
    strncpy(msg.msgname, messageName, CMSG_typenameLen);
    if (v != NULL) {
      for (int i = 0; i < Nv && i < CMSG_nUserData; i++) {
        msg.floatData[i] = v[firstElementIndex + i];
      }
    }
    msg.intData[0] = 0;
    msg.readerTime = tm;
    msg.userTime1 = (double)vi;
    sendMessage(&msg);
  } else {
    rapidjson::Document doc;
    doc.SetObject();
    rapidjson::Document::AllocatorType& allocator = doc.GetAllocator();
    doc.AddMember("msgtype", rapidjson::Value(messageType, allocator), allocator);
    doc.AddMember("msgname", rapidjson::Value(messageName, allocator), allocator);
    rapidjson::Value fields(rapidjson::kObjectType);
    const FrameMetaInfo *fmeta = reader_->getFrameMetaInfo();
    for (int i = 0; i < Nv; i++) {
      long arrIdx;
      long el = fmeta->elementToFieldIdx(i, &arrIdx);
      FieldMetaInfo &field = fmeta->field[el];
      if (field.N <= 1) {
        // non-array field
        fields.AddMember(rapidjson::Value(field.name, allocator), rapidjson::Value(v[firstElementIndex + i]), allocator);      
      } else {
        // array field
        rapidjson::Value arrayField(rapidjson::kArrayType);
        for (int j = arrIdx; j < field.N && i < Nv; j++, i++) {
          arrayField.PushBack(v[firstElementIndex + i], allocator);
        }
        fields.AddMember(rapidjson::Value(field.name, allocator), arrayField, allocator);
      }
    }
    doc.AddMember("fields", fields, allocator);
    doc.AddMember("time", tm, allocator);
    doc.AddMember("vIdx", int(vi), allocator);    
    sendJsonMessage(doc);
  }
}

void cSimpleMessageSender::sendEventMessage(FLOAT_DMEM *v, int Nv, const char * text, FLOAT_DMEM ref, long vi, double tm)
{
  if (!useJsonFormat) {
    cComponentMessage msg;
    memset(&msg, 0, sizeof(msg));
    strncpy(msg.msgtype, messageType, CMSG_typenameLen);
    strncpy(msg.msgname, messageName, CMSG_typenameLen);
    strncpy(msg.msgtext, text, CMSG_textLen);
    if (v != NULL) {
      for (int i = 0; i < Nv && i < CMSG_nUserData; i++) {
        msg.floatData[i] = v[i];
      }
    }
    msg.floatData[1] = ref;
    msg.intData[0] = (int)condition;
    msg.readerTime = tm;
    msg.userTime1 = (double)vi;
    sendMessage(&msg);
  } else {
    // currently unimplemented
  }
}

void cSimpleMessageSender::eventMessage(FLOAT_DMEM v, long vi, double tm)
{
  int Nv = 1;
  if (condition == COND_GTEQ && v >= threshold) {
    sendEventMessage(&v, Nv, "greater equal", threshold, vi, tm);
  } else if (condition == COND_GT && v > threshold) {
    sendEventMessage(&v, Nv, "greater", threshold, vi, tm);
  } else if (condition == COND_EQ && v == threshold) {
    sendEventMessage(&v, Nv, "equal", threshold, vi, tm);
  } else if (condition == COND_LEEQ && v <= threshold) {
    sendEventMessage(&v, Nv, "lesser equal", threshold, vi, tm);
  } else if (condition == COND_LE && v < threshold) {
    sendEventMessage(&v, Nv, "lesser", threshold, vi, tm);
  } else if (condition == COND_GTEQ_S) {
    if (v >= threshold) {
      if (!condFlag) {
        sendEventMessage(&v, Nv, "begin greater equal", threshold, vi, tm);
        condFlag = true;
      }
    } else {
      condFlag = false;
    }
  } else if (condition == COND_GT_S) {
    if (v > threshold) {
      if (!condFlag) {
        sendEventMessage(&v, Nv, "begin greater", threshold, vi, tm);
        condFlag = true;
      }
    } else {
      condFlag = false;
    }
  } else if (condition == COND_EQ_S) {
    if (v == threshold) {
      if (!condFlag) {
        sendEventMessage(&v, Nv, "begin equal", threshold, vi, tm);
        condFlag = true;
      }
    } else {
      condFlag = false;
    }
  } else if (condition == COND_LEEQ_S) {
    if (v <= threshold) {
      if (!condFlag) {
        sendEventMessage(&v, Nv, "begin lesser equal", threshold, vi, tm);
        condFlag = true;
      }
    } else {
      condFlag = false;
    }
  } else if (condition == COND_LE_S) {
    if (v < threshold) {
      if (!condFlag) {
        sendEventMessage(&v, Nv, "begin lesser", threshold, vi, tm);
        condFlag = true;
      }
    } else {
      condFlag = false;
    }
  } 
}

eTickResult cSimpleMessageSender::myTick(long long t)
{
  cVector *vec = reader_->getNextFrame();
  if (vec == NULL) return TICK_SOURCE_NOT_AVAIL;  // no data available

  long vi = vec->tmeta->vIdx;   // the frame's virtual index (frame number from start of system)
  double tm = vec->tmeta->time; // the frame's time (data time from start of system)
  
  if (dataElementIndex < vec->N && dataElementIndex >= 0) {
    if (sendPeriodically) {
      sendPeriodicMessage(vec->data, dataElementIndex, 1, vi, tm);
    } else {
      eventMessage(vec->data[dataElementIndex], vi, tm);
    }
  } else if (dataElementIndex == -1) {
    if (sendPeriodically) {
      sendPeriodicMessage(vec->data, 0, vec->N, vi, tm);
    }
  }
  return TICK_SUCCESS;
}


cSimpleMessageSender::~cSimpleMessageSender()
{
}

