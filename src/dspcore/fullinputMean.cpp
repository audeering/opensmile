/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


/*  openSMILE component:


*/


#include <dspcore/fullinputMean.hpp>

#define MODULE "cFullinputMean"

SMILECOMPONENT_STATICS(cFullinputMean)

SMILECOMPONENT_REGCOMP(cFullinputMean)
{
  SMILECOMPONENT_REGCOMP_INIT

  scname = COMPONENT_NAME_CFULLINPUTMEAN;
  sdescription = COMPONENT_DESCRIPTION_CFULLINPUTMEAN;

  // we inherit cVectorProcessor configType and extend it:
  SMILECOMPONENT_INHERIT_CONFIGTYPE("cDataProcessor")
  
    SMILECOMPONENT_IFNOTREGAGAIN( {}
    //ct->setField("expandFields", "expand fields to single elements, i.e. each field in the output will correspond to exactly one element in the input [not yet implemented]", 0);
    //ct->setField("htkcompatible", "1 = HTK compatible mean subtraction (this ignores (i.e. does not subtract the mean of) the 0th MFCC or energy field (and its deltas) specified by the option 'idx0')", 0);
    //ct->setField("idx0", "if > 0, index of field to ignore for mean subtraction (attention: 1st field is 0)", 0);
    ct->setField("mvn", "1 = perform mean + variance (stddev) normalisation, overrides 'meanNorm' option"
        " and forces meanNorm=amean.", 0);
    ct->setField("meanNorm", "Type of mean normalisation: amean, rqmean, absmean  (arithmetic, root squared/quadratic, absolute value mean).", "amean");
    ct->setField("symmSubtract", "1 = Perform symmetric subtraction of rqmean or absmean u. I.e. for negative values add u and for positive values subtract u.", 0);
    ct->setField("subtractClipToZero", "1 = If symmSubtract is enabled and a value would change sign, clip it to 0. Otherwise, clip negative values to 0 when subtracting any kind of mean. 0 = do nothing special.", 0);
    ct->setField("specEnorm", "performs spectral magnitude energy normalisation", 0);
    ct->setField("htkLogEnorm","performs HTK compatible energy normalisation on all input fields instead of the default action of mean subtraction. The energy normalisation subtracts the maximum value of each value in the sequence and adds 1.",0);
    ct->setField("multiLoopMode", "1 = Support the new tick loop mode which can have unlimited EOI iterations. In this mode the means will be collected until the EOI condition is signalled. During the EOI condition nothing will be done (except computing means from the remaining data). During the next non EOI condition, the means will be subtracted from the old input, and (if new data is available - e.g. from a next segment) a new set of means will be computed. If old and new data is processed, the cycle begins anew. If this option is disabled, the means are subtracted while the first EOI condition is signalled. This is for compatibility with old behaviour of the tick loop.", 0);
    ct->setField("printMeans", "1 = print the mean vector once it has been computed.", 0);
    ct->setField("printStddevs", "1 = print the standard deviation vector once it has been computed (if MVN=1 is set).", 0);
    ct->setField("excludeZeros", "1 = exclude 0 values or NaN values from mean/variance computation and normalistaion. Use e.g. for F0 contours. Only supported for mean type 'amean'.", 0);
  )

  SMILECOMPONENT_MAKEINFO(cFullinputMean);
}

SMILECOMPONENT_CREATE(cFullinputMean)


// TODO:
// option to do something with the means:
//   print
//   write to second level repeatedly ("splitter")
//   write to second level once

//-----

cFullinputMean::cFullinputMean(const char *_name) :
  cDataProcessor(_name),
  print_means_(0),
  first_frame_(1),
  reader_pointer_(0),
  reader_pointer2_(0),
  reader_pointer3_(0),
  multi_loop_mode_(0),
  mean_type_(MEANTYPE_AMEAN),
  spec_enorm_(0),
  symm_subtract_(0),
  symm_subtract_clip_to_zero_(0),
  flag_(0),
  eoi_flag_(0),
  means_(NULL),
  means2_(NULL),
  variances_(NULL),
  variances2_(NULL),
  n_means_(0),
  n_means2_(0),
  n_variances_(0),
  n_variances2_(0),
  mvn_(false),
  n_means_arr_(NULL), n_means2_arr_(NULL),
  n_variances_arr_(NULL), n_variances2_arr_(NULL)
{
}

