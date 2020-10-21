/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


#ifndef __KWSJ_RESULT_H
#define __KWSJ_RESULT_H


// STL includes for the queue
#include <queue>

// A message received from the turn detector.
struct TurnMsg {
    TurnMsg() : time(0), vIdx(0), isForcedTurnEnd(0) {}
    double time;     // smile time or whatever
    long vIdx;        // vector index
    int isForcedTurnEnd;  // forced turn end = at end of input
    long midx;
};

// A queue of turn detector messages.
typedef std::queue<TurnMsg> TurnMsgQueue;


/* keyword spotter result message */
#define MAXNUMKW 100
// old message:
typedef struct kresult {
  int numOfKw;
  int turnDuration;
  double kwStartTime[MAXNUMKW]; // time in seconds
  const char* keyword[MAXNUMKW];
  float kwConf[MAXNUMKW];
} Kresult;


// new message:
#define JULIUSRESULT_FLAG_KEYWORD   1

class juliusResult {
  public:
    int numW;
    char ** word; // word array
    unsigned int * wordFlag;
    char ** phstr; // phoneme string for each word (phonemes are space separated)
    float * conf;
    float * start;
    float * end;
    long userFlag;
    double amScore, lmScore, score;
    double duration;

    juliusResult() : numW(0), word(NULL), phstr(NULL), conf(NULL), start(NULL), end(NULL), wordFlag(NULL), amScore(0.0), lmScore(0.0), score(0.0), duration(0.0), userFlag(0) {}

    juliusResult( int N ) : numW(N), word(NULL), phstr(NULL), conf(NULL), start(NULL), end(NULL), wordFlag(NULL), amScore(0.0), lmScore(0.0), score(0.0), duration(0.0), userFlag(0) {
      if (numW > 0) {
        word = (char**) calloc(1, sizeof(char*) * numW * 2);
        phstr = word + numW;
        conf = (float*) calloc(1, sizeof(float) * numW * 3);
        start = conf + numW;
        end = start + numW;
        wordFlag = (unsigned int*)calloc(1,sizeof(unsigned int) * numW);
      }
    }

    ~juliusResult() {
      int i;
      if (word != NULL) {
        for (i=0; i<numW*2; i++) { if (word[i] != NULL) free(word[i]); }
        free(word);
      }
      if (conf != NULL) { free(conf); }
      if (wordFlag != NULL) { free(wordFlag); }
    }
};

#endif // __KWSJ_RESULT_H
