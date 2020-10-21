/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


/*  openSMILE component:

TUM keyword spotter (Julius version)

*/


#ifndef __CJULIUSSINK_HPP
#define __CJULIUSSINK_HPP

#include <core/smileCommon.hpp>

//#define HAVE_JULIUSLIB
#ifdef HAVE_JULIUSLIB

#include <core/dataSink.hpp>

// STL includes for the queue
#include <queue>

// apr seems to define its own true, so we undefine it here... (we just have to make sure that we include this header file after any other apr/semaine/etc. headers
#ifdef TRUE
#undef TRUE
#endif 

extern "C" {
#undef LOG_DEBUG
#include <julius/juliuslib.h>
// julius defines min, max, abs, which collides with GNU stdlib and XQilla abs()
// there might be more of these silly lowercase(!) defines... Isn't it common practice to have all defines in uppercase where possible??
#undef abs
#undef min
#undef max
}


#define COMPONENT_DESCRIPTION_CJULIUSSINK "keyword spotter using Julius decoder"
#define COMPONENT_NAME_CJULIUSSINK "cJuliusSink"

#define MAX_PHONEME_STRLEN  5

#include <classifiers/julius/kwsjKresult.h>

#undef class




class DLLEXPORT cJuliusSink : public cDataSink {
  private:
    TurnMsgQueue tsq;   // turn start queue
    TurnMsgQueue teq;   // turn end queue
    int maxTurnQue;
    int running;
    int nopass2;
    int offlineMode;
    int noTurns;
    int purgeQue;
    long midx;

    const char * configfile;
    smileMutex terminatedMtx;
    smileThread decoderThread;
    smileMutex dataFlgMtx;
    smileCond  tickCond;
    int juliusIsSetup, juliusIsRunning;
    double period;

    int sendKwsResult;
    int sendJuliusResult;
    const char * kwsResultRecp;
    const char * juliusResultRecp;

 

    // number of silence frames to keep at beginning and end of turn
    int preSil, postSil;
    long lag, endWait;
    int nPre, nPost;
    long curVidx, vIdxStart, vIdxEnd;

	  const char * logfile;
    FILE *fp;
    Recog *recog;
    Jconf *jconf;

    int decoderThreadPriority;

    bool terminated;
    int dataFlag;
    int turnEnd; int turnStart; int isTurn;
    const cVector *curVec;

    double turnStartSmileTime, turnStartSmileTimeLast, turnStartSmileTimeCur;

    const char *keywordList;
    char **keywords;
    int kwIndexStart[27*26];
    int kwIndexEnd[27*26];
    int numKw;
    int nExclude;
    const char **excludeWords;

    /*
    char * sentence;
    int speakingIndex,seqIdx;
    int nFeaturesSelected;
    int *featureIndex;
    long long curTime;
    int wasSpeaking;
  */  
    
/*
int prevSpeakingIndex;
    int thisSpeakingIndex;
    int countdown;
    bool outputtrigger;
*/

    /* required by julius output functions ? */
    int writelen;
    int wst;

    int setupJulius();
    int startJuliusDecoder();

    void setupCallbacks(Recog *recog, void *data);

//    void juPutHypoPhoneme(WORD_ID *seq, int n, WORD_INFO *winfo);
    void juAlignPass1Keywords(RecogProcess *r, HTK_Param *param);

    int excludeWord(const char * w);
    juliusResult * fillJresult(Sentence *s, WORD_INFO *winfo, int numOfFrames);
    void loadKeywordList();
    int makeKwIndex(const char *kw);
    int isKeyword(const char *word);
    int tagKeywords(juliusResult *r);
    void keywordFilter(juliusResult *r);

    int checkMessageQueue();
    
    
  protected:
    SMILECOMPONENT_STATIC_DECL_PR
    int printResult;

    virtual void myFetchConfig() override;
    //virtual int myConfigureInstance() override;
    virtual int myFinaliseInstance() override;
    virtual eTickResult myTick(long long t) override;

    //virtual void processResult(long long tick, long frameIdx, double time) override;
    virtual int processComponentMessage(cComponentMessage *_msg) override;

  public:
    //static sComponentInfo * registerComponent(cConfigManager *_confman);
    //static cSmileComponent * create(const char *_instname);
    SMILECOMPONENT_STATIC_DECL

   
    void ATKresultThread();
    cJuliusSink(const char *_name);

    void juliusDecoderThread();

      
      /* callbacks for julius : */

    int getFv(float *vec, int n);

    LOGPROB cbUserlmUni(WORD_INFO *winfo, WORD_ID w, LOGPROB ngram_prob);
    LOGPROB cbUserlmBi(WORD_INFO *winfo, WORD_ID context, WORD_ID w, LOGPROB ngram_prob);
    LOGPROB cbUserlmLm(WORD_INFO *winfo, WORD_ID *contexts, int clen, WORD_ID w, LOGPROB ngram_prob);

    void cbResultPass1(Recog *recog, void *dummy);
    void cbResultPass1Current(Recog *recog, void *dummy);
    void cbStatusPass1Begin(Recog *recog, void *dummy);

    void cbResultPass2(Recog *recog, void *dummy);

    virtual ~cJuliusSink();
};



#endif //HAVE_JULIUSLIB



#endif // __CJULIUSSINK_HPP
