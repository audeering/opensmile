/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


/*  openSMILE component:

data windower.
takes frames from one level, applies window function and saves to other level

*/


#include <dspcore/windower.hpp>
#include <math.h>

#define MODULE "cWindower"



SMILECOMPONENT_STATICS(cWindower)

SMILECOMPONENT_REGCOMP(cWindower)
{
  SMILECOMPONENT_REGCOMP_INIT
  scname = COMPONENT_NAME_CWINDOWER;
  sdescription = COMPONENT_DESCRIPTION_CWINDOWER;

  // we inherit cVectorProcessor configType and extend it:
  SMILECOMPONENT_INHERIT_CONFIGTYPE("cVectorProcessor")
  
  SMILECOMPONENT_IFNOTREGAGAIN(
    ct->setField("gain","This option allows you to specify a scaling factor by which window function (which is by default normalised to max. 1) should be multiplied by ",1.0);
    ct->setField("offset","This specifies an offset which will be added to the samples after multiplying with the window function",0.0);
    ct->setField("xscale", "(NOT YET IMPLEMENTED!) A scale factor applied to the y-axis when computing the window. A factor of 2 will compute the window over twice the window length, and then clip it to the window, i.e. only the left half of the window function will be the actual window. In case of xscale=0.5, for example, the second half of the actual window will be zero, while the first half is the window function. Use in conjunction with 'xshift' paramter.", 1.0);
    ct->setField("xshift", "Specifies a shift of the window center to the left (negative) or right (positive) as percentage of the window length (ignoring xscale parameter). Allowed range is from -100% to +100%.", 0.0);
    ct->setField("winFunc","Window function:\n   Hann [Han] (= raised cosine window; use this, if you want to resynthesis from the spectral domain, also use 50% overlap in the framer!),\n   Hamming [Ham],\n   Rectangular [Rec],\n   Gauss [Gau],\n   Sine / Cosine [Sin],\n   Triangular [Tri],\n   Bartlett [Bar],\n   Bartlett-Hann [BaH],\n   Blackmann [Bla],\n   Blackmann-Harris [BlH],\n   Lanczos [Lac]", "Han");
    ct->setField("sigma","Standard deviation for the Gaussian window relative to half the window length. I.e. at N/2 - (N/2 * sigma) the window amplitude will be 0.5.", 0.4);
    ct->setField("alpha0","alpha0 for Blackmann(-Harris) / Bartlett-Hann windows (optional!)",0.0,0,0);
    ct->setField("alpha1","alpha1 for Blackmann(-Harris) / Bartlett-Hann windows (optional!)",0.0,0,0);
    ct->setField("alpha2","alpha2 for Blackmann(-Harris) / Bartlett-Hann windows (optional!)",0.0,0,0);
    ct->setField("alpha3","alpha3 for Blackmann-Harris window (optional!)",0.0,0,0);
    ct->setField("alpha","alpha for the Blackmann window",0.16);
    ct->setField("fade", "Fade percentage (0 - 0.5) for fading edges to 0 with a half raised cosine function (0 = no fade, default)", 0.0);
    ct->setField("squareRoot","1 = use square root of 'winFunc' as actual window function (e.g. to get a root raised cosine window).",0);
    ct->setField("saveWindowToFile", "Save window to text file (comma separated coefficients) of given name. If NULL (default) nothing will be saved.", (const char*)NULL);
  )

  SMILECOMPONENT_MAKEINFO(cWindower);
}

SMILECOMPONENT_CREATE(cWindower)

//-----

cWindower::cWindower(const char *_name) :
  cVectorProcessor(_name),
  win(NULL)
{

}

void cWindower::myFetchConfig()
{
  cVectorProcessor::myFetchConfig();

  offset=getDouble("offset");
  gain=getDouble("gain");

  const char *winF = getStr("winFunc");

  winFunc = winFuncToInt(winF);
  if (winFunc == WINF_UNKNOWN) {
    SMILE_IERR(1,"unkown window function '%s' specified in config file! setting window function to 'rectangular' (none)!",winF);
    winFunc = WINF_RECTANGLE;
  }
  
  if (winFunc == WINF_GAUSS) sigma = getDouble("sigma");
  
  if (winFunc == WINF_BLACKMAN) {
    if (isSet("alpha0") && isSet("alpha1") && isSet("alpha2")) {
      alpha0 = getDouble("alpha0");
      alpha1 = getDouble("alpha1");
      alpha2 = getDouble("alpha2");
    } else {
      alpha = getDouble("alpha");
      alpha0 = (1.0-alpha)*0.5;
      alpha1 = 0.5;
      alpha2 = alpha*0.5;
    }
  }
  if (winFunc == WINF_BLACKHARR) {
    if (isSet("alpha0")) alpha0 = getDouble("alpha0");
    else alpha0 = 0.35875;
    if (isSet("alpha1")) alpha1 = getDouble("alpha1");
    else alpha1 = 0.48829;
    if (isSet("alpha2")) alpha2 = getDouble("alpha2");
    else alpha2 = 0.14128;
    if (isSet("alpha3")) alpha3 = getDouble("alpha3");
    else alpha3 = 0.01168;
  }
  if (winFunc == WINF_BARTHANN) {
    if (isSet("alpha0")) alpha0 = getDouble("alpha0");
    else alpha0 = 0.62;
    if (isSet("alpha1")) alpha1 = getDouble("alpha1");
    else alpha1 = 0.48;
    if (isSet("alpha2")) alpha2 = getDouble("alpha2");
    else alpha2 = 0.38;
  }
  xshift = getDouble("xshift");
  xscale = getDouble("xscale");
  fade = getDouble("fade");
  if (fade > 0.5) {
    fade = 0.5;
    SMILE_IWRN(1, "'fade' option cannot be > 0.5. Setting to max. value 0.5.");
  }
  if (fade < 0) {
    fade = 0.0;
    SMILE_IWRN(1, "'fade' option cannot be < 0.0. Setting to min. value 0.0 (no fade).");
  }
  squareRoot = getInt("squareRoot");
  saveWindowToFile = getStr("saveWindowToFile");
}

