/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


/*  openSMILE component:

BLSTM RNN processor

*/


#ifndef __CRNNPROCESSOR_HPP
#define __CRNNPROCESSOR_HPP

#include <core/smileCommon.hpp>

#ifdef BUILD_RNN
#include <core/dataProcessor.hpp>
#include <rnn/rnn.hpp>

#define COMPONENT_DESCRIPTION_CRNNPROCESSOR "BLSTM RNN processor."
#define COMPONENT_NAME_CRNNPROCESSOR "cRnnProcessor"

class cRnnProcessor : public cDataProcessor {
  private:
    const char *netfile;
    char *classlabels_;
    const char **classlabelArr_;
    long nClasses;
    cNnRnn *rnn;
    cRnnNetFile net;
    FLOAT_NN *in;
    FLOAT_DMEM *out;
    int printConnections;
    cVector *frameO;
    int jsonNet;
    int net_created_;

  protected:
    SMILECOMPONENT_STATIC_DECL_PR

    virtual void myFetchConfig() override;
    virtual int myConfigureInstance() override;
    virtual int myFinaliseInstance() override;
    virtual eTickResult myTick(long long t) override;

    virtual int setupNewNames(long nEl) override;

  public:
    SMILECOMPONENT_STATIC_DECL
    
    cRnnProcessor(const char *_name);

    virtual ~cRnnProcessor();
};


#endif // BUILD_RNN

#endif // __CRNNPROCESSOR_HPP
