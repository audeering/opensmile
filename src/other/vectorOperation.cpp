/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


/*  openSMILE component:

do elementary operations on vectors 
(i.e. basically everything that does not require history or context,
 everything that can be performed on single vectors w/o external data (except for constant parameters, etc.))

*/



#include <other/vectorOperation.hpp>
#include <cmath>

#define MODULE "cVectorOperation"


SMILECOMPONENT_STATICS(cVectorOperation)

SMILECOMPONENT_REGCOMP(cVectorOperation)
{
  SMILECOMPONENT_REGCOMP_INIT
  scname = COMPONENT_NAME_CVECTOROPERATION;
  sdescription = COMPONENT_DESCRIPTION_CVECTOROPERATION;

  // we inherit cVectorProcessor configType and extend it:
  SMILECOMPONENT_INHERIT_CONFIGTYPE("cVectorProcessor")

  // if the inherited config type was found, we register our configuration variables
  SMILECOMPONENT_IFNOTREGAGAIN( {} // <- this is only to avoid compiler warnings...
    // name append has a special role: it is defined in cDataProcessor, and can be overwritten here:
      // if you set description to NULL, the existing description will be used, thus the following call can
      // be used to update the default value:
    //ct->setField("nameAppend",NULL,"processed");
  ct->setField("operation","A string which specifies the type of operation to perform:\n   norm = normalise vector length (euclidean norm, L2) to 1\n   nr1 = normalise range to +1, -1\n   nr0 = normalise range to +1, 0\n   nma = divide by the maximum absolute value\n   mul = multiply vector by param1\n   add = add param1 to each element\n   log = compute natural logarithm\n   lgA = compute logarithm to base param1\n   nl1 = normalise vector sum (L1 norm) to 1\n   sqrt = compute square root\n   pow = take values to the power of param1\n   exp = raise param1 to the power of the vector elements\n   ee = raise the base e to the power of the vector elements\n   abs = take absolute value of each element\n   agn = add Gaussian noise with mean param1 and std.dev. param2\n   min = take the min of vector and param1\n   max = take the max of vector and param1\n   sum = compute sum of vector elements, there will be a single output only\n   ssm = compute sum of squared vector elements, there will be a single output only\n   ll1 = compute sum of vector elements normalised by the number of vector elements, there will be a single output only\n   ll2 = compute euclidean length (root of sum of squares normalised by vector length), there will be a single output only\n   fla(tten) = flattening of comb filter energy spectra, as in 2007 ICASSP Paper and Ballroom dance style recognition.\n   dBp = convert a power to decibel with 10*log10(x).\n   dBv = convert an amplitude/magnitude/voltage to decibel with 20*log10(x)\n   fconv_aaa_bbb = convert frequency from scale aaa to scale bbb\n     lin = linear (Hz)\n     bark = Bark (Traunmueller, 1990)\n     mel = Mel-scale\n     oct = semitone/octave scale (music), param1 = freq. of first note in Hz.", "norm");
  ct->setField("param1","parameter 1", 1.0);
  ct->setField("param2","parameter 2", 1.0);
  ct->setField("logfloor","floor for log operation", 0.000000000001);
  ct->setField("powOnlyPos","if 'operation' = 'pow', do not take negative values to the power of 'param1'; instead, output 0. This is necessary to avoid 'nan' values if the exponent is rational. ", 0);
  ct->setField("nameBase","base of output feature name when performing n->1 mapping operations (currently 'euc' and 'sum')", (const char*)NULL);
  ct->setField("appendOperationToName", "(1/0 = yes/no) append the operation name from the 'operation' option to the feature name. This will override any nameAppend option (inherited from cDataProcessor).", 0);
  )

  // The configType gets automatically registered with the config manger by the SMILECOMPONENT_IFNOTREGAGAIN macro
  
  // we now create out sComponentInfo, including name, description, success status, etc. and return that
  SMILECOMPONENT_MAKEINFO(cVectorOperation);
}

SMILECOMPONENT_CREATE(cVectorOperation)

//-----

cVectorOperation::cVectorOperation(const char *_name) :
  cVectorProcessor(_name),
  param1(0.0),
  param2(0.0),
  nameBase(NULL),
  operationString_(NULL),
  targFreqScaleStr(NULL)
{

}