void cFullinputMean::myFetchConfig()
{
  cDataProcessor::myFetchConfig();

  int e_norm_mode = (int)getInt("htkLogEnorm");
  mvn_ = (getInt("mvn") == 1);
  exclude_zeros_ = (getInt("excludeZeros") == 1);
  print_means_ = (int)getInt("printMeans");
  print_variances_ = (int)getInt("printStddevs");
  multi_loop_mode_ = (int)getInt("multiLoopMode");
  symm_subtract_ = (int)getInt("symmSubtract");
  symm_subtract_clip_to_zero_ = (int)getInt("subtractClipToZero");
  spec_enorm_ = (int)getInt("specEnorm");
  if (mvn_) {
    mean_type_ = MEANTYPE_AMEAN;
    SMILE_IMSG(3, "Forcing meanNorm = amean for mvn == 1.");
  } else {
    const char * s = getStr("meanNorm");
    if (s != NULL) {
      if (!strncmp(s, "rqm", 3)) {
        mean_type_ = MEANTYPE_RQMEAN;
      } else if (!strncmp(s, "ame", 3)) {
        mean_type_ = MEANTYPE_AMEAN;
      } else if (!strncmp(s, "absm", 4)) {
        mean_type_ = MEANTYPE_ABSMEAN;
      } else {
        COMP_ERR("Unknown mean type set for option 'meanNorm'. See the help (-H) for supported types.");
      }
    }
  }
  if (mean_type_ != MEANTYPE_AMEAN && exclude_zeros_) {
    SMILE_IWRN(1, "exclude_zeros_ will be deactivated because meanNorm != amean!");
    exclude_zeros_ = false;
  }
  if (e_norm_mode) {
    mean_type_ = MEANTYPE_ENORM;
  }
}

// Reads new data, computes stats in means_ and n_means_.
eTickResult cFullinputMean::readNewData()
{
  cVector *vec = reader_->getNextFrame();
  if (vec == NULL) 
    return TICK_SOURCE_NOT_AVAIL;
  if (means_ == NULL) {
    means_ = new cVector(vec->N);
    n_means_arr_ = new long[vec->N];
    if (mean_type_ == MEANTYPE_RQMEAN) {
      for (int i = 0; i < vec->N; i++) {
        means_->data[i] = vec->data[i] * vec->data[i];
      }
    } else if (mean_type_ == MEANTYPE_ABSMEAN) {
      for (int i = 0; i < vec->N; i++) {
        means_->data[i] = (FLOAT_DMEM)fabs(vec->data[i]);
      }
    } else {
      if (mean_type_ == MEANTYPE_AMEAN && exclude_zeros_) {
        for (int i = 0; i < vec->N; i++) {
          if (vec->data[i] != 0.0) {
            means_->data[i] = vec->data[i];
            n_means_arr_[i] = 1;
          }
        }
      } else {
        for (int i = 0; i < vec->N; i++) {
          means_->data[i] = vec->data[i];
        }
      }
    }
    n_means_ = 1;
  } else {
    if (mean_type_ == MEANTYPE_ENORM) {
      for (int i = 0; i < vec->N; i++) {
        if (vec->data[i] > means_->data[i])
          means_->data[i] = vec->data[i];
      }
    } else if (mean_type_ == MEANTYPE_AMEAN) {
      if (exclude_zeros_) {
        for (int i = 0; i < vec->N; i++) {
          if (vec->data[i] != 0.0) {
            means_->data[i] += vec->data[i];
            n_means_arr_[i]++;
          }
        }
      } else {
        for (int i = 0; i < vec->N; i++) {
          means_->data[i] += vec->data[i];
        }
      }
      n_means_++;
    } else if (mean_type_ == MEANTYPE_RQMEAN) {
      for (int i = 0; i < vec->N; i++) {
        means_->data[i] += vec->data[i] * vec->data[i];
      }
      n_means_++;
    } else if (mean_type_ == MEANTYPE_ABSMEAN) {
      for (int i = 0; i < vec->N; i++) {
        means_->data[i] += (FLOAT_DMEM)fabs(vec->data[i]);
      }
      n_means_++;
    }
  }
  return TICK_SUCCESS;
}

