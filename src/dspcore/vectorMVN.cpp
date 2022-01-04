/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


/*  openSMILE component: cVectorMVN

This component extends the base class cVectorTransform and implements mean/variance normalisation

*/

#include <dspcore/vectorMVN.hpp>
#include <cmath>

#define MODULE "cVectorMVN"



SMILECOMPONENT_STATICS(cVectorMVN)

SMILECOMPONENT_REGCOMP(cVectorMVN)
{
  SMILECOMPONENT_REGCOMP_INIT
  scname = COMPONENT_NAME_CVECTORMVN;
  sdescription = COMPONENT_DESCRIPTION_CVECTORMVN;

  // we inherit cVectorProcessor configType and extend it:
  SMILECOMPONENT_INHERIT_CONFIGTYPE("cVectorTransform")
  
  SMILECOMPONENT_IFNOTREGAGAIN(
    ct->setField("nameAppend",(const char *)NULL,(const char *)NULL);
    ct->setField("meanEnable","1 = enable normalisation to 0 mean",1);
    ct->setField("stdEnable","1 = enable standardisation to stddev 1",1);
    ct->setField("normEnable","1 = enable normalisation (scaling) to range -1 to +1, if meanEnable=1 (x-mean)/((max-min)*0.5), or to range 0..1 if meanEnable=0  (x-min)/(max-min) ; (this can NOT be used in conjunction with stdEnable)",0);
    ct->setField("minMaxNormEnable","1 = enable normalisation (scaling) to range -1 to +1 with only min and max values: x=2*(x-min)/(max-min)-1",0);
    ct->setField("spectralFlooring","1 = enable spectral subtraction by flooring all spectral bins below the mean (to the value of 'specFloor') and not touching bins above the mean (except if you set subtractMeans=1). (works only if meanEnable=1 is the only normalisation option set)",0);
    ct->setField("subtractMeans","(only relevant if spectralFlooring=1) : 1 = if input values are above the mean, subtract the mean (else floor to 'specFloor').",0);
    ct->setField("specFloor","The value to which bins will be set that are floored.",0.0000000001);
    ct->setField("htkLogEnorm","1 = enable HTK compatible log energy normalisation (this also sets normEnable=1, meanEnable=0)",0);
  )

  SMILECOMPONENT_MAKEINFO(cVectorMVN);
}



SMILECOMPONENT_CREATE(cVectorMVN)

//-----

cVectorMVN::cVectorMVN(const char *_name) :
  cVectorTransform(_name)
{
}

void cVectorMVN::myFetchConfig()
{
  cVectorTransform::myFetchConfig();

  meanEnable = getInt("meanEnable");
  stdEnable = getInt("stdEnable");
  normEnable = getInt("normEnable");
  minMaxNormEnable = getInt("minMaxNormEnable");

  specFloor = (FLOAT_DMEM)getDouble("specFloor");
  subtractMeans = getInt("subtractMeans");
  spectralFlooring = getInt("spectralFlooring");
  htkLogEnorm = getInt("htkLogEnorm");
  if (htkLogEnorm) {
    meanEnable = 0; normEnable = 1; stdEnable = 0; minMaxNormEnable = 0;
  }
  SMILE_IDBG(2,"htkLogEnorm = '%i'",htkLogEnorm);

  if (stdEnable && normEnable ) {
    SMILE_IERR(1,"Only ONE of stdEnable and normEnable can be enabled!! Setting normEnable=0, stdEnable=1 !");
    normEnable = 0;
  }
  if ( !(meanEnable || stdEnable || normEnable || minMaxNormEnable) ) {
    SMILE_IERR(1,"No normalisation option is enabled! At least one must be selected. Defaulting to meanEnable=1 !");
    meanEnable = 1;
  }

  SMILE_IDBG(2,"stdEnable = '%i'",stdEnable);
  SMILE_IDBG(2,"meanEnable = %i",meanEnable);
  SMILE_IDBG(2,"normEnable = '%i'",normEnable);
}

