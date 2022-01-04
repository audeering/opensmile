/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


/*  openSMILE component:

functionals meta-component
 
*/


#include <core/dataMemory.hpp>
#include <core/componentManager.hpp>
#include <functionals/functionals.hpp>

#include <algorithm>
#include <math.h>

#define MODULE "cFunctionals"


#define N_BLOCK_ALLOC 50

SMILECOMPONENT_STATICS(cFunctionals)

SMILECOMPONENT_REGCOMP(cFunctionals)
{
  SMILECOMPONENT_REGCOMP_INIT
  
  scname = COMPONENT_NAME_CFUNCTIONALS;
  sdescription = COMPONENT_DESCRIPTION_CFUNCTIONALS;

  // we inherit cWinToVecProcessor configType and extend it:
  // use _compman to find cFunctionalXXXXX component types...
  // add corresponding config sub-types to our config type here....
  SMILECOMPONENT_INHERIT_CONFIGTYPE("cWinToVecProcessor")

  char *funclist=NULL;

  SMILECOMPONENT_IFNOTREGAGAIN_BEGIN

    // we return rA=1 twice, to allow cFunctionalXXXX to register!
    if (_iteration <= 2) {
      rA=1;
      SMILECOMPONENT_MAKEINFO(cFunctionals);
    }

    if (_compman != NULL) {
  
      int nTp = _compman->getNtypes();
      int i,j=0;
      for (i=0; i<nTp; i++) {
        const char * tp = _compman->getComponentType(i,1);
        if (tp!=NULL) {
          if (!strncmp(tp,"cFunctional",11)&&strncmp(tp,COMPONENT_NAME_CFUNCTIONALS,COMPONENT_NAME_CFUNCTIONALS_LENGTH)) {
             // find beginning "cFunctional" but not our own type (cFunctionals)
            const char *fn = tp+11;
            j++;
            if (funclist != NULL) {
              char * tmp = funclist;
              funclist = myvprint("%s      %i.\t%s \t\t%s\n",funclist,j,fn,_compman->getComponentDescr(i));
              free(tmp);
            } else {
              funclist = myvprint("     (#) \t(name)    \t\t(description)\n      %i.\t%s \t\t%s\n",j,fn,_compman->getComponentDescr(i));
            }
            // add config type
            char *x = myvprint("functional sub-config of type %s",tp);
            ct->setField(fn,x,_confman->getTypeObj(tp),NO_ARRAY,DONT_FREE);
            free(x);
          }
        }
      }
    
    } else { // cannot proceed without component manager!
      rA=1;
      SMILECOMPONENT_MAKEINFO(cFunctionals);
    }

    char *x = myvprint("Array that defines the enabled functionals\n    The following functionals are available (sub-components) (Attention: the names are case-SENSITIVE!):\n%s",funclist);
    ct->setField("functionalsEnabled",x,(const char*)NULL, ARRAY_TYPE);
    free(x);
    free(funclist);

    // set more fields here:
    // NOT possible : ct->setField("globalDuration","output duration of input segment as one value in output (this is useful for variable length inputs)",0);
    ct->setField("nonZeroFuncts","If this is set to 1, functionals are only applied to input values unequal 0. If this is set to 2, functionals are only applied to input values greater than 0.",0);

    ct->setField("functNameAppend","Specifies a string prefix to append to the functional name (which is appended to the input feature name)",(const char*)NULL);
    ct->setField("masterTimeNorm","This option specifies how all components should normalise times, if they generate output values related to durations. You can change the 'norm' parameter of individual functional components to overwrite this master value. You can choose one of the following normalisation methods: \n   'segment' (or: 'turn') : normalise to the range 0..1, the result indicates relative turn length )\n   'second'  (absolute time in seconds) \n   'frame' (absolute time in number of frames of input level)","segment");
    ct->setField("preserveFields", "If set to 1, preserves the field structure (and metadata, TODO!), of the input vector structure. If set to 0 (default) the output will only have fields with a single element.", 0);

  SMILECOMPONENT_IFNOTREGAGAIN_END

  SMILECOMPONENT_MAKEINFO(cFunctionals);
}

SMILECOMPONENT_CREATE(cFunctionals)