void cFullinputMean::meanSubtract(cVector *vec)
{
  if (mvn_) {
    for (int i = 0; i < variances2_->N; i++) {
      if (variances2_->data[i] == 0.0)
        vec->data[i] = 0.0;
      else
        vec->data[i] = (vec->data[i] - means2_->data[i])
          / variances2_->data[i];
      if (symm_subtract_clip_to_zero_ && vec->data[i] < 0)
        vec->data[i] = 0.0;
    }
  } else {
    if (mean_type_ == MEANTYPE_ENORM) {
      for (int i = 0; i < means2_->N; i++) {
        vec->data[i] -= means2_->data[i] - (FLOAT_DMEM)1.0;
      }
    } else if (mean_type_ == MEANTYPE_AMEAN) {
      for (int i = 0; i < means2_->N; i++) {
        vec->data[i] -= means2_->data[i];
        if (symm_subtract_clip_to_zero_ && vec->data[i] < 0) vec->data[i] = 0.0;
      }
    } else if (mean_type_ == MEANTYPE_RQMEAN || mean_type_ == MEANTYPE_ABSMEAN) {
      if (symm_subtract_) {
        for (int i = 0; i < means2_->N; i++) {
          if (vec->data[i] >= 0) {
            vec->data[i] -= means2_->data[i];
          } else {
            vec->data[i] += means2_->data[i];
          }
        }
      } else if (symm_subtract_clip_to_zero_) {
        for (int i = 0; i < means2_->N; i++) {
          if (vec->data[i] >= means2_->data[i]) {
            vec->data[i] -= means2_->data[i];
          } else {
            if (vec->data[i] <= -means2_->data[i]) {
              vec->data[i] += means2_->data[i];
            } else {
              vec->data[i] = 0.0;
            }
          }
        }
      } else {
        for (int i = 0; i < means2_->N; i++) {
          vec->data[i] -= means2_->data[i];
        }
      }
    }
  }
}

int cFullinputMean::finaliseMeans()
{
  if (mean_type_ == MEANTYPE_AMEAN && exclude_zeros_) {
    for (int i = 0; i < means_->N; i++) {
      if (n_means_arr_[i] > 0) {
        means_->data[i] /= (FLOAT_DMEM)n_means_arr_[i];
      }
    }
    if (print_means_) {
      for (int i = 0; i < means_->N; i++) {
        SMILE_PRINT("means[%i] = %f  (n = %ld)", i, means_->data[i],
            n_means_arr_[i]);
      }
    }
  } else if (mean_type_ != MEANTYPE_ENORM) {
    if (n_means_ > 0) {
      FLOAT_DMEM nM = (FLOAT_DMEM)n_means_;
      for (int i = 0; i < means_->N; i++) {
        means_->data[i] /= nM;
      }
      if (print_means_) {
        for (int i = 0; i < means_->N; i++) {
          SMILE_PRINT("means[%i] = %f  (n = %ld)", i, means_->data[i], n_means_);
        }
      }
    }
  }
  // copy means_ to means2_
  if (means2_ != NULL) {
    delete means2_;
  }
  if (n_means2_arr_ != NULL) {
    delete[] n_means2_arr_;
  }
  // swap
  means2_ = means_;
  means_ = NULL;
  n_means2_arr_ = n_means_arr_;
  n_means_arr_ = NULL;
  n_means2_ = n_means_;
  n_means_ = 0;
  return n_means2_;
}

int cFullinputMean::finaliseVariances() {
  if (exclude_zeros_) {
    for (int i = 0; i < variances_->N; i++) {
      if (n_variances_arr_[i] > 0) {
        // computes standard deviations
        variances_->data[i] = sqrt(variances_->data[i]
          / (FLOAT_DMEM)n_variances_arr_[i]);
        if (n_variances_arr_[i] != n_means2_arr_[i])
          SMILE_IWRN(2, "n_variances (%ld) != n_means2_ (%ld)_",
              n_variances_arr_[i], n_means2_arr_[i]);
      }
    }
    if (print_variances_) {
      for (int i = 0; i < variances_->N; i++) {
        SMILE_PRINT("variances[%i] = %f  (n = %ld)", i,
            variances_->data[i], n_variances_arr_[i]);
      }
    }
  } else {
    FLOAT_DMEM N = (FLOAT_DMEM)n_variances_;
    if (n_variances_ != n_means2_)
      SMILE_IWRN(2, "n_variances (%ld) != n_means2_ (%ld)_", n_variances_,
          n_means2_);
    if (N > 0.0) {
      for (int i = 0; i < variances_->N; i++) {
        // computes standard deviations
        variances_->data[i] = sqrt(variances_->data[i] / N);
      }
    }
    if (print_variances_) {
      for (int i = 0; i < variances_->N; i++) {
        SMILE_PRINT("variances[%i] = %f  (n = %ld)", i,
            variances_->data[i], n_variances_);
      }
    }
  }
  if (variances2_ != NULL) {
    delete variances2_;
  }
  if (n_variances2_arr_ != NULL) {
    delete n_variances2_arr_;
  }
  // swap
  variances2_ = variances_;
  variances_ = NULL;
  n_variances2_arr_ = n_variances_arr_;
  n_variances_arr_ = NULL;
  n_variances2_ = n_variances_;
  n_variances_ = 0;
  return n_variances2_;
}