void cVectorOperation::myFetchConfig()
{
  cVectorProcessor::myFetchConfig();

  param1 = (FLOAT_DMEM)getDouble("param1");
  param2 = (FLOAT_DMEM)getDouble("param2");
  powOnlyPos = (int)getInt("powOnlyPos");
  fscaleA = SPECTSCALE_LINEAR;
  fscaleB = SPECTSCALE_LINEAR;

  const char * op = getStr("operation");
  operation=VOP_NORMALISE;
  if (!strncmp(op,"nor",3)) {
    operation=VOP_NORMALISE;
  } else if (!strncmp(op,"nr1",3)) {
    operation=VOP_NORM_RANGE1;
  } else if (!strncmp(op,"nr0",3)) {
    operation=VOP_NORM_RANGE0;
  } else if (!strncmp(op,"nma",3)) {
    operation=VOP_NORM_MAX_ABS;
  } else if (!strncmp(op,"mul",3)) {
    operation=VOP_MUL;
  } else if (!strncmp(op,"add",3)) {
    operation=VOP_ADD;
  } else if (!strncmp(op,"log",3)) {
    operation=VOP_LOG;
  } else if (!strncmp(op,"lgA",3)) {
    operation=VOP_LOGA;
    if (param1 <= 0) {
      SMILE_IWRN(1,"log-base cannot be negative! setting base to exp(1) (-> natural logarithm)");
      param1 = (FLOAT_DMEM)std::exp(1.0);
    }
    if (param1 == 1.0) {
      SMILE_IWRN(1,"log-base cannot be 1.0! setting base to exp(1) (-> natural logarithm)");
      param1 = (FLOAT_DMEM)std::exp(1.0);
    }
  } else if (!strncmp(op,"nl1",3)) {
    operation=VOP_NORMALISE_L1;
  } else if (!strncmp(op,"sqr",3)) {
    operation=VOP_SQRT;
  } else if (!strncmp(op,"pow",3)) {
    operation=VOP_POW;
  } else if (!strncmp(op,"exp",3)) {
    operation=VOP_EXP;
  } else if (!strncmp(op,"ee",2)) {
    operation=VOP_E;
  } else if (!strncmp(op,"abs",3)) {
    operation=VOP_ABS;
  } else if (!strncmp(op,"agn",3)) {
    operation=VOP_AGN;
  } else if (!strncmp(op,"min",3)) {
    operation=VOP_MIN;
  } else if (!strncmp(op,"max",3)) {
    operation=VOP_MAX;
  } else if (!strncmp(op,"sum",3)) {
    operation=VOP_X_SUM;
  } else if (!strncmp(op,"ssm",3)) {
    operation=VOP_X_SUMSQ;
  } else if (!strncmp(op,"ll1",3)) {
    operation=VOP_X_L1;
  } else if (!strncmp(op,"ll2",3)) {
    operation=VOP_X_L2;
  } else if (!strncmp(op,"fla",3)) {
      operation=VOP_FLATTEN;
  } else if (!strncmp(op,"dBp",3)) {
    operation=VOP_DB_POW;
  } else if (!strncmp(op,"dBv",3)) {
    operation=VOP_DB_MAG;
  } else if (!strncmp(op,"fconv_",6)) {
    const char * strA = op+6;
    const char * strB = NULL;
    operation=VOP_FCONV;
    if (!strncmp(strA, "lin_", 4)) {
      fscaleA = SPECTSCALE_LINEAR;
      strB = strA + 4;
    }
    else if (!strncmp(strA, "linear_", 7)) {
      fscaleA = SPECTSCALE_LINEAR;
      strB = strA + 7;
    }
    else if (!strncmp(strA, "bark_", 5)) {
      fscaleA = SPECTSCALE_BARK;
      strB = strA + 5;
    }
    else if (!strncmp(strA, "mel_", 4)) {
      fscaleA = SPECTSCALE_MEL;
      strB = strA + 4;
    }
    else if (!strncmp(strA, "oct_", 4)) {
      fscaleA = SPECTSCALE_SEMITONE;
      strB = strA + 4;
    }
    else if (!strncmp(strA, "octave_", 7)) {
      fscaleA = SPECTSCALE_SEMITONE;
      strB = strA + 7;
    }
    else if (!strncmp(strA, "semitone_", 9)) {
      fscaleA = SPECTSCALE_SEMITONE;
      strB = strA + 9;
    }
    else if (!strncmp(strA, "semi_", 5)) {
      fscaleA = SPECTSCALE_SEMITONE;
      strB = strA + 5;
    }
    else {
      SMILE_IERR(1, "Unknown frequency scales '%s'. Using 'lin'(ear) for scale A and B.", strA);
      strB = NULL;
    }

    if (strB != NULL) {
      if (!strncmp(strB, "lin", 3)) {
        fscaleB = SPECTSCALE_LINEAR;
        targFreqScaleStr = "linearHzScale";
      }
      else if (!strncmp(strB, "bark", 4)) {
        fscaleB = SPECTSCALE_BARK;
        targFreqScaleStr = "barkScale";
      }
      else if (!strncmp(strB, "mel", 3)) {
        fscaleB = SPECTSCALE_MEL;
        targFreqScaleStr = "melScale";
      }
      else if (!strncmp(strB, "oct", 3)) {
        fscaleB = SPECTSCALE_SEMITONE;
        targFreqScaleStr = "octaveScale";
      }
      else if (!strncmp(strB, "semi_", 4)) {
        fscaleB = SPECTSCALE_SEMITONE;
        targFreqScaleStr = "octaveScale";
      } else {
        SMILE_IERR(1, "unknown operation B '%s' specified in config file. Using 'lin'(ear) for scale B.", strB);
      }
    } else {
      SMILE_IERR(1, "unknown operation B '%s' specified in config file. Using 'lin'(ear) for scale B.", strB);
    }
    SMILE_PRINT("fscale conversion: %i to %i  ", fscaleA, fscaleB);
  }

  if (getInt("appendOperationToName")) {
    if (operation == VOP_LOGA) {
      operationString_ = myvprint("%s%.1f", op, param1);
    } else {
      operationString_ = myvprint("%s", op);
    }
  }
  nameBase = getStr("nameBase");

  logfloor = (FLOAT_DMEM)getDouble("logfloor");
  if (logfloor <= 0) { 
    SMILE_IWRN(1,"log-floor cannot be <= 0 ! setting to 0.00000000001 !");
    logfloor = (FLOAT_DMEM)0.000000000001;
  }
}

