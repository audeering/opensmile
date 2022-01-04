/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


/*  openSMILE component:

rule based voice activity detector

*/


#include <dsp/vadV1.hpp>

#define MODULE "cVadV1"

// default values (can be changed via config file...)
#define N_PRE  10
#define N_POST 20

SMILECOMPONENT_STATICS(cVadV1)

SMILECOMPONENT_REGCOMP(cVadV1)
{
  SMILECOMPONENT_REGCOMP_INIT
  scname = COMPONENT_NAME_CVADV1;
  sdescription = COMPONENT_DESCRIPTION_CVADV1;

  // we inherit cVectorProcessor configType and extend it:
  SMILECOMPONENT_INHERIT_CONFIGTYPE("cDataProcessor")
  SMILECOMPONENT_IFNOTREGAGAIN(
    ct->setField("threshold","The minimum rms/log energy threshold to use (or the actual rms energy threshold, if disableDynamicVAD==1)",-13.0); // 0.0005
    ct->setField("disableDynamicVAD","1/0 = yes/no, whether dynamic VAD is disabled (default is enabled)",0);
    ct->setField("debug","1/0 enable/disable vad debug output",0);
  )
  SMILECOMPONENT_MAKEINFO(cVadV1);
}

SMILECOMPONENT_CREATE(cVadV1)

//-----

cVadV1::cVadV1(const char *_name) :
  cDataProcessor(_name),
  turnState(0), actState(0),
  useRMS(1),
  threshold(0.0),
  nPost(N_POST),
  nPre(N_PRE),
  cnt1(0), cnt2(0),
  startP(0),
  recFramer(NULL),
  recComp(NULL),
  statusRecp(NULL),
  disableDynamicVAD(0),
  spec(NULL), div0(0.0),
  t0histIdx(0),
  f0v_0(0.0), ent_0(0.0), E_0(0.0),
  nInit(0),
  uF0v(0.0), uEnt(0.0), uE(0.0),
  vF0v(0.0), vEnt(0.0), vE(0.0),
  tuF0v(0.0), tuEnt(0.0), tuE(0.0),
  tvF0v(0.0), tvEnt(0.0), tvE(0.0),
  vadFuzHidx(0), vadBin(0),
  F0vHidx(0), entHidx(0), EHidx(0),
  tF0vHidx(0), tentHidx(0), tEHidx(0),
  nInitT(0), nInitN(0)
{
  t0hist[0] = 0.0;
  t0hist[1] = 0.0;
  t0hist[2] = 0.0;
  t0hist[3] = 0.0;
  
  ar1 = (FLOAT_DMEM)( 1.0 - exp(-10.0/20.0) ); // THIS DEPENDS ON THE INPUT FRAME RATE.. ASSUMING 10ms HERE!
  ar0 = (FLOAT_DMEM)( 1.0 - exp(-10.0/200.0) ); // THIS DEPENDS ON THE INPUT FRAME RATE.. ASSUMING 10ms HERE!

  arU = (FLOAT_DMEM)0.005; //(FLOAT_DMEM)( 1.0 - exp(-10.0/100.0) ); // THIS DEPENDS ON THE INPUT FRAME RATE.. ASSUMING 10ms HERE!
  arV = (FLOAT_DMEM)0.005; //(FLOAT_DMEM)( 1.0 - exp(-10.0/500.0) ); // THIS DEPENDS ON THE INPUT FRAME RATE.. ASSUMING 10ms HERE!
  
  int i;
  for (i=0; i<FUZBUF; i++) {
    vadFuzH[i] = 0.0;
  }
  for(i=0; i<FTBUF; i++) {
    F0vH[i] = 0;
    entH[i] = 0;
    EH[i] = 0;
  }
}

void cVadV1::myFetchConfig()
{
  cDataProcessor::myFetchConfig();
  
/*  nPre = getInt("nPre");
  SMILE_IDBG(2,"nPre = %i",nPre);

  nPost = getInt("nPost");
  SMILE_IDBG(2,"nPost = %i",nPost);
*/
  debug = getInt("debug");
  disableDynamicVAD = getInt("disableDynamicVAD");

  minE = (FLOAT_DMEM)getDouble("threshold");
}

int cVadV1::setupNewNames(long nEl)
{
  findInputMapping();

  writer_->addField( "vadBin" );
  writer_->addField( "vadFuz" );
  writer_->addField( "vadSmo" );
  namesAreSet_ = 1;

  return 1;
}

