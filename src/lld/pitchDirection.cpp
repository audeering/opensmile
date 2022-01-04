/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


/*  openSMILE component:

read pitch data and compute pitch direction estimates
input fields: F0 or F0env


This component detects:

pseudo-syllables, pseudo-syllable rate

pitch contour per pseudo syllable:
rise/fall
fall/rise
rise
fall
flat

genral pitch contour per syllable
rise
fall
flat

syllable length, mean syllable length (TODO: use energy here and adjacent syllables for more accurate results..?)

*/


#include <lld/pitchDirection.hpp>

#define MODULE "cPitchDirection"

SMILECOMPONENT_STATICS(cPitchDirection)

SMILECOMPONENT_REGCOMP(cPitchDirection)
{
  SMILECOMPONENT_REGCOMP_INIT

    scname = COMPONENT_NAME_CPITCHDIRECTION;
  sdescription = COMPONENT_DESCRIPTION_CPITCHDIRECTION;

  // we inherit cDataProcessor configType and extend it:
  SMILECOMPONENT_INHERIT_CONFIGTYPE("cDataProcessor")

    SMILECOMPONENT_IFNOTREGAGAIN( {}
  //ct->setField("offset","offset that this dummy component adds to the values",1.0);
  ct->setField("ltbs","The size of the long-term average buffer in seconds",0.20);
  ct->setField("stbs","The size of the short-term average buffer in seconds",0.05);
  ct->setField("directionMsgRecp","Recipient component(s) for per syllable event-based pitch direction message (rise/fall/rise-fall/fall-rise message are sent only if and as ofter as a such event occurs on a syllable)",(const char *)NULL);
  ct->setField("speakingRateBsize","The buffer size for computation of speaking rate (in input frames, typical frame rate 100 fps)",100);
  ct->setField("F0direction","1 = enable output of F0 direction as numeric value (fall: -1.0 / flat: 0.0 / rise: 1.0)",1);
  ct->setField("directionScore","1 = enable output of F0 direction score (short term mean - long term mean)",1);
  ct->setField("speakingRate","1 = enable output of current speaking rate in Hz (is is output for every frame, thus, a lot of redundancy here)",0);
  ct->setField("F0avg","1 = enable output of long term average F0",0);
  ct->setField("F0smooth","1 = enable output of exponentially smoothed F0",0);
  ct->setField("onlyTurn","1 = send pitch direction messages (directionMsgRecp) only during speech turns (voice activity) (according to turnStart/turnEnd messages received from cTurnDetector)",0);
  ct->setField("turnStartMessage","Use this option to define a custom message name for turn start messages, i.e. if you want to use voice activity start/end messages instead","turnStart");
  ct->setField("turnEndMessage","Use this option to define a custom message name for turn end messages, i.e. if you want to use voice activity start/end messages instead","turnEnd");
  ct->setField("F0fieldname","The name of the F0 data field to use for syllable detection and pitch direction analysis","F0");
  ct->setField("F0envFieldname","The name of the F0 envelope data field to use for syllable detection and pitch direction analysis","F0env");
  ct->setField("LoudnessFieldname","The name of the 'Loudness' data field (see cIntensity component) to use for syllable nuclei detection","loudness");
  ct->setField("RMSenergyFieldname","The name of the RMS energy data field to use for syllable detection","pcm_RMSenergy");

  //TODO: add invertTurn option
  )

    SMILECOMPONENT_MAKEINFO(cPitchDirection);
}


SMILECOMPONENT_CREATE(cPitchDirection)

//-----

