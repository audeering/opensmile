/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


/*  openSMILE component: cCens

Computes CENS features from Chroma features.

*/


#include <lld/cens.hpp>

#define MODULE "cCens"

/*Library:
sComponentInfo * registerMe(cConfigManager *_confman) {
  cDataProcessor::registerComponent(_confman);
}
*/

SMILECOMPONENT_STATICS(cCens)

SMILECOMPONENT_REGCOMP(cCens)
{
  if (_confman == NULL) return NULL;
  int rA = 0;

  scname = COMPONENT_NAME_CCENS;
  sdescription = COMPONENT_DESCRIPTION_CCENS;


  // we inherit cDataSource configType and extend it:
  SMILECOMPONENT_INHERIT_CONFIGTYPE("cVectorProcessor")
  
  SMILECOMPONENT_IFNOTREGAGAIN(
    ct->setField("nameAppend",NULL,"CENS");
    ct->setField("copyInputName",NULL,0);
    ct->setField("window","The window function to use for temporal CENS smoothing; one of these: han (Hanning), ham (Hamming), bar (Bartlett)","han");
    ct->setField("downsampleRatio","The integer ratio at which to downsample the resulting sequence of vectors. I.e. a value of 4 will average 4 frames and output 1 CENS frame.",10);
    ct->setField("winlength","The length of the CENS smoothing window, in frames.",41);
    ct->setField("winlength_sec","The length of the CENS smoothing window, in seconds. This will be rounded upwards (ceil) to the closest length in frames. It overrides winlength, if set.",0.41,0,0);
    ct->setField("l2norm","1/0 = enable/disable normalisation of CENS vectors by their L2-norm.",1);
   ) 

  SMILECOMPONENT_MAKEINFO(cCens);
}

SMILECOMPONENT_CREATE(cCens)

//-----

cCens::cCens(const char *_name) :
  cVectorProcessor(_name),
    winf(NULL), buffer(NULL), bptr(NULL), dsidx(NULL)
{

}

void cCens::myFetchConfig()
{
  cVectorProcessor::myFetchConfig();
  
  const char *wstr = getStr("window");
  if (wstr != NULL) {
    if (!strncmp(wstr,"han",3)) {
      window = WINF_HANNING;
    } else if (!strncmp(wstr,"ham",3)) {
      window = WINF_HAMMING;
    } else if (!strncmp(wstr,"bar",3)) {
      window = WINF_BARTLETT;
    } else  {
      SMILE_IERR(1,"invalid window function '%s', see help for valid strings! (case sensitive!); defaulting to Hanning window",wstr);
      window = WINF_HANNING;
    }
  }

 
  SMILE_IDBG(2,"window shape = %i",window);

  l2norm = getInt("l2norm");
  winlength = getInt("winlength");

  downsampleRatio = getInt("downsampleRatio");
  SMILE_IDBG(2,"downsampleRatio = %i",downsampleRatio);

  if (isSet("winlength_sec")) {
    double winlength_sec = getDouble("winlength_sec");

    double period = reader_->getLevelT();
    if (period > 0) {
      winlength = (long)ceil(winlength_sec / period);
    } else {
      winlength = (long)ceil(winlength_sec);
    }
  }

  if (downsampleRatio < 1) downsampleRatio=1;
  if (winlength < 1) winlength=1;

}

int cCens::configureWriter(sDmLevelConfig &c)
{
  // adjust output period and effective frame length (downsampling)
  c.T = c.T * (double)downsampleRatio;
  c.lastFrameSizeSec = c.frameSizeSec;
  c.frameSizeSec = c.frameSizeSec * (double)downsampleRatio;
  return 1;
}

