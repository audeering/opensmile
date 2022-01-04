/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


/*  openSMILE component:

do elementary element-wise binary operations on vectors 

*/


#include <other/vectorBinaryOperation.hpp>
#include <limits>

#define MODULE "cVectorBinaryOperation"

#define MAX_LINE_LENGTH 2048

SMILECOMPONENT_STATICS(cVectorBinaryOperation)

SMILECOMPONENT_REGCOMP(cVectorBinaryOperation)
{
  SMILECOMPONENT_REGCOMP_INIT

    scname = COMPONENT_NAME_CVECTORBINARYOPERATION;
  sdescription = COMPONENT_DESCRIPTION_CVECTORBINARYOPERATION;

  // we inherit cDataProcessor configType and extend it:
  SMILECOMPONENT_INHERIT_CONFIGTYPE("cDataProcessor")

  SMILECOMPONENT_IFNOTREGAGAIN(
  ct->setField("fieldNames","An array of exact (case-sensitive) names of features / data elements to process (for subtraction, division and power, there must be exactly two fields). All the data vectors must have the same dimension.",(const char *)NULL, ARRAY_TYPE);
  ct->setField("operation","A string which specifies the type of element-wise operation to perform:\n   add = add vectors\n   sub = subtract vector 2 to vector 1 (in the order given by the 'fieldNames' parameter), 0\n   mul = multiply vectors \n   div = divide vector 1 by vector 2, 0\n   pow = take values of vector 1 to the power of values of vector 2\n   min = take the min\n   max = take the max.","add");
  ct->setField("newName","New name to assign to the resulting features / data elements (optional).",(const char *)NULL);
  ct->setField("powOnlyPos","if 'operation' = 'pow', do not take negative values to the power of 'param1'; instead, output 0. This is necessary to avoid 'nan' values if the exponent is rational. ",0);
  ct->setField("dummyMode","1 = don't set up output level names. Use this option temporarily, to get a working set-up where you can read the input level names, to set up your selection list.",0);
  ct->setField("divZeroOutputVal1", "1 = In case of 'div' operation, output value1 in case of value2 == 0 (= division by zero). 0 = output 0 when division by zero.", 1);
  )

    SMILECOMPONENT_MAKEINFO(cVectorBinaryOperation);
}


SMILECOMPONENT_CREATE(cVectorBinaryOperation)

//-----

cVectorBinaryOperation::cVectorBinaryOperation(const char *_name) :
cDataProcessor(_name),
names(NULL),
newName(NULL),
dimension(0),
startIdx(NULL),
vecO(NULL)
{
}

void cVectorBinaryOperation::myFetchConfig()
{
  cDataProcessor::myFetchConfig();

  dummyMode = getInt("dummyMode");
  newName = getStr("newName");
  powOnlyPos = (int)getInt("powOnlyPos");
  
  const char *op = getStr("operation");
  if (!strncmp(op,"add",3)) {
    operation = VBOP_ADD;
  } else if (!strncmp(op,"sub",3)) {
    operation = VBOP_SUB;
  } else if (!strncmp(op,"mul",3)) {
    operation = VBOP_MUL;
  } else if (!strncmp(op,"div",3)) {
    operation = VBOP_DIV;
  } else if (!strncmp(op,"pow",3)) {
    operation = VBOP_POW;
  } else if (!strncmp(op,"min",3)) {
    operation = VBOP_MIN;
  } else if (!strncmp(op,"max",3)) {
    operation = VBOP_MAX;
  } else if (strncmp(op,"add",3)) {
    SMILE_IERR(1,"unknown operation '%s' specified in config file.",op);
    operation = VBOP_ADD;
  }
  divZeroOutputVal1_ = (getInt("divZeroOutputVal1") == 1);
  // load names of selected features:
  nSel = getArraySize("fieldNames");
  if (nSel<=1 || (nSel>2 && !(operation==VBOP_ADD || operation==VBOP_MUL))) {
    SMILE_IERR(1,"Wrong number of vectors selected.");
    COMP_ERR("stopping here");
  }

  names = (char**)calloc(1,sizeof(char*) * nSel);
  int i;
  for (i=0; i<nSel; i++) {
    names[i] = (char *)getStr_f(myvprint("fieldNames[%i]",i));
    SMILE_IDBG(2,"fieldNames: '%s'",names[i]);
  }
  startIdx = (long *)calloc(1,sizeof(long) * nSel);
}

/*
int cVectorBinaryOperation::myConfigureInstance()
{
int ret=1;
// if you would like to change the write level parameters... do so HERE:

//
// .. or override configureWriter() to do so, after the reader config is available!
//
ret *= cDataProcessor::myConfigureInstance();
return ret;
}
*/

/*
int cVectorBinaryOperation::configureWriter(const sDmLevelConfig *c)
{

// we must return 1, in order to indicate configure success (0 indicates failure)
return 1;
}
*/


