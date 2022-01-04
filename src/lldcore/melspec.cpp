/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/

/*  openSMILE component:

compute mel spectrum from fft magnitude

*/


#include <lldcore/melspec.hpp>

#define MODULE "cMelspec"

SMILECOMPONENT_STATICS(cMelspec)

SMILECOMPONENT_REGCOMP(cMelspec)
{
  SMILECOMPONENT_REGCOMP_INIT
  
  scname = COMPONENT_NAME_CMELSPEC;
  sdescription = COMPONENT_DESCRIPTION_CMELSPEC;

  // we inherit cVectorProcessor configType and extend it:
  SMILECOMPONENT_INHERIT_CONFIGTYPE("cVectorProcessor")
  SMILECOMPONENT_IFNOTREGAGAIN(
    //ct->setField("nameAppend", NULL, "FrqBand");
    ct->setField("nBands","The number of Mel/Bark/Semitone band filters the filterbank from 'lofreq'-'hifreq' contains.",26);
    ct->setField("lofreq","The lower cut-off frequency of the filterbank (Hz)",20.0);
    ct->setField("hifreq","The upper cut-off frequency of the filterbank (Hz)",8000.0);
    ct->setField("usePower","Set this to 1, to use the power spectrum instead of magnitude spectrum, i.e. if set this squares the input data",0);
    ct->setField("showFbank","If this is set to 1, the bandwidths and centre frequencies of the filters in the filterbank are printed to openSMILE log output (console and/or file)",0,0,0);
    ct->setField("htkcompatible","1 = enable htk compatible output (audio sample scaling -32767..+32767 instead of openSMILE's -1.0..1.0)",1);
    ct->setField("inverse","[NOT YET FULLY TESTED] 1 = compute fft magnitude spectrum from mel spectrum; Note that if this option is set, 'nBands' specifies the number of fft bands to create!",0,0,0);
    ct->setField("specScale","The frequency scale to design the critical band filterbank in (this is the scale in which the filter centre frequencies are placed equi-distant): \n  mel = Mel-frequency scale (m = 1127 ln (1+f/700))\n   bark = Bark scale approximation (Critical band rate z): z = [26.81 / (1.0 + 1960/f)] - 0.53\n   bark_schroed = Bark scale approximation due to Schroeder (1977): 6*ln( f/600 + [(f/600)^2+1]^0.5 )\n   bark_speex = Bark scale approximation as used in Speex codec package\n   semi = semi-tone scale with first note (0) = 'firstNote' (default 27.5Hz)  (s=12*log(f/firstNote)/log(2)) [experimental]\n   log = logarithmic scale with base 'logScaleBase' (default = 2)\n   lin(ear) = linear Hz scale.", "mel");
    ct->setField("bwMethod","The method to use to compute filter bandwidth:\n  lr  : use centre frequencies of left and right neighbours (standard way for mel-spectra and mfcc)\n  erb : bandwidth based on critical bandwidth approximation (ERB), choose this option for computing HFCC instead of MFCC.\n  custom: use the 'halfBwTarg' option to specify a custom effective rectangular bandwidth of the triangular filters - this bandwidth is constant for all filters and independent of the center frequency.", "lr");
    ct->setField("halfBwTarg", "If bwMethod=='custom' then this options gives the effective rectangular bandwidth of the triangular filters in the target frequency scale (default mel). If showFbank=1 the actual bandwidth in Hz for each center frequency will be printed at startup.", 1.0);
    ct->setField("logScaleBase","The base for log scales (a log base of 2.0 - the default - corresponds to an octave target scale)", 2.0,0,0);  
    ct->setField("firstNote","The first note (in Hz) for a semi-tone scale", 27.5,0,0);
    ct->setField("overrideFrameSizeSec", "In case that the original FFT frame size in seconds cannot automatically be read from the input level meta data (i.e. for average spectra in a multi-frame-size setting), use this to manually override it and force the filters to be created based on the given frame size assumption.", 0.0);
  )

  SMILECOMPONENT_MAKEINFO(cMelspec);
}

SMILECOMPONENT_CREATE(cMelspec)

//-----