void cVadV1::findInputMapping()
{
  const FrameMetaInfo * fmeta = reader_->getFrameMetaInfo();

  long specField = fmeta->findFieldByPartialName( "lspFreq" );

  if (specField < 0) { specField = 0; } // default : fallbak to 0th field

  if (specField >= 0) {
    specIdx = fmeta->fieldToElementIdx( specField );
    if (specIdx >= 0) {
      specN = fmeta->field[specField].N;
    } else { specN = 0; }
  } else {
    specIdx = -1; specN = 0;
  }
  lsfN = fmeta->field[0].N;
  
  F0rawIdx = fmeta->fieldToElementIdx( fmeta->findFieldByPartialName( "F0raw" ) );
  voiceProbIdx = fmeta->fieldToElementIdx( fmeta->findFieldByPartialName( "voiceProb" ) );
  eIdx = fmeta->fieldToElementIdx( fmeta->findFieldByPartialName( "LOG" ) );

  spec = (FLOAT_DMEM*)calloc(1,sizeof(FLOAT_DMEM)*(specN));
  int i;
  for(i=0; i<specN; i++) {
    spec[i] = ( (FLOAT_DMEM)i * ((FLOAT_DMEM)3.0/(FLOAT_DMEM)specN) + (FLOAT_DMEM)0.2 ); 
  }
}


FLOAT_DMEM cVadV1::pitchVariance(FLOAT_DMEM curF0raw)
{
  // convert to period in seconds:
  FLOAT_DMEM Tp=curF0raw;
  //if (curF0raw != 0.0) Tp = 1.0/curF0raw;

  // add new value to history:
  t0hist[t0histIdx++] = Tp;
  if (t0histIdx >= 8) t0histIdx = 0;

  // mean over 8 frames:
  FLOAT_DMEM m = (FLOAT_DMEM)0.125 * (t0hist[0]+t0hist[1]+t0hist[2]+t0hist[3]+t0hist[4]+t0hist[5]+t0hist[6]+t0hist[7]);
  //FLOAT_DMEM m = 0.125 * (t0hist[0]+t0hist[1]+t0hist[2]+t0hist[3]);

  int i;
  FLOAT_DMEM v = (FLOAT_DMEM)0.0;
  for (i=0; i<8; i++) {
    v += (t0hist[i] - m) * (t0hist[i] - m);
  }

  return (FLOAT_DMEM)sqrt((double)v/8.0);
}

