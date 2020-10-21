/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


/*
 * Some code in this file is taken from the Julius LVCSR engine, which is copyright by:
 *
 * Copyright (c) 1991-2007 Kawahara Lab., Kyoto University
 * Copyright (c) 2000-2005 Shikano Lab., Nara Institute of Science and Technology
 * Copyright (c) 2005-2007 Julius project team, Nagoya Institute of Technology
 * All rights reserved
 *
 * See the file JULIUS_LICENSE for more details
 *
 */


#include <classifiers/julius/juliusSink.hpp>
#define MODULE "cJuliusSink"

#ifdef HAVE_JULIUSLIB

SMILECOMPONENT_STATICS(cJuliusSink)

SMILECOMPONENT_REGCOMP(cJuliusSink)
{
  SMILECOMPONENT_REGCOMP_INIT
  scname = COMPONENT_NAME_CJULIUSSINK;
  sdescription = COMPONENT_DESCRIPTION_CJULIUSSINK;

  // we inherit cDataSink configType and extend it:
  SMILECOMPONENT_INHERIT_CONFIGTYPE("cDataSink")

    SMILECOMPONENT_IFNOTREGAGAIN_BEGIN
    ct->makeMandatory(ct->setField("configfile","Julius configfile to load (path is relative to smile executable)","kws.cfg"));
    ct->setField("logfile","Julius logfile (default=(null) : no log file)",(const char*)NULL);
    ct->setField("debug","1 = show julius debug log output, 0 = don't show",0);
    ct->setField("preSil","extra (silence) amount of data at beginning of turn to keep (in seconds)",0.3);
    ct->setField("postSil","extra (silence) amount of data at end of turn to keep (in seconds)",0.4);
    ct->setField("nopass2","1 = do not perform pass2 (you must also set the '-1pass' option in the julius config file!)",0);
    ct->setField("printResult","print output packages to console (1/0=yes/no)",1);

    ct->setField("juliusResultRecp","component(s) to send 'juliusResult' messages to (use , to separate multiple recepients), leave blank (NULL) to not send any messages. The 'juliusResult' message contains the full julius result package, which includes the full word string, alignments (if available) and the word scores as well as am and lm scores.",(const char *) NULL);
    ct->setField("kwsResultRecp","component(s) to send 'asrKeywordOutput' messages to (use , to separate multiple recepients), leave blank (NULL) to not send any messages. The keyword result package only contains selected keywords (given in 'keywordList') from the full julius result package.",(const char *) NULL);
    ct->setField("keywordList","name of text file to load keyword list from. The file must contain the keywords (all uppercase) sorted in alphabetical order, each keyword on a line.",(const char*)NULL);
    ct->setField("excludeWords","array of strings (words) to exclude from result packages. Typically you would use this to exclude sentence start and end marks such as <s> or </s>. The default is an empty list, i.e. nothing will be excluded.",(const char *)NULL,ARRAY_TYPE);

    ct->setField("noTurns","1 = don't use turn segmentation info, simply pass all available input data to julius (you can either do VAD with julius then, or use this feature in offline mode when you use presegmented turns) (0 = segment input based on cTurnDetector messages).",0);
    ct->setField("offlineMode","1 = off-line evaluation mode (this changes the exit behaviour of the decoder; in online mode the decoder will always return 1 (busy) when decoding; in off-line mode it will return 0 if it is processing if not in the EOI processing loop. This option will also set maxTurnQue=0 (infinite).",0);
    ct->setField("maxTurnQue","maximum number of speech turns to hold in que, set this to 0 (infinite) for offline evaluations, and 2 (or more) for online mode",2);
    ct->setField("decoderThreadPriority","the thread priority of the decoder thread (currently only supported on windows), values -15 (idle) to 15 (real-time), while 0 is normal. This should be lower as or equal to the priority of the openSMILE main thread!",0);

  SMILECOMPONENT_IFNOTREGAGAIN_END

    SMILECOMPONENT_MAKEINFO(cJuliusSink);
}

SMILECOMPONENT_CREATE(cJuliusSink)

//-----

cJuliusSink::cJuliusSink(const char *_name) :
cDataSink(_name),
configfile(NULL),
logfile(NULL),
printResult(0),
juliusIsSetup(0), juliusIsRunning(0),
dataFlag(0),
turnEnd(0), turnStart(0), isTurn(0), endWait(-1),
lag(0), nPre(0), nPost(0),
curVidx(0), vIdxStart(0), vIdxEnd(0), wst(0), writelen(0), juliusResultRecp(NULL), kwsResultRecp(NULL), period((FLOAT_DMEM)0.0),
turnStartSmileTimeLast((FLOAT_DMEM)0.0), turnStartSmileTime((FLOAT_DMEM)0.0), turnStartSmileTimeCur((FLOAT_DMEM)0.0),
decoderThread(NULL), running(0),
sendKwsResult(0), sendJuliusResult(0), numKw(0),
keywordList(NULL), keywords(NULL),
excludeWords(NULL), nExclude(0),
purgeQue(0), midx(0)
{
  smileMutexCreate(terminatedMtx);
  smileMutexCreate(dataFlgMtx);
  smileCondCreate(tickCond);
}