//-----

cFunctionals::cFunctionals(const char *_name) :
  cWinToVecProcessor(_name),
  functTp(NULL),
  functI(NULL),
  functN(NULL),
  functTpI(NULL),
  functObj(NULL),
  nFunctTpAlloc(0),
  nFunctTp(0),
  nonZeroFuncts(0),
  functNameAppend(NULL),
  timeNorm(TIMENORM_UNDEFINED)
{

}


int cFunctionals::myConfigureInstance()
{
  int i,j;

  int ret = cWinToVecProcessor::myConfigureInstance();
  if (!ret)
    return 0;

  nonZeroFuncts = getInt("nonZeroFuncts");
  SMILE_IDBG(2,"nonZeroFuncts = %i \n",nonZeroFuncts);

  if (getInt("preserveFields")) {
    wholeMatrixMode = 1;
    processFieldsInMatrixMode = 1;
    SMILE_IDBG(2, "Whole Matrix Mode enabled with per field processing.");
  }

  functNameAppend = getStr("functNameAppend");
  SMILE_IDBG(2,"functNameAppend = '%s' \n",functNameAppend);

  if (isSet("masterTimeNorm")) {
    const char*Norm = getStr("masterTimeNorm");
    if (Norm != NULL) {
      if (!strncmp(Norm,"seg",3)) timeNorm=TIMENORM_SEGMENT;
      else if (!strncmp(Norm,"tur",3)) timeNorm=TIMENORM_SEGMENT;
      else if (!strncmp(Norm,"sec",3)) timeNorm=TIMENORM_SECOND;
      else if (!strncmp(Norm,"fra",3)) timeNorm=TIMENORM_FRAME;
    }
  } else {
    timeNorm = TIMENORM_UNDEFINED;
  }

  cComponentManager *_compman = getCompMan();
  if (_compman != NULL) {
    int nTp = _compman->getNtypes();
    nFunctTp = 0;
    for (i=0; i<nTp; i++) {
      const char * tp = _compman->getComponentType(i,1);
      if (tp!=NULL) {
        if (!strncmp(tp,"cFunctional",11)&&strcmp(tp,COMPONENT_NAME_CFUNCTIONALS)) {
           // find beginning "cFunctional" but not our own type (cFunctionals)
          const char *fn = tp+11;
          if (nFunctTpAlloc == nFunctTp) { // realloc:
            functTp = (char **)crealloc( functTp, sizeof(char*)*(nFunctTpAlloc+N_BLOCK_ALLOC), nFunctTpAlloc );
            functTpI = (int *)crealloc( functTpI, sizeof(int)*(nFunctTpAlloc+N_BLOCK_ALLOC), nFunctTpAlloc );
            functI = (int *)crealloc( functI, sizeof(int)*(nFunctTpAlloc+N_BLOCK_ALLOC), nFunctTpAlloc );
            functN = (int *)crealloc( functN, sizeof(int)*(nFunctTpAlloc+N_BLOCK_ALLOC), nFunctTpAlloc );
            functObj = (cFunctionalComponent **)crealloc( functObj, sizeof(cFunctionalComponent *)*(nFunctTpAlloc+N_BLOCK_ALLOC), nFunctTpAlloc );
            nFunctTpAlloc += N_BLOCK_ALLOC;
          }
          functTp[nFunctTp] = strdup(fn);
          functTpI[nFunctTp] = i;
          nFunctTp++;
        }
      }
    }
  }
  SMILE_IDBG(2,"found %i cFunctionalXXXX component types.",nFunctTp);

  // fetch enabled functionals list
  nFunctionalsEnabled = getArraySize("functionalsEnabled");
  nFunctValues = 0;
  requireSorted = 0;
  for (i=0; i<nFunctionalsEnabled; i++) {
    const char *fname = getStr_f(myvprint("functionalsEnabled[%i]",i));
    char *tpname = myvprint("cFunctional%s",fname);
    for (j=0; j<nFunctTp; j++) {
      if (!strcmp(functTp[j],fname)) {
        functI[i] = j;
        break;
      }
    }
// TODO: find duplicates in functionalsEnabled Array!!!

    if (j<nFunctTp) {
      // and create corresponding component instances...
        SMILE_IDBG(3,"creating Functional object 'cFunctional%s'.",fname);
        char *_tmp = myvprint("%s.%s",getInstName(),fname);
        cFunctionalComponent * tmp = (cFunctionalComponent *)(_compman->createComponent)(_tmp,tpname);
        free(_tmp);
        if (tmp==NULL) OUT_OF_MEMORY;
        tmp->setComponentEnvironment(_compman, -1, this);
        tmp->setTimeNorm(timeNorm);
        functN[i] = tmp->getNoutputValues();
        requireSorted += tmp->getRequireSorted();
        nFunctValues += functN[i];
        functObj[i] = tmp;
        //functTp[i]  = strdup(fname);
    } else {
      SMILE_IERR(1,"Functional object '%s' specified in 'functionalsEnabled' array, however no type 'cFunctional%s' exists!",fname,fname);
      functObj[i] = NULL;
      functN[i] = 0;
      free(tpname);
      return 0;
      //functTp[i]  = NULL;
    }
    free(tpname);
  }
  if (requireSorted) { 
    SMILE_IDBG(2,"%i Functional components require sorted data.",requireSorted);
  }

  return ret;
}

