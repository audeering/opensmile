/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


/*  openSMILE component:

 example of a vector processor

*/



#include <rnn/rnnProcessor.hpp>

#define MODULE "cRnnProcessor"

#ifdef BUILD_RNN

SMILECOMPONENT_STATICS(cRnnProcessor)

SMILECOMPONENT_REGCOMP(cRnnProcessor)
{
  SMILECOMPONENT_REGCOMP_INIT
  scname = COMPONENT_NAME_CRNNPROCESSOR;
  sdescription = COMPONENT_DESCRIPTION_CRNNPROCESSOR;

  // we inherit cVectorProcessor configType and extend it:
  SMILECOMPONENT_INHERIT_CONFIGTYPE("cDataProcessor")

  // if the inherited config type was found, we register our configuration variables
  SMILECOMPONENT_IFNOTREGAGAIN( {} // <- this is only to avoid compiler warnings...
    // name append has a special role: it is defined in cDataProcessor, and can be overwritten here:
    //ct->setField("nameAppend",NULL,"processed");
    ct->setField("netfile","The file which contains the trained network created by rnnlib",(const char *)NULL);
    ct->setField("jsonNet", "1/0 = yes/no. Flag that indicates whether 'netfile' is in json format (1) (trained with current) or in rnnlib format (0).", 0);
    ct->setField("classlabels","Give a string of comma separated (NO spaces allowed!!) class names (e.g. class1,class2,class3) for a classification or transcription task",(const char*)NULL);
    ct->setField("printConnections","1/0 = yes/no : print human readable information on the network layers on connections",0);
    //ct->setField("printInputStats","1/0 = yes/no : print input weight sums (can be used for feature selection...)",0);
  )

  // The configType gets automatically registered with the config manger by the SMILECOMPONENT_IFNOTREGAGAIN macro
  
  // we now create out sComponentInfo, including name, description, success status, etc. and return that
  SMILECOMPONENT_MAKEINFO(cRnnProcessor);
}

SMILECOMPONENT_CREATE(cRnnProcessor)

//-----

cRnnProcessor::cRnnProcessor(const char *_name) :
  cDataProcessor(_name), frameO(NULL), nClasses(0),
  classlabels_(NULL), classlabelArr_(NULL), net_created_(0)
{

}

void cRnnProcessor::myFetchConfig()
{
  cDataProcessor::myFetchConfig();
  
  netfile = getStr("netfile");
  SMILE_IDBG(2,"netfile = '%s'",netfile);

  jsonNet = getInt("jsonNet");

  const char * classlabels = getStr("classlabels");
  SMILE_IDBG(2,"classlabels = '%s'", classlabels);
  if (classlabels != NULL) {
    classlabels_ = strdup(classlabels);
    // count number of classes:
    nClasses = 1;
    char *cl = classlabels_;
    while(1) {
      char *x = strchr(cl,',');
      if (x!=NULL) {
        cl=x+1;
        nClasses++;
      } else {
        break;
      }
    }

    // allocate memory for classes array
    classlabelArr_ = (const char **)calloc(1,sizeof(const char *)*nClasses);

    // parse class names
    long i=0;
    cl = classlabels_;
    while(1) {
      char *x = strchr(cl,',');
      if (x!=NULL) {
        *x=0;
        classlabelArr_[i] = (const char *)cl;
        if (*(x+1) == 0) { nClasses--; break; }
        cl = x+1;
        i++;
      } else {
        classlabelArr_[i] = (const char *)cl;
        i++;
        break;
      }
    }
   
  }
  
  printConnections = getInt("printConnections");
  SMILE_IDBG(2,"printConnections = %i",printConnections);
}


int cRnnProcessor::myConfigureInstance()
{
  int ret = cDataProcessor::myConfigureInstance();
  if (ret) {
    if (net.loaded == 0) {
      if (jsonNet) {
        ret = smileRnn_loadNetJson(netfile,net);
      } else {
        ret = smileRnn_loadNet(netfile,net);
      }
    }
    //TODO: create individual nets for each idxi, or only one net if "processArrayFields=0"
    // thus: move this code to  setupNamesForField
  }
  return ret;
}

int cRnnProcessor::setupNewNames(long nEl)
{
  if ((net.task == NNTASK_CLASSIFICATION)||(net.task == NNTASK_TRANSCRIPTION)) {
    long i;
   if (net.outputSize) printf("outputsize: %li\n",net.outputSize);
   if (nClasses) printf("classes: %li\n",nClasses);
    for (i=0; i<MIN(nClasses,net.outputSize); i++) {
      addNameAppendField("RNNoutpAct", classlabelArr_[i], 1);
    }
    if (net.outputSize > nClasses) {
      if (net.task == NNTASK_TRANSCRIPTION) {
        for (i=nClasses; i<net.outputSize-1; i++) {
          addNameAppendField("RNNoutpAct","unlabelled",1);
        }
        addNameAppendField("RNNoutpAct","ctcOther",1);
      } else {
        for (i=nClasses; i<net.outputSize; i++) {
          addNameAppendField("RNNoutpAct","unlabelled",1);
        }
      }
    }
  } else {
    addNameAppendField("RNNoutpAct", nameAppend_, net.outputSize);
  }
	namesAreSet_ = 1;
  return 1;
}



int cRnnProcessor::myFinaliseInstance()
{
  int ret = cDataProcessor::myFinaliseInstance();

  if (ret) {
    /*
    if (getInt("printInputStats")) {
      FLOAT_NN *wg = NULL;
      long N = smileRnn_getInputSelection(net, &wg);
      long i;
      if (wg != NULL) {
        SMILE_PRINT("input weighting:");
        for (i=0; i<N; i++) {
          SMILE_PRINT("%i: %f",i,wg[i]);
        }
      } else {
        SMILE_IERR(1,"input weighting information is not available");
      }
    }
    */
    if (net_created_ == 0) {
      ret = smileRnn_createNet(net,rnn,printConnections);
      if (ret) {
        in = (FLOAT_NN*)malloc(sizeof(FLOAT_NN)*net.inputSize);
        out = (FLOAT_DMEM*)malloc(sizeof(FLOAT_DMEM)*net.outputSize);
        frameO = new cVector(net.outputSize);
        net_created_ = 1;
      }
    }
  }
  return ret;
}


// a derived class should override this method, in order to implement the actual processing
eTickResult cRnnProcessor::myTick(long long t)
{
  if (!writer_->checkWrite(1))
    return TICK_DEST_NO_SPACE;

  cVector * frame = reader_->getNextFrame();
  if (frame == NULL) return TICK_SOURCE_NOT_AVAIL;

  // pass current vector to net
  long i, N=0;
  for (i=0; i<MIN(frame->N,net.inputSize); i++) { in[i] = (FLOAT_NN)(frame->data[i]); }
  rnn->forward(in, MIN(frame->N,net.inputSize));
  
  // get output
  const FLOAT_NN *outp = rnn->getOutput(&N);
  
  //copy to *dst;
  for (i=0; i<MIN(frameO->N,N); i++) {
    frameO->data[i] = (FLOAT_DMEM)outp[i];
  }

  writer_->setNextFrame(frameO);

  return TICK_SUCCESS;
}

cRnnProcessor::~cRnnProcessor()
{
  if (in != NULL) free(in);
  if (out != NULL) free(out);
  if (rnn != NULL) delete rnn;
  if (classlabels_ != NULL) free(classlabels_);
  if (classlabelArr_ != NULL) free(classlabelArr_);
  if (frameO != NULL) delete frameO;
}

#endif // BUILD_RNN