cMelspec::cMelspec(const char *_name) :
  cVectorProcessor(_name),
  filterCoeffs_(NULL),  filterCfs_(NULL),
  chanMap_(NULL), nLoF_(NULL), nHiF_(NULL),
  nBands_(26),
  usePower_(0),
  htkcompatible_(1), inverse_(0),
  specScale_(SPECTSCALE_MEL)
{

}

void cMelspec::myFetchConfig()
{
  cVectorProcessor::myFetchConfig();
  
  nBands_ = getInt("nBands");
  SMILE_IDBG(2,"nBands = %i",nBands_);
  lofreq_ = (FLOAT_DMEM)getDouble("lofreq");
  SMILE_IDBG(2,"lofreq = %f",lofreq_);
  hifreq_ = (FLOAT_DMEM)getDouble("hifreq");
  SMILE_IDBG(2,"hifreq = %f",hifreq_);

  usePower_ = getInt("usePower");
  if (usePower_) { SMILE_IDBG(2,"using power spectrum"); }

  inverse_ = getInt("inverse");
  if (inverse_) { SMILE_IDBG(2,"inverse mode"); }

  const char * bwMethod = getStr("bwMethod");
  hfcc_ = 0;
  customBandwidth_ = 0;
  if (!strncasecmp(bwMethod, "erb", 3)) {
    hfcc_=1;
  } else if (!strncasecmp(bwMethod, "lr", 2)) {
    //hfcc=0;
  } else if (!strncasecmp(bwMethod, "cus", 3)) {
    customBandwidth_ = 1;
  } else {
    SMILE_IWRN(2,"unknown 'bwMethod' : %s (see help for possible values). Reverting to use 'lr' method, which is the MFCC standard method.",bwMethod);
  }
  halfBwTarg_ = getDouble("halfBwTarg");

  htkcompatible_ = getInt("htkcompatible");
  if (htkcompatible_) { SMILE_IDBG(2,"HTK compatible output is enabled"); }

  if (!htkcompatible_) {
    const char * specScaleStr = getStr("specScale");
    if (!strcasecmp(specScaleStr,"mel")) {
      specScale_ = SPECTSCALE_MEL;
    } else if (!strcasecmp(specScaleStr,"bark")) {
      specScale_ = SPECTSCALE_BARK;
    } else if (!strcasecmp(specScaleStr,"bark_speex")) {
      specScale_ = SPECTSCALE_BARK_SPEEX;
    } else if (!strcasecmp(specScaleStr,"bark_schroed")) {
      specScale_ = SPECTSCALE_BARK_SCHROED;
    } else if (!strncasecmp(specScaleStr, "semi", 4)) {
      specScale_ = SPECTSCALE_SEMITONE;
      firstNote_ = getDouble("firstNote");
    } else if (!strncasecmp(specScaleStr, "lin", 3)) {
      specScale_ = SPECTSCALE_LINEAR;
    } else if (!strncasecmp(specScaleStr, "log", 3)) {
      specScale_ = SPECTSCALE_LOG;
      logScaleBase_ = getDouble("logScaleBase");
      if ((logScaleBase_ <= 0.0)||(logScaleBase_==1.0)) {
        SMILE_IERR(1,"logScaleBase must be > 0.0 and != 1.0 ! You have set it to: %f (I will set it to 2.0 now, but you are advised to correct your configuration!)",logScaleBase_);
        logScaleBase_ = 2.0;
      }
    } else {
      SMILE_IERR(1,"unknown frequency scale '%s' (see -H for possible values), assuming 'htkmel'!",specScaleStr);
      specScale_ = SPECTSCALE_MEL;;
    }
    SMILE_IDBG(2,"specScale = '%s'",specScaleStr);
  } else {
    specScale_ = SPECTSCALE_MEL;
    customBandwidth_ = 0;
    halfBwTarg_ = 1.0;
  }

  if (specScale_ == SPECTSCALE_LOG) param_ = logScaleBase_;
  else if (specScale_ == SPECTSCALE_SEMITONE) param_ = firstNote_;
  else param_ = 0.0;
}

