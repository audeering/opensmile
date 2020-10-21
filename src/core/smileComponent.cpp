/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


/*
openSMILE component hierarchy:

cSmileComponent  (basic cmdline and config functionality)
 |
 v
cProcessorComponent  - contains: Reader, Writer, ref to data mem, one or more compute components, plus interconnection logic
cComputeComponent    - implements a specific algo, clearly defined io interface
                       I: vector, O: vector
                       I: matrix, O: vector
                       (O: matrix ?? )
cReaderComponent     - reads data from dataMemory and provides frame or matrix (also registers read request)
cWriterComponent     - writes data to dataMemory (takes vector, (or matrix??)), also registers Level (data memory at the end will check, if all dependencies are fullfilled..?)
*/


/*
   component base class,
   
   with register and config functionality
   
   
 //
   
 */

// enable this for debugging to enable profiling support for each component
#define DO_PROFILING    0
// enable this to output the profiling result after each tick via SMILE_IMSG(2,...)
#define PRINT_PROFILING 0

/* good practice for dynamic component library compatibility (will be added soon..):

  define a global register method:

  sComponentInfo * registerMe(cConfigManager *_confman) {
    sMyComponent::registerComponent(_confman);
  }

  also: add a pointer to this method to global component list...
*/

#include <core/smileComponent.hpp>
#include <core/componentManager.hpp>

#define MODULE "cSmileComponent"

const char *tickResultStr(eTickResult res) {
  switch (res) {
    case TICK_INACTIVE: return "TICK_INACTIVE";
    case TICK_SUCCESS: return "TICK_SUCCESS";
    case TICK_SOURCE_NOT_AVAIL: return "TICK_SOURCE_NOT_AVAIL";
    case TICK_EXT_SOURCE_NOT_AVAIL: return "TICK_EXT_SOURCE_NOT_AVAIL";
    case TICK_DEST_NO_SPACE: return "TICK_DEST_NO_SPACE";
    case TICK_EXT_DEST_NO_SPACE: return "TICK_EXT_DEST_NO_SPACE";
    default: return "invalid value";
  }
}

SMILECOMPONENT_STATICS(cSmileComponent)

// static, must be overriden by derived component class, these are only examples...!!!!

// register component will return NULL, if the component could not be registered (fatal error..)
// it will return a valid struct, with rA=1, if some information, e.g. sub-configTypes are still missing
SMILECOMPONENT_REGCOMP(cSmileComponent)
{
  SMILECOMPONENT_REGCOMP_INIT
  scname = COMPONENT_NAME_XXXX;
  sdescription = COMPONENT_DESCRIPTION_XXXX;

  // configure your component's configType:
  SMILECOMPONENT_CREATE_CONFIGTYPE
  
  //ConfigType *ct = new ConfigType(scname); if (ct == NULL) OUT_OF_MEMORY;

  ct->setField("f1", "this is an example int", 0);
  if (ct->setField("subconf", "this is config of sub-component",
                  _confman->getTypeObj("nameOfSubCompType"), NO_ARRAY, DONT_FREE) == -1) {
     rA=1; // if subtype not yet found, request , re-register in the next iteration
  }

  // ...
  SMILECOMPONENT_IFNOTREGAGAIN( {} )

  // change cSmileComponent to cMyComponent !
  SMILECOMPONENT_MAKEINFO(cSmileComponent);
  //return makeInfo(_confman, scname, sdescription, cSmileComponent::create, rA);
}


SMILECOMPONENT_CREATE_ABSTRACT(cSmileComponent)

// staic, internal....
sComponentInfo * cSmileComponent::makeInfo(cConfigManager *_confman, const char *_name, const char *_description, cSmileComponent * (*create) (const char *_instname), int regAgain, int _abstract, int _nodmem)
{
  sComponentInfo *info = new sComponentInfo;
  if (info == NULL) OUT_OF_MEMORY;
  info->componentName = _name;
  info->description = _description;
  info->create = create;
  info->registerAgain = regAgain;
  info->abstract = _abstract;
  info->noDmem = _nodmem;
  info->next = NULL;
  
  return info;
}


//--------------------- dynamic:

cSmileComponent * cSmileComponent::getComponentInstance(const char * name) const {
  if (compman_ != NULL)
    return compman_->getComponentInstance(name);
  else
    return NULL;
}

const char * cSmileComponent::getComponentInstanceType(const char * name) const {
  if (compman_ != NULL)
    return compman_->getComponentInstanceType(name);
  else
    return NULL;
}

cSmileComponent * cSmileComponent::createComponent(const char *name, const char *component_type)
{
  if (compman_ != NULL) {
    return compman_->createComponent(name, component_type);
  } else {
    return NULL;
  }
}

cSmileLogger *cSmileComponent::getLogger() const
{ 
  if (compman_ != NULL)
    return compman_->getLogger();
  else
    return NULL;
}