int cVectorOperation::setupNamesForField(int i, const char*name, long nEl)
{
  if (operation == VOP_FCONV) {
    if (nameAppend_ == NULL || nameAppend_[0] == 0) {
      nameAppend_ = targFreqScaleStr;
    }
  }
  else if (operation > VOP_X) {
    if (nameAppend_ == NULL || nameAppend_[0] == 0) {
      if (operation == VOP_X_SUM) nameAppend_ = "sum";
      if (operation == VOP_X_SUMSQ) nameAppend_ = "sumSq";
      if (operation == VOP_X_L1) nameAppend_ = "lengthL1norm";
      if (operation == VOP_X_L2) nameAppend_ = "lengthL2norm";
    }
    nEl = 1;
  } else {
    if (operationString_ != NULL) {
      nameAppend_ = operationString_;
    }
  }
  if (nameBase != NULL && nameBase[0] != 0) {
    name = nameBase;
  }
  return cVectorProcessor::setupNamesForField(i, name, nEl);
}

/*
int cVectorOperation::setupNewNames(long ni)
{
  return 0;

  long No=0;

  if (operation > VOP_X) {
    No = 1;
    if (copyInputName_) {
      if (operation == VOP_X_SUM) addNameAppendField(nameBase,"sum",1,0);
      if (operation == VOP_X_SUMSQ) addNameAppendField(nameBase,"sumSq",1,0);
      if (operation == VOP_X_L1) addNameAppendField(nameBase,"lengthL1norm",1,0);
      if (operation == VOP_X_L2) addNameAppendField(nameBase,"lengthL2norm",1,0);
      namesAreSet_ = 1;
    } else {
      if (nameAppend_ != NULL && nameAppend_[0] != 0) {
        addNameAppendField(NULL, nameAppend_, 1, 0);
      }
    }
  } else {
    // TODO: append name of operation to output!
      // set nameAppend variable?
    if (operationString_ != NULL) {
      nameAppend_ = operationString_;
    }
  }
  return No;
}
*/

