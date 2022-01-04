/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


/*  openSMILE component:

example dataSink:
reads data from data memory and outputs it to console/logfile (via smileLogger)
this component is also useful for debugging

*/


#ifndef __CSVMSINK_HPP
#define __CSVMSINK_HPP

#include <core/smileCommon.hpp>

#ifdef BUILD_SVMSMO

#include <core/dataSink.hpp>
#define BUILD_COMPONENT_SvmSink
#define COMPONENT_DESCRIPTION_CSVMSINK "This is an example of a cDataSink descendant. It reads data from the data memory and prints it to the console. This component is intended as a template for developers."
#define COMPONENT_NAME_CSVMSINK "cSvmSink"


#define SMILESVM_KERNEL_UNKNOWN 0  // unknown or unsupported kernel type

// supported kernel types:
#define SMILESVM_KERNEL_LINEAR        1
#define SMILESVM_KERNEL_POLYNOMIAL    2
#define SMILESVM_KERNEL_RBF           3
#define SVMSINK_WINNING_CLASS_PROB	  0
#define SVMSINK_WINNING_CLASS_VOTE	  1


struct smileBinarySvmModel
{
  int nSV;
  int has_logit;
  char * c1; int ic1;
  char * c2; int ic2;
  FLOAT_DMEM * SV;
  FLOAT_DMEM bias;
  double logit_coeff1;
  double logit_intercept;
};


class smileSvmModel
{

private:
  int showStatsDebug;
  int lineNr;
  int kernelType;
  int nClasses;
  int nPairs;
  char ** classnames;
  int vectorDim;
  char ** attributeNames;
  struct smileBinarySvmModel * binSvm;
  int * ftSelMap;
  int nFtSel;
  int ignoreLogitModel;

  const char * model;

  cSmileComponent * parent;
  const char * getInstName() { return parent->getInstName(); }

  int prepareModelStruct();
  int parseKernelType(char *line);
  int parseNClasses(char * line);
  char ** parseClassnames(char * line, int nClasses);
  int parseInt(char * line);
  FLOAT_DMEM parseDouble(char * line);
  FLOAT_DMEM parseBias(char * line);
  int parseLogisticModel(struct smileBinarySvmModel *_s, char *inp);
  int parseClassPair(struct smileBinarySvmModel *_s, char *inp);
  int parseSVs(FLOAT_DMEM **_SV, int *_nSV, char * line, int idx);
  int parseEnd(char * line);

  // get the distance from the hyperplane of model <index> for the vector in <v>
  double evalBinSvm(FLOAT_DMEM *v, int index);
  double evalLogit(int index, double d);
  FLOAT_DMEM * getPairwiseProbabilities(double **ps);

public:
  smileSvmModel(cSmileComponent *_parent, const char * _model) : parent(_parent), model(_model),
    classnames(NULL), attributeNames(NULL), binSvm(NULL), ftSelMap(NULL), nFtSel(0), showStatsDebug(0),
    ignoreLogitModel(0)
    {}

  void setIgnoreLogitModel() {
    ignoreLogitModel = 1;
  }

  // load the model from file
  int load();

  int getNclasses() { return nClasses; }

  // compute the output for a given vector v (length N, must match vectorDim variable (vector size in model))
  int evaluate(FLOAT_DMEM *v , long N, const char ** winningClass = NULL, FLOAT_DMEM ** probs=NULL, FLOAT_DMEM *conf=NULL, int winningClassMethod=SVMSINK_WINNING_CLASS_VOTE, int usePairwiseCoupling=1);

  const char * className(int i) {
    if (i<nClasses && i >= 0) 
      return classnames[i];
    else
      return NULL;
  }

  int getClassIndex(const char * c);
  void setStatsDebug() { showStatsDebug = 1; }

  int buildFtSelMap(int n, const char * name);
  int checkFtSelMap();

  ~smileSvmModel();
};


class cSvmSink : public cDataSink {
  private:
    const char * model;
    const char * resultRecp;
    const char * resultMessageName;
    const char * resultFile;
    const char * winningClassMethodName;
    const char * instName_;
    int sendResult;
    int printResult;
    int saveResult;
    bool appendSaveResult_;
    int printParseableResult;
    int ignoreLogitModel;
    int winningClassMethod, usePairwiseCoupling;

  protected:
    SMILECOMPONENT_STATIC_DECL_PR
    
    smileSvmModel * modelObj;

    virtual void myFetchConfig() override;
    //virtual int myConfigureInstance() override;
    virtual int myFinaliseInstance() override;
    virtual eTickResult myTick(long long t) override;

  public:
    SMILECOMPONENT_STATIC_DECL
    
    cSvmSink(const char *_name);

    virtual ~cSvmSink();
};


#endif // BUILD_SVMSMO

#endif // __CSVMSINK_HPP