void cJuliusSink::myFetchConfig()
{
  cDataSink::myFetchConfig();

  configfile = getStr("configfile");
  SMILE_IDBG(2,"ASR configfile to load = '%s'",configfile);

  logfile = getStr("logfile");
  if (logfile != NULL) { SMILE_IDBG(2,"Julius logfile = '%s'",logfile); }

  keywordList = getStr("keywordList");
  if (keywordList != NULL) { SMILE_IDBG(2,"keyword selection list to use = '%s'",keywordList); }

  juliusResultRecp = getStr("juliusResultRecp");
  SMILE_IDBG(2,"juliusResultRecp = '%s'",juliusResultRecp);
  if (juliusResultRecp != NULL) sendJuliusResult = 1;

  kwsResultRecp = getStr("kwsResultRecp");
  SMILE_IDBG(2,"kwsResultRecp = '%s'",kwsResultRecp);
  if (kwsResultRecp != NULL) sendKwsResult = 1;

  printResult = getInt("printResult");
  SMILE_IDBG(2,"printResult = %i",printResult);

  nopass2 = getInt("nopass2");
  noTurns = getInt("noTurns");
  offlineMode = getInt("offlineMode");
  maxTurnQue = getInt("maxTurnQue");
  if (offlineMode) maxTurnQue = 0;

  nExclude = getArraySize("excludeWords");
  if (nExclude>0) {
    excludeWords = (const char**)calloc(1,sizeof(const char*)*nExclude);
    int i;
    for (i=0; i<nExclude; i++) { 
      excludeWords[i] = getStr_f(myvprint("excludeWords[%i]",i)); 
      //SMILE_IDBG(2,"excludeWords[%i] = '%s'",i,excludeWords[i] );
    }
  }

  decoderThreadPriority = getInt("decoderThreadPriority");
  if (decoderThreadPriority < -15) decoderThreadPriority = -15;
  if (decoderThreadPriority > 15) decoderThreadPriority = 15;

}


/********************************** Julius callback loaders ***********************************/

int external_fv_read_loader(void * cl, float *vec, int n) 
{
  // retval -2 : error
  // retval -3 : segment input
  // retval -1 : end of input
  if (cl!=NULL) return ((cJuliusSink*)cl)->getFv(vec, n);
  else return -1; 
}


static void result_pass2_loader(Recog *recog, void *dummy)
{
  cJuliusSink * cl = (cJuliusSink *)(recog->hook);
  if (cl!=NULL) cl->cbResultPass2(recog, dummy);
}



static void status_pass1_begin_loader(Recog *recog, void *dummy)
{
  cJuliusSink * cl = (cJuliusSink *)(recog->hook);
  if (cl!=NULL) cl->cbStatusPass1Begin(recog, dummy);
}

static void result_pass1_loader(Recog *recog, void *dummy)
{
  cJuliusSink * cl = (cJuliusSink *)(recog->hook);
  if (cl!=NULL) cl->cbResultPass1(recog, dummy);
}

void result_pass1_current_loader(Recog *recog, void *dummy)
{
  cJuliusSink * cl = (cJuliusSink *)(recog->hook);
  if (cl!=NULL) cl->cbResultPass1Current(recog, dummy);
}



/********************************** Julius output formatting functions ******************************/

void cJuliusSink::juAlignPass1Keywords(RecogProcess *r, HTK_Param *param)
{
//  int n;
  Sentence *s;
  SentenceAlign *now;//, *prev;


  s = &(r->result.pass1);
  /* do forced alignment if needed */
  if ((r->config->annotate.align_result_word_flag)&&(s->word_num>0)) {
    //now = result_align_new();
    //if (s->align == NULL) 
      now = result_align_new(); //else now = s->align;
    word_align(s->word, s->word_num, param, now, r);
    if (s->align == NULL) //result_align_free(s->align);
      s->align = now;
    else {
      result_align_free(s->align);
      s->align = now;
    }
//    else prev->next = now;
//    prev = now;
  }
} 

/*
void cJuliusSink::juPutHypoPhoneme(WORD_ID *seq, int n, WORD_INFO *winfo)
{
  int i,j;
  WORD_ID w;
  static char buf[MAX_HMMNAME_LEN];

  if (seq != NULL) {
    for (i=0;i<n;i++) {
      if (i > 0) printf(" |");
      w = seq[i];
      for (j=0;j<winfo->wlen[w];j++) {
        center_name(winfo->wseq[w][j]->name, buf);
        printf(" %s", buf);
      }
    }
  }
  printf("\n");  
}
*/

int cJuliusSink::excludeWord(const char *w) 
{
  int i;
  if (w==NULL) return 1;
  for (i=0; i<nExclude; i++) {
    //SMILE_IDBG(3,"word='%s'   excludeWords[%i]='%s'",w,i,excludeWords[i]);
    if ((excludeWords[i] != NULL) && (!strcmp(w,excludeWords[i]))) return 1;
  }
  return 0;
}

juliusResult * cJuliusSink::fillJresult(Sentence *s, WORD_INFO *winfo, int numOfFrames)
{
  juliusResult * res = NULL;
  static char buf[MAX_HMMNAME_LEN];
  int i,wi;

  if (s->word != NULL && s->word_num > 0) {
    res = new juliusResult(s->word_num);
    wi=0;
    for (i=0;i<res->numW;i++) {
      if (!excludeWord(winfo->woutput[s->word[wi]])) {

        res->word[i] = strdup(winfo->woutput[s->word[wi]]);
        if (s->align != NULL) {
          res->start[i] = (float)(period * (double)(s->align->begin_frame[wi]));
          res->end[i] = (float)(period * (double)(s->align->end_frame[wi]));
        }
        if (s->confidence != NULL) {
          res->conf[i] = s->confidence[wi];
          if (res->conf[i] <= 0.0) res->conf[i] = 0.2;
        } else {
          res->conf[i] = 0.2;
        }

        // allocate temporary phoneme string
        int len = sizeof(char)* winfo->wlen[s->word[wi]] * MAX_PHONEME_STRLEN - 1;
        char * ph = (char*)malloc(len+1);
        char * phc = ph; *ph = 0; size_t ptr = 0; 
  /*      // copy phoneme sequence to string
        for (j=0;j<winfo->wlen[s->word[wi]];j++) {
          center_name(winfo->wseq[s->word[wi]][j]->name, buf);
          strncpy(phc,buf,MIN((unsigned)(len-ptr),strlen(buf)));
          size_t x = strlen(buf);
          phc += x;
          ptr += x;
          if (len-ptr > 0) { *(phc++) = ' '; ptr++; }
          if (len-ptr >= 0) *(phc) = 0;
          //printf("strlen(buf) = %i\n",strlen(buf));
        }*/
        // save phoneme string for word i
        res->phstr[i] = ph;
      } else { 
        i--; 
        res->numW--; 
      }
      wi++;
    }
    res->duration = (double)numOfFrames * period;
    res->amScore = s->score_am;
    res->lmScore = s->score_lm;
    res->score = s->score;
  }
  return res;
}