// We need setupNamesForField if in wholeMatrixMode with ProcessFieldsInMatrixMode

int cFunctionals::setupNamesForElement(int idxi, const char*name, long nEl)
{
  // in a winToVecProcessor , nEl should always be 1!
  int i, j;
  for (i=0; i<nFunctionalsEnabled; i++) {
    if ( (functN[i] > 0) && (functObj[i] != NULL) ) {
      for (j=0; j<functN[i]; j++) {
        char * newname;
        if (functNameAppend != NULL) {
          newname = myvprint("%s__%s_%s",name,functNameAppend,functObj[i]->getValueName(j));
        } else {
          newname = myvprint("%s_%s",name,functObj[i]->getValueName(j));
        }

        const FrameMetaInfo * fmeta = reader_->getFrameMetaInfo();
        int ao = 0;
        if (fmeta != NULL && idxi < fmeta->N) {
          ao = fmeta->field[idxi].arrNameOffset;
        }
        long nElementsOut = functObj[i]->getNumberOfElements(j);
        if (nElementsOut > 0) {
          writer_->addField(newname, nEl * nElementsOut, ao);
          if (fmeta != NULL && idxi < fmeta->N) {
            // copy metadata (e.g. frequency axis labels...)
            functObj[i]->setFieldMetaData(writer_, fmeta, idxi, nEl * nElementsOut);
/*            double * buf = (double *)malloc(fmeta->field[idxi].infoSize);
            memcpy(buf, fmeta->field[idxi].info, fmeta->field[idxi].infoSize);
            writer_->setFieldInfo(-1, // last field added
                fmeta->field[idxi].dataType, buf,
                fmeta->field[idxi].infoSize);*/
          }
          // TODO: we need to pass arrNameOffset through setupNamesForElement
          // however, this will require modification of the call in winToVecProcessor
          // and thus will require several descendant classes to be updated!
        }
        free(newname);
        j += nElementsOut - 1;
      }
    }
  }
  /*
  if (globalDuration) {
    writer->addField("inputDuration");
  }
  // ^ we cannot do this in this type of component..!
  */
  return nFunctValues * nEl;
}

// this must return the multiplier, i.e. the vector size returned for each input element (e.g. number of functionals, etc.)
int cFunctionals::getMultiplier()
{
  return nFunctValues;
}

int cFunctionals::doProcessMatrix(int idx, cMatrix *rows, FLOAT_DMEM *y, long nOut)
{
  // call doProcess for each row...
  cMatrix *tmpRow = NULL;
  FLOAT_DMEM *tmpY = (FLOAT_DMEM *)calloc(1, sizeof(FLOAT_DMEM) * nOut);
  FLOAT_DMEM *curY = tmpY;
  long nFuncts = 0;
  if (rows != NULL) {
    for (int i = 0; i < rows->N; i++) {
      tmpRow = rows->getRow(i, tmpRow);
      long Mu = doProcess(i, tmpRow, curY);
      curY += Mu;
      if (nFuncts == 0) nFuncts = Mu;
    }
    // re-order output vector
    for (int j = 0; j < nFuncts; j++) {
      for (int i = 0; i < rows->N; i++) {
        *y = tmpY[i*nFuncts + j];
        y++;
      }
    }
  }
  if (rows->N * nFuncts != nOut) {
    SMILE_IERR(2, "something is wrong in doProcessMatrix in cFunctionals. expected # outputs %i vs. real num outputs %i (%i * %i)", nOut, rows->N * nFuncts, rows->N, nFuncts);
  }
  free(tmpY);
  if (tmpRow != NULL) {
    delete tmpRow;
  }
  return rows->N * nFuncts;
}