// a derived class should override this method, in order to implement the actual processing
int cVectorOperation::processVector(const FLOAT_DMEM *src, FLOAT_DMEM *dst, long Nsrc, long Ndst, int idxi) // idxi=input field index
{
  // do domething to data in *src, save result to *dst
  // NOTE: *src and *dst may be the same...
  FLOAT_DMEM sum =0.0;
  FLOAT_DMEM max, min, range;
  FLOAT_DMEM factor = 1.0;
  long i;
  switch(operation) {
    case VOP_NORMALISE:
      // normalise L2 norm of all incoming vectors
      for (i=0; i<MIN(Nsrc,Ndst); i++) {
        sum+=src[i]*src[i];
      }
      if (sum <= 0.0) {
        sum = 1.0;
      }
      sum = sqrt(sum/(FLOAT_DMEM)(MIN(Nsrc,Ndst)));
      for (i=0; i<MIN(Nsrc,Ndst); i++) {
        dst[i] = src[i] / sum;
      }

      break;

    case VOP_NORMALISE_L1:

      // normalise L1 norm of all incoming vectors
      for (i=0; i<MIN(Nsrc,Ndst); i++) {
        sum+=src[i];
      }
      if (sum <= 0.0) {
        sum = 1.0;
      }
      for (i=0; i<MIN(Nsrc,Ndst); i++) {
        dst[i] = src[i] / sum;
      }

      break;

    case VOP_NORM_RANGE0:
    case VOP_NORM_RANGE1:

       // normalise vector range
       max=src[0]; min=src[0];
       for (i=1; i<MIN(Nsrc,Ndst); i++) {
         if (src[i] > max) max = src[i];
         if (src[i] < min) min = src[i];
       }
       range = max-min; if (range == 0.0) range = 1.0;

       if (operation == VOP_NORM_RANGE0) { // 0..1
         for (i=0; i<MIN(Nsrc,Ndst); i++) {
           dst[i] = (src[i] - min) / range;
         }
       } else { // VOP_NORM_RANGE1 : -1 .. +1
         range *= 0.5;
         for (i=0; i<MIN(Nsrc,Ndst); i++) {
           dst[i] = ( (src[i] - min) / range ) - (FLOAT_DMEM)1.0;
         }
       }
       break;


    case VOP_NORM_MAX_ABS:
      max=fabs(src[0]);
      for (i=1; i<MIN(Nsrc,Ndst); i++) {
        if (fabs(src[i]) > max) max = fabs(src[i]);
      }
      if (max != 0.0) {
        for (i=0; i<MIN(Nsrc,Ndst); i++) {
          dst[i] = src[i] / max;
        }
      }
      break;


    case VOP_ADD:
      for (i=0; i<MIN(Nsrc,Ndst); i++) {
        dst[i] = src[i] + param1;
      }
      break;

    case VOP_MUL:
      for (i=0; i<MIN(Nsrc,Ndst); i++) {
        dst[i] = src[i] * param1;
      }
      break;

    case VOP_LOG:
      for (i=0; i<MIN(Nsrc,Ndst); i++) {
        if (src[i]>logfloor) {
          dst[i] = std::log(src[i]);
        } else {
          dst[i] = std::log(logfloor);
        }
      }
      break;

    case VOP_LOGA:
      sum = std::log(param1);
      for (i=0; i<MIN(Nsrc,Ndst); i++) {
        if (src[i]>logfloor) {
          dst[i] = std::log(src[i]) / sum;
        } else {
          dst[i] = std::log(logfloor) / sum;
        }
      }
      break;

    case VOP_SQRT:
      for (i=0; i<MIN(Nsrc,Ndst); i++) {
        if (src[i]>0.0) {
          dst[i] = std::sqrt(src[i]);
        } else {
          dst[i] = 0.0;
        }
      }
      break;

    case VOP_POW:
      for (i=0; i<MIN(Nsrc,Ndst); i++) {
        if (powOnlyPos) {
          if (src[i]>0.0) {
            dst[i] = std::pow(src[i],param1);
          } else {
            dst[i] = 0.0;
          }
        } else {
          dst[i] = std::pow(src[i],param1);
        }
      }
      break;

    case VOP_EXP:
      for (i=0; i<MIN(Nsrc,Ndst); i++) {
        dst[i] = std::pow(param1,src[i]);
      }
      break;
    
    case VOP_E:
      for (i=0; i<MIN(Nsrc,Ndst); i++) {
        dst[i] = std::exp(src[i]);
      }
      break;

    case VOP_ABS:
      for (i=0; i<MIN(Nsrc,Ndst); i++) {
        dst[i] = fabs(src[i]);
      }
      break;

    case VOP_AGN:
      for (i = 0; i < MIN(Nsrc,Ndst); i++) {
        dst[i] = src[i] + param2 * (FLOAT_DMEM)gnGenerator() + param1;
      }
      break;

    case VOP_MIN:
      for (i = 0; i < MIN(Nsrc,Ndst); i++) {
        if (src[i] < param1) {
          dst[i] = src[i];
        } else {
          dst[i] = param1;
        }
      }
      break;

    case VOP_MAX:
      for (i=0; i<MIN(Nsrc,Ndst); i++) {
        if (src[i] > param1) {
          dst[i] = src[i];
        } else {
          dst[i] = param1;
        }
      }
      break;

    case VOP_X_SUM:
      dst[0] = 0.0;
      for (i=0; i<Nsrc; i++) {
        dst[0] += src[i];
      }
      break;

   case VOP_X_SUMSQ:
      dst[0] = 0.0;
      for (i=0; i<Nsrc; i++) {
        dst[0] += src[i]*src[i];
      }
      break;

    case VOP_X_L1:
      dst[0] = 0.0;
      for (i=0; i<Nsrc; i++) {
        dst[0] += src[i];
      }
      if (Nsrc > 0) dst[0] /= (FLOAT_DMEM)Nsrc;
      break;

    case VOP_X_L2:
      dst[0] = 0.0;
      for (i=0; i<Nsrc; i++) {
        dst[0] += src[i]*src[i];
      }
      if (dst[0] > 0.0) dst[0] = sqrt(dst[0]);
      if (Nsrc > 0) dst[0] /= (FLOAT_DMEM)Nsrc;
      break;

    case VOP_FLATTEN:
      if (Nsrc > 0) {
        FLOAT_DMEM m1=0.0,m2=0.0;
        for (i=0; i<MIN(Nsrc,5); i++) {
          m1 += src[i];
        } m1 /= (FLOAT_DMEM)(MIN(Nsrc,5));
        for (i=MAX(0,Nsrc-5); i<Nsrc; i++) {
          m2 += src[i];
        } m2 /= (FLOAT_DMEM)(MIN(Nsrc,5));
        FLOAT_DMEM slope = (m1-m2)/(FLOAT_DMEM)Nsrc;
        for (i=0; i<MIN(Nsrc,Ndst); i++) {
          dst[i] = src[i] - m2 - slope * (FLOAT_DMEM)(Nsrc-i);
        }
      }
      break;

    case VOP_DB_POW:
      factor = (FLOAT_DMEM)(10.0 / std::log(10.0));
      for (i=0; i<MIN(Nsrc,Ndst); i++) {
        if (src[i]>logfloor) {
          dst[i] = factor * std::log(src[i]);
        } else {
          dst[i] = factor * std::log(logfloor);
        }
      }
      break;
    case VOP_DB_MAG:
      factor = (FLOAT_DMEM)(20.0 / std::log(10.0));
      for (i=0; i<MIN(Nsrc,Ndst); i++) {
        if (src[i]>logfloor) {
          dst[i] = factor * std::log(src[i]);
        } else {
          dst[i] = factor * std::log(logfloor);
        }
      }
      break;
    case VOP_FCONV:
      for (i=0; i<MIN(Nsrc,Ndst); i++) {
        double ftmp = src[i];
        if (fscaleA != SPECTSCALE_LINEAR) {
          ftmp = smileDsp_specScaleTransfInv(ftmp, fscaleA, param1);
        }
        dst[i] = (FLOAT_DMEM)smileDsp_specScaleTransfFwd(ftmp, fscaleB, param1);
      }
      break;
    default:  // no operation
      for (i=0; i<MIN(Nsrc,Ndst); i++) {
        dst[i] = src[i];
      }
      break;
  }
  return 1;
}


// Code contributed by Felix Weninger
// Gaussian noise generator
double cVectorOperation::gnGenerator() {
  static bool haveNumber = false;
  static double number = 0.0;
  if (haveNumber) {
    haveNumber = false;
    return number;
  }
  else {
    double q = 2.0;
    double x, y;
    // Use Polar Method to obtain normally distributed random numbers.
    while (q > 1.0) {
        x = ((double)rand() / RAND_MAX) * 2.0 - 1.0;
        y = ((double)rand() / RAND_MAX) * 2.0 - 1.0;
        q = x * x + y * y;
    }
    double z = -2.0 * log(q) / q;
    number = y * sqrt(z);
    haveNumber = true;
    return x * sqrt(z);
  }
}


cVectorOperation::~cVectorOperation()
{
  if (operationString_ != NULL) {
    free(operationString_);
  }
}