int cJuliusSink::makeKwIndex(const char *kw)
{
//printf(">>-<< word : '%s' \n",kw);
  int idx= -1;
  int lt = ((int)kw[0]-(int)'A');
//printf(">>-<< lt : %i \n",lt);
  if ((lt >= 0)&&(lt<26)) {
    idx = lt * 27;
    if (kw[1] != 0) {
      int lt2 = ((int)kw[1]-(int)'A');
//printf(">>-<< lt2 : %i \n",lt2);
      if ((lt2 >= 0)&&(lt2<26)) {
        idx += lt2 + 1;
      }
    }
  }
//printf(">>-<< idx : %i \n",idx);
  return idx;
}

void cJuliusSink::loadKeywordList()
{
  // load keyword list
  if (keywordList != NULL) {
    FILE *kwl = fopen(keywordList,"r");
    if (kwl != NULL) {
      char *line=NULL; size_t len=0; size_t r=-1;
      r = smile_getline(&line, &len, kwl);
      if ((r != (size_t)-1)&&(line!=NULL)) {
        char *tmp=NULL;
        numKw = strtol(line,&tmp,10);
        if ((tmp == NULL)&&(*tmp == 0)) {
          SMILE_IERR(1,"expected the number of keywords in keyword list file on the first line in the keyword list file '%s'. The first line of the file must be a number, we found the following: '%s' (make sure to remove any whitespaces from the line, so it contains only a digits).",keywordList,line);
          numKw = 0;
        }
      }
      if (numKw > 0) { 
        keywords = (char**)malloc(sizeof(char *)*numKw);
        long kwNum = 0;
        do {      
          r = smile_getline(&line, &len, kwl);
          char *tmp = line;
          if (tmp != NULL) {
            // remove newline:
            size_t x = strlen(tmp);
            if (tmp[x-1] == '\n') { tmp[x-1] = 0; x--; }
            if (tmp[x-1] == '\r') { tmp[x-1] = 0; x--; }
            // space removal code:
            while ((*tmp==' ')||(*tmp=='\t')) { tmp++; x--; }
            while ( (x>0)&& ((*(tmp+x-1) == ' ')||(*(tmp+x-1) == '\t')) ) { *(tmp+x-1)=0; x--; }
            if (x>0) { // add keyword:
              keywords[kwNum] = strdup(tmp);
              kwNum++;
            }
          }
        } while ((r != (size_t)-1) && (kwNum<numKw)); 
        SMILE_IMSG(3,"loaded %i of %i expected keywords from '%s'.",kwNum,numKw,keywordList);
      }
      fclose(kwl);
      if (line != NULL) free(line);
    } else {
      SMILE_IERR(1,"cannot open keyword file '%s', check the filename and the file permissions!",keywordList);
    }

    // create index for first two letters to speed up search
    int i; char lastw[2]; lastw[0] = 'A'; lastw[1] = 0; int lastIdx = -1;
    for (i=0; i<26*27; i++) { kwIndexStart[i] = -1; }
    for (i=0; i<26*27; i++) { kwIndexEnd[i] = -1; }
    
    for (i=0; i<numKw; i++) {
      if (keywords[i][0] != 0) {
        if ((keywords[i][0] != lastw[0])||(keywords[i][1] != lastw[1])) { 
          if (lastIdx >= 0) {
            kwIndexEnd[ lastIdx ] = i;
          }
          int idx = makeKwIndex(keywords[i]);
          if ((idx >= 0)&&(idx<27*26)) {
            kwIndexStart[ idx ] = i;
            lastIdx = idx;
          }
          lastw[0] = keywords[i][0]; 
          lastw[1] = keywords[i][1];
        }
      }
    }

  }
}

int cJuliusSink::isKeyword(const char *word)
{
  int iskw = 0;
  if ((word != NULL)&&(word[0] != 0)) {
    int i;
    int idx = makeKwIndex(word);
    int s=0, e=numKw; // the word index in the keyword list
    if ((idx >= 0)&&(idx<27*26)) { 
      s = kwIndexStart[idx]; 
      e = kwIndexEnd[idx];
      //printf("ISKWW: w='%s' idx=%i s=%i  e=%i\n",word,idx,s,e);
      if (s>=0) {
        for (i=s; i<e; i++) {
          //printf("  ISKWW: kw[%i]='%s' idx=%i  s=%i  e=%i\n",i,keywords[i],idx,s,e);
          if (!strcmp(word,keywords[i])) { iskw = 1; break; }
        }
      }
    }
  }
  return iskw;
}


// return value: number of keywords
int cJuliusSink::tagKeywords(juliusResult *r)
{
  int nkw = 0;
  if (r==NULL) return 0;
  if ((keywordList != NULL)&&(keywords != NULL)) {
    int i; 
    for (i=0; i<r->numW; i++) {
      if (isKeyword(r->word[i])) {
        r->wordFlag[i] |= JULIUSRESULT_FLAG_KEYWORD;
        nkw++;
      }
    }
  }
  return nkw;
}

// you must call tagKeywords before calling this function
// this function removes all non keywords, and updates r->numW
// all unused word/phoneme strings are freed
void cJuliusSink::keywordFilter(juliusResult *r)
{
  if (keywordList != NULL) {
    int i; int kwc = 0;
    for (i=0; i<r->numW; i++) {
      if (r->wordFlag[i] & JULIUSRESULT_FLAG_KEYWORD) {
        if (kwc != i) { // copy to left...
          r->conf[kwc] = r->conf[i];
          r->start[kwc] = r->start[i];
          r->end[kwc] = r->end[i];
          r->wordFlag[kwc] = r->wordFlag[i];

          if (r->word[kwc] != NULL) free(r->word[kwc]);
          r->word[kwc] = r->word[i]; r->word[i] = NULL;
          if (r->phstr[kwc] != NULL) free(r->phstr[kwc]);
          r->phstr[kwc] = r->phstr[i]; r->phstr[i] = NULL;
        }
        kwc++;
      }
    }
    for (i=kwc; i<r->numW; i++) {
      if (r->word[i] != NULL) free(r->word[i]);
      r->word[i] = NULL;
      if (r->phstr[i] != NULL) free(r->phstr[i]);
      r->phstr[i] = NULL;
    }
    r->numW = kwc;
  }
}