// a derived class should override this method, in order to implement the actual processing
eTickResult cVadV1::myTick(long long t)
{
  if (!writer_->checkWrite(1))
    return TICK_DEST_NO_SPACE;

  // get next frame from dataMemory
  //printf("tick");
  cVector *vec = reader_->getNextFrame();
  if (vec == NULL) return TICK_SOURCE_NOT_AVAIL;
  //printf(" OK \n");

  cVector *vec0 = new cVector(3);  // TODO: move vec0 to class...
  
  FLOAT_DMEM *src = vec->data;
  FLOAT_DMEM *dst = vec0->data;


/*
// get LSF difference vector:
  if (lsfIdx>=0) {
    int j=0, i;
    for(i=lsfIdx; i<lsfIdx+lsfN-1; i++) {
      //lsf[j] = src[i+1]-src[i];
      lsf[j] = src[i];
      printf("lsf[%i] %f \n",j,lsf[j]);
      j++;
    }
    //lsf[j] = 3.15 - src[i];
    lsf[j] = src[i];
    printf("lsf[%i] %f \n",j,lsf[j]);
  } else { printf("no lsf!\n"); }
*/
  //printf("sepcidx %i specN %i\n",specIdx,specN);
  int j=0; int i;

  // get energy
  FLOAT_DMEM E = 0.0;
  if (eIdx >= 0) {
    E = src[eIdx];
  }
  // check if disableDynamicVAD is set
  if (disableDynamicVAD) {
    if (E > threshold) {
      dst[0] = 1.0;
      dst[1] = 1.0;
      dst[2] = 1.0;
    } else {
      dst[0] = 0.0;
      dst[1] = 0.0;
      dst[2] = 0.0;
    }

    // save to dataMemory
    writer_->setNextFrame(vec0);
    delete vec0;

    return TICK_SUCCESS;
  }

  // compute LSP diversion
  FLOAT_DMEM div = 0.0;
  for(i=0; i<specN; i++) {
    //FLOAT_DMEM x = ( (FLOAT_DMEM)i * (3.0/(FLOAT_DMEM)specN) + 0.14 ) - spec[i]; 
    FLOAT_DMEM x = spec[i] - src[i+specIdx]; 
    div += x*x;
  }
  //printf ("LSF div: %f\n",div);
  //div /= specN;
  //div = sqrt(div);
  //printf ("LSF div: %f\n",div);

  // compute spectral entropy:
  FLOAT_DMEM ent = smileStat_entropy(src,lsfN);

  // compute pitch variance
  FLOAT_DMEM f0v=0.0;
 // if (F0rawIdx >= 0) {
    //printf("f0raw %f [%i]\n",src[F0rawIdx],F0rawIdx);
 //   f0v = pitchVariance(src[F0rawIdx]);
 // }
  f0v=div;

  FLOAT_DMEM f0prob=0.0;
  if (voiceProbIdx >= 0) {
    //printf("f0raw %f [%i]\n",src[F0rawIdx],F0rawIdx);
    f0prob = src[voiceProbIdx];
  }



  // now smooth the 3 contours
  if (ent > ent_0) {
    ent = ar0 * (ent - ent_0) + ent_0;
  } else {
    ent = ar1 * (ent - ent_0) + ent_0;
  }

  if (f0v > f0v_0) {
    f0v = ar0 * (f0v - f0v_0) + f0v_0;  //f0v = ar * f0v + (1.0-ar) f0v_0;
  } else {
    f0v = ar1 * (f0v - f0v_0) + f0v_0;  //f0v = ar * f0v + (1.0-ar) f0v_0;
  }

  if (E < E_0) {
    E = ar0 * (E - E_0) + E_0;
  } else {
    E = ar1 * (E - E_0) + E_0;
  }

  ent_0 = ent;
  f0v_0 = f0v;
  E_0 = E;

  //int vadBin = 0;
  FLOAT_DMEM vadFuz=0.0;
  FLOAT_DMEM vadSmo=0.0;

  if ((nInit < NINIT)) { // prepare initial thresholds
    if (nInit > 10) {
    uF0v += f0v;
    uEnt += ent;
    uE   += E;
    
    F0vH[F0vHidx++] = f0v;
    entH[entHidx++] = ent;
    EH[EHidx++] = E;
    }
    nInit ++; 
    vadBin=0;
  } else {
    if (nInit == NINIT) { // compute inital thresholds
      FLOAT_DMEM nn = ((FLOAT_DMEM)nInit) - (FLOAT_DMEM)10.0;

      // means
      uF0v /= nn;
      uEnt /= nn;
      uE   /= nn;

      // standard deviations:
      int i;
      for (i=0; i<nInit-10; i++) {
        vF0v += (F0vH[i] - uF0v) * (F0vH[i] - uF0v);
        vEnt += (entH[i] - uEnt) * (entH[i] - uEnt);
        vE   += (EH[i] - uE)     * (EH[i] - uE);
      }
      vF0v /= nn;
      vF0v = sqrt(vF0v);

      vEnt /= nn;
      vEnt = sqrt(vEnt);

      vE /= nn;
      vE = sqrt(vE);
      nInitN = nInit;
    }
    
    // compute thresholds
    FLOAT_DMEM th1ent = uEnt;
    FLOAT_DMEM th1e = uE;
    //FLOAT_DMEM th1f0v = 1.0/72.0 * vF0v;
    FLOAT_DMEM th1f0v = uF0v;

    FLOAT_DMEM th2ent = uEnt - vEnt;
    FLOAT_DMEM th2e = uE + vE;
    //FLOAT_DMEM th2f0v = 1.0/36.0 * vF0v;
    FLOAT_DMEM th2f0v = uF0v + vF0v;

    FLOAT_DMEM th3ent = uEnt - (FLOAT_DMEM)2.0*vEnt;
    FLOAT_DMEM th3e = uE + (FLOAT_DMEM)1.0*vE;
    //FLOAT_DMEM th3f0v = 1.0/27.0 * vF0v;
    FLOAT_DMEM th3f0v = uF0v + (FLOAT_DMEM)2.0* vF0v;

    FLOAT_DMEM th4ent = uEnt - (FLOAT_DMEM)3.0*vEnt;
    FLOAT_DMEM th4e = uE + (FLOAT_DMEM)2.0*vE;
    //FLOAT_DMEM th4f0v = 1.0/18.0 * vF0v;
    FLOAT_DMEM th4f0v = uF0v + (FLOAT_DMEM)3.0* vF0v;
    
    FLOAT_DMEM th5ent = uEnt - (FLOAT_DMEM)5.0*vEnt;
    FLOAT_DMEM th5e = uE + (FLOAT_DMEM)4.0*vE;
    //FLOAT_DMEM th5f0v = 1.0/9.0 * vF0v;
    FLOAT_DMEM th5f0v = uF0v + (FLOAT_DMEM)5.0* vF0v;

    
    FLOAT_DMEM tth0ent = tuEnt + (FLOAT_DMEM)0.5*tvEnt;
    FLOAT_DMEM tth0e = tuE - (FLOAT_DMEM)0.1*tvE;
    //FLOAT_DMEM th1f0v = 1.0/72.0 * vF0v;
    FLOAT_DMEM tth0f0v = tuF0v - (FLOAT_DMEM)0.5*tvF0v;

    FLOAT_DMEM tth1ent = tuEnt - (FLOAT_DMEM)0.5*tvEnt;
    FLOAT_DMEM tth1e = tuE + (FLOAT_DMEM)0.1*tvE;
    //FLOAT_DMEM th1f0v = 1.0/72.0 * vF0v;
    FLOAT_DMEM tth1f0v = tuF0v + (FLOAT_DMEM)0.5*tvF0v;

    FLOAT_DMEM tth2ent = tuEnt + (FLOAT_DMEM)1.0*tvEnt;
    FLOAT_DMEM tth2e = tuE - (FLOAT_DMEM)0.5*tvE;
    //FLOAT_DMEM th1f0v = 1.0/72.0 * vF0v;
    FLOAT_DMEM tth2f0v = tuF0v - (FLOAT_DMEM)2.0*tvF0v;

    FLOAT_DMEM tth3ent = tuEnt + (FLOAT_DMEM)3.0*tvEnt;
    FLOAT_DMEM tth3e = tuE - (FLOAT_DMEM)2.0*tvE;
    //FLOAT_DMEM th1f0v = 1.0/72.0 * vF0v;
    FLOAT_DMEM tth3f0v = tuF0v - (FLOAT_DMEM)3.0*tvF0v;

  
    // perform VAD
    FLOAT_DMEM vadEnt, vadE, vadF0v;

    if (ent < th5ent) { // inverse
      vadEnt = (FLOAT_DMEM)1.0;
    } else if (ent < th4ent) {
      vadEnt = (FLOAT_DMEM)0.8;
    } else if (ent < th3ent) {
      vadEnt = (FLOAT_DMEM)0.6;
    } else if (ent < th2ent) {
      vadEnt = (FLOAT_DMEM)0.4;
    } else if (ent < th1ent) {
      vadEnt = (FLOAT_DMEM)0.2;
    } else { 
      vadEnt = (FLOAT_DMEM)0.0;
    }

    if ((tuEnt > (FLOAT_DMEM)0.0)&&(tth2ent<th4ent)) {
      if (ent > tth3ent) {
        vadEnt -= (FLOAT_DMEM)0.3;
      } else if (ent > tth2ent) {
        vadEnt -= (FLOAT_DMEM)0.2;
      } else if (ent < tth1ent) {
        vadEnt = (FLOAT_DMEM)1.0;
      }  
    } else if ((tth2ent>th4ent)) {
      //vadEnt -= 0.2; // penalty, if turns are very equal to noise
    }


    if (vadEnt < (FLOAT_DMEM)0.0) vadEnt = 0;
 
    if (E < th1e) {
      vadE = (FLOAT_DMEM)0;
    } else if (E < th2e) {
      vadE = (FLOAT_DMEM)0.2;
    } else if (E < th3e) {
      vadE = (FLOAT_DMEM)0.4;
    } else if (E < th4e) {
      vadE = (FLOAT_DMEM)0.6;
    } else if (E < th5e) {
      vadE = (FLOAT_DMEM)0.8;
    } else { 
      vadE = (FLOAT_DMEM)1.0;
    }

    if ((tuE > (FLOAT_DMEM)0.0)&&(tth2e<th4e)) {
      if (E < tth3e) {
        vadE -= (FLOAT_DMEM)0.2;
      } else if (E < tth2e) {
        vadE -= (FLOAT_DMEM)0.2;
      } else if (E > tth1e) {
        vadE = (FLOAT_DMEM)1.0;
      }  
    } else if ((tth2e<th4e)&&(tuE>(FLOAT_DMEM)0.0)) {
      //vadE -= 0.2; // penalty, if turns are very equal to noise
    }
    if (vadE < (FLOAT_DMEM)0.0) vadE = 0;


    if (f0v < th1f0v) {  
      vadF0v = (FLOAT_DMEM)0.0;
    } else if (f0v < th2f0v) {
      vadF0v = (FLOAT_DMEM)0.2;
    } else if (f0v < th3f0v) {
      vadF0v = (FLOAT_DMEM)0.4;
    } else if (f0v < th4f0v) {
      vadF0v = (FLOAT_DMEM)0.6;
    } else if (f0v < th5f0v) {
      vadF0v = (FLOAT_DMEM)0.8;
    } else { 
      vadF0v = (FLOAT_DMEM)1.0;
    }

    if ((tuF0v > (FLOAT_DMEM)0.0)&&(tth2f0v<th4f0v)) {
      if (f0v < tth3f0v) {
        vadF0v -= (FLOAT_DMEM)0.2;
      } else if (f0v < tth2f0v) {
        vadF0v -= (FLOAT_DMEM)0.2;
      } else if (f0v > tth1f0v) {
        vadF0v = (FLOAT_DMEM)1.0;
      }  
    } else if ((tth2f0v<th4f0v)&&(tuF0v>(FLOAT_DMEM)0.0)) {
      //vadF0v -= 0.2; // penalty, if turns are very equal to noise
    }
    if (vadF0v < (FLOAT_DMEM)0.0) vadF0v = 0;

    if (debug) {
      printf("VADent %f  %f  %f   %f | %f %f\n",vadEnt, ent, uEnt, vEnt, tuEnt, tvEnt);
      printf("VADdiv %f  %f  %f   %f | %f %f\n",vadF0v, f0v, uF0v, vF0v, tuF0v, tvF0v);
      printf("VADE   %f  %f  %f   %f | %f %f\n",vadE, E, uE, vE, tuE, tvE);
    }

    vadFuz = (FLOAT_DMEM)0.45 * vadEnt + (FLOAT_DMEM)0.25 * vadE +  (FLOAT_DMEM)0.30 * vadF0v;
    //vadFuz = (FLOAT_DMEM)0.55 * vadEnt + (FLOAT_DMEM)0.45 * vadE  * vadF0v;

    //vadFuz = vadEnt * vadE  * vadF0v;

    // maintain history:
    vadFuzH[vadFuzHidx++] = vadFuz;
    if (vadFuzHidx >= FUZBUF) vadFuzHidx = 0;

    //printf("VADfuz %f\n",vadFuz);

    // determine vad bin:
    int i;
    FLOAT_DMEM sum = 0.0;
    for (i=0; i<FUZBUF; i++) {
      sum += vadFuzH[i];
    }
    sum /= (FLOAT_DMEM)FUZBUF;

    vadSmo = sum;

    // ... the big magic ;-)
    if ((sum > (FLOAT_DMEM)0.50)&&(E > minE)) {
      if (vadBin==0) { turnSum = 0.0; turnN=0.0; /*printf("turnstart\n");*/ }
      vadBin = 1;
      turnSum += sum; turnN += 1.0;
    } else {
      if (vadBin == 1) {
        //printf("turnConf = %f\n",turnSum/turnN);
      }
      vadBin = 0;
    }


    if ((vadBin == 0)&&(vadFuz < 0.5)) {    
      // dynamic threshold update:

      F0vH[F0vHidx++] = f0v;
      if (F0vHidx >= FTBUF) F0vHidx = 0;
      entH[entHidx++] = ent;
      if (entHidx >= FTBUF) entHidx = 0;
      EH[EHidx++] = E;
      if (EHidx >= FTBUF) EHidx = 0;
  
      if (nInit < FTBUF) nInit++;
      else {
        int i;
        FLOAT_DMEM sumEnt=0.0;
        FLOAT_DMEM sumF0v=0.0;
        FLOAT_DMEM sumE=0.0;
        for (i=0; i< FTBUF; i++) {
          sumEnt += entH[i];
          sumE += EH[i];
          sumF0v += F0vH[i];
        }
        sumEnt /= (FLOAT_DMEM)FTBUF;
        sumE /= (FLOAT_DMEM)FTBUF;
        sumF0v /= (FLOAT_DMEM)FTBUF;

        uEnt = ((FLOAT_DMEM)1.0-arU) * uEnt + (arU)*sumEnt;
        uF0v = ((FLOAT_DMEM)1.0-arU) * uF0v + (arU)*sumF0v;
        uE = ((FLOAT_DMEM)1.0-arU) * uE + (arU)*sumE;

        // standard deviations:
        FLOAT_DMEM stEnt=(FLOAT_DMEM)0.0;
        FLOAT_DMEM stF0v=(FLOAT_DMEM)0.0;
        FLOAT_DMEM stE=(FLOAT_DMEM)0.0;
        for (i=0; i< FTBUF; i++) {
          stEnt += ( entH[i] - sumEnt ) * ( entH[i] - sumEnt );
          stE += ( EH[i] - sumE ) * ( EH[i] - sumE );
          stF0v += ( F0vH[i] - sumF0v ) * ( F0vH[i] - sumF0v );
        }
        stEnt /= (FLOAT_DMEM)FTBUF;
        stE /= (FLOAT_DMEM)FTBUF;
        stF0v /= (FLOAT_DMEM)FTBUF;

        vEnt = ((FLOAT_DMEM)1.0-arV) * vEnt + (arV)*sqrt(stEnt);
        vF0v = ((FLOAT_DMEM)1.0-arV) * vF0v + (arV)*sqrt(stF0v);
        vE = ((FLOAT_DMEM)1.0-arV) * vE + (arV)*sqrt(stE);
        //nInitN=0;
      }

      int j=0;
      for(i=specIdx; i<specIdx+specN; i++) {
        spec[j] = (FLOAT_DMEM)0.995*spec[j] + (FLOAT_DMEM)0.005*src[i]; 
        j++;
      }


    } else if ((vadFuz > 0.6)&&(vadBin==1)&&(turnN>20.0)) {

      tF0vH[tF0vHidx++] = f0v;
      if (tF0vHidx >= FTBUF) tF0vHidx = 0;
      tentH[tentHidx++] = ent;
      if (tentHidx >= FTBUF) tentHidx = 0;
      tEH[tEHidx++] = E;
      if (tEHidx >= FTBUF) tEHidx = 0;

      if (nInitT < FTBUF) nInitT++;
      else {
        FLOAT_DMEM sumEnt=0.0;
        FLOAT_DMEM sumF0v=0.0;
        FLOAT_DMEM sumE=0.0;
        for (i=0; i< FTBUF; i++) {
          sumEnt += tentH[i];
          sumE += tEH[i];
          sumF0v += tF0vH[i];
        }
        sumEnt /= (FLOAT_DMEM)FTBUF;
        sumE /= (FLOAT_DMEM)FTBUF;
        sumF0v /= (FLOAT_DMEM)FTBUF;

        tuEnt = ((FLOAT_DMEM)1.0-arU) * tuEnt + (arU)*sumEnt;
        tuF0v = ((FLOAT_DMEM)1.0-arU) * tuF0v + (arU)*sumF0v;
        tuE = ((FLOAT_DMEM)1.0-arU) * tuE + (arU)*sumE;

        // standard deviations:
        FLOAT_DMEM stEnt=0.0;
        FLOAT_DMEM stF0v=0.0;
        FLOAT_DMEM stE=0.0;
        for (i=0; i< FTBUF; i++) {
          stEnt += ( tentH[i] - sumEnt ) * ( tentH[i] - sumEnt );
          stE += ( tEH[i] - sumE ) * ( tEH[i] - sumE );
          stF0v += ( tF0vH[i] - sumF0v ) * ( tF0vH[i] - sumF0v );
        }
        stEnt /= (FLOAT_DMEM)FTBUF;
        stE /= (FLOAT_DMEM)FTBUF;
        stF0v /= (FLOAT_DMEM)FTBUF;

        tvEnt = ((FLOAT_DMEM)1.0-arV) * tvEnt + (arV)*sqrt(stEnt);
        tvF0v = ((FLOAT_DMEM)1.0-arV) * tvF0v + (arV)*sqrt(stF0v);
        tvE = ((FLOAT_DMEM)1.0-arV) * tvE + (arV)*sqrt(stE);
        //nInitT=0;
      }

    }

  }

//  printf("VADbin %i\n",vadBin);

  dst[0] = (FLOAT_DMEM)vadBin;
  dst[1] = (FLOAT_DMEM)vadFuz;
  dst[2] = (FLOAT_DMEM)vadSmo;

  // save to dataMemory
  writer_->setNextFrame(vec0);
  delete vec0;

  return TICK_SUCCESS;
}

cVadV1::~cVadV1()
{
  if (spec != NULL) free(spec);
}