void cVectorMVN::allocTransformData(struct sTfData *tf, int Ndst, int idxi) 
{
  // TODO:  get loaded transform0
  struct sTfData * tf0 = getTransformInitData();
  if ((tf0->vectors != NULL)&&(tf0->head.vecSize > 0)&&(tf0->head.nVec > 0)) {
    // a valid init file has been loaded
    // check nVec and correct our enable options if necessary (also warn the user)
    if (tf0->head.nVec == 1) { // mean only
      if (stdEnable||normEnable||(!meanEnable)) {
        meanEnable=1; stdEnable=0; normEnable=0;
        SMILE_IWRN(1,"Your loaded transform init file seems to have 1 vector, assuming mean only normalisation. This, however, contradicts the 'enableX' options set in the config, you should check this!");
      }
    } else if (tf0->head.nVec == 2) { // mvn 
      if (minMaxNormEnable) {
        meanEnable=0; stdEnable=0; normEnable=0; minMaxNormEnable = 1;
      }
      else if (((!stdEnable)||normEnable||(!meanEnable)) && (!minMaxNormEnable)) {
        meanEnable=1; stdEnable=1; normEnable=0;
        SMILE_IWRN(1,"Your loaded transform init file seems to have 2 vectors, assuming mean and variance normalisation. This, however, contradicts the 'enableX' options set in the config, you should check this!");
      }
    } else if (tf0->head.nVec == 3) { // mrn
      if (stdEnable||(!normEnable)||(!meanEnable)) {
        meanEnable=1; stdEnable=0; normEnable=1;
        SMILE_IWRN(1,"Your loaded transform init file seems to have 3 vectors, assuming mean and range normalisation. This, however, contradicts the 'enableX' options set in the config, you should check this!");
      }
    }
  }

  tf->head.nUserdata=0;
  if (stdEnable) {
    tf->head.nGroups = 2;
    tf->head.nVec = 2;
  } else if (normEnable) {
    tf->head.nGroups = 3;
    tf->head.nVec = 3;
  } else if (minMaxNormEnable) {
    /*
    if (normEnable==1 && stdEnable == 0) { // assume min/max norm data...
      tf->head.typeID = TRFTYPE_MRN;
    } else if (stdEnable == 1) {
      tf->head.typeID = TRFTYPE_MVN;
    }
    */
    tf->head.nGroups = 2;
    tf->head.nVec = 2;
  } else { // mean only...
    tf->head.nGroups = 1;
    tf->head.nVec = 1;
  }

  if (stdEnable) tf->head.typeID = TRFTYPE_MVN;
  else if (minMaxNormEnable) tf->head.typeID = TRFTYPE_MVN; /* compatibility for loading old bin format min max norm files produced by arffnorm, TODO: make this more compatible with a (to add..) option for minMaxEnable in the new smileTf format */
  else if (normEnable) tf->head.typeID = TRFTYPE_MRN;
  else if (meanEnable) tf->head.typeID = TRFTYPE_MNN;
  tf->vectors = (double *)malloc(sizeof(double)*tf->head.nVec*tf->head.vecSize);

  //return 1;
}