/************************************ Callback handlers ************************************************/


void cJuliusSink::cbStatusPass1Begin(Recog *recog, void *dummy)
{
  if (!recog->jconf->decodeopt.realtime_flag) {
    VERMES("### Recognition: 1st pass (LR beam)\n");
  }
  wst = 0;
}


void cJuliusSink::cbResultPass1Current(Recog *recog, void *dummy)
{
  RecogProcess *r;
  r=recog->process_list;
  if (! r->live) return;
  if (! r->have_interim) return;
  Sentence *s = &(r->result.pass1);

  if ((s->word_num>0)&&(!isAbort())) {
    //juAlignPass1Keywords(r, r->am->mfcc->param);

    // this call creates a new class instance which will be freed after sending the message
    // since the message can be sent to multiple receipients, the receivers may not free the object!
    juliusResult *jr = fillJresult(s, r->lm->winfo, r->result.num_frame);
    if (jr == NULL) return;

    tagKeywords(jr);

    //create smile message:
    if (juliusResultRecp != NULL) {
      cComponentMessage msg("juliusResult");
      msg.intData[0] = 0; // pass: 0=current, 1=pass1, 2=pass2
      msg.custData = jr;
      msg.userTime1 = turnStartSmileTimeCur;
      msg.userTime2 = turnStartSmileTimeCur + ((double)(jr->duration));
      sendComponentMessage( juliusResultRecp, &msg );
      SMILE_IDBG(3,"sending 'juliusResult' message to '%s'",juliusResultRecp);
    }

    //output content of r:
    if (printResult) {
      int kc = 0;
      printf("--- current hypothesis:  ");
      for (kc=0;kc<(jr->numW);kc++) {
        if (jr->wordFlag[kc] & JULIUSRESULT_FLAG_KEYWORD) {
          printf("!%s! ",jr->word[kc]);
        } else {
          printf("%s ",jr->word[kc]);
        }
      }
      printf("\n"); fflush(stdout);
    }
    if (kwsResultRecp != NULL) {
      // filter keywords based on previous keyword 'tagging':
      //keywordFilter(jr);

      cComponentMessage msg("asrKeywordOutput");
      msg.intData[0] = 0; // pass: 0=current, 1=pass1, 2=pass2
      msg.custData = jr;
      msg.userTime1 = turnStartSmileTimeCur;
      msg.userTime2 = turnStartSmileTimeCur + ((double)(jr->duration));
      sendComponentMessage( kwsResultRecp, &msg );
      SMILE_IDBG(3,"sending 'asrKeywordOutput' message to '%s'",kwsResultRecp);
    }
    delete jr;

  }
}



void cJuliusSink::cbResultPass1(Recog *recog, void *dummy)
{
  int pass = 1;
  RecogProcess *r;
  r=recog->process_list;
  if (! r->live) return;
  if (r->have_interim) return ;
  Sentence *s = &(r->result.pass1);
  if (r->result.status < 0) return;	/* search already failed  */
  //if (r->config->successive.enabled) return; /* short pause segmentation */
  //  int len;
//  boolean have_progout = TRUE;
//  if (isAbort()) return;

  if (nopass2) {
    lockMessageMemory();
    if (tsq.empty()&&(!noTurns)) running = 0;  // TODO: check this condition &&(!noTurn) ...
    unlockMessageMemory();
  }

  if ((s->word_num>0)&&(!isAbort())) {
    juAlignPass1Keywords(r, r->am->mfcc->param);

    // this call creates a new class instance which will be freed after sending the message
    // since the message can be sent to multiple receipients, the receivers may not free the object!
    juliusResult *jr = fillJresult(s, r->lm->winfo, r->result.num_frame);
    tagKeywords(jr);

    //create smile message:
    if (juliusResultRecp != NULL) {
      cComponentMessage msg("juliusResult");
      msg.intData[0] = pass; // pass: 0=current, 1=pass1, 2=pass2
      msg.custData = jr;
      msg.userTime1 = turnStartSmileTimeCur;
      msg.userTime2 = turnStartSmileTimeCur + ((double)(jr->duration));
      sendComponentMessage( juliusResultRecp, &msg );
      SMILE_IDBG(3,"sending 'juliusResult' message (pass %i) to '%s'",pass,juliusResultRecp);
    }


//output content of r:
    if (printResult) {
      int kc = 0;

      printf("--- PASS %i :  ",pass);
      for (kc=0;kc<(jr->numW);kc++) {
        if (jr->wordFlag[kc] & JULIUSRESULT_FLAG_KEYWORD) {
          printf("!%s! ",jr->word[kc]);
        } else {
          printf("%s ",jr->word[kc]);
        }
      }
      printf("\n"); fflush(stdout);

      printf(" confidence:  ");
      for (kc=0;kc<(jr->numW);kc++) {
        printf("%.3f ",jr->conf[kc]);
      }
      printf("\n"); fflush(stdout);

      printf("   phonemes:  ",pass);
      for (kc=0;kc<(jr->numW);kc++) {
        if (jr->phstr[kc] != NULL) {
          if (jr->wordFlag[kc] & JULIUSRESULT_FLAG_KEYWORD) {
            printf("!%s! | ",jr->phstr[kc]);
          } else {
            printf("%s | ",jr->phstr[kc]);
          }
        }
      }
      printf("\n\n"); fflush(stdout);

    }

    if (kwsResultRecp != NULL) {
      // filter keywords based on previous keyword 'tagging':
      //keywordFilter(jr);

      cComponentMessage msg("asrKeywordOutput");
      msg.intData[0] = pass; // pass: 0=current, 1=pass1, 2=pass2
      msg.custData = jr;
      msg.userTime1 = turnStartSmileTimeCur;
      msg.userTime2 = turnStartSmileTimeCur + ((double)(jr->duration));
      sendComponentMessage( kwsResultRecp, &msg );
      SMILE_IDBG(3,"sending 'asrKeywordOutput' message (pass %i) to '%s'",pass,kwsResultRecp);
    }

#if 0
    if (debug2_flag) {
    /* logical HMMs */
      printf("pass1_best_HMMseq_logical:");
      for (i=0;i<num;i++) {
        for (j=0;j<winfo->wlen[seq[i]];j++) {
          printf(" %s", winfo->wseq[seq[i]][j]->name);
        }
        if (i < num-1) printf(" |");
      }
      printf("\n");
    }
#endif
  } 

}