struct sWindowerConfigParsed * cWindower::getWindowerConfigParsed()
{
  struct sWindowerConfigParsed * c = (struct sWindowerConfigParsed *)malloc(sizeof(struct sWindowerConfigParsed));
  c->winFunc = winFunc;
  c->squareRoot = squareRoot;
  c->offset = offset;
  c->gain = gain;
  c->sigma = sigma;
  c->alpha = alpha;
  c->alpha0 = alpha0;
  c->alpha1 = alpha1;
  c->alpha2 = alpha2;
  c->alpha3 = alpha3;
  c->win = win;
  c->frameSizeFrames = frameSizeFrames;
  return c;
}

int cWindower::myFinaliseInstance()
{
  int ret = cVectorProcessor::myFinaliseInstance();

  if (ret) {
    frameSizeFrames = reader_->getLevelN()/reader_->getLevelNf();
    precomputeWinFunc();
  }

  return ret;
}


void cWindower::precomputeWinFunc()
{
  if (!isConfigured()) return;
  long i;
  if (win != NULL) free(win);
  switch(winFunc) {
    case WINF_RECTANGLE: win = smileDsp_winRec(frameSizeFrames); break;
    case WINF_HANNING:   win = smileDsp_winHan(frameSizeFrames); break;
    case WINF_HAMMING:   win = smileDsp_winHam(frameSizeFrames); break;
    case WINF_TRIANGLE:  win = smileDsp_winTri(frameSizeFrames); break;
    case WINF_BARTLETT:  win = smileDsp_winBar(frameSizeFrames); break;
    case WINF_SINE:      win = smileDsp_winSin(frameSizeFrames); break;
    case WINF_GAUSS:     win = smileDsp_winGau(frameSizeFrames,sigma); break;
    case WINF_BLACKMAN:  win = smileDsp_winBla(frameSizeFrames,alpha0,alpha1,alpha2); break;
    case WINF_BLACKHARR: win = smileDsp_winBlH(frameSizeFrames,alpha0,alpha1,alpha2,alpha3); break;
    case WINF_BARTHANN:  win = smileDsp_winBaH(frameSizeFrames,alpha0,alpha1,alpha2); break;
    case WINF_LANCZOS:   win = smileDsp_winLac(frameSizeFrames); break;
    default: SMILE_IERR(1,"unknown window function ID (%i) !",winFunc); win=NULL;
  }
  if ((win != NULL)&&(squareRoot)) {
    for (i=0; i<frameSizeFrames; i++) {
      if (win[i] >= 0.0) win[i] = sqrt(win[i]);
      else {
        SMILE_IERR(1,"window function '%s' apparently has negative values (%f) (bug?), taking the square root of this function is not possible, please correct your config! (at current, the square root of all non-negative values is computed and negative values are converted to zeros)",getStr("winFunc"),win[i]);
        win[i] = 0.0;
      }
    }
  }
  if ((win != NULL) && (fade > 0.0)) {
    long fadeSize = (long)((double)frameSizeFrames * fade);
    for (i=0; i<fadeSize; i++) {
      double a = -0.5 * (cos(M_PI * (double)i / (double)fadeSize) - 1.0);
      win[i] *= a;
      win[frameSizeFrames - i - 1] *= a;
    }
  }
  if ((win != NULL)&&(gain!=1.0)) { // apply gain
    for (i=0; i<frameSizeFrames; i++) {
      win[i] *= gain;
    }
  }
  // apply xshift
  long shiftAbs = (long)((double)frameSizeFrames * xshift);
  if (shiftAbs < 0) {  // shift left
    shiftAbs *= -1;
    for(i = shiftAbs; i < frameSizeFrames; i++) {
      win[i - shiftAbs] = win[i];
    }
    for(i = frameSizeFrames - shiftAbs; i < frameSizeFrames; i++) {  // zero pad to the right
      win[i] = 0.0;
    }
  } else if (shiftAbs > 0) {  // shift right
    for(i = frameSizeFrames - shiftAbs - 1; i >= 0; i--) {
      win[i + shiftAbs] = win[i];
    }
    for(i = 0; i < shiftAbs; i++) {  // zero pad to the left
      win[i] = 0.0;
    }
  }
}

// a derived class should override this method, in order to implement the actual processing
int cWindower::processVector(const FLOAT_DMEM *src, FLOAT_DMEM *dst, long Nsrc, long Ndst, int idxi) // idxi=input field index
{
  if (win != NULL) {
    double *w = win;
    long n;
    for (n=0; n<Ndst; n++) { *dst = *src * (FLOAT_DMEM)(*(w++)) + (FLOAT_DMEM)offset; src++; dst++; }
  }
  return 1;
}


cWindower::~cWindower()
{
  if (win!=NULL) free(win);
}