int cMelspec::dataProcessorCustomFinalise()
{
  // allocate for multiple configurations..
  if (filterCoeffs_ == NULL) filterCoeffs_ = (FLOAT_DMEM**)multiConfAlloc();
  if (filterCfs_ == NULL) filterCfs_ = (FLOAT_DMEM**)multiConfAlloc();
  if (chanMap_ == NULL) chanMap_ = (long**)multiConfAlloc();
  if (nLoF_ == NULL) nLoF_ = (long *)multiConfAlloc();
  if (nHiF_ == NULL) nHiF_ = (long *)multiConfAlloc();
  return cVectorProcessor::dataProcessorCustomFinalise();
}


void cMelspec::configureField(int idxi, long myN, long nOut)
{
  // compute filters:   // TODO?:: compute filters for each FIELD (however, only if fields are of different blocksize!)
  const sDmLevelConfig *c = reader_->getLevelConfig();
  double frameSizeSec;
  if (isSet("overrideFrameSizeSec")) {
    frameSizeSec = getDouble("overrideFrameSizeSec");
    SMILE_IMSG(1, "overriding input frame size sec. (%f) with manual setting due to given option overrideFrameSizeSec (%f)",
        c->frameSizeSec, frameSizeSec);
  } else {
    if (c->frameSizeSec > 0.0) {
      frameSizeSec = c->frameSizeSec;
    } else {
      if (c->lastFrameSizeSec != 0.0) {
        SMILE_IWRN(2, "Using lastFrameSizeSec [%f] (from the input level of the input level...) because frameSizeSec of the current input level is 0 (functionals or other asynchronuous data).", c->lastFrameSizeSec);
        frameSizeSec = c->lastFrameSizeSec;
      } else {
        SMILE_IERR(1, "cannot determine frame size of input spectrum! (frameSizeSec and lastFrameSizeSec are both 0). This can happen when multiple stages combine different frame sizes and/or asynchronuous processing is applied (use overrideFrameSizeSec to manually specify the correct size.)");
        COMP_ERR("aborting.");
      }
    }
  }
  computeFilters(myN, frameSizeSec, getFconf(idxi));
}

int cMelspec::setupNamesForField(int i, const char*name, long nEl)
{
  return cVectorProcessor::setupNamesForField(i,name,nBands_);
}

// MEL FUNCTIONS ----------------------------------------