void cJuliusSink::cbResultPass2(Recog *recog, void *dummy)
{
  int pass = 2;
  RecogProcess *r;
  r=recog->process_list;
  if (! r->live) return;
  //Sentence *s = &(r->result.pass1);
  //if (r->result.status < 0) return;	/* search already failed  */
  //if (r->config->successive.enabled) return; /* short pause segmentation */
  //  int len;
//  boolean have_progout = TRUE;
//  if (isAbort()) return;

  lockMessageMemory();
  if (tsq.empty()&&(!noTurns)) running = 0;  // TODO: check this condidition &&(!noTurn) ...
  unlockMessageMemory();

  if (printResult) {

    if (r->result.status < 0) {
      switch(r->result.status) {
    case J_RESULT_STATUS_REJECT_POWER:
      printf("<input rejected by power>\n");
      break;
    case J_RESULT_STATUS_TERMINATE:
      printf("<input teminated by request>\n");
      break;
    case J_RESULT_STATUS_ONLY_SILENCE:
      printf("<input rejected by decoder (silence input result)>\n");
      break;
    case J_RESULT_STATUS_REJECT_GMM:
      printf("<input rejected by GMM>\n");
      break;
    case J_RESULT_STATUS_REJECT_SHORT:
      printf("<input rejected by short input>\n");
      break;
    case J_RESULT_STATUS_FAIL:
      printf("<search failed>\n");
      break;
      }
      return;
    }

  }

  /* get sentence */
  //  num = r->result.sentnum;
//assume just one sentence:
  //num = 1;
//consider just sentence with index n=0
  //n = 0;
  Sentence *s = &(r->result.sent[0]);


  if ((s->word_num>0)&&(!isAbort())) {
    //juAlignPass1Keywords(r, r->am->mfcc->param);

    // this call creates a new class instance which will be freed after sending the message
    // since the message can be sent to multiple receipients, the receivers may not free the object!
    juliusResult *jr = fillJresult(s, r->lm->winfo, r->result.num_frame);
    tagKeywords(jr);

    //create smile message:
    if (juliusResultRecp != NULL) {
      cComponentMessage msg("juliusResult");
      msg.intData[0] = pass; // pass: 0=current, 1=pass1, 2=pass2
      msg.custData = jr;
      msg.userTime1 = turnStartSmileTimeCur;
      msg.userTime2 = turnStartSmileTimeCur + ((double)(jr->duration));
      sendComponentMessage( juliusResultRecp, &msg );
      SMILE_IDBG(3,"sending 'juliusResult' message (pass %i) to '%s'",pass,juliusResultRecp);
    }

    //output content of r:
    if (printResult) {
      int kc = 0;

      printf("--- PASS %i :  ",pass);
      for (kc=0;kc<(jr->numW);kc++) {
        if (jr->wordFlag[kc] & JULIUSRESULT_FLAG_KEYWORD) {
          printf("!%s! ",jr->word[kc]);
        } else {
          printf("%s ",jr->word[kc]);
        }
      }
      printf("\n"); fflush(stdout);

      printf(" confidence:  ");
      for (kc=0;kc<(jr->numW);kc++) {
        printf("%.3f ",jr->conf[kc]);
      }
      printf("\n"); fflush(stdout);

      printf("   phonemes:  ",pass);
      for (kc=0;kc<(jr->numW);kc++) {
        if (jr->phstr[kc] != NULL) {
          if (jr->wordFlag[kc] & JULIUSRESULT_FLAG_KEYWORD) {
            printf("!%s! | ",jr->phstr[kc]);
          } else {
            printf("%s | ",jr->phstr[kc]);
          }
        }
      }
      printf("\n\n"); fflush(stdout);
    }

    if (kwsResultRecp != NULL) {
      // filter keywords based on previous keyword 'tagging':
      //keywordFilter(jr);

      cComponentMessage msg("asrKeywordOutput");
      msg.intData[0] = pass; // pass: 0=current, 1=pass1, 2=pass2
      msg.custData = jr;
      msg.userTime1 = turnStartSmileTimeCur;
      msg.userTime2 = turnStartSmileTimeCur + ((double)(jr->duration));
      sendComponentMessage( kwsResultRecp, &msg );
      SMILE_IDBG(3,"sending 'asrKeywordOutput' message (pass %i) to '%s'",pass,kwsResultRecp);
    }

  }



//  int i, j;
//  int len;
//  char ec[5] = {0x1b, '[', '1', 'm', 0};

/*  
  if (verbose_flag) {
    if (r->lmtype == LM_DFA) {
      if (multigram_get_all_num(r->lm) > 1) {
	      printf("grammar%d: %d\n", n+1, s->gram_id);
      }
    }
  }*/


}