int cVectorBinaryOperation::setupNewNames(long nEl)
{
  if (dummyMode) {
    addNameAppendField(NULL,"dummy");
    namesAreSet_ = 1;
    return 1;
  }
  // match our list of selected names to the list of names obtained from the reader
  long i,j;

  int N = reader_->getLevelNf();
  
  // analyse type of field specified in selection, and extract fieldname
  if (names[0] != NULL) {
  const char *fieldname=names[0];

    // check if field exists
    int __N=0;
    int arrNameOffset=0;
    int found = 0;
    int elIdx = 0;
    for (i=0; i<N; i++) { // for each field
      const char *tmp = reader_->getFieldName(i,&__N,&arrNameOffset);
      if (!strcmp(fieldname,tmp)) {
        // match found, ok
        found = 1;
        startIdx[0] = elIdx;
        dimension = __N;
        if (newName==NULL) {
          writer_->addField(fieldname, __N, arrNameOffset);
        } else {
          writer_->addField(newName, __N, arrNameOffset);
        }
        break;
      }
      elIdx += __N;
    }

    // add to writer, if found
    if (!found) {
      SMILE_IERR(1,"field '%s' not found in input!",names[0]); 
      return 0;
    } 
  }

  for (j=1; j<nSel; j++) {
    const char *fieldname=NULL;

    // analyse type of field specified in selection, and extract fieldname (if array)
    if (names[j] != NULL) {
      fieldname=names[j];
      // check if field exists
      int __N=0;
      int arrNameOffset=0;
      int found = 0;
      int elIdx = 0;
      for (i=0; i<N; i++) { // for each field
        const char *tmp = reader_->getFieldName(i,&__N,&arrNameOffset);
        if (!strcmp(fieldname,tmp)) {
          if (__N!=dimension) {
            SMILE_IERR(1,"field '%s' has incompatible dimension (%i instead of %i)!",names[j],__N,dimension); 
            return 0;
          }
          // match found, ok
          found = 1;
          startIdx[j] = elIdx;
          break;
        }
        elIdx += __N;
      }
      if (!found) {
        SMILE_IERR(1,"field '%s' not found in input!",names[j]);
        return 0;
      } 
    }
  }
  namesAreSet_ = 1;
  return 1;
}

/*
int cVectorBinaryOperation::myFinaliseInstance()
{
return cDataProcessor::myFinaliseInstance();
}
*/

eTickResult cVectorBinaryOperation::myTick(long long t)
{
  if (dummyMode) return TICK_INACTIVE;

  SMILE_IDBG(4,"tick # %i, processing value vector",t);

  if (!writer_->checkWrite(1))
    return TICK_DEST_NO_SPACE;

  // get next frame from dataMemory
  cVector *vec = reader_->getNextFrame();
  if (vec == NULL) 
    return TICK_SOURCE_NOT_AVAIL;

  if (vecO == NULL) vecO = new cVector(dimension);
  long i;
  int j;

  switch (operation) {
      case VBOP_ADD: 
        for (i=0; i<dimension; i++) {
          vecO->data[i] = 0;
          for (j=0; j<nSel; j++){
            vecO->data[i] += vec->data[startIdx[j]+i];
          }
        }
        break;

      case VBOP_SUB: 
        for (i=0; i<dimension; i++) {
          vecO->data[i] = vec->data[startIdx[0]+i] - vec->data[startIdx[1]+i];
        }
        break;

      case VBOP_MUL: 
        for (i=0; i<dimension; i++) {
          vecO->data[i] = 1;
          for (j=0; j<nSel; j++){
            vecO->data[i] *= vec->data[startIdx[j]+i];
          }
        }
        break;

      case VBOP_DIV: 
        for (i=0; i<dimension; i++) {
          FLOAT_DMEM denom = vec->data[startIdx[1]+i];
          if (divZeroOutputVal1_) {
            vecO->data[i] = vec->data[startIdx[0]+i];
          } else {
            vecO->data[i] = 0.0;
          }
          if (denom != 0.0)
            vecO->data[i] = vec->data[startIdx[0]+i] / denom;
        }
        break;

      case VBOP_POW:
        if (powOnlyPos) {
          for (i=0; i<dimension; i++) {
            if (vec->data[startIdx[0]+i]>0) {
              vecO->data[i] = pow(vec->data[startIdx[0]+i],vec->data[startIdx[1]+i]);
            } else {
              vecO->data[i] = 0;
            }
          } 
        } else {
          for (i=0; i<dimension; i++) {
            vecO->data[i] = pow(vec->data[startIdx[0]+i],vec->data[startIdx[1]+i]);
          }
        }
        break;

      case VBOP_MIN: 
        for (i=0; i<dimension; i++) {
          vecO->data[i] = std::numeric_limits<FLOAT_DMEM>::infinity();
          for (j=0; j<nSel; j++){
            if (vecO->data[i]>vec->data[startIdx[j]+i]) {
              vecO->data[i] = vec->data[startIdx[j]+i];
            }
          }
        }
        break;

      case VBOP_MAX: 
        for (i=0; i<dimension; i++) {
          vecO->data[i] = -std::numeric_limits<FLOAT_DMEM>::infinity();
          for (j=0; j<nSel; j++){
            if (vecO->data[i]<vec->data[startIdx[j]+i]) {
              vecO->data[i] = vec->data[startIdx[j]+i];
            }
          }
        }
        break;
  }
  
  vecO->setTimeMeta(vec->tmeta);

  // save to dataMemory
  writer_->setNextFrame(vecO);

  return TICK_SUCCESS;
}


cVectorBinaryOperation::~cVectorBinaryOperation()
{
  if (vecO != NULL) delete(vecO);
  if (startIdx!=NULL) free(startIdx);
  if (names!=NULL) free(names); 
}

