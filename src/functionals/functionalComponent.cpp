/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


/*  openSMILE component:

a single statistical functional or the like....

*/


#include <functionals/functionalComponent.hpp>

#define MODULE "cFunctionalComponent"


SMILECOMPONENT_STATICS(cFunctionalComponent)

SMILECOMPONENT_REGCOMP(cFunctionalComponent)
{
  SMILECOMPONENT_REGCOMP_INIT
  scname = COMPONENT_NAME_CFUNCTIONALCOMPONENT;
  sdescription = COMPONENT_DESCRIPTION_CFUNCTIONALCOMPONENT;

  // configure your component's configType:
  SMILECOMPONENT_CREATE_CONFIGTYPE
  //  ConfigType *ct = new ConfigType(scname); if (ct == NULL) OUT_OF_MEMORY;
  SMILECOMPONENT_IFNOTREGAGAIN( {} )
  //   ct->setField(""...);
 
  SMILECOMPONENT_MAKEINFO_NODMEM_ABSTRACT(cFunctionalComponent);
}

SMILECOMPONENT_CREATE_ABSTRACT(cFunctionalComponent)

//-----

cFunctionalComponent::cFunctionalComponent(const char *name,
    int nTotal, const char*names[]) :
  cSmileComponent(name),
  nTotal_(nTotal),
  enab(NULL),
  functNames(names),
  nEnab(0),
  timeNorm(TIMENORM_SEGMENT),
  timeNormIsSet(0),
  T(0.0)
{
  if (nTotal_>0)
    enab = (int*)calloc(1,sizeof(int)*nTotal_);
}

void cFunctionalComponent::parseTimeNormOption()
{
  if (isSet("norm")) { timeNormIsSet = 1; }

  const char*Norm = getStr("norm");
  if (Norm != NULL) {
    if (!strncmp(Norm,"tur",3)) timeNorm=TIMENORM_SEGMENT;
    else if (!strncmp(Norm,"seg",3)) timeNorm=TIMENORM_SEGMENT;
    else if (!strncmp(Norm,"sec",3)) timeNorm=TIMENORM_SECOND;
    else if (!strncmp(Norm,"fra",3)) timeNorm=TIMENORM_FRAME;
  }

  SMILE_IDBG(2,"timeNorm = %i (set= %i)\n",timeNorm,timeNormIsSet);
}

void cFunctionalComponent::myFetchConfig()
{
  int i; 
  for (i=0; i<nTotal_; i++) {
    if (enab[i]) nEnab++;
  }
}

// is called by cFunctionals::setupNamesForElement after each new field has been added
void cFunctionalComponent::setFieldMetaData(cDataWriter *writer,
    const FrameMetaInfo *fmeta, int idxi, long nEl) {
  double * buf = (double *)malloc(fmeta->field[idxi].infoSize);
  memcpy(buf, fmeta->field[idxi].info, fmeta->field[idxi].infoSize);
  writer->setFieldInfo(-1 /* last field added */,
      fmeta->field[idxi].dataType, buf,
      fmeta->field[idxi].infoSize);
}

const char* cFunctionalComponent::getValueName(long i)
{
  int j=0; int n=-1;
  if (functNames==NULL) return NULL;
  for (j=0; j<nTotal_; j++) {
    if (enab[j]) n++;
    if (i==n) return functNames[j];
  }
  return NULL;
}

long cFunctionalComponent::process(FLOAT_DMEM *in, FLOAT_DMEM *inSorted, FLOAT_DMEM *out, long Nin, long Nout)
{
  SMILE_IERR(1,"dataType FLOAT_DMEM not yet supported in component '%s' of type '%s'",getTypeName(), getInstName());
  return 0;
}

cFunctionalComponent::~cFunctionalComponent()
{
  if (enab!=NULL) free(enab);
}