cSmileComponent::cSmileComponent(const char *instname) :
  iname_(NULL),
  id_(-1),
  compman_(NULL),
  parent_(NULL),
  cfname_(NULL),
  isRegistered_(0),
  isConfigured_(0),
  isFinalised_(0),
  isReady_(0),
  confman_(NULL),
  override_(0),
  manualConfig_(0),
  cname_(NULL),
  EOIlevel_(0),
  EOI_(0),
  EOIcondition_(0),
  paused_(0),
  runMe_(1),
  doProfile_(DO_PROFILING),
  printProfile_(PRINT_PROFILING),
  profileCur_(0.0), profileSum_(0.0)
{
  smileMutexCreate(messageMtx_);
  if (instname == NULL) COMP_ERR("cannot create cSmileComponent with instanceName == NULL!");
  iname_ = strdup(instname);
  cfname_ = iname_;
}

void cSmileComponent::setComponentEnvironment(cComponentManager *compman, int id, cSmileComponent *parent)
{
  if (compman != NULL) {
    compman_ = compman;
    confman_ = compman->getConfigManager();
    id_ = id;
  } else {
    SMILE_IERR(3, "setting NULL componentManager in cSmileComponent::setComponentEnvironment!");
  }
  parent_ = parent;
  mySetEnvironment();
}

int cSmileComponent::sendComponentMessage(const char *recepient, cComponentMessage *msg)
{
  int ret=0;
  if (compman_ != NULL) {
    if (msg != NULL) msg->sender = getInstName();
    ret = compman_->sendComponentMessage(recepient, msg);
  }
  return ret;
}

char * cSmileComponent::jsonMessageToString(const rapidjson::Document& doc) {
  rapidjson::StringBuffer str;
  rapidjson::Writer<rapidjson::StringBuffer> writer(str);
  doc.Accept(writer);
  const char * tmpString = str.GetString();
  if (tmpString != NULL)
    return strdup(str.GetString());
  return NULL;
}

int cSmileComponent::sendJsonComponentMessage(const char * recepient,
    const rapidjson::Document& doc) {
  // this can send arbitrary JSON to another component
  cComponentMessage msg("_CONTAINER", "jsonObject");
  // TODO: might need to serialize to text instead of encoding pointers?
  msg.custData = jsonMessageToString(doc); //(void *)&doc;
  msg.custDataType = CUSTDATA_TEXT;
  int ret = sendComponentMessage(recepient, &msg);
  if (msg.custData != NULL) {
    free(msg.custData);
  }
  return ret;
}

rapidjson::Document * cSmileComponent::parseJsonMessage(const char *text,
    rapidjson::Document::AllocatorType * allocator)
{
  // parse text to json document
  rapidjson::Document * doc;
  if (allocator != NULL)
    doc = new rapidjson::Document(allocator);
  else
    doc = new rapidjson::Document();
  if (doc == NULL)
    return NULL;
  if (doc->Parse<0>(text).HasParseError()) {
    SMILE_IERR(1, "smileControlServer::parseJsonMessage: JSON parse error in parseJsonMessage: %s",
        doc->GetParseError());
    delete doc;
    return NULL;
  }
  return doc;
}

rapidjson::Document * cSmileComponent::receiveJsonComponentMessage(
    cComponentMessage *msg) {
  // this can send arbitrary JSON to another component
  if (msg != NULL &&
      !strncmp(msg->msgtype, "_CONTAINER", 10) &&
      !strncmp(msg->msgname, "jsonObject", 10) &&
      msg->custData != NULL && msg->custDataType == CUSTDATA_TEXT) {
    return parseJsonMessage((const char*)msg->custData, NULL);
  } else {
    return NULL;
  }
}

double cSmileComponent::getSmileTime() const
{ 
  if (compman_ != NULL) {
    return compman_->getSmileTime();
  }
  return 0.0;
}

int cSmileComponent::isAbort() const
{ 
  if (compman_ != NULL) return compman_->isAbort();
  else return 0; 
}

void cSmileComponent::abortProcessing() {
  compman_->requestAbort();
}

void cSmileComponent::signalDataAvailable() {
  compman_->signalDataAvailable();    
}

int cSmileComponent::finaliseInstance() 
{
  if (!isConfigured_) {
    SMILE_IDBG(7,"finaliseInstance called on a not yet successfully configured instance '%s'",getInstName());
    return 0;
  }
  if (isFinalised_) return 1;
  isFinalised_ = myFinaliseInstance();
  isReady_ = isFinalised_;
  return isFinalised_;
}

void cSmileComponent::startProfile(long long t, int EOI)
{
  startTime_ = std::chrono::high_resolution_clock::now();
}

void cSmileComponent::endProfile(long long t, int EOI)
{
  endTime_ = std::chrono::high_resolution_clock::now();
  typedef std::chrono::duration<double> seconds;
  profileCur_ = std::chrono::duration_cast<seconds>(endTime_ - startTime_).count();
  profileSum_ += profileCur_;
  if (printProfile_) {
    SMILE_IMSG(2, "~~~~profile~~~~ cur=%f  sum=%f  tick=%i\n", getProfile(0), getProfile(1), t);
  }
}

cSmileComponent::~cSmileComponent()
{
  if ((iname_ != cfname_)&&(cfname_!=NULL)) free (cfname_);
  if (iname_ != NULL) free(iname_);
  smileMutexDestroy(messageMtx_);
}