int cFullinputMean::doVarianceComputation()
{
  cVector * vec = reader_->getFrame(reader_pointer3_);
  if (vec != NULL) {
    if (variances_ == NULL) {
      variances_ = new cVector(vec->N);
      if (exclude_zeros_) {
        for (int i = 0; i < vec->N; i++) {
          if (vec->data[i] != 0.0) {
            FLOAT_DMEM tmp = (vec->data[i] - means2_->data[i]);
            variances_->data[i] = tmp * tmp;
            n_variances_arr_[i] = 1;
          }
        }
      } else {
        for (int i = 0; i < vec->N; i++) {
          FLOAT_DMEM tmp = (vec->data[i] - means2_->data[i]);
          variances_->data[i] = tmp * tmp;
        }
      }
      n_variances_ = 1;
    } else {
      // TODO: make sure we can leave our read pointer (in the data memory) behind to ensure that the data does not get overwritten!!
      // NOTE: the easiest way to do this is to add a second data reader...
      //       the best way is to add multi read pointer functionality to the dataReader class
      if (exclude_zeros_) {
        for (int i = 0; i < vec->N; i++) {
          if (vec->data[i] != 0.0) {
            FLOAT_DMEM tmp = (vec->data[i] - means2_->data[i]);
            variances_->data[i] += tmp * tmp;
            n_variances_arr_[i]++;
          }
        }
      } else {
        for (int i = 0; i < vec->N; i++) {
          FLOAT_DMEM tmp = (vec->data[i] - means2_->data[i]);
          variances_->data[i] += tmp * tmp;
        }
      }
      n_variances_++;
    }
    reader_pointer3_++;
    return 1;
  }
  return 0;
}

eTickResult cFullinputMean::doMeanSubtract()
{
  if (!writer_->checkWrite(1))
    return TICK_DEST_NO_SPACE;
  cVector * vec = reader_->getFrame(reader_pointer2_);
  if (vec != NULL) {
    // TODO: make sure we can leave our read pointer (in the data memory) behind to ensure that the data does not get overwritten!!
    // NOTE: the easiest way to do this is to add a second data reader...
    //       the best way is to add multi read pointer functionality to the dataReader class
    meanSubtract(vec);
    writer_->setNextFrame(vec);
    reader_pointer2_++;
    return TICK_SUCCESS;
  } else {
    // TODO: find out if the frame is lost, if so, skip up to next readable frame and print error message
    return TICK_SOURCE_NOT_AVAIL;
  }
}