void cJuliusSink::setupCallbacks(Recog *recog, void *data)
{
  //  callback_add(recog, CALLBACK_EVENT_PROCESS_ONLINE, status_process_online, data);
  //  callback_add(recog, CALLBACK_EVENT_PROCESS_OFFLINE, status_process_offline, data);
  //  callback_add(recog, CALLBACK_EVENT_SPEECH_READY, status_recready, data);
  //  callback_add(recog, CALLBACK_EVENT_SPEECH_START, status_recstart, data);
  //  callback_add(recog, CALLBACK_EVENT_SPEECH_STOP, status_recend, data);
  //  callback_add(recog, CALLBACK_EVENT_RECOGNITION_BEGIN, status_recognition_begin, data);
  //  callback_add(recog, CALLBACK_EVENT_RECOGNITION_END, status_recognition_end, data);
  //  if (recog->jconf->decodeopt.segment) { /* short pause segmentation */
  //    callback_add(recog, CALLBACK_EVENT_SEGMENT_BEGIN, status_segment_begin, data);
  //    callback_add(recog, CALLBACK_EVENT_SEGMENT_END, status_segment_end, data);
  //  }

  callback_add(recog, CALLBACK_EVENT_PASS1_BEGIN, status_pass1_begin_loader, data); //sets wst=0

  //  {
  //    JCONF_SEARCH *s;
  //    boolean ok_p;
  //    ok_p = TRUE;
  //    for(s=recog->jconf->search_root;s;s=s->next) {
  //      if (s->output.progout_flag) ok_p = FALSE;
  //    }
  //    if (ok_p) {      
  //      have_progout = FALSE;
  //    } else {
  //      have_progout = TRUE;
  //    }
  //  }
  //  if (!recog->jconf->decodeopt.realtime_flag && verbose_flag && ! have_progout) {
  //    callback_add(recog, CALLBACK_EVENT_PASS1_FRAME, frame_indicator, data);
  //  }

  callback_add(recog, CALLBACK_RESULT_PASS1_INTERIM, result_pass1_current_loader, data);
  callback_add(recog, CALLBACK_RESULT_PASS1, result_pass1_loader, data);

  //#ifdef WORD_GRAPH
  //  callback_add(recog, CALLBACK_RESULT_PASS1_GRAPH, result_pass1_graph, data);
  //#endif
  //  callback_add(recog, CALLBACK_EVENT_PASS1_END, status_pass1_end, data);
  //  callback_add(recog, CALLBACK_STATUS_PARAM, status_param, data);
  //  callback_add(recog, CALLBACK_EVENT_PASS2_BEGIN, status_pass2_begin, data);
  //  callback_add(recog, CALLBACK_EVENT_PASS2_END, status_pass2_end, data);

  callback_add(recog, CALLBACK_RESULT, result_pass2_loader, data); // rejected, failed

  //  callback_add(recog, CALLBACK_RESULT_GMM, result_gmm, data);
  //  /* below will be called when "-lattice" is specified */
  //  callback_add(recog, CALLBACK_RESULT_GRAPH, result_graph, data);
  //  /* below will be called when "-confnet" is specified */
  //  callback_add(recog, CALLBACK_RESULT_CONFNET, result_confnet, data);
  //
  //  //callback_add_adin(CALLBACK_ADIN_CAPTURED, levelmeter, data);
}


int cJuliusSink::setupJulius()
{
  try {

    int argc=3;
    char* argv[3] = {"julius","-C",NULL};
    if (configfile != NULL)
      argv[2]=strdup(configfile);
    else
      argv[2]=strdup("kws.cfg");

    
    /* create a configuration variables container */
    jconf = j_jconf_new();
    if (j_config_load_args(jconf, argc, argv) == -1) {
      COMP_ERR("error parsing julius decoder options, this might be a bug. see juliusSink.cpp!");
      j_jconf_free(jconf);
      free(argv[2]);
      return 0;
    }
    free(argv[2]);

    /* output system log to a file */
    if (getInt("debug") != 1) {
      jlog_set_output(NULL);
    }

    /* here you can set/modify any parameter in the jconf before setup */
    /* we add our external feature callback here */
    jconf->input.type = INPUT_VECTOR;
    jconf->input.speech_input = SP_EXTERN;
    jconf->decodeopt.realtime_flag = TRUE; // ??? 
    jconf->ext.period = (float)(reader_->getLevelT());
    jconf->ext.veclen = reader_->getLevelN();
    jconf->ext.userptr = (void *)this;
    jconf->ext.fv_read = external_fv_read_loader;

    /* Fixate jconf parameters: it checks whether the jconf parameters
    are suitable for recognition or not, and set some internal
    parameters according to the values for recognition.  Modifying
    a value in jconf after this function may be errorous.
    */
    if (j_jconf_finalize(jconf) == FALSE) {
      SMILE_IERR(1,"error finalising julius jconf in setupJulius()!");
      j_jconf_free(jconf);
      return 0;
    }

    /* create a recognition instance */
    recog = j_recog_new();
    /* use user-definable data hook to set a pointer to our class instance */
    recog->hook = (void *)this;
    /* assign configuration to the instance */
    recog->jconf = jconf;
    /* load all files according to the configurations */
    if (j_load_all(recog, jconf) == FALSE) {
      SMILE_IERR(1, "Error in loading model for Julius decoder");
      j_recog_free(recog);
      return 0;
    }

    loadKeywordList();

    /* checkout for recognition: build lexicon tree, allocate cache */
    if (j_final_fusion(recog) == FALSE) {
      fprintf(stderr, "ERROR: Error while setup work area for recognition\n");
      j_recog_free(recog);
      if (logfile) fclose(fp);
      return 0;
    }

    /* set-up the result callback hooks */
    setupCallbacks(recog, NULL);

    /* output system information to log */
    j_recog_info(recog);

    terminated = FALSE;
  }
  catch (int) { }

  juliusIsSetup=1;

  return 1;
}