// blocksize is size of fft block, _T is period of fft frames
// sampling period is reconstructed by: _T/((blocksize-1)*2)
int cMelspec::computeFilters( long blocksize, double frameSizeSec, int idxc )
{
  FLOAT_DMEM *filterCoeffs = filterCoeffs_[idxc];
  FLOAT_DMEM *filterCfs = filterCfs_[idxc];
  long * chanMap = chanMap_[idxc];
  long nBands;

  if (inverse_) {
    nBands = blocksize;
    blocksize = nBands_;
  } else {
    if (blocksize < nBands_) {
      SMILE_IERR(1,"nBands (%i) is greater than dimension of the input vector (%i)! This does not work... not computing mfcc filters!",nBands_,blocksize);
      return 0;
    }
    nBands = nBands_;
  }

  if (filterCoeffs != NULL) { free(filterCoeffs);   }
  if (chanMap != NULL) { free(chanMap);   }
  if (filterCfs != NULL) { free(filterCfs);   }

  bs_ = blocksize;
  if (hfcc_ || customBandwidth_) {  // || custom filter
    // independent coefficients for every band
    filterCoeffs = (FLOAT_DMEM*)calloc(1,sizeof(FLOAT_DMEM) * blocksize * nBands);
    // channel map is different here: start and end fft bins of each band's filter
    chanMap = (long*)malloc(sizeof(long) * nBands * 2);
  } else {
    filterCoeffs = (FLOAT_DMEM*)calloc(1,sizeof(FLOAT_DMEM) * blocksize);
    chanMap = (long*)malloc(sizeof(long) * blocksize);
  }
  filterCfs = (FLOAT_DMEM*)malloc(sizeof(FLOAT_DMEM) * (nBands+2));

  FLOAT_DMEM N = (FLOAT_DMEM) ((blocksize-1)*2);
  // TODO: use frequency info struct if available!! (see cPlp or cSpectral for examples...)
  // TODO!! use frq[] from metadata to also be able to work with non-linear frequency scales as input...?
  FLOAT_DMEM F0 = (FLOAT_DMEM)(1.0/frameSizeSec);
  FLOAT_DMEM Fs = (FLOAT_DMEM)(N/frameSizeSec);
  FLOAT_DMEM M = (FLOAT_DMEM)nBands;
  
  if ((lofreq_ < 0.0)||(lofreq_>Fs/2.0)||(lofreq_>hifreq_))
    lofreq_ = 0.0;
  if ((hifreq_<lofreq_)||(hifreq_>Fs/2.0)||(hifreq_<=0.0))
    hifreq_ = Fs/(FLOAT_DMEM)2.0; // Hertz(NtoFmel(blocksize+1,F0));
  FLOAT_DMEM LoF = (FLOAT_DMEM)smileDsp_specScaleTransfFwd(lofreq_,
      specScale_, param_);  // Lo Cutoff Freq (mel)
  FLOAT_DMEM HiF = (FLOAT_DMEM)smileDsp_specScaleTransfFwd(hifreq_,
      specScale_, param_);  // Hi Cutoff Freq (mel)
  nLoF_[idxc] = FtoN(lofreq_,F0);  // Lo Cutoff Freq (fft bin)
  nHiF_[idxc] = FtoN(hifreq_,F0);  // Hi Cutoff Freq (fft bin)
  if (nLoF_[idxc] > blocksize) nLoF_[idxc] = blocksize;
  if (nHiF_[idxc] > blocksize) nHiF_[idxc] = blocksize;
  if (nLoF_[idxc] < 0) nLoF_[idxc] = 0; //  exclude DC component??
  if (nHiF_[idxc] < 0) nHiF_[idxc] = 0;

  // TODO: option for fully flexible filter bank: n (cf, bw) pairs
  int showFbank = getInt("showFbank");
  if (hfcc_) {
    // custom bandwidth hfcc bank:

    // centre frequencies (similar to standard mfcc, but still a bit different for first and last filter)
    double B,C,b,c,a, bh, ch, ah;
    int m,n;

    // Moore and Glasberg's ERB coefficients for Hz scaling
    a = 0.00000623;
    b = 0.09339;
    c = 28.52;

    // first:
    double fl1 = lofreq_;
    double fc1, fh1;

    ah = 0.5/(700.0 + fl1);
    bh = 700.0/(700.0 + fl1);
    ch = -fl1/2.0 * (1.0 + 700.0/(700.0+fl1));

    B = (b-bh)/(a-ah);
    C = (c-ch)/(a-ah);
    fc1 = 0.5*(-B+sqrt(B*B - 4*C));
    fh1 = 2.0 * (a*fc1*fc1 + b*fc1 + c) + fl1; 

    // last:
    double fhN = hifreq_;
    double fcN, flN;

    ah = -0.5/(700.0 + fhN);
    bh = -700.0/(700.0 + fhN);
    ch = fhN/2.0 * (1.0 + 700.0/(700.0+fhN));

    B = (b-bh)/(a-ah);
    C = (c-ch)/(a-ah);
    fcN = 0.5*(-B+sqrt(B*B - 4*C));
    flN = -2.0 * (a*fcN*fcN + b*fcN + c) + fhN; 

    // equidistant spacing on mel-scale from fc1 to fcN of nBands-2 filters
    double fcNmel = smileDsp_specScaleTransfFwd(fcN, specScale_, param_);
    double fc1mel = smileDsp_specScaleTransfFwd(fc1, specScale_, param_);

    FLOAT_DMEM mBandw = (FLOAT_DMEM)(fcNmel-fc1mel)/(M-(FLOAT_DMEM)1.0);
    for (m=0; m<nBands-1; m++) {
      filterCfs[m] = (FLOAT_DMEM)fc1mel + (FLOAT_DMEM)m*mBandw;
    }
    filterCfs[m] = (FLOAT_DMEM)fcNmel;
    // Fill frequency axis info struct (frame metadata)
    double *_info = (double*)malloc(sizeof(double) * nBands);
    for (int m = 1; m <= nBands; m++) {
      _info[m - 1] = (double)smileDsp_specScaleTransfInv(filterCfs[m], specScale_, param_);
    }
    writer_->setFieldInfo(-1, DATATYPE_SPECTRUM_BANDS_MAG, _info, nBands * sizeof(double));
    for (m=0; m<nBands; m++) {
      double fc = smileDsp_specScaleTransfInv(filterCfs[m], specScale_, param_);
      double ERB7 = a*fc*fc + b* fc + c + 700.0;
      double fl,fh;

      fl = -(ERB7) + sqrt((ERB7*ERB7) + fc*(fc+1400.0));
      fh = fl + 2*(ERB7-700.0);

      if (showFbank) {
        SMILE_IMSG(1,"Band %i : center = %f Hz (fl: %f , fh: %f Hz ; ERB: %f Hz)",m,fc,fl,fh,(fh-fl)/2.0);
      }

      // start and end fft bins in chanMap
      long flI = chanMap[m*2] =   FtoN((FLOAT_DMEM)fl,F0); // start
      long fcI = FtoN((FLOAT_DMEM)fc,F0);
      long fhI = chanMap[m*2+1] = FtoN((FLOAT_DMEM)fh,F0); // end

      //// triangular filters
      // rising slope
      if (fl != fc) {
        for (n=MAX(flI,0); (n<=fcI)&&(n<blocksize); n++) {
          double f = NtoF(n,F0);
          filterCoeffs[m*blocksize + n] = (FLOAT_DMEM)( (f-fl)/(fc-fl) );
        }
      } else {
        filterCoeffs[m*blocksize + MAX(fcI, 0)] = 1.0;
      }
      // falling slope
      if (fh != fc) {
        for (n=MAX(fcI,0)+1; (n<=fhI)&&(n<blocksize); n++) {
          double f = NtoF(n,F0);
          filterCoeffs[m*blocksize + n] = (FLOAT_DMEM)( (fh-f)/(fh-fc) );
        }
      }
    }
  } else if (customBandwidth_) {
    // custom bandwidth triangular filter bank:
    // first:
    double fl1 = lofreq_;
    double fc1 =  fl1;
    // last:
    double fhN = hifreq_;
    double fcN = fhN;
    // equidistant spacing on target-scale from fc1 to fcN of nBands-2 filters
    double fcNtarg = smileDsp_specScaleTransfFwd(fcN, specScale_, param_);
    double fc1targ = smileDsp_specScaleTransfFwd(fc1, specScale_, param_);
    FLOAT_DMEM mBandw = (FLOAT_DMEM)(fcNtarg-fc1targ)/(M+(FLOAT_DMEM)0.0);
    if (showFbank) {
      SMILE_IMSG(1, "Constant spacing of filters in target frequency scale = %f", mBandw);
    }
    // center frequencies
    for (int m = 0; m < nBands; m++) {
      filterCfs[m] = (FLOAT_DMEM)fc1targ + (FLOAT_DMEM)m * mBandw;
    }
    filterCfs[nBands - 1] = (FLOAT_DMEM)fcNtarg;
    // Fill frequency axis info struct (frame metadata)
    double *_info = (double*)malloc(sizeof(double) * nBands);
    for (int m = 0; m < nBands; m++) {
      _info[m] = (double)smileDsp_specScaleTransfInv(filterCfs[m], specScale_, param_);
    }
    writer_->setFieldInfo(-1, DATATYPE_SPECTRUM_BANDS_MAG, _info, nBands * sizeof(double));
    // computes filters for each band:
    for (int m = 0; m < nBands; m++) {
      double fc = smileDsp_specScaleTransfInv(filterCfs[m], specScale_, param_);
      double fl = fc - halfBwTarg_;
      double fh = fc + halfBwTarg_;
      if (fl < lofreq_) {
        fl = lofreq_;
      }
      if (fh > hifreq_) {
        fh = hifreq_;
      }
      if (showFbank) {
        SMILE_IMSG(1,"Band %i : center = %f Hz (fl: %f , fh: %f Hz ; bw/2 = %f)", m, fc, fl, fh, (fh-fl)/2.0);
      }
      // start and end fft bins in chanMap
      long flI = chanMap[m*2] =   FtoN((FLOAT_DMEM)fl, F0); // start
      long fcI = FtoN((FLOAT_DMEM)fc, F0);
      long fhI = chanMap[m*2+1] = FtoN((FLOAT_DMEM)fh, F0); // end
      //// triangular filters
      if (fl != fc) {
        for (int n=MAX(flI,0); (n<=fcI)&&(n<blocksize); n++) {
          double f = NtoF(n,F0);
          filterCoeffs[m*blocksize + n] = (FLOAT_DMEM)( (f-fl)/(fc-fl) );
        }
      } else {
        filterCoeffs[m*blocksize + MAX(fcI, 0)] = 1.0;
      }
      // falling slope
      if (fh != fc) {
        for (int n=MAX(fcI,0)+1; (n<=fhI)&&(n<blocksize); n++) {
          double f = NtoF(n,F0);
          filterCoeffs[m*blocksize + n] = (FLOAT_DMEM)( (fh-f)/(fh-fc) );
        }
      }
    }
  } else {
    // standard mel filter bank:
    int m,n;
    // compute mel center frequencies
    FLOAT_DMEM mBandw = (HiF-LoF)/(M+(FLOAT_DMEM)1.0); // half bandwidth of mel bands
    for (m=0; m<=nBands+1; m++) {
      filterCfs[m] = LoF+(FLOAT_DMEM)m*mBandw;
    }
    // Fill frequency axis info struct (frame metadata)
    if (inverse_) {
      double *_info = (double*)malloc(sizeof(double) * blocksize);
      float F0 = (float)(1.0/frameSizeSec);
      for (m=0; m < blocksize; m++) {
        _info[m] = m * F0;
      }
      writer_->setFieldInfo(-1, DATATYPE_SPECTRUM_BANDS_MAG, _info, blocksize * sizeof(double));
    } else {
      double *_info = (double*)malloc(sizeof(double) * nBands);
      for (m=1; m <= nBands; m++) {
        _info[m-1] = (double)smileDsp_specScaleTransfInv(filterCfs[m], specScale_, param_);
      }
      writer_->setFieldInfo(-1, DATATYPE_SPECTRUM_BANDS_MAG, _info, nBands * sizeof(double));
    }
    if (showFbank) {
      SMILE_IMSG(1, "Band -1 (left bound) : center = %f Hz",
          smileDsp_specScaleTransfInv(filterCfs[0], specScale_, param_));
      for (m = 1; m <= nBands; m++) {
        SMILE_IMSG(1,"Band %i : center = %f Hz ; bw/2 = %f Hz", m - 1,
            smileDsp_specScaleTransfInv(filterCfs[m], specScale_, param_),
            (smileDsp_specScaleTransfInv(filterCfs[m + 1], specScale_, param_)
                - smileDsp_specScaleTransfInv(filterCfs[m - 1], specScale_, param_)) / 2.0);
      }
      SMILE_IMSG(1, "Band %i (right bound) : center = %f Hz", nBands,
          smileDsp_specScaleTransfInv(filterCfs[nBands + 1], specScale_, param_));
    }

    // compute channel mapping table:
    m = 0;
    for (n=0; n<blocksize; n++) {
      if ( (n<=nLoF_[idxc])||(n>=nHiF_[idxc]) ) chanMap[n] = -3;
      else {
        while (filterCfs[m] < NtoFmel(n,F0)) {
          if (m>nBands) break;
          m++;
        }
        chanMap[n] = m-2;
      }
    }

    // compute filter weights (rising slope only):
    m = 0;
    FLOAT_DMEM nM;
    for (n=nLoF_[idxc];n<nHiF_[idxc];n++) {
      nM = NtoFmel(n,F0);
      while ((nM > filterCfs[m+1]) && (m<=nBands)) m++;
      filterCoeffs[n] = ( filterCfs[m+1] - nM )/(filterCfs[m+1] - filterCfs[m]);
    }

  }

  filterCoeffs_[idxc] = filterCoeffs;
  filterCfs_[idxc] = filterCfs;
  chanMap_[idxc] = chanMap;
  return 0;
}