int cCens::setupNamesForField(int i, const char*name, long nEl)
{
  if (winf == NULL) winf = (double**)multiConfAlloc();
  if (buffer == NULL) buffer = (FLOAT_DMEM**)multiConfAlloc();
  if (bptr == NULL) bptr = multiConfAllocLong();
  if (dsidx == NULL) dsidx = multiConfAllocLong();

  if (window == WINF_HANNING) {
    winf[i] = smileDsp_winHan(winlength);
  } else if (window == WINF_HAMMING) {
    winf[i] = smileDsp_winHam(winlength);
  } else if (window == WINF_BARTLETT) {
    winf[i] = smileDsp_winBar(winlength);
  } else { // .... should not happen...
    COMP_ERR("invalid window function... (%i) !",window);
  }

  buffer[i] = (FLOAT_DMEM*)calloc(1,sizeof(FLOAT_DMEM)*(winlength+1)*nEl);
  bptr[i] = 1; dsidx[i] = 0;

  return cVectorProcessor::setupNamesForField(i,name,nEl);
}

void cCens::chromaDiscretise(const FLOAT_DMEM *in, FLOAT_DMEM *out, long N)
{
  int i;
  for (i = 0;i < N; i++) {
    if (in[i] >= 0.4) out[i] = 4.0;
    else if ( (in[i] >= 0.2)&&(in[i]<0.4) ) out[i] = 3.0;
    else if ( (in[i] >= 0.1)&&(in[i]<0.2) ) out[i] = 2.0;
    else if ( (in[i] >= 0.05)&&(in[i]<0.1) ) out[i] = 1.0;
    else out[i] = 0.0;
  }
}

// a derived class should override this method, in order to implement the actual processing
int cCens::processVector(const FLOAT_DMEM *src, FLOAT_DMEM *dst, long Nsrc, long Ndst, int idxi) // idxi=input field index
{
  // manage internal history for reasons of simplicity (we could get it from the dataMemory too, but then we would have to take care of multiple fields (i.e. this class must be a dataProcessor), and of the correct input level buffersize!
  FLOAT_DMEM *_buf = buffer[idxi];  
  double *_win = winf[idxi];  
  long bp = bptr[idxi];
  // A note on the buffer: the first frame  (index 0) is reserved for the temporary frame
  // thus, the ring buffer indicies start at frame index 1 to winlength (not winlength-1)

  int i,j;
  double sum = 0.0;
  
  // quantise chroma vector:
  chromaDiscretise(src,dst,MIN(Nsrc,Ndst));

  // store in buffer at pointer position
  for (i=0; i<MIN(Nsrc,Ndst); i++) {
    _buf[bp*Nsrc+i] = dst[i];
  }
  
  // manage ring-buffer index
  bptr[idxi]++;
  if (bptr[idxi] > winlength) bptr[idxi] = 1;
  //bptr[idxi] = (bptr[idxi]+1)%winlength;
  
  // downsample
  if ((dsidx[idxi]%downsampleRatio)==0) {
    // convole with window ...
    for (i = 0; i < MIN(Nsrc,Ndst); i++) {
      _buf[i] = 0.0;
      for (j = 0; j < winlength; j++) {
        long bpj = bp-j;
        if (bpj < 1) bpj+=winlength;
        _buf[i] += _buf[bpj*Nsrc+i] * (FLOAT_DMEM)_win[j];
      }
    }

    // normalise with L2 norm
    if (l2norm) {
      // compute norm
      double n = 0.0;
      for (i = 0; i < MIN(Nsrc,Ndst); i++) {
        n += (double)_buf[i] * (double)_buf[i];
      }
      // normalise or assign normalised unit vector in case the norm is 0
      if (n > 0.0) {
        n = sqrt(n);
        for (i = 0; i < MIN(Nsrc,Ndst); i++) {
          dst[i] = _buf[i] / (FLOAT_DMEM)n;
        }
      } else {
        FLOAT_DMEM uv = (FLOAT_DMEM)(1.0 / sqrt((FLOAT_DMEM)(MIN(Nsrc,Ndst))));
        for (i = 0; i < MIN(Nsrc,Ndst); i++) {
          dst[i] = uv;
        }
      }

    } else {

      // save as output:
      for (i = 0; i < MIN(Nsrc,Ndst); i++) {
        dst[i] = _buf[i];
      }

    }

    return 1; // write output data
  } 
  
  return 0; // do not write output data!
}

cCens::~cCens()
{
  multiConfFree1D(bptr);
  multiConfFree1D(dsidx);
  multiConfFree(buffer);
  multiConfFree(winf);
}