int cJuliusSink::myFinaliseInstance()
{
  int ret = cDataSink::myFinaliseInstance();
  if (ret==0) return 0;

  // setup pre/post silence config:
  float _preSil = (float)getDouble("preSil");
  float _postSil = (float)getDouble("postSil");
  double _T = reader_->getLevelT();
  if (_T!=0.0) preSil = (int)ceil(_preSil/_T);
  else preSil = (int)(_preSil);
  if (_T!=0.0) postSil = (int)ceil(_postSil/_T);
  else postSil = (int)(_postSil);

  period = _T;

  // setup julius, if not already set up (-> myFinaliseInstance could be called more than once...)
  if (!juliusIsSetup) ret *= setupJulius();

  return ret;
}

/* code to receive and process turnStart and turnEnd messages */
int cJuliusSink::processComponentMessage( cComponentMessage *_msg )
{
  if (noTurns) return 0;

  if (isMessageType(_msg,"turnEnd")) {
    if (( teq.size() > (unsigned)maxTurnQue )&&(maxTurnQue > 0))  {
      // note, if this happens, we must terminate the current turn!?
      if (isTurn) {
        purgeQue = 1;
        SMILE_IMSG(3,"forced end of asr processing due to turn start/end que overflow (id=%i) [maxQue = %i]",teq.front().midx,maxTurnQue);
      } else {
        SMILE_IMSG(3,"discarded whole last turn due to turn start/end que overflow (s_id=%i e_id=%i) [maxQue = %i]",tsq.front().midx,teq.front().midx,maxTurnQue);
        tsq.pop();
      }
      teq.pop();
    }
    TurnMsg tem;
    tem.time = _msg->userTime1;
    tem.vIdx = (long)(_msg->floatData[1]);
    tem.isForcedTurnEnd = _msg->intData[1];
    tem.midx = midx++;
    teq.push(tem);
    
    /*turnEnd=1;
    nPost = (long)(_msg->floatData[0]);
    vIdxEnd = (long)(_msg->floatData[1]);
    turnStartSmileTimeLast = turnStartSmileTime;*/
    return 1;
  }
  if (isMessageType(_msg,"turnStart")) {
    TurnMsg tsm;
    tsm.time = _msg->userTime1;
    tsm.vIdx = (long)(_msg->floatData[1]);
    tsm.isForcedTurnEnd = 0;
    tsm.midx = midx++;
    tsq.push(tsm);
    vIdxStart = tsm.vIdx;
    /*
    turnStart=1;
    nPre = (long)(_msg->floatData[0]);
    vIdxStart = (long)(_msg->floatData[1]);
    turnStartSmileTime = _msg->userTime1;*/
    return 1;
  }
  return 0;
}

int cJuliusSink::checkMessageQueue()
{
    int end = 0;

    lockMessageMemory();

    // TODO: check size of message queue in online mode:
    // if messages at the front of the queue are too old, throw them away

    // handle pre/post silence and turn detector interface
    // turn start message received?
    if (!tsq.empty() && !isTurn) {
      SMILE_IDBG(4, "turn start message retrieved from queue");
      // process old turnEnd message first...
/*      if (!teq.empty() && teq.front().vIdx < tsq.front().vIdx && 
           ( (teq.front().vIdx <= curVidx + postSil) ||
              (!teq.front().isForcedTurnEnd && teq.front().vIdx <= curVidx) )) {
printf("XXXX TEMrFO %i (cur=%i) [%i]\n",teq.front().vIdx,curVidx,teq.front().midx);
        teq.pop();
        end = 1;   // stop waiting
        SMILE_IDBG(3,"processed turn end message!");  
      }*/
      running = 1;
      turnStartSmileTimeCur = tsq.front().time;
      vIdxStart = tsq.front().vIdx;
      curVidx = tsq.front().vIdx - preSil;
      isTurn = 1; // begin decoding
//printf("XXXX TSMr %i (cur=%i) [%i] (%s)\n",vIdxStart,curVidx,tsq.front().midx,getInstName());
      SMILE_IMSG(3,"processed turn start message! (midx=%i)",tsq.front().midx); 
      tsq.pop();
      
    }
    // turn end message received?
    if (!teq.empty() || purgeQue) { 
      SMILE_IDBG(4, "turn end message retrieved from queue!"); 
      // end of turn reached?
      // if turn end is "forced" (end of input), then don't wait for silence at turn end
      // - otherwise we will wait forever ...
//printf("XXXX TEMx %i (cur=%i) [%i]\n",teq.front().vIdx,curVidx,teq.front().isForcedTurnEnd);
      if (( (!teq.front().isForcedTurnEnd) && curVidx >= teq.front().vIdx + postSil) || 
          ( (teq.front().isForcedTurnEnd) && curVidx >= teq.front().vIdx ) || purgeQue)
      { 
//printf("XXXX TEMr %i (cur=%i) [%i] (%s)\n",teq.front().vIdx,curVidx,teq.front().midx,getInstName());
        isTurn = 0; 
        end=1; 
        SMILE_IMSG(3,"turn end reached! purgeQue=%i (midx=%i)",purgeQue,teq.front().midx);  
        if (!purgeQue) teq.pop();
        purgeQue = 0;
      }
      // if no frames have been written...
      /*if (curVidx == vIdxStart) { 
        SMILE_IWRN(2,"turn (vIdx %i - vIdx %i) has 0 frames (curVidx = %i)!",vIdxStart,vIdxEnd,curVidx);
        turnEnd=0; isTurn=0;
      }*/
    }
//printf("XXX isTurn=%i\n",isTurn);
    unlockMessageMemory();

    return end;
}