int cVectorMVN::transformData(const struct sTfData * tf, const FLOAT_DMEM *src, FLOAT_DMEM *dst, long Nsrc, long Ndst, int idxi)
{
  long i;
  long N = tf->head.vecSize;
  double *mean = tf->vectors;
  double *stddev = NULL;
  double *min = NULL;
  double *max = NULL;
  

  if (minMaxNormEnable) {
    max = mean;
    min = mean+N;

    for (i=0; i<MIN(Nsrc,Ndst); i++) {
      if (!std::isfinite(max[i])) {
        SMILE_IERR(2,"NON-finite value: max at index %i, flooring this value with %f",i,0.0);
        max[i] = 0.0;
      } else if (!std::isfinite(min[i])) {
        SMILE_IERR(2,"NON-finite value: min at index %i, flooring this value with %f",i,0.0);
        min[i] = 0.0;
      } else {
        if (max[i]-min[i] != 0.0) {
          dst[i] = (FLOAT_DMEM)2.0 * (src[i]-(FLOAT_DMEM)min[i])/((FLOAT_DMEM)max[i] - (FLOAT_DMEM)min[i]) - (FLOAT_DMEM)1.0;
        } else {
          dst[i] = 0.0;
        }
      }
    }

  } else {

    if (stdEnable) {
      stddev = mean+N;
    }
    if (normEnable) {
      min = mean+N;
      max = min+N;
    }

    if (stdEnable) {  // MVN
      if (meanEnable) {
        for (i=0; i<MIN(Nsrc,Ndst); i++) {
          if (stddev[i] == 0.0 || !std::isfinite(stddev[i])) {
            SMILE_IERR(2,"NON-finite or zero value: stddev at index %i, flooring this value with %f\n",i,STDDEV_FLOOR);
            stddev[i] = STDDEV_FLOOR;
          } else if (stddev[i] <= 0.0) stddev[i] = STDDEV_FLOOR;
          dst[i] = (src[i]-(FLOAT_DMEM)mean[i])/(FLOAT_DMEM)stddev[i];
        }
      } else {
        for (i=0; i<MIN(Nsrc,Ndst); i++) {
          if (stddev[i] == 0.0 || !std::isfinite(stddev[i])) {
            SMILE_IERR(2,"NON-finite value: stddev at index %i, flooring this value with %f\n",i,STDDEV_FLOOR);
            stddev[i] = STDDEV_FLOOR;
          }
          if (stddev[i] <= 0.0) stddev[i] = STDDEV_FLOOR;
          dst[i] = ( (src[i]-(FLOAT_DMEM)mean[i])/(FLOAT_DMEM)stddev[i] + (FLOAT_DMEM)mean[i] );
        }
      }
    } else if (normEnable) {
      if (meanEnable) {
        for (i=0; i<MIN(Nsrc,Ndst); i++) {
          double range = (max[i]-min[i])/2.0;
          if (range <= 0.0) range = 1.0;
          dst[i] = (src[i]-(FLOAT_DMEM)mean[i])/(FLOAT_DMEM)range;
        }
      } else {
        if (htkLogEnorm) {
          for (i=0; i<MIN(Nsrc,Ndst); i++) {
            // HTK compatible energy normalisation...
            dst[i] = ( (FLOAT_DMEM)1.0-((FLOAT_DMEM)max[i]-src[i]) );
          }
        } else { // subtract min and divide by range  (normalises to 0..1)
          for (i=0; i<MIN(Nsrc,Ndst); i++) {
            double range = (max[i]-min[i]);
            if (range <= 0.0) range = 1.0;
            //dst[i] = ( (src[i]-(FLOAT_DMEM)mean[i])/(FLOAT_DMEM)range + (FLOAT_DMEM)mean[i]);
            dst[i] = ( (src[i]-(FLOAT_DMEM)min[i])/(FLOAT_DMEM)range);
          }
        }

      }
    } else if (meanEnable) {
      if (spectralFlooring) {
        for (i=0; i<MIN(Nsrc,Ndst); i++) {
          if (src[i]<(FLOAT_DMEM)mean[i]) {
            dst[i] = specFloor;
          } else {
            if (subtractMeans) {
              dst[i] = src[i] - (FLOAT_DMEM)mean[i];
            } else {
              dst[i] = src[i];
            }
          }
        }
      } else {
        for (i=0; i<MIN(Nsrc,Ndst); i++) {
          dst[i] = src[i]-(FLOAT_DMEM)mean[i];
        }
      }
    } else {
      return 0;
    }

  }

  return MIN(Nsrc,Ndst);
}

//TODO: support updates for minMaxNormEnable