cPitchDirection::cPitchDirection(const char *_name) :
cDataProcessor(_name), myVec(NULL), F0field(-1), F0envField(0), 
stbuf(NULL), ltbuf(NULL), F0non0(0.0), lastF0non0(0.0),
ltbsFrames(2),stbsFrames(1),
stbufPtr(0), ltbufPtr(0),
bufInit(0),
insyl(0),
f0cnt(0), lastE(0.0),
startE(0.0), maxE(0.0), minE(0.0), endE(0.0),
sylen(0), maxPos(0), minPos(0), sylenLast(0),
inpPeriod(0.0),
timeCnt(0.0), sylCnt(0), lastSyl(100.0),
startF0(0.0), lastF0(0.0), maxF0(0.0), minF0(0.0),
maxF0Pos(0), minF0Pos(0),
longF0Avg(0.0),
directionMsgRecp(NULL),
speakingRateBsize(20),
F0directionOutp(0),
directionScoreOutp(0),
speakingRateOutp(0),
F0avgOutp(0), F0smoothOutp(0),
nEnabled(0), 
isTurn(0), onlyTurn(0), resetTurnData(0),
turnStartMessage(NULL), turnEndMessage(NULL), invertTurn(0),
curSpkRate(0), nBuf0(0), nBuf1(0), nSyl0(0), nSyl1(0)
{
}

void cPitchDirection::myFetchConfig()
{
  cDataProcessor::myFetchConfig();

  stbs = (FLOAT_DMEM)getDouble("stbs");
  ltbs = (FLOAT_DMEM)getDouble("ltbs");
  if (ltbs < stbs) {
    ltbs = stbs;
    SMILE_IWRN(2,"long term buffer cannot be smaller than short term buffer! please check the config! (I will set ltbs=stbs)");
  }
  SMILE_IDBG(2,"stbs = %f",stbs);
  SMILE_IDBG(2,"ltbs = %f",ltbs);

  directionMsgRecp = getStr("directionMsgRecp");
  speakingRateBsize = getInt("speakingRateBsize");

  F0directionOutp = getInt("F0direction");
  directionScoreOutp = getInt("directionScore");
  speakingRateOutp = getInt("speakingRate");
  F0avgOutp = getInt("F0avg");
  F0smoothOutp = getInt("F0smooth");

  onlyTurn = getInt("onlyTurn");

  turnStartMessage = getStr("turnStartMessage");
  turnEndMessage = getStr("turnEndMessage");

  //TODO: add invertTurn option

  // load all configuration parameters you will later require fast and easy access to here:

  //offset = getDouble("offset");
  // note, that it is "polite" to output the loaded parameters at debug level 2:
  //SMILE_IDBG(2,"offset = %f",offset);
}

/*  This method is rarely used. It is only there to improve readability of component code.
It is called from cDataProcessor::myFinaliseInstance just before the call to configureWriter.
I.e. you can do everything that you would do here, also in configureWriter()
However, if you implement the method, it must return 1 in order for the configure process to continue!
*/
/*
int cPitchDirection::configureReader() 
{
return 1;
}
*/

int cPitchDirection::configureWriter(sDmLevelConfig &c) 
{
  // if you would like to change the write level parameters... do so HERE:

  //c.T = 0.01; /* don't forget to set the write level sample/frame period */
  //c.nT = 100; /* e.g. 100 frames buffersize for ringbuffer */

  if (c.T != 0.0) {
    stbsFrames = (long)ceil(stbs / c.T);
    ltbsFrames = (long)ceil(ltbs / c.T);
  } else {
    stbsFrames = (long)round(stbs);
    ltbsFrames = (long)round(ltbs);
  }
  inpPeriod = c.T;

  return 1; /* success */
}

/* You shouldn't need to touch this....
int cPitchDirection::myConfigureInstance()
{
int ret = cDataProcessor::myConfigureInstance();
return ret;
}
*/

/*
Do what you like here... this is called after the input names and number of input elements have become available, 
so you may use them here.
*/
/*
int cPitchDirection::dataProcessorCustomFinalise()
{

return 1;
}
*/


/* 
Use setupNewNames() to freely set the data elements and their names in the output level
The input names are available at this point, you can get them via reader->getFrameMeta()
Please set "namesAreSet" to 1, when you do set names
*/