// this is called from julius decoder thread... 
int cJuliusSink::getFv(float *vec, int n)
{ 
  int ret=0;

  smileMutexLock(dataFlgMtx);

  if (terminated) { 
    smileMutexUnlock(dataFlgMtx);  
    recog->process_want_terminate = TRUE;
    return -1; 
  }  

  // we should wait for main thread to provide data, then set a flag that data was read..
  //SMILE_IDBG(4,"yes... julius requests features from us (n=%i)!",n);

  // wait for turn start
  int end=0;
  if (!noTurns) {
    // Here we have to use a do loop, because otherwise turn end messages
    // will never be detected.
    do {

      end = checkMessageQueue();
      if (end) ret = -3;

      if (!isTurn && !end) {
        smileCondWaitWMtx( tickCond, dataFlgMtx ); // wait one smile tick for things to change
      }

      if (terminated) { 
        smileMutexUnlock(dataFlgMtx);  
        recog->process_want_terminate = TRUE;
        return -1; 
      }  

    } while ((!isTurn)&&(!end));
  } else { 
    // if noTurn option == 1, we always set isTurn to true (1)
    isTurn = 1; 
  }

  if (isTurn && !end) {
    int result=0;
    curVec = NULL;
    while (curVec == NULL)  {
      curVec = reader_->getFrame(curVidx, -1, 0, &result);
      // check if curVidx is behind ringbuffer storage space!!
      if (result & DMRES_OORleft) {
        long tmp = reader_->getMinR()+10;
        if (tmp > curVidx) {
          SMILE_IWRN(2,"ASR processing tried to get feature data behind beginning of ringbuffer, which is no more available! Skipping a few frames to catch up... (%i) : %i -> %i",tmp-curVidx, curVidx, tmp);
          curVidx = tmp;
        }
      }
      if (curVec == NULL) { 
        if (noTurns) {
          if (isEOI()) { // TODO: test this condition!
            ret = -3; break;
          }
        } else {
          if (checkMessageQueue()) {
            ret = -3;
            break;
          }
        }
        smileCondWaitWMtx( tickCond, dataFlgMtx );  

        if (terminated) { 
          smileMutexUnlock(dataFlgMtx);  
          recog->process_want_terminate = TRUE;
          return -1; 
        }  
      }
      else {
        curVidx++;
      }
    } // while (curVec == NULL) 

  }

  //printf("turn: %f n=%i\n",curVec->dataF[curVec->N-1],curVec->N);
  smileMutexUnlock(dataFlgMtx);

  int i;
  if (curVec != NULL) {
    for (i=0; i<MIN( curVec->N, n ); i++) {
      vec[i] = (float)(curVec->dataF[i]);
    }
  } else {
    for (i=0; i<n; i++) {
      vec[i] = 0.0;
    }
  }

  return ret;
}

/**** Julius Decoder Thread *****/

SMILE_THREAD_RETVAL juliusThreadRunner(void *_obj)
{
  cJuliusSink * __obj = (cJuliusSink *)_obj;
  if (_obj != NULL) {
    // ensures log messages created in this thread are sent to the global logger
    __obj->getLogger()->useForCurrentThread();
    __obj->juliusDecoderThread();
  }
  SMILE_THREAD_RET;
}

void cJuliusSink::juliusDecoderThread()
{
  bool term;
  smileMutexLock(terminatedMtx);
  term = terminated;
  smileMutexUnlock(terminatedMtx);

  /* enter recongnition loop */
  int ret = j_open_stream(recog, NULL);
  switch(ret) {
      case 0:			/* succeeded */
        break;
      case -1:      		/* error */
        /* go on to next input */
        //continue;
      case -2:			/* end of recognition process */
        if (jconf->input.speech_input == SP_RAWFILE) {
          //          fprintf(stderr, "%d files processed\n", file_counter);
        } else if (jconf->input.speech_input == SP_STDIN) {
          fprintf(stderr, "reached end of input on stdin\n");
        } else {
          fprintf(stderr, "failed to begin input stream\n");
        }
        return;
  }
  
  /* start recognizing the stream */
  ret = j_recognize_stream(recog);
}

int cJuliusSink::startJuliusDecoder()
{
  juliusIsRunning = 1;
  int result = (int)smileThreadCreate( decoderThread, juliusThreadRunner, this );
  /* TODO: set thread priority ... */
#ifdef __WINDOWS
  SMILE_IMSG(3,"current decoderThread priority = %i",GetThreadPriority(decoderThread));
  SetThreadPriority(decoderThread, decoderThreadPriority);
  SMILE_IMSG(3,"julius decoderThread priority now set to %i",GetThreadPriority(decoderThread));
#endif
  return result;
}

eTickResult cJuliusSink::myTick(long long t)
{
  if (!juliusIsRunning) {
    if (!startJuliusDecoder()) return TICK_INACTIVE;
  }
  smileCondSignal( tickCond );
  reader_->catchupCurR();

  // tick success?
  lockMessageMemory();
  int res = running;
  unlockMessageMemory();

  // TODO: smile request SLEEP... if asr is processing data, request a sleep if all other components retun 0 (i.e. if processing would end if we return 0...)

  if ((getLastNrun() == 1) && res) {
    // we are the only component running...
    // --> sleep some time in this thread!
    if (isEOI()) {
      smileSleep((int)(reader_->getLevelT()*5000.0));
    } else {
      smileSleep((int)(reader_->getLevelT()*1000.0));
    } 
    // in offline mode we have to return 0 to enter EOI processing loop
    // (this enables us to access CMN data produced by a cFullinputMean component
    if (offlineMode && !isEOI()) res = 0;
  }
/*
smileMutexLock(terminatedMtx);
  if (terminated == FALSE) res = 1;
  smileMutexUnlock(terminatedMtx);
*/

  return res ? TICK_SUCCESS : TICK_INACTIVE;
}


cJuliusSink::~cJuliusSink()
{
  int i;

  smileMutexLock(dataFlgMtx);
  terminated = TRUE;
  smileCondSignal( tickCond );
  smileMutexUnlock(dataFlgMtx);
  
  if (!(decoderThread == NULL)) smileThreadJoin(decoderThread);

  /* release all */
  j_recog_free(recog);

  
  //if (logfile) fclose(fp);

  smileMutexDestroy(terminatedMtx);
  smileMutexDestroy(dataFlgMtx);
  smileCondDestroy(tickCond);

  if (excludeWords != NULL) free(excludeWords);
  if (keywords != NULL) {
    for (i=0; i<numKw; i++) {
      if (keywords[i] != NULL) free(keywords[i]);
    }
    free(keywords);
  }
}


#endif // HAVE_JULIUSLIB