int cVectorMVN::updateTransformExp(struct sTfData * tf, const FLOAT_DMEM *src, int idxi)
{
  long i;
  long N = tf->head.vecSize;
  double *mean = tf->vectors;
  double *stddev = NULL;
  double *min = NULL;
  double *max = NULL;

  /* update means */ 
  for (i=0; i<N; i++) {
    mean[i] = alpha * (mean[i] - (double)src[i]) + (double)src[i];
  }

  /* update stddev */ 
  if (stdEnable) {
    stddev = mean+N;
    for (i=0; i<N; i++) {
      double x = (src[i] - mean[i]);
      x *= x; // square it
      stddev[i] = sqrt( alpha * (stddev[i]*stddev[i] - x) + x );
    }
  }

  /* update range */ 
  if (normEnable) {
    min = mean+N;
    max = min+N;
    FLOAT_DMEM alphaM = (((FLOAT_DMEM)1.0 - alpha)/(FLOAT_DMEM)2.0 + alpha);
    for (i=0; i<N; i++) {
      if ((double)src[i] > max[i]) max[i] = (double)src[i];
      else { max[i] = alphaM * (double)src[i]; }
      if ((double)src[i] < min[i]) min[i] = (double)src[i];
      else { min[i] = (double)src[i] / alphaM; }
    }
  }

  return 1;
}

int cVectorMVN::updateTransformBuf(struct sTfData * tf, const FLOAT_DMEM *src, FLOAT_DMEM *buf, long Nbuf, long wrPtr, int idxi)
{
  long i;
  long N = tf->head.vecSize;
  double *mean = tf->vectors;
  double *stddev = NULL;
  double *min = NULL;
  double *max = NULL;

  /* update means */ 
  if (Nbuf == fixedBufferFrames) {
    for (i=0; i<N; i++) {
      double sum = mean[i] * (double)(Nbuf);
      sum += src[i];
      // remove last input in buffer from sum in means[]:
      sum -= buf[i + N*wrPtr];
      mean[i] = sum/(double)(Nbuf);
    }
  } else {
    // TODO: maybe use initial value better here??
    for (i=0; i<N; i++) {
      double sum = mean[i] * (double)(Nbuf);
      sum += src[i];
      mean[i] = sum/(double)(Nbuf+1);
    }
  }

  /* update stddev */ 
  if (stdEnable) {
    stddev = mean+N;
    if (Nbuf == fixedBufferFrames) {
      for (i=0; i<N; i++) {
        double x = (src[i] - mean[i])*(src[i] - mean[i]);
        double sum = stddev[i]*stddev[i] * (double)(Nbuf);
        sum += x;
        // remove last input in buffer from sum in means[]:
        sum -= (buf[i + N*wrPtr] - mean[i])*(buf[i + N*wrPtr] - mean[i]); 
          // ^^^!!! WARNING: the mean has changed, so this approximation leads to some errors...
          //          easy workaround: always compute variance over FULL buffer... :-(
          //          hard workaround: manage a second ring-buffer where the means are stored...
        stddev[i] = sqrt(sum/(double)(Nbuf));
      }
    } else {
      for (i=0; i<N; i++) {
        double x = (src[i] - mean[i])*(src[i] - mean[i]);
        double sum = stddev[i]*stddev[i] * (double)(Nbuf);
        sum += x;
          // TODO: see above, maybe add adding mean values to buffer...
        stddev[i] = sqrt(sum/(double)(Nbuf+1));
      }
    }
  }

  /* update range */ 
  if (normEnable) {
    min = mean+N;
    max = min+N;
    if (Nbuf == fixedBufferFrames) {
      for (i=0; i<N; i++) {
        if ((double)src[i] > max[i]) max[i] = (double)src[i];
        else if ((double)buf[i + N*wrPtr] == max[i]) { // we have to rescan the full buffer for a new max if the old max is the last element in the buffer ...
          long j;
          FLOAT_DMEM newmax;
          if (wrPtr != 0) newmax = buf[i + N*0];
          else newmax = buf[i + N*1];
          for (j=0; j<Nbuf; j++) {
            if (j!=wrPtr) {
              if (buf[i+N*j] > newmax) { newmax = buf[i+N*j]; }
            }
          }
          if (src[i] > newmax) max[i] = (double)src[i];
          else max[i] = (double)newmax;
        }

        
        if ((double)src[i] < min[i]) min[i] = (double)src[i];
        else if ((double)buf[i + N*wrPtr] == min[i]) { // we have to rescan the full buffer for a new min if the old min is the last element in the buffer ...
          long j;
          FLOAT_DMEM newmin;
          if (wrPtr != 0) newmin = buf[i + N*0];
          else newmin = buf[i + N*1];
          for (j=0; j<Nbuf; j++) {
            if (j!=wrPtr) {
              if (buf[i+N*j] < newmin) { newmin = buf[i+N*j]; }
            }
          }
          if (src[i] < newmin) min[i] = (double)src[i];
          else min[i] = (double)newmin;
        }

      }
    } else {
      for (i=0; i<N; i++) {
        if (Nbuf == 0) {
          max[i] = (double)src[i];
          min[i] = (double)src[i];
        } else {
          if ((double)src[i] > max[i]) max[i] = (double)src[i];
          if ((double)src[i] < min[i]) min[i] = (double)src[i];
        }
      }

    }
  }

  return 1;
}