int cPitchDirection::setupNewNames(long nEl) 
{
  // find required input fields
  const FrameMetaInfo * fmeta = reader_->getFrameMetaInfo();
  F0field = fmeta->findField( getStr("F0fieldname") ); // ByPartialName
  F0envField = fmeta->findField( getStr("F0envFieldname") ); // ByPartialName
  LoudnessField = fmeta->findFieldByPartialName( getStr("LoudnessFieldname") ); // ByPartialName
  RMSField = fmeta->findFieldByPartialName( getStr("RMSenergyFieldname") ); // ByPartialName

  if (F0field < 0) { SMILE_IERR(1,"no input field '%s' found, this is required! Use a pitch component as input and check the name, see the 'F0fieldname' option.",getStr("F0fieldname")); F0field=0; }
  if (F0envField < 0) { SMILE_IERR(1,"no input field '%s' found, this is required! Use a pitch component as input and check the name, see the 'F0envFieldname' option.", getStr("F0envFieldname")); F0envField = 0; }
  if ((LoudnessField < 0)&&(RMSField<0)) { SMILE_IERR(1,"no input field 'RMSenergy' or 'loudness' found, one of these is required! Use an energy or intensity component as second input."); }

  int n=0;

  if (F0directionOutp) {
    writer_->addField("F0direction"); n++;
  }
  if (directionScoreOutp) { // smean-lmean
    writer_->addField("directionScore"); n++;
  }
  if (speakingRateOutp) {
    writer_->addField("speakingRate"); n++;
  }
  if (F0avgOutp) {
    writer_->addField("F0avg"); n++;
  }
  if (F0smoothOutp) {
    writer_->addField("F0smooth"); n++;
  }

  if (n==0) { // force at least one field
    F0directionOutp=1;
    writer_->addField("F0direction"); n++;
    SMILE_IWRN(1,"no output enabled, at least one output field is required though. Enabling F0directionOutp !");
  }

  nEnabled = n;

  namesAreSet_ = 1;
  return n;
}


/*
If you don't use setupNewNames() you may set the names for each input field by overwriting the following method:
*/
/*
int cPitchDirection::setupNamesForField( TODO )
{
// DOC TODO...
}
*/

int cPitchDirection::myFinaliseInstance()
{
  int ret = cDataProcessor::myFinaliseInstance();
  if (ret) {
    // do all custom init stuff here... 
    // e.g. allocating and initialising memory (which is not used before, e.g. in setupNames, etc.),
    // loading external data, 
    // etc.

    // allocate memory for long/short term average buffers
    if (stbs > 0) {
      stbuf = (FLOAT_DMEM*)calloc(1,sizeof(FLOAT_DMEM)*stbsFrames);
    }
    if (ltbs > 0) {
      ltbuf = (FLOAT_DMEM*)calloc(1,sizeof(FLOAT_DMEM)*ltbsFrames);
    }

  }
  return ret;
}


int cPitchDirection::processComponentMessage( cComponentMessage *_msg )
{
  const char * endM = NULL;
  if (turnEndMessage == NULL) endM = "turnEnd";
  else endM = turnEndMessage;

  if (isMessageType(_msg,endM)) {
    if (invertTurn) {
      isTurn = 1; resetTurnData = 1;
    } else {
      isTurn = 0;
    }
    SMILE_IDBG(3,"received turnEnd (%s)",endM);
    return 1;
  }

  const char * startM = NULL;
  if (turnStartMessage == NULL) startM = "turnStart";
  else startM = turnStartMessage;

  if (isMessageType(_msg,startM)) {
    if (invertTurn) {
      isTurn = 0;
    } else {
      isTurn = 1;
      resetTurnData = 1;
    }
    SMILE_IDBG(3,"received turnStart (%s)",startM);
    return 1;
  }
  return 0;
}


void cPitchDirection::sendPitchDirectionResult(int _result, double _smileTime, const char * _directionMsgRecp) 
{
  if (_directionMsgRecp != NULL) {
    int _isTurn;
    lockMessageMemory();
    _isTurn = isTurn;
    unlockMessageMemory();

    if (isTurn||(!onlyTurn)) {

      cComponentMessage _msg("pitchDirection");
      _msg.userTime1 = _smileTime;
      _msg.intData[0] = _result;
     // printf("XXXXX send pitch %i\n\n",_result);
      sendComponentMessage(_directionMsgRecp, &_msg);

    }
  }
}