int cMelspec::processVector(const FLOAT_DMEM *src, FLOAT_DMEM *dst, long Nsrc, long Ndst, int idxi) // idxi=input field index
{
  int m,n;
  idxi=getFconf(idxi);
  FLOAT_DMEM *_filterCoeffs = filterCoeffs_[idxi];
  FLOAT_DMEM *_filterCfs = filterCfs_[idxi];
  long *_chanMap = chanMap_[idxi];
  FLOAT_DMEM *_src = NULL;

  if (inverse_) {

    if (htkcompatible_) {
      _src = (FLOAT_DMEM*)malloc(sizeof(FLOAT_DMEM)*Nsrc);
      if (src==NULL) OUT_OF_MEMORY;

      for (m=0; m<Nsrc; m++) {
        if (usePower_) {
          _src[m] = src[m] / (FLOAT_DMEM)(32767.0*32767.0);
        } else {
          _src[m] = src[m] / (FLOAT_DMEM)32767.0;
        }
      }
      src = _src;
    }

    // inverse mel
    // do the mel filtering by multiplying with the filters and summing up
    memset(dst, 0, Ndst*sizeof(FLOAT_DMEM));

    if (hfcc_) {
      SMILE_IERR(1,"Inverse HFCC not yet implemented!!");
    } else if (customBandwidth_) {
      SMILE_IERR(1,"Inverse Filter with customBandwidth not yet implemented!!");
    } else {
      FLOAT_DMEM a;
      for (n=nLoF_[idxi]; n<MIN(Ndst,nHiF_[idxi]); n++) {
        m = _chanMap[n];
        if (m>-2) {
          if (m>-1) {
            a = src[m] * _filterCoeffs[n];
            dst[n] += a;
            if (m < Nsrc-1) {
              a = src[m+1] * ((FLOAT_DMEM)1.0 - _filterCoeffs[n]);
              dst[n] += a;
            }
          }
        }
      }
    }

    // inverse power spectrum:
    if (usePower_) {
      for (n=0; n<Ndst; n++) {
        if (dst[n] > 0.0) {
          dst[n] = sqrt(dst[n]);
        } else { dst[n] = 0.0; }
      }
    }

    if ((htkcompatible_)&&(_src!=NULL)) free((void *)_src);

  } else {
    // copy & square the fft magnitude
    if (usePower_) {
      _src = (FLOAT_DMEM*)malloc(sizeof(FLOAT_DMEM)*Nsrc);
      if (src==NULL) OUT_OF_MEMORY;
      for (n=0; n<Nsrc; n++) {
        _src[n] = src[n]*src[n];
      }
      src = _src;
    }

    // do the mel filtering by multiplying with the filters and summing up
    memset(dst, 0, Ndst*sizeof(FLOAT_DMEM));

    if (hfcc_ || customBandwidth_) {
      // TODO: perform alternate (less optimised) multiplication for custom bandwidth methods.
      for (m=0; m<nBands_; m++) {
        long n1, n2;
        n1 = _chanMap[m*2];
        n2 = _chanMap[m*2+1];
        for (n=MAX(n1,nLoF_[idxi]); (n<=n2)&&(n<nHiF_[idxi]); n++) {
          dst[m] += (FLOAT_DMEM)( (double)src[n] * (double)_filterCoeffs[m*Nsrc+n] );
        }
      }

    } else {
      double a;
      for (n=nLoF_[idxi]; n<nHiF_[idxi]; n++) {
        m = _chanMap[n];
        a = (double)src[n] * (double)_filterCoeffs[n];
        if (m>-2) {
          if (m>-1) dst[m] += (FLOAT_DMEM)a;
          if (m < nBands_-1)
            dst[m+1] += src[n] - (FLOAT_DMEM)a;
        }
      }

    }

    if ((usePower_)&&(_src!=NULL)) free((void *)_src);

    if (htkcompatible_) {
      for (m=0; m<nBands_; m++) {
        if (usePower_) {
          dst[m] *= (FLOAT_DMEM)(32767.0*32767.0);
        } else {
          dst[m] *= (FLOAT_DMEM)32767.0;
        }
        // the above is for HTK compatibility....
        // HTK does not scale the input sample values to -1 / +1
        // thus, we must multiply by the max 16bit sample value again.
      }
    }
  }
  return 1;
}

cMelspec::~cMelspec()
{
  multiConfFree(filterCoeffs_);
  multiConfFree(chanMap_);
  multiConfFree(filterCfs_);
  if (nLoF_ != NULL)
    free(nLoF_);
  if (nHiF_ != NULL) 
    free(nHiF_);
}

