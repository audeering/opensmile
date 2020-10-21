/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


/*  openSMILE component:

example dataSource
writes data to data memory...

*/


#ifndef __CBOWPRODUCER_HPP
#define __CBOWPRODUCER_HPP

#include <core/smileCommon.hpp>
#include <core/dataSource.hpp>

#define BUILD_COMPONENT_BowProducer
#define COMPONENT_DESCRIPTION_CBOWPRODUCER "This component produces a bag-of-words vector from the keyword spotter result message."
#define COMPONENT_NAME_CBOWPRODUCER "cBowProducer"

#undef class
class DLLEXPORT cBowProducer : public cDataSource {
  private:
    const char *kwList;
    const char *prefix;
    const char *textfile;
    const char *singleSentence;
    int count;
    int dataFlag;
    int kwListPrefixFilter;
    int syncWithAudio;

    int numKw;
    char ** keywords;
    
    FILE * filehandle;
    long lineNr;
    char * line;
    size_t line_n;
    int txtEof;

    int loadKwList();
    int buildBoW( cComponentMessage *_msg );
    int buildBoW( const char * str );

  protected:
    SMILECOMPONENT_STATIC_DECL_PR
    
    virtual void myFetchConfig() override;
    //virtual int myConfigureInstance() override;
    //virtual int myFinaliseInstance() override;
    virtual int processComponentMessage(cComponentMessage *_msg) override;
    virtual eTickResult myTick(long long t) override;

    virtual int configureWriter(sDmLevelConfig &c) override;
    virtual int setupNewNames(long nEl) override;

  public:
    SMILECOMPONENT_STATIC_DECL
    
    cBowProducer(const char *_name);

    virtual ~cBowProducer();
};




#endif // __CBOWPRODUCER_HPP