//TODO: sync with turn status...
// receive turn messages
// send only if turn=1
//

eTickResult cPitchDirection::myTick(long long t)
{
  /* actually process data... */
  SMILE_IDBG(4,"tick # %i, processing value vector",t);

  if (!writer_->checkWrite(1))
    return TICK_DEST_NO_SPACE;

  // get next frame from dataMemory
  cVector *vec = reader_->getNextFrame();

  if (vec == NULL) return TICK_SOURCE_NOT_AVAIL;

  // ALGO:
  // Rising/Falling pitch is determined at end of vowels (end of pitch regions)
  // using two moving window averages (long term and short term)
  // We use intensity/loudness, (F0env,) and F0 to determine vowel/syllable positions

  FLOAT_DMEM f0eNow=0.0;
  FLOAT_DMEM f0now =0.0;
  FLOAT_DMEM loudn =0.0;
  if (F0envField >= 0) f0eNow = vec->data[F0envField];
  if (F0field >= 0) f0now = vec->data[F0field];
  if (LoudnessField >= 0) loudn = vec->data[LoudnessField];
  else if (RMSField >= 0) loudn = vec->data[RMSField];
  /*
  #ifdef DEBUG
  float dbg[3];
  dbg[0] = (float)f0eNow;
  dbg[1] = (float)f0s;
  if (!insyl) dbg[1] = 0.0;
  dbg[2] = (float)loudn;
  //saveFloatVector_csv("pitchdir.dat",dbg,3,1);
  #endif
  */
  // f0now == 0 ==> unvoiced

  if (f0now != 0.0) {
    lastF0non0 = F0non0;
    F0non0 = f0now;
  }

  /* -- speaking rate buffer cycling -- */
  if (nBuf0 < speakingRateBsize) {
    nBuf0++; 
    if (nBuf0 == speakingRateBsize) {
      curSpkRate = (double)nSyl0 / ( (double)nBuf0 * reader_->getLevelT());
    }
  }
  if (nBuf1 < speakingRateBsize*2) nBuf1++;
  else {
    curSpkRate = (double)nSyl1 / ( (double)nBuf1 * reader_->getLevelT());
    nBuf1 -= nBuf0;
    nSyl1 -= nSyl0;
    nSyl0 = 0;
    nBuf0 = 0;
  }
  /* ---------*/

  // only if first syllable encountered and last syllable encountered not longer than X sec. ago
  lastSyl += inpPeriod;
  if (lastSyl < 0.8 ) { 
    timeCnt += inpPeriod;
  } else {
    timeCnt = 0.0;
  }

  long i; 
  // fill buffer:
  if (!bufInit) {
    stbuf[stbufPtr] = f0eNow;
    ltbuf[ltbufPtr] = f0eNow;
    if (++stbufPtr >= stbsFrames) stbufPtr = 0;

    if (++ltbufPtr >= ltbsFrames) {
      ltbufPtr = 0;
      bufInit = 1;
      // compute initial sums!
      ltSum=0.0; stSum=0.0;
      for (i=0; i<ltbsFrames; i++) {
        ltSum+=ltbuf[i];
      }
      for (i=0; i<stbsFrames; i++) {
        stSum+=stbuf[i];
      }
    }
  } else {
    // find pseudo-syllables (energetic voiced segments)
    if (!insyl) { // in unvoiced part
      if (f0now > 0.0) { // detect beginning
        if (f0cnt >= 1) {
          /* syl. start */

          if (nBuf0 < speakingRateBsize) nSyl0++;  /* syllable rate counters */
          nSyl1++;  /* syllable rate counters */

          insyl = 1; sylen = f0cnt;
          f0cnt=0; lastSyl = 0.0;
          startF0 = (lastF0+f0now)*(FLOAT_DMEM)0.5;
          f0s = startF0;
          maxF0 = MAX(lastF0,f0now);
          minF0 = MIN(lastF0,f0now);
          maxF0Pos = 0;
          minF0Pos = 0;
          nFall=0; nRise=0; nFlat=0;
        }
        f0cnt++; 
        if (startE == 0.0) { 
          minE = maxE = startE = lastE;
        }
      } else { f0cnt = 0; startE = 0.0; maxE = 0.0; minE = 0.0; }
    } else { // in syllable part
      if (f0now <= 0.0) { // detect end
        if (f0cnt >= 1) {

          /* syl. end */
          // syllable energy verification:

          /*
          if (( 0.5*(startE+endE) < 0.975*maxE )&&( MIN(startE,endE) < 1.05*minE )) {
          printf("E-verify OK\n");
          }
          */

          // other stuff:
          insyl = 0;

          if (sylen > 3) {
            endE = lastE;
            sylenLast = sylen-f0cnt;
            f0cnt=0; 
            sylCnt++; lastSyl=0.0;
            /* //syllable rate computed with each new syllable
            if (timeCnt > 0.0) {
              printf("pseudo-syllable nucleus end (sylen=%i): syl rate = %f Hz\n",sylen,(double)sylCnt / timeCnt);
            }
            */

            // pitch contour classification:
            FLOAT_DMEM endF0 = f0s; //0.5*(F0non0+lastF0non0);
            //lastF0 : end
            //startF0 : start
            // maxF0 / minF0 : start/end
            FLOAT_DMEM conf = (FLOAT_DMEM)sylen;
            if (conf > 10.0) conf = 10.0;
            conf *= 30.0;
            int score=0; int rf=0; int result = -1;

            // determine type of pitch rise/fall

            if (endF0 > (FLOAT_DMEM)pow(startF0,(FLOAT_DMEM)1.01)) {
              if (startF0 != 0.0) 
                score = (int)((endF0-startF0)/startF0*conf);
              if (score >= 1) {
//                printf("Syl: Rise %i\n",score);
                rf=1;
                result = 0;
              }
            }
            else if (endF0 < (FLOAT_DMEM)pow(startF0,(FLOAT_DMEM)(1.0/1.01))) {
              if (startF0 != 0.0) 
                score = (int)((startF0-endF0)/startF0*conf);
              if (score >= 1) {
//                printf("Syl: Fall %i\n",score);
                rf=1;
                result = 1;
              }
              //printf("Syl: Fall %i\n",(int)((startF0-endF0)/startF0*conf));
            }

            if ((!rf)&&(maxF0 > (FLOAT_DMEM)pow(endF0,(FLOAT_DMEM)1.01))&&(maxF0 > (FLOAT_DMEM)pow(startF0,(FLOAT_DMEM)1.01))) {
              if (startF0 != 0.0) {
//                printf("Syl: Rise->Fall %i , %i\n",(int)((maxF0-startF0)/startF0*conf),(int)((maxF0-endF0)/startF0*conf));
                if (result >= 0) {
                  if (score < 15) result = 2;
                } else { result = 2; }
              }
            }
            if ((!rf)&&(minF0 < (FLOAT_DMEM)pow(endF0,(FLOAT_DMEM)(1.0/1.01)))&&(minF0 < (FLOAT_DMEM)pow(startF0,(FLOAT_DMEM)(1.0/1.01)))) {
              if (startF0 != 0.0) {
                //printf("Syl: Fall->Rise %i , %i\n",(int)((startF0-minF0)/startF0*conf),(int)((endF0-minF0)/startF0*conf));
                if (result >= 0) {
                  if (score < 15) result = 3;
                } else { result = 3; }
              }
            }

            // check if result is in line with majority vote over syllable
            if ((result == 0)||(result==1)) {

              if ((nFall>nRise)&&(nFall>nFlat)) {
                //printf("MajVote: Falling\n");
                if (result == 0) result = -1;
              } else
                if ((nRise>nFall)&&(nRise>nFlat)) {
                  //printf("MajVote: Rising\n");
                  if (result == 1) result = -1;
                } else { 
                  //printf("MajVote: Flat\n"); 
                  result = -1; 
                }

            }

            // send result:
            if (result >= 0)  {
              if (result == 0) printf("  __^^__ pitch UP\n");
              else if (result == 1) printf("  __vv__ pitch DOWN\n");
              sendPitchDirectionResult(result,vec->tmeta->smileTime,directionMsgRecp);
            }

          }
        }
        f0cnt++;
      } else { f0cnt = 0; }

      if (insyl) { // check if status has changed..
        //monitor energy levels: loudness must be low at beginning and end, and higher in the middle
        if (loudn > maxE) { maxE = loudn; maxPos = sylen; }
        if (loudn < minE) { minE = loudn; minPos = sylen; }
        f0s = (FLOAT_DMEM)(0.5)*f0s + (FLOAT_DMEM)(0.5) * F0non0; /* smoothed F0 */
        if (f0s > maxF0) { maxF0 = f0s; maxF0Pos = sylen; }
        if (f0s < minF0) { minF0 = f0s; minF0Pos = sylen; }
        sylen++; lastSyl=0.0;
        if (longF0Avg == 0.0) longF0Avg = F0non0;
        longF0Avg = (FLOAT_DMEM)(0.02)*F0non0 + (FLOAT_DMEM)(0.98)*longF0Avg;

        double lmean = ltSum/(double)ltbsFrames;
        double smean = stSum/(double)stbsFrames;
        if (smean > pow(lmean,1.02)) {
          nRise++;
          //dir = 1.0;
        } else 
          if (smean < pow(lmean,1.0/1.02)) {
            nFall++;
            //dir = -1.0;
          } else {
            nFlat++;
            //dir = 0.0;
          }
      }
    }
    lastF0 = f0now;
    lastE = loudn;


  }

  FLOAT_DMEM dir = 0.0;
  double smean = 0.0, lmean = 0.0;

  if (insyl) { //(f0now > 0.0) {
    //// cycle buffer and evaluate data
    // mean of ltbuf:
    ltSum -= (double)ltbuf[ltbufPtr];
    ltbuf[ltbufPtr] = f0s;
    ltSum += (double)f0s;
    if (++ltbufPtr >= ltbsFrames) ltbufPtr = 0;

    // mean of stbuf:
    stSum -= (double)stbuf[stbufPtr];
    stbuf[stbufPtr] = f0s;
    stSum += (double)f0s;
    if (++stbufPtr >= stbsFrames) stbufPtr = 0;

    // detect rising/falling pitch continuously
    lmean = ltSum/(double)ltbsFrames;
    smean = stSum/(double)stbsFrames;
    if (smean > pow(lmean,1.01)) {
      //printf("PITCH : rising\n"); 
      dir = 1.0;
    } else 
      if (smean < pow(lmean,1.0/1.01)) {
        //printf("PITCH : falling\n"); 
        dir = -1.0;
      } else {
        //printf("PITCH : flat\n"); 
        dir = 0.0;
      }
  }

  if (myVec == NULL) myVec = new cVector(nEnabled);

  // if you create a new vector here and pass it to setNextFrame(),
  // then be sure to assign a valid tmeta info for correct timing info:
  // e.g.:
  myVec->setTimeMeta(vec->tmeta);

  int n=0;
  if (F0directionOutp) {
    myVec->data[n] = dir; n++;
  }
  if (directionScoreOutp) { // smean-lmean
    myVec->data[n] = (FLOAT_DMEM)(smean-lmean); n++;
  }
  if (speakingRateOutp) {
    myVec->data[n] = (FLOAT_DMEM)(curSpkRate); n++; // TODO!!
  }
  if (F0avgOutp) {
    myVec->data[n] = (FLOAT_DMEM)( ltSum/(double)ltbsFrames ); n++;
  }
  if (F0smoothOutp) {
    myVec->data[n] = (float)f0s; n++;
    if (!insyl) { myVec->data[n] = (FLOAT_DMEM)(0.0); }
  }
  
  // save to dataMemory
  writer_->setNextFrame(myVec);

  return TICK_SUCCESS;
}


cPitchDirection::~cPitchDirection()
{
  // cleanup...
  if (myVec != NULL) delete myVec;
  if (stbuf != NULL) free(stbuf);
  if (ltbuf != NULL) free(ltbuf);
}