eTickResult cFullinputMean::myTick(long long t)
{
  long i;
  if (multi_loop_mode_) {
    if (isEOI()) {
    // EOI MODE
      if (!multi_loop_mode_) {
        // in non-multiloop mode, du mean subtraction here for backwards compatibility...
        if (eoi_flag_ == 0) {
          finaliseMeans();
          eoi_flag_ = 1;
        }
        if (n_means2_ > 0) {
          return doMeanSubtract();
        } else {
          SMILE_IERR(1, "No mean data available, no input was read in the first tick loop.");
          return TICK_INACTIVE;
        }
      } else {
        eoi_flag_ = 1;
        // read the remaining parts of the old data until no data can be read...
        // e.g. delta features etc.
        return readNewData();
      }
    } else {
    // NON EOI MODE
      eTickResult ret = TICK_INACTIVE;
      if (eoi_flag_) {
        if (eoi_flag_ == 1) {
          // the real end of the data input
          // normalise means_
          finaliseMeans();
          // set reader0 pointer...
          reader_pointer2_ = reader_pointer_;
          reader_pointer3_ = reader_pointer_;
          if (mvn_) {
            eoi_flag_ = 3;
            first_frame_ = 0;
          } else {
            eoi_flag_ = 2;
            first_frame_ = 1;
          }
        }
        if (eoi_flag_ == 3) {  // variance computation
          if (doVarianceComputation() == 0) {
            finaliseVariances();
            eoi_flag_ = 2;
            first_frame_ = 1;
          } else {
            ret = TICK_SUCCESS;
          }
        } else if (eoi_flag_ == 2) {
          // we've got data to process (mean subtract, etc., use means2_ here)
          if (means2_ != NULL) {
            ret = doMeanSubtract();
          }
        }
      }
      if (first_frame_) {
        // the current read pointer, save it for later, when we need to extract means starting here
        reader_pointer_ = reader_->getCurR();
        first_frame_ = 0;
      }
      eTickResult ret2 = readNewData();
      // return TICK_SUCCESS only if at least one of new or old data frames has been read.
      if (ret == TICK_SUCCESS || ret2 == TICK_SUCCESS)
        return TICK_SUCCESS;
      else if (ret == TICK_DEST_NO_SPACE || ret2 == TICK_DEST_NO_SPACE)
        return TICK_DEST_NO_SPACE;
      else if (ret == TICK_SOURCE_NOT_AVAIL || ret2 == TICK_SOURCE_NOT_AVAIL)
        return TICK_SOURCE_NOT_AVAIL;
      else
        return TICK_INACTIVE;
    }
  } else {  // old code for compatibility...
    // TODO: remove this code and test if the above will do the same...
    // TOOD: this code does not support MVN, migrate to the code above...

    if (isEOI()) {
      if (means_ == NULL) {
        SMILE_IWRN(1,"sequence too short, cannot compute statistics (mean or max value)!");
        long N = reader_->getLevelN();
        means_ = new cVector( N );
        for (i=0; i<N; i++) {
          means_->data[i] = 0;
        }
        n_means_ = 1;
      }

      if (!writer_->checkWrite(1))
        return TICK_DEST_NO_SPACE;

      if (flag_==0) { 
        reader_->setCurR(0); flag_ = 1; 
        if (mean_type_ != MEANTYPE_ENORM) {
          FLOAT_DMEM nM = (FLOAT_DMEM)n_means_;
          if (nM <= 0.0) nM = 1.0;
          for (i=0; i<means_->N; i++) {
            means_->data[i] /= nM;
          }
        }
      }
      cVector *vec = reader_->getNextFrame();
      if (vec== NULL) return TICK_SOURCE_NOT_AVAIL;
      if (mean_type_ == MEANTYPE_ENORM) {
        for (i=0; i<means_->N; i++) {
          vec->data[i] -= means_->data[i] - (FLOAT_DMEM)1.0;  // x - max + 1 
        }
      } else {
        for (i=0; i<means_->N; i++) {
          vec->data[i] -= means_->data[i];
        }
      }
      writer_->setNextFrame(vec);
      return TICK_SUCCESS;      
    } else {
      // compute means, do not write data
      cVector *vec = reader_->getNextFrame();
      if (vec == NULL) return TICK_SOURCE_NOT_AVAIL;

      if (means_ == NULL) {
        means_ = new cVector( vec->N );
        for (i=0; i<vec->N; i++) {
          means_->data[i] = vec->data[i];
        }
        n_means_ = 1;
      } else {
        if (mean_type_ == MEANTYPE_ENORM) {
          for (i=0; i<vec->N; i++) {
            if (vec->data[i] > means_->data[i]) means_->data[i] = vec->data[i];
          }
        } else {
          for (i=0; i<vec->N; i++) {
            means_->data[i] += vec->data[i];
          }
          n_means_++;
        }
      }
      return TICK_SUCCESS;
    }
  }
}


cFullinputMean::~cFullinputMean()
{
  if (means_ != NULL)
    delete means_;
  if (means2_ != NULL)
    delete means2_;
  if (variances_ != NULL)
    delete variances_;
  if (variances2_ != NULL)
    delete variances2_;
  if (n_means_arr_ != NULL)
    delete n_means_arr_;
  if (n_means2_arr_ != NULL)
    delete n_means2_arr_;
  if (n_variances_arr_ != NULL)
    delete n_variances_arr_;
  if (n_variances2_arr_ != NULL)
    delete n_variances2_arr_;
}

