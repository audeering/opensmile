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


#ifndef __CRNNSINK_HPP
#define __CRNNSINK_HPP

#include <core/smileCommon.hpp>

#ifdef BUILD_RNN
#include <core/dataSink.hpp>
#include <rnn/rnn.hpp>

#define COMPONENT_DESCRIPTION_CRNNSINK "This is an example of a cDataSink descendant. It reads data from the data memory and prints it to the console. This component is intended as a template for developers."
#define COMPONENT_NAME_CRNNSINK "cRnnSink"





class cRnnSink : public cDataSink {
  private:
    FILE *outfile;
    FILE *outfileC;
    const char *netfile;
    const char *actoutput;
    const char *classoutput;
    char *classlabels;
    const char **classlabelArr;
    long nClasses;
    int ctcDecode;
    cNnRnn *rnn;
    cRnnNetFile net;
    FLOAT_NN *in;
    FLOAT_DMEM *out;
    int lasti;
    int printConnections;

  protected:
    SMILECOMPONENT_STATIC_DECL_PR
    
    //cRnnWeightVector *createWeightVectorFromLine(char *line);
    //int loadNet(const char *filename);
    
//    int findPeepWeights(unsigned long id);
    //int findWeights(unsigned long idFrom, unsigned long idTo);
    //cNnLSTMlayer *createLstmLayer(int i, int idx, int dir=LAYER_DIR_FWD);
    // create a network from a successfully loaded net config file (loadNet function)
    //int createNet();
    
    virtual void myFetchConfig() override;
    virtual int myConfigureInstance() override;
    virtual int myFinaliseInstance() override;
    virtual eTickResult myTick(long long t) override;

  public:
    SMILECOMPONENT_STATIC_DECL
    
    cRnnSink(const char *_name);

    virtual ~cRnnSink();
};


#endif // BUILD_RNN

#endif // __CRNNSINK_HPP