int cVectorMVN::updateTransformAvg(struct sTfData * tf, const FLOAT_DMEM *src, int idxi)
{
  long i;
  long N = tf->head.vecSize;
  double *mean = tf->vectors;
  double *stddev = NULL;
  double *min = NULL;
  double *max = NULL;

  FLOAT_DMEM nVal=(FLOAT_DMEM)(nFrames-1);

  /* update means */ 
  for (i=0; i<N; i++) {
    double sum = mean[i] * (double)(nVal+weight);
    sum += (double)src[i];
    mean[i] = sum/(double)(nVal+weight+1.0);
  }

  /* update stddev */ 
  if (stdEnable) {
    stddev = mean+N;
    for (i=0; i<N; i++) {
      double sum = stddev[i]*stddev[i] * (double)(nVal+weight);
      sum += ((double)src[i] - mean[i])*((double)src[i] - mean[i]);
      stddev[i] = sqrt(sum/(double)(nVal+weight+1.0));
    }
  }

  /* update range */ 
  if (normEnable) {
    min = mean+N;
    max = min+N;
    for (i=0; i<N; i++) {
      if ((double)src[i] < min[i]) min[i] = (double)src[i];
      if ((double)src[i] > max[i]) max[i] = (double)src[i];
    }
  }

  return 1;
}

int cVectorMVN::updateTransformAvgI(struct sTfData * tf, const FLOAT_DMEM *src, FLOAT_DMEM *buf, long * _bufferNframes, long Nbuf, long wrPtr, int idxi)
{
  return updateTransformAvg(tf,src,idxi);
  /*long i;
  long N = tf->head.vecSize;
  double *mean = tf->head.vectors;
  double *stddev = NULL;
  double *min = NULL;
  double *max = NULL;

  FLOAT_DMEM nVal=(FLOAT_DMEM)(nFrames-1);
*/
/* all there is to do... ??
  on turn reset, preserve current mean values (do not init from transform0, instead update transform0?)
  reset nFrames though...

  */

#if 0
  /* update means */ 
  for (i=0; i<N; i++) {
    double sum = mean[i] * (double)(nVal+weight);
    sum += (double)src[i];
    mean[i] = sum/(double)(nVal+weight+1.0);
  }

  /* update stddev */ 
  if (stdEnable) {
    stddev = mean+N;
    for (i=0; i<N; i++) {
      double sum = stddev[i]*stddev[i] * (double)(nVal+weight);
      sum += ((double)src[i] - mean[i])*((double)src[i] - mean[i]);
      stddev[i] = sqrt(sum/(double)(nVal+weight+1.0));
    }
  }

  /* update range */ 
  if (normEnable) {
    min = mean+N;
    max = min+N;
    for (i=0; i<N; i++) {
      if ((double)src[i] < min[i]) min[i] = (double)src[i];
      if ((double)src[i] > max[i]) max[i] = (double)src[i];
    }
  }
#endif

  //return 1;
}

cVectorMVN::~cVectorMVN()
{
}