// idxi is index of input element
// row is the input row
// y is the output vector (part) for the input row
int cFunctionals::doProcess(int idxi, cMatrix *row, FLOAT_DMEM*y)
{
  // copy row to matrix... simple memcpy here
  //  memcpy(y,row->data,row->nT*sizeof(FLOAT_DMEM));
  // return the number of components in y!!
  if (row->nT <= 0) {
    SMILE_IWRN(2,"not processing input row of size <= 0 !");
    return 0;
  }
  SMILE_IDBG(4, "cFunctionals::doProcess (nT = %ld) idxi %i\n", row->nT, idxi);

  long i; int ok=0; long NN=row->nT;
  FLOAT_DMEM * unsorted = row->data;
  FLOAT_DMEM * sorted=NULL;
  
  if (nonZeroFuncts) {
    NN = 0;
    unsorted = (FLOAT_DMEM*)malloc(sizeof(FLOAT_DMEM)*row->nT);
    if (nonZeroFuncts == 2) {
      for (i=0; i<row->nT; i++) {
        if (row->data[i] > 0.0) unsorted[NN++] = row->data[i];
      }
    } else {
      for (i=0; i<row->nT; i++) {
        if (row->data[i] != 0.0) unsorted[NN++] = row->data[i];
      }
    }
  }

  if (requireSorted) {
    sorted = (FLOAT_DMEM*)malloc(sizeof(FLOAT_DMEM)*NN);
    memcpy(sorted, unsorted, sizeof(FLOAT_DMEM) * NN);
    std::sort(sorted, sorted + NN);
  }

  // find max and min value, also compute arithmetic mean
  // these 3 values are required by a lot of functionals, so we do it here..
  FLOAT_DMEM *x=unsorted;
  FLOAT_DMEM min=*x,max=*x;
  double mean=*x;
  FLOAT_DMEM *xE = unsorted+NN;
  while (++x<xE) {
    if (*x<min) min=*x;
    if (*x>max) max=*x;
    mean += (double)*x;
  } mean /= (double)NN;
  
  FLOAT_DMEM *curY = y;
  for (i=0; i<nFunctionalsEnabled; i++) {
    if (functObj[i] != NULL) {
      functObj[i]->setInputPeriod(getInputPeriod());
      int ret;
      ret = functObj[i]->process( unsorted, sorted, min, max, (FLOAT_DMEM)mean, curY, NN, functN[i] );
      if (ret < functN[i]) {
        int j;
        for (j=ret; j<functN[i]; j++) curY[j] = 0.0;
      }
      if (ret>0) ok++;
  	  curY += functN[i];
    }
  }

  if (requireSorted) {
    free(sorted);
  }
  if (nonZeroFuncts) {
    free(unsorted);
  }
  
  return nFunctValues;

}

cFunctionals::~cFunctionals()
{
  int i;
  if (functTp != NULL) {
    for (i=0; i<nFunctTpAlloc; i++)
      if (functTp[i] != NULL) free(functTp[i]);
    free(functTp);
  }
  if (functI != NULL) free(functI);
  if (functN != NULL) free(functN);
  if (functTpI != NULL) free(functTpI);
  if (functObj != NULL) {
    for (i=0; i<nFunctTpAlloc; i++)
      if (functObj[i] != NULL) delete(functObj[i]);
    free(functObj);
  }
}

////  to implement in a cFunctionalXXXX object:
/*
(myFetchConfig)
NO myCOnfigure / my Finalise !!
getNoutputValues
getValueName
getRequireSorted
process
*/
