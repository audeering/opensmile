/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/

/*

Smile Util:

* modular DSP functions
* math functions
* sort algoritms
* wave file I/O functions
* vector file I/O functions (for dumping debug data)

... and other utility functions 

*/
//------ remove this include if you are using smileUtil as standalone code...
//#include <exceptions.hpp>
//--------------------

#include <smileutil/smileUtil.h>
#include <string.h>

//#include <smileTypes.hpp>


/*******************************************************************************************
 ***********************=====   Misc functions   ===== **************************************
 *******************************************************************************************/

int smileUtil_stripline(char ** _line)
{
  char * line;
  int len;

  if (_line != NULL) line = *_line;
  else return 0;

  len=(int)strlen(line);
  // strip newline characters at end of line
  if (len>0) { if (line[len-1] == '\n') { line[len-1] = 0; len--; } }
  if (len>0) { if (line[len-1] == '\r') { line[len-1] = 0; len--; } }
  // strip whitespaces at beginning and end of line
  while (((line[0] == ' ')||(line[0] == '\t'))&&(len>=0)) { line[0] = 0; line++; len--; }
  while ((len>=0)&&((line[len-1] == ' ')||(line[len-1] == '\t'))) { line[len-1] = 0; len--; }
  
  *_line = line; // <- this is a new pointer, so caller must save old pointer for a call to free!!
  return len; 
}


/*******************************************************************************************
 ***********************=====   Sort functions   ===== **************************************
 *******************************************************************************************/

/** inplace quicksort algorithms **/

/* QuickSort algorithm for a float array with nEl elements */
void smileUtil_quickSort_float(float *arr, long nEl)
{
  #ifdef MAX_LEVELS
  #undef MAX_LEVELS
  #endif
  #define  MAX_LEVELS  300

  float piv;
  long beg[MAX_LEVELS], end[MAX_LEVELS],swap;
  long i=0, L, R;

  beg[0]=0; end[0]=nEl;
  while (i>=0) {
    L=beg[i]; R=end[i]-1;
    if (L<R) {
      piv=arr[L]; 
      while (L<R) {
        while (arr[R]>=piv && L<R) R--; if (L<R) arr[L++]=arr[R];
        while (arr[L]<=piv && L<R) L++; if (L<R) arr[R--]=arr[L]; }
      arr[L]=piv; beg[i+1]=L+1; end[i+1]=end[i]; end[i++]=L;
      if (end[i]-beg[i]>end[i-1]-beg[i-1]) {
        swap=beg[i]; beg[i]=beg[i-1]; beg[i-1]=swap;
        swap=end[i]; end[i]=end[i-1]; end[i-1]=swap;
      }
    } else { i--; }
  }
}

/* QuickSort algorithm for a double array with nEl elements */
void smileUtil_quickSort_double(double *arr, long nEl)
{
  #ifndef MAX_LEVELS
  #define MAX_LEVELS  300
  #endif

  double piv;
  long beg[MAX_LEVELS], end[MAX_LEVELS],swap;
  long i=0, L, R;
  
  beg[0]=0; end[0]=nEl;
  while (i>=0) {
    L=beg[i]; R=end[i]-1;
    if (L<R) {
      piv=arr[L];
      while (L<R) {
        while (arr[R]>=piv && L<R) R--; if (L<R) arr[L++]=arr[R];
        while (arr[L]<=piv && L<R) L++; if (L<R) arr[R--]=arr[L]; }
      arr[L]=piv; beg[i+1]=L+1; end[i+1]=end[i]; end[i++]=L;
      if (end[i]-beg[i]>end[i-1]-beg[i-1]) {
        swap=beg[i]; beg[i]=beg[i-1]; beg[i-1]=swap;
        swap=end[i]; end[i]=end[i-1]; end[i-1]=swap;
      }
    } else { i--; }
  }
}

/* QuickSort algorithm for a FLOAT_DMEM array with nEl elements */
void smileUtil_quickSort_FLOATDMEM(FLOAT_DMEM *arr, long nEl)
{
  #ifndef MAX_LEVELS
  #define MAX_LEVELS  300
  #endif

  FLOAT_DMEM piv;
  long beg[MAX_LEVELS], end[MAX_LEVELS],swap;
  long i=0, L, R;
  
  beg[0]=0; end[0]=nEl;
  while (i>=0) {
    L=beg[i]; R=end[i]-1;
    if (L<R) {
      piv=arr[L];
      while (L<R) {
        while (arr[R]>=piv && L<R) R--; if (L<R) arr[L++]=arr[R];
        while (arr[L]<=piv && L<R) L++; if (L<R) arr[R--]=arr[L]; }
      arr[L]=piv; beg[i+1]=L+1; end[i+1]=end[i]; end[i++]=L;
      if (end[i]-beg[i]>end[i-1]-beg[i-1]) {
        swap=beg[i]; beg[i]=beg[i-1]; beg[i-1]=swap;
        swap=end[i]; end[i]=end[i-1]; end[i-1]=swap;
      }
    } else { i--; }
  }
}

/* Reverse the order in an array of elements, i.e. swap first and last element, etc. */
void smileUtil_reverseOrder_FLOATDMEM(FLOAT_DMEM *arr, long nEl)
{
  long i; int range;
  FLOAT_DMEM tmp;
  if (nEl % 2 == 0) {
    range = nEl>>1;
  } else {
    range = (nEl-1)>>1;
  }
  for (i=0; i<nEl>>1; i++) {
    tmp = arr[i];
    arr[i] = arr[nEl-i-1];
    arr[nEl-i-1] = tmp;
  }
}


/*******************************************************************************************
 ***********************=====   Filter functions   ===== **************************************
 *******************************************************************************************/



/* allocate workspace (history matrix) for a temporal median filter */
FLOAT_DMEM * smileUtil_temporalMedianFilterInit(long N, long T)
{
  FLOAT_DMEM *ws = (FLOAT_DMEM*)calloc(1,sizeof(FLOAT_DMEM)*(N*(T+1)+2+T));
  // NOTE: first two floats of workspace are N and T
  if (ws != NULL) {
    ws[0] = (FLOAT_DMEM)N;
    ws[1] = (FLOAT_DMEM)T;
  }
  return ws;
}



/* allocate workspace (history matrix) for a temporal median filter */
FLOAT_DMEM * smileUtil_temporalMedianFilterInitSl(long N, long Ns, long T)
{
  FLOAT_DMEM *ws = (FLOAT_DMEM*)calloc(1,sizeof(FLOAT_DMEM)*(N*(Ns+1)*(T+1)+2+2*T));
  // NOTE: first two floats of workspace are N and T
  if (ws != NULL) {
    ws[0] = (FLOAT_DMEM)(N*(Ns+1));
    ws[1] = (FLOAT_DMEM)T;
  }
  return ws;
}

/* free the temporal median filter workspace and return NULL */
FLOAT_DMEM * smileUtil_temporalMedianFilterFree(FLOAT_DMEM *workspace)
{
  if (workspace != NULL) free(workspace);
  return NULL;
}

/*
  Perform median filter of each element in frame x (over time) using a history matrix given in *workspace
  The workspace must be created with smileUtil_temporalMedianFilterInit.
  workspace : ptr el0 el0 el0(t-1)... el0(t) ; ptr el1 el1 el1(t-1) ... el1(t)
*/
void smileUtil_temporalMedianFilter(FLOAT_DMEM *x, long N, FLOAT_DMEM *workspace)
{
  long i;
  long _N;
  long Nw;
  long T, T1;
  FLOAT_DMEM *ws;

  if (workspace == NULL) return;
  if (N<=0) return;
  
  
  // check for matching N and find minimal _N we will work with
  Nw = (long)workspace[0];
  if (Nw > N) _N = N;
  else _N = Nw;
  T = (long)workspace[1];
  T1 = T+1;

  ws = workspace + Nw*(T+1)+2;

  for (i=0; i<_N; i++) { // apply median filter to each element 0.._N-1
    long ws0 = i*T1+2;

    // add new element to history
    long ptr = (long)(workspace[ws0])+1;
    workspace[ws0+(ptr++)] = x[i];
    if (ptr > T) ptr = 1;
    workspace[ws0] = (FLOAT_DMEM)(ptr-1);

    // compute median and save in vector x
    x[i] = smileMath_median(&(workspace[ws0+1]), T, ws);
  }
}

/*
  Perform median filter of each element in frame x (over time) using a history matrix given in *workspace
  The workspace must be created with smileUtil_temporalMedianFilterInit.
  workspace : ptr el0 el0 el0(t-1)... el0(t) ; ptr el1 el1 el1(t-1) ... el1(t)
  **> Filter with slave data (Ns is number of slave elements for each element in x (total size of x thus is N*Ns))
      The workspace must be allocated for N*(Ns+1) elements!
*/
void smileUtil_temporalMedianFilterWslave(FLOAT_DMEM *x, long N, long Ns, FLOAT_DMEM *workspace)
{
  long i,j;
  long _N;
  long Nw;
  long T, T1;
  FLOAT_DMEM *ws;

  if (workspace == NULL) return;
  if (N<=0) return;
  
  
  // check for matching N and find minimal _N we will work with
  Nw = (long)workspace[0];
  if (Nw > N) _N = N;
  else _N = Nw;
  T = (long)workspace[1];
  T1 = T+1;

  ws = workspace + Nw*(T+1)+2;

  for (i=0; i<_N; i++) { // apply median filter to each element 0.._N-1
    long ws0 = i*T1+2;

    // add new element to history
    long ptr = (long)(workspace[ws0])+1;
    workspace[ws0+(ptr++)] = x[i];
    if (ptr > T) ptr = 1;
    workspace[ws0] = (FLOAT_DMEM)(ptr-1);

    // add slave elements to history
    if (Nw >= N*(Ns+1)) {
      for (j=1; j<=Ns; j++) {
        long ws0 = (i+j*Nw/(Ns+1))*T1+2;
        long ptr = (long)(workspace[ws0])+1;
        workspace[ws0+(ptr++)] = x[i+j*N];
        if (ptr > T) ptr = 1;
        workspace[ws0] = (FLOAT_DMEM)(ptr-1);
      }
    }

    // compute median and save in vector x
    x[i] = smileMath_medianOrdered(&(workspace[ws0+1]), T, ws);

    // use indicies in ws to sort slave data (if workspace is large enough)
    if (Nw >= N*Ns) {
      for (j=1; j<=Ns; j++) {
        if (T&1) { // odd
          long ws0 = (i+j*Nw/(Ns+1))*T1+2;
          long ptr = (long)(workspace[ws0]+(FLOAT_DMEM)1.0-ws[0]);
          if (ptr < 1) ptr += T;
          x[i+j*N] = workspace[ws0+ptr]; 
        } else { // even
          long ws0 = (i+j*Nw/(Ns+1))*T1+2;
          long ptr0 = (long)(workspace[ws0]+(FLOAT_DMEM)1.0-ws[0]);
          long ptr1 = (long)(workspace[ws0]+(FLOAT_DMEM)1.0-ws[1]);
          if (ptr0 < 1) ptr0 += T;
          if (ptr1 < 1) ptr1 += T;
          x[i+j*N] = (FLOAT_DMEM)0.5 * (workspace[ws0+ptr0] + workspace[ws0+ptr1]);
        }
      }
    }
  }
}

/*******************************************************************************************
 ***********************=====   FIR filters   ===== ****************************************
 *******************************************************************************************/

// FIR impulse responses are clipped to a finite length N
// This function fades out the impulse response linearly in the
// left and right border regions (10% of the full length at each side)
// NOTE: the fadeout function breaks normalisation!
void smileDsp_impulse_response_linearFadeout(sSmileDspImpulseResponse *ir) {
  if (ir != NULL) {
    int N = ir->N / 10;
    int i;
    for (i = 0; i < N; i++) {
      ir->h[i] *= ((float)i/(float)N);
      ir->h[N - 1 - i] *= ((float)i/(float)N);
    }
  }
}

// FIR impulse responses are clipped to a finite length N
// This function fades out the impulse response linearly in the
// left and right border regions (10% of the full length at each side)
// NOTE: the fadeout function breaks normalisation!
// Use normalise = true to normalise the output.
void smileDsp_impulse_response_gaussFadeout(sSmileDspImpulseResponse *ir,
    //float sigma=0.5, bool normalise=false) {
    float sigma, bool normalise) {
  if (sigma < 0.000001) {
    sigma = 0.000001;
  }
  if (ir != NULL) {
    float Nf = (float)ir->N;
    float Nf2 = Nf/2.0;
    float sigmaNf2 = sigma * Nf2;
    FLOAT_DMEM *h = ir->h;
    float mine = (-Nf2 + 1.0)/sigmaNf2;
    float ming = exp(-0.5 * mine * mine);
    float scale = 1.0/(1.0 - ming);
    float sum = 0.0;
    float n;
    for (n = -Nf2 + 1.0; n < Nf2; n += 1.0) {
      float e = n/(sigmaNf2);
      float g = scale * (exp( -0.5 * e * e) - ming);
      *h = *h * g;
      if (normalise) {
        sum += *h;
      }
      h++;
    }
    if (normalise) {
      int i;
      for (i = 0; i < ir->N; i++) {
        ir->h[i] /= sum;
      }
    }
  }
}

// Initialises an impulse response structure for N taps
sSmileDspImpulseResponse *smileDsp_impulse_response_init(int N, 
    sSmileDspImpulseResponse *s) {
  if (N <= 0) {
    return NULL;
  }
  if (s == NULL) {
    s = (sSmileDspImpulseResponse *)
          calloc(1, sizeof(sSmileDspImpulseResponse));
  } else {
    if (s->h != NULL) {
      free(s->h);
    }
  }
  s->offset = 0;
  s->N = N;
  s->h = (FLOAT_DMEM *)calloc(1, sizeof(FLOAT_DMEM) * N);
  return s;
}

// Normalises an impulse response, so that the convolution of a signal
// with the impulse response has the same gain/amplitude as the input signal
void smileDsp_normalise_impulse_response(sSmileDspImpulseResponse *ir) {
  float sum = 0.0;
  int i;
  for (i = 0; i < ir->N; i++) {
    sum += fabs(ir->h[i]);
  }
  for (i = 0; i < ir->N; i++) {
    ir->h[i] /= sum;
  }
}

// Sinc impulse response for a low-pass filter
// with Gaussian fadeout (fixed sigma 0.5) (if fadeout = true ; preferred!)
// If highpass is set to true, the filter is "inverted"
// in the frequency domain, i.e. it becomes a high-pass filter.
void smileDsp_sincGauss_impulse_response(sSmileDspImpulseResponse *ir,
    float cutoffHz, float Ts, float sigma, bool highpass, 
    bool fadeout) {
  // float sigma=0.5, bool highpass=false,     bool fadeout=true
  if (cutoffHz < 1.0) {
    cutoffHz = 1.0;
  }
  if (ir != NULL) {
    float a = 2.0 * cutoffHz;
    float b = a * M_PI * Ts;
    float Nf = (float)ir->N;
    float Nf2 = Nf/2.0;
    FLOAT_DMEM *h = ir->h;
    float n;
    ir->offset = ir->N/2;
    for (n = -Nf2 + 1.0; n < Nf2; n += 1.0) {
      if (n != 0.0) {
        float x = b * n;
        *(h++) = (FLOAT_DMEM)(a * sin(x) / x);
      } else {
        *(h++) = (FLOAT_DMEM)(1.0);
      }
    }
    if (fadeout) {
      smileDsp_impulse_response_gaussFadeout(ir, sigma, true);
    } else {
      smileDsp_normalise_impulse_response(ir);
    }
    if (highpass) {
      // transformation of low-pass IR to highpass IR:
      float n;
      h = ir->h;
      for (n = -Nf2 + 1.0; n < Nf2; n += 1.0) {
        *h = -(*h);
        if (n == 0.0) {
          *h += 1.0;
        }
        h++;
      }
    }
  }
}

// Creates a gammatone bandpass impulse response of length ir->N
// with center frequency fc and bandwidth bw for a sampling period of Ts
// set N in ir before calling this function!
void smileDsp_gammatone_impulse_response(sSmileDspImpulseResponse *ir,
    float fc, float bw, float Ts, float a, int order, bool fadeout) {
    //float a = 1.0, int order = 1, bool fadeout=true
  if (ir != NULL) {
    int n;
    for (n = 0; n < ir->N; n++) {
      ir->h[n] = a * Ts * pow((float)n, (float)(order - 1))
        * exp(-2.0 * M_PI * bw * n * Ts) * cos(2.0 * M_PI * fc * n * Ts);
    }
    if (fadeout) {
      smileDsp_impulse_response_gaussFadeout(ir, 0.5, true);
    } else {
      smileDsp_normalise_impulse_response(ir);
    }
  }
}

// Creates a gabor bandpass impulse response of length ir->N
// with center frequency fc and bandwidth bw for a sampling period of Ts
// set N in ir before calling this function!
void smileDsp_gabor_impulse_response(sSmileDspImpulseResponse *ir, float fc, 
    float bw, float Ts, bool fadeout) {
  if (ir != NULL) {
    float b2 = bw * sqrt(2.0 * M_PI) * Ts;
    float c = 2.0 * M_PI * fc * Ts;
    float Nf = (float)ir->N;
    FLOAT_DMEM *h = ir->h;
    float n;
    b2 = - b2 * b2;
    for (n = -Nf/2.0 + 1.0; n < Nf/2.0; n += 1.0) {
      *(h++) = exp(b2 * n * n) * cos(c * n);
    }
    ir->offset = ir->N/2;
    if (fadeout) {
      smileDsp_impulse_response_gaussFadeout(ir, 0.5, true);
    } else {
      smileDsp_normalise_impulse_response(ir);
    }
  }
}

// Initializes a block convolver work area
sSmileDspConvolverState *smileDsp_block_convolve_init(int blocksize, sSmileDspImpulseResponse *ir,
    sSmileDspConvolverState *s) {
  if (s == NULL) {
    s = (sSmileDspConvolverState *)calloc(1, sizeof(sSmileDspConvolverState));
  } else {
    if (s->x_buf != NULL) {
      free(s->x_buf);
    }
  }
  s->blocksize = blocksize;
  s->bufsize = ir->N;
  s->ir.offset = ir->offset;
  s->ir.N = ir->N;
  s->ir.h = ir->h;
  s->x_buf_ptr = 0;
  s->x_buf = (FLOAT_DMEM*)calloc(1, sizeof(FLOAT_DMEM) * s->bufsize);
  return s;
}

// Frees a block convolver work area
void smileDsp_block_convolve_destroy(sSmileDspConvolverState * s,
    bool noFreeIr, bool noFreePtr) {
  if (s != NULL) {
    if (!noFreeIr && s->ir.h != NULL) {
      free(s->ir.h);
    }
    if (s->x_buf != NULL) {
      free(s->x_buf);
    }
    if (!noFreePtr) {
      free(s);
    }
  }
}

// Performs a discrete convolution on a block of N input samples in *x.
// The output will be saved in *y.
// Using a larger blocksize will reduce the overhead in
//   a) calling this function
//   b) rotating the buffer of past samples
// N must be >= s->blocksize
// outputStep: the distance (in samples between two output samples (for matrix output))
// outputOffset: the index of the first output
void smileDsp_block_convolve(sSmileDspConvolverState *s, FLOAT_DMEM *x,
    FLOAT_DMEM *y, int N, int outputStep, int outputOffset) {
  int i;
  int n_to_copy;
  if (N < s->blocksize) {
    // error! or adapt blocksize, etc.?
    return;
  }
  y += outputOffset;
  // the actual convolution loop
  for (i = 0; i < N; i++) {
    FLOAT_DMEM *h = s->ir.h;
    int i_pt = s->x_buf_ptr + i + s->bufsize;
    int j;
    *y = 0.0;
    for (j = 0; j < s->ir.N; j++) {
      if (i >= j) {  // use signal
        *y += *h * x[i - j];
      } else {  // use past buffer
        *y += *h * s->x_buf[(i_pt - j) % s->bufsize];
      }
      h++;
    }
    y += outputStep;
  }
  // update past buffer
  if (s->bufsize < N) {
    n_to_copy = s->bufsize;
  } else {
    n_to_copy = N;
  }
  x += N - n_to_copy;
  for (i = 0; i < n_to_copy; i++) {
    s->x_buf[s->x_buf_ptr] = x[i];
    s->x_buf_ptr++;
    s->x_buf_ptr %= s->bufsize;
  }
}


/*******************************************************************************************
 ***********************=====   Math functions   ===== **************************************
 *******************************************************************************************/

static FLOAT_DMEM expLimDmem = 0.0;

FLOAT_DMEM smileMath_logistic(FLOAT_DMEM x)
{
  if (expLimDmem == 0.0)
    expLimDmem = log(FLOAT_DMEM_MAX);
  if (x > expLimDmem) { return 1.0; }
  else if (x < -expLimDmem) { return 0.0; }
  return 1.0 / (1.0 + exp(-x));
}

FLOAT_DMEM smileMath_tanh(FLOAT_DMEM x) {
  return (FLOAT_DMEM)2.0 * smileMath_logistic((FLOAT_DMEM)2.0 * x) - (FLOAT_DMEM)1.0;
}

// linear up to +- limit1, then sigmoid converging at +-limit2
FLOAT_DMEM smileMath_ratioLimit(FLOAT_DMEM x,
    FLOAT_DMEM limit1, FLOAT_DMEM excessToLimit2) {
  if (x > limit1) {
    FLOAT_DMEM y = smileMath_tanh((sqrt(x - limit1 + 1.0) - 1.0) / (excessToLimit2 * 0.5)) * excessToLimit2 + limit1;
    return y;
  } else if (x < -limit1) {
    FLOAT_DMEM y = smileMath_tanh(-(sqrt(-1.0*(x + limit1) + 1.0) - 1.0) / (excessToLimit2 * 0.5)) * excessToLimit2 - limit1;
    return y;
  }
  // linear in [-limit1 ; 0; limit1]
  return x;
}

/*
  median of vector x
  (workspace can be a pointer to an array of N FLOAT_DMEMs which is used to sort the data in x without changing x)
  (if workspace is NULL , the function will allocate and free the workspace internally)
*/
FLOAT_DMEM smileMath_median(const FLOAT_DMEM *x, long N, FLOAT_DMEM *workspace)
{
  long i;
  FLOAT_DMEM median=0.0;
  FLOAT_DMEM *tmp = workspace;
  if (tmp == NULL) tmp = (FLOAT_DMEM*)malloc(sizeof(FLOAT_DMEM)*N);
  if (tmp==NULL) return 0.0;
  for (i=0; i<N; i++) { tmp[i] = x[i]; }
  //memcpy(tmp, x, sizeof(FLOAT_DMEM)*N);
  
  smileUtil_quickSort_FLOATDMEM(tmp,N);
  if (N&1) { // easy median for odd N
    median = tmp[N>>1];
  } else { // median as mean of the two middle elements for even N
    median = (FLOAT_DMEM)0.5 * (tmp[N/2]+tmp[N/2-1]);
  }
  if (workspace == NULL) free(tmp);
  return median;
}

/*
  median of vector x
  (workspace can be a pointer to an array of 2*N (!) FLOAT_DMEMs which is used to sort the data in x without changing x)
  (if workspace is NULL , the function will allocate and free the workspace internally)
  THIS function should return the original vector index of the median in workspace[0] (and workspace[1] if N is even), to use this functionality you must provide a workspace pointer!
*/
FLOAT_DMEM smileMath_medianOrdered(const FLOAT_DMEM *x, long N, FLOAT_DMEM *workspace)
{
  long i,j;
  long oi0=0, oi1=0;
  FLOAT_DMEM median=0.0;
  FLOAT_DMEM *tmp = workspace;
  if (tmp == NULL) tmp = (FLOAT_DMEM*)malloc(sizeof(FLOAT_DMEM)*2*N);
  if (tmp==NULL) return 0.0;

  
  for (i=0; i<N; i++) { tmp[i] = x[i]; }
  //memcpy(tmp, x, sizeof(FLOAT_DMEM)*N);

  for (i=0; i<N; i++) {
    tmp[N+i] = (FLOAT_DMEM)i;
  }
  
  // we cannot use quicksort, since it doesn't preserve the original indexing
  //smileUtil_quickSort_FLOATDMEM(tmp,N);
  for (i=0; i<N; i++) {
    for (j=i+1; j<N; j++) {
      if (tmp[i] > tmp[j]) { //swap data and indicies
        FLOAT_DMEM t = tmp[i]; // swap data
        tmp[i] = tmp[j];
        tmp[j] = t;
        t = tmp[i+N]; // swap indicies
        tmp[i+N] = tmp[j+N];
        tmp[j+N] = t;
      }
    }
  }

  if (N&1) { // easy median for odd N
    median = tmp[N>>1];
    tmp[0] = tmp[N+(N>>1)];
  } else { // median as mean of the two middle elements for even N
    median = (FLOAT_DMEM)0.5 * (tmp[N>>1]+tmp[(N>>1)-1]);
    tmp[0] = tmp[N+(N>>1)-1];
    tmp[1] = tmp[N+(N>>1)];
  }
  if (workspace == NULL) free(tmp);
  return median;
}

/* check if number is power of 2 (positive or negative) */
long smileMath_isPowerOf2(long x)
{
  if (x==1) return 1;  // 1 is a power of 2
  if (((x&1) == 0)&&(x != 0)) { // only even numbers > 1
    x=x>>1;
    while ((x&1) == 0) { x=x>>1;  }
    return ((x==1)||(x==-1));
  }
  return 0;
}

/* round to nearest power of two */
long smileMath_roundToNextPowOf2(long x)
{
  // round x up to nearest power of 2
  unsigned long int flng = (unsigned long int)x;
  unsigned long int fmask = 0x8000;
  while ( (fmask & flng) == 0) { fmask = fmask >> 1; }
  // fmask now contains the MSB position
  if (fmask > 1) {
    if ( (fmask>>1)&flng ) { flng = fmask<<1; }
    else { flng = fmask; }
  } else {
    flng = 2;
  }

  return (long)flng;
}

double smileMath_log2(double x)
{
  return log(x)/log(2.0);
}

/* round up to next power of 2 */
long smileMath_ceilToNextPowOf2(long x)
{
  long y = smileMath_roundToNextPowOf2(x);
  if (y<x) y *= 2;
  return y;
}

/* round down to next power of two */
long smileMath_floorToNextPowOf2(long x)
{
  long y = smileMath_roundToNextPowOf2(x);
  if (y>x) y /= 2;
  return y;
}

FLOAT_DMEM smileMath_crossCorrelation(const FLOAT_DMEM * x, long Nx, const FLOAT_DMEM * y, long Ny)
{
  long N = MIN(Nx,Ny);
  long i;
  double cc = 0.0;
  double mx = 0.0;
  double my = 0.0;
  double nx = 0;
  double ny = 0;
  for (i = 0; i < N; i++) {
    mx += x[i];
    my += y[i];
  }
  mx /= (double)N;
  my /= (double)N;
  for (i=0; i<N; i++) {
    cc += (x[i] - mx) * (y[i] - my);
    nx += (x[i] - mx) * (x[i] - mx);
    ny += (y[i] - my) * (y[i] - my);
  }
  cc /= sqrt(nx) * sqrt(ny);
  return cc;
}

/***** vector math *******/

FLOAT_DMEM smileMath_cosineDistance(FLOAT_DMEM * a, FLOAT_DMEM * b, int N)
{
  double prod = 0.0;
  double prodA = 0.0;
  double prodB = 0.0;
  double distDn = 0.0;
  int i;
  for (i = 0; i < N; i++) {
    prod += a[i] * b[i];
    prodA += a[i] * a[i];
    prodB += b[i] * b[i];
  }
  distDn = sqrt(prodA) * sqrt(prodB);
  if (distDn > 0.0) {
    double cd = (FLOAT_DMEM)(prod / distDn);
    // converts the angular distance to an actual distance metric (smaller is lower distance)
    return 2.0 - (cd + 1.0);
  } else {
    return (FLOAT_DMEM)0.0;
  }
}

// computes angle distance in degrees (return value)
// as well as "radial" distance (difference
// of vector lengths, independent of the angle).
//
// cos(a) = scalar(x*y)/(mag(x)*mag(y));
FLOAT_DMEM smileMath_vectorAngle(FLOAT_DMEM *x, FLOAT_DMEM *y, long N,
    FLOAT_DMEM *radialDistance)
{
  long i;
  FLOAT_DMEM lx = smileMath_vectorLengthEuc(x, N);
  FLOAT_DMEM ly = smileMath_vectorLengthEuc(y, N);
  FLOAT_DMEM lxy = lx * ly;
  FLOAT_DMEM scalar = 0.0;
  for (i = 0; i < N; i++) {
    scalar += x[i] * y[i];
  }
  if (radialDistance != NULL) {
    *radialDistance = lx - ly;
  }
  if (lxy > 0.0) {
    // returns vector angle in radians -pi to +pi
    //printf("scalar: %.4f, lxy: %.4f\n", scalar, lxy);
    double cosv = scalar / lxy;
    // we need this range check to avoid nan return of acos() due
    // to rounding errors if scalar/lxy is slightly > 1 or < -1
    if (cosv > 1.0)
      cosv = 1.0;
    if (cosv < -1.0)
      cosv = -1.0;
    return acos(cosv);
  }
  return 0.0;
}

FLOAT_DMEM smileMath_vectorDistanceEuc(FLOAT_DMEM *x, FLOAT_DMEM *y, long N)
{
  long i;
  FLOAT_DMEM dist = 0.0;
  for (i = 0; i < N; i++) {
    dist += (x[i] - y[i]) * (x[i] - y[i]);
  }
  return sqrt(dist);
}

/* computes euclidean norm of given vector x */
FLOAT_DMEM smileMath_vectorLengthEuc(FLOAT_DMEM *x, long N)
{
  long i; FLOAT_DMEM norm = 0.0;
  for (i=0; i<N; i++) norm += x[i]*x[i];
  return (FLOAT_DMEM)sqrt(norm);
}

/* compute L1 norm (absolute sum) of given vector x */
FLOAT_DMEM smileMath_vectorLengthL1(FLOAT_DMEM *x, long N)
{
  long i; FLOAT_DMEM norm = 0.0;
  for (i=0; i<N; i++) norm += (FLOAT_DMEM)fabs(x[i]);
  return norm;
}

/* normalise euclidean length of x to 1 */
FLOAT_DMEM smileMath_vectorNormEuc(FLOAT_DMEM *x, long N)
{
  FLOAT_DMEM norm = smileMath_vectorLengthEuc(x,N);
  long i; 
  if (norm > 0.0) for (i=0; i<N; i++) x[i] /= norm;
  return norm;
}

/* normalise vector sum to 1 */
FLOAT_DMEM smileMath_vectorNormL1(FLOAT_DMEM *x, long N)
{
  FLOAT_DMEM norm = smileMath_vectorLengthL1(x,N);
  long i; 
  if (norm > 0.0) for (i=0; i<N; i++) x[i] /= norm;
  return norm;
}

/* normalise values of vector x to range [min - max] */
void smileMath_vectorNormMax(FLOAT_DMEM *x, long N, FLOAT_DMEM min, FLOAT_DMEM max)
{
  long i;
  FLOAT_DMEM _min=x[0];
  FLOAT_DMEM _max=x[0];
  FLOAT_DMEM scale;
  for (i=0; i<N; i++) {
    if (x[i] < _min) _min = x[i];
    else if (x[i] > _max) _max = x[i];
  }
  if (_max==_min) scale = 1.0;
  else scale = (max-min)/(_max-_min);
  for (i=0; i<N; i++) {
    x[i] = (x[i]-_min)*scale+min;
  }
}

/* returns second largest value of vector and indices of 2nd and 1st largest element */
FLOAT_DMEM smileMath_vectorMax2(FLOAT_DMEM *x, long N, long *maxPos, long *secondMaxPos) {
  long i;
  long maxI = -1;
  long max2I = -1;
  for (i = 0; i < N; i++) {
    if (x[i] > x[max2I] || max2I == -1) {
      max2I = i;
    }
    if (x[i] > x[maxI] || maxI == -1) {
      max2I = maxI;
      maxI = i;
    }
  }
  if (max2I < 0)
    max2I = 0;
  if (maxI < 0)
    maxI = 0;
  if (maxPos != NULL)
    *maxPos = maxI;
  if (secondMaxPos != NULL) 
    *secondMaxPos = max2I;
  return x[max2I];
}

FLOAT_DMEM smileMath_vectorMax(FLOAT_DMEM *x, long N, long *maxPos) {
  long i;
  long maxI = 0;
  for (i = 0; i < N; i++) {
    if (x[i] > x[maxI])
      maxI = i;
  }
  if (maxPos != NULL)
    *maxPos = maxI;
  return x[maxI];
}


/* compute the arithmetic mean of vector x */
FLOAT_DMEM smileMath_vectorAMean(FLOAT_DMEM *x, long N)
{
  long i; 
  FLOAT_DMEM sum = 0.0;
  for (i = 0; i < N; i++) 
    sum += x[i];
  return sum / (FLOAT_DMEM)N;
}

/* root of each element in a vector */
void smileMath_vectorRoot(FLOAT_DMEM *x, long N)
{
  long i;
  for (i = 0; i < N; i++) { 
    if (x[i] >= (FLOAT_DMEM)0.0) 
      x[i] = (FLOAT_DMEM)sqrt(x[i]); 
  }
}

/* root of each element in a vector */
void smileMath_vectorRootD(double *x, long N)
{
  long i;
  for (i=0; i<N; i++) { if (x[i]>=0.0) x[i]=sqrt(x[i]); }
}

/**** complex number math ****/

/* compute A/B , store in C */
void smileMath_complexDiv(double ReA, double ImA, double ReB, double ImB, double *ReC, double *ImC)
{
  double r, den;
  double R=0,I=0;

  if (fabs (ReB) >= fabs (ImB)) {
    if (ReB != 0.0) {
      r = ImB / ReB;
      den = ReB + r * ImB;
      if (den != 0.0) {
        R = (ReA + ImA*r ) / den; // R = (ReA  + r*ImA ) / den;
        I = (ImA - r*ReA) / den; // I = (ImA * ReB - r * ReA) / den;
      }
    }
  } else {
    if (ImB != 0.0) {
      r = ReB / ImB;
      den = ImB + r * ReB;
      if (den != 0.0) {
        R = (ReA * r + ImA) / den;
        I = (ImA * r - ReA) / den;
      }
    }
  }
  if (ReC != NULL) *ReC = R;
  if (ImC != NULL) *ImC = I;
}

double smileMath_complexAbs(double Re, double Im)
{
  return sqrt (Re*Re + Im*Im);
}

/* fix roots to inside the unit circle */
// ensure all roots are within the unit circle
// if abs(root) > 1  (outside circle)
// then root = 1 / root*
//
// *roots is an array of n complex numbers (2*n doubles)
void smileMath_complexIntoUnitCircle(double *roots, int n)
{
  long i;
  for (i=0; i<n; i++) {
    long i2 = i*2;
    // if abs(root) > 1.0 
    if (smileMath_complexAbs(roots[i2],roots[i2+1]) > 1.0) {
      // root = 1.0 / root*
      smileMath_complexDiv(1.0 , 0.0 , roots[i2], -roots[i2+1], &roots[i2], &roots[i2+1]);
    }
  }
}



// constructs a parabola from three points (parabolic interpolation)
// returns: peak x of parabola, and optional (if not NULL) the y value of the peak in *y and the steepness in *_a
double smileMath_quadFrom3pts(double x1, double y1, double x2, double y2, double x3, double y3, double *y, double *_a)
{
  double den = x1*x1*x2 + x2*x2*x3 + x3*x3*x1 - x3*x3*x2 - x2*x2*x1 - x1*x1*x3;
  if (den != 0.0) {
    double a = (y1*x2 + y2*x3 + y3*x1 - y3*x2 - y2*x1 - y1*x3)/den;
    double b = (x1*x1*y2 + x2*x2*y3 + x3*x3*y1 - x3*x3*y2 - x2*x2*y1 - x1*x1*y3) / den;
    double c = (x1*x1*x2*y3 + x2*x2*x3*y1 + x3*x3*x1*y2 - x3*x3*x2*y1 - x2*x2*x1*y3 - x1*x1*x3*y2) / den;
    if (a != 0.0) {
      double x;
      if (_a != NULL) *_a = a;
      x = -b/(2.0*a);
      if (y!=NULL) *y = c - a*x*x;
      return x;
    } 
  } 
      
  // fallback to peak picking if we can't construct a parabola
  if (_a!=NULL) *_a = 0.0;
  if ((y1>y2)&&(y1>y3)) { if (y!=NULL) *y = y1; return x1; }
  else if ((y2>y1)&&(y2>y3)) { if (y!=NULL) *y = y2; return x2; }
  else if ((y3>y1)&&(y3>y2)) { if (y!=NULL) *y = y3; return x3; }
  
  // fallback to keep compiler happy.. this will only happen if all input values are equal:
  if (y!=NULL) *y = y1; return x1;
}


/*******************************************************************************************
 ***********************=====   DSP functions   ===== **************************************
 *******************************************************************************************/

/* Hermansky 1990, JASA , scale corrected to max == 1 */
double smileDsp_equalLoudnessWeight(double frequency) 
{
  double w = 2.0*M_PI*frequency;
  double w2 = w*w;
  double c = w2 + 6300000.0;
  if (c > 0.0) {
  //  printf("x = %f\n",((w2 +56.8*1000000.0)*w2*w2 ) / (c*c * (w2 + 380000000.0)));
    //return 1.755e27*((w2 +56.8e6)*w2*w2 ) / (c*c * (w2 + 0.38e9)*(w2*w2*w2 + 9.58e26));
    return ( 1e32 * ((w2 +56.8e6)*w2*w2 ) / (c*c * (w2 + 0.38e9)*(w2*w2*w2*w + 1.7e31))  );
  } else {
    return 0.0;
  }
}

double smileDsp_equalLoudnessWeight_htk(double frequency) 
{
  /* HTK version of the PLP equal loudness curve:*/
  double f2 = (frequency * frequency);
  double fs = f2/(f2 + 1.6e5);
  return fs * fs * ((f2 + 1.44e6)/(f2 + 9.61e6));
}

/* frequency weighting for computing the psychoacoustic measure of spectral sharpness */
double smileDsp_getSharpnessWeightG(double frq, int frqScale, double param)
{
  if (frqScale != SPECTSCALE_BARK) {
    // transform to linear scale...
    frq = smileDsp_specScaleTransfInv(frq,frqScale,param);
    // ...then from linear to bark
    frq = smileDsp_specScaleTransfFwd(frq,SPECTSCALE_BARK,0.0);
  }
  // get Zwicker's weighting g(z):
  if (frq <= 16.0) {
    return 1.0;
  } else {
    return pow( (frq-16.0)/4.0, 1.5849625 /*log(3.0)/log(2.0)*/ ) + 1.0;
  }
}

  /*======= spectral scaling functions === */

// equivalent rectangular bandwidth (in linear frequency)
// of critical band filter at a given linear centre frequency
double smileDsp_ERB(double x)
{
  return 0.00000623*x*x + 0.09339*x + 28.52;
}

// +++ forward scale transformation function (source (linear) -> target) +++
// 'x' : is the (linear) source frequency in Hertz
// 'scale' : is one of the following:
//   SPECTSCALE_LOG, SPECTSCALE_SEMITONE, SPECTSCALE_BARK, 
//   SPECTSCALE_BARK_SCHROED, SPECTSCALE_BARK_SPEEX, SPECTSCALE_MEL
// 'param' is either:
//   logScaleBase if scale == SPECTSCALE_LOG
//   firstNote    if scale == SPECTSCALE_SEMITONE
double smileDsp_specScaleTransfFwd(double x, int scale, double param)
{
  switch(scale) {
    case SPECTSCALE_LOG: 
      if (x>0) {
        return log(x)/log(param); // param = logScaleBase
      } else { 
        return 0.0; 
      }
    case SPECTSCALE_SEMITONE:
      if (x/param>1.0) // param = firstNote
        return 12.0 * smileMath_log2(x / param); // param = firstNote
      else return 0.0;
    case SPECTSCALE_BARK_OLD:
      if (x>0) {
        return (26.81 / (1.0 + 1960.0/x)) - 0.53;
      } else { 
        return 0.0; 
      }
    case SPECTSCALE_BARK: // Bark scale according to : H. Traunm�ller (1990) "Analytical expressions for the tonotopic sensory scale" J. Acoust. Soc. Am. 88: 97-100.   
      if (x>0) {
        //OLD: return (26.81 / (1.0 + 1960.0/x)) - 0.53;
        // now: exact for high and low frequencies(!)
        double zz = (26.81 / (1.0 + 1960.0/x)) - 0.53;
        if (zz < 2) {
          return (0.85 * zz + 0.3);
        } else if (zz > 20.1) {
          return (1.22*zz - 0.22 * 20.1); 
        } else {
          return zz;
        }
      } else {
        return 0.0;
      }
    case SPECTSCALE_BARK_SCHROED:
      if (x>0) {
        double f6 = x/600.0;
        return (6.0 * log(f6 + sqrt(f6*f6 + 1.0) ) );
      } else return 0.0;
    case SPECTSCALE_BARK_SPEEX:
        return (13.1*atan(.00074*x)+2.24*atan(x*x*1.85e-8)+1e-4*x);
    case SPECTSCALE_MEL: // Mel scale according to: L.L. Beranek (1949) Acoustic Measurements, New York: Wiley. 
      if (x>0.0) {
        return 1127.0 * log(1.0 + x/700.0);
      } else return 0.0;
    case SPECTSCALE_LINEAR: 
    default:
      return x;
  }
  return x;
}

// +++ inverse scale transformation function (linear -> target) +++
// 'x' : is the (non-linear) source frequency in Bark/Mel/etc. scale ...
// 'scale' : is one of the following:
//   SPECTSCALE_LOG, SPECTSCALE_SEMITONE, SPECTSCALE_BARK, 
//   SPECTSCALE_BARK_SCHROED, SPECTSCALE_BARK_SPEEX, SPECTSCALE_MEL
// 'param' is either:
//   logScaleBase if scale == SPECTSCALE_LOG
//   firstNote    if scale == SPECTSCALE_SEMITONE
// return value: linear frequency in Hertz
double smileDsp_specScaleTransfInv(double x, int scale, double param)
{
  double zz;
  double z0;
  switch(scale) {
    case SPECTSCALE_LOG: 
      return exp(x * log(param)); // param = logScaleBase
    case SPECTSCALE_SEMITONE:
      return param * pow(2.0, x/12.0); // param = firstNote
    case SPECTSCALE_BARK_OLD: { // Bark scale according to : H. Traunm�ller (1990) "Analytical expressions for the tonotopic sensory scale" J. Acoust. Soc. Am. 88: 97-100.   
      double z0 = (x+0.53)/26.81;
      if (z0 != 1.0) return (1960.0 * z0)/(1.0-z0);
      else return 0.0;
      }
    case SPECTSCALE_BARK:
      {
      zz = x;
      if (x > 20.1) {
        zz = (x + 0.22*20.1)/1.22;
      } else if (x < 2) {
        zz = (x - 0.3)/0.85;
      }
      z0 = 26.81 / (zz + 0.53);
      if (z0 != 1.0) {
        return 1960.0 / (z0 - 1.0);
      } else { 
        return 0.0; 
      }
      }
    case SPECTSCALE_BARK_SCHROED:
      return 600.0 * sinh(x/6.0);
      //return 0.0;
    case SPECTSCALE_BARK_SPEEX:
      fprintf(stderr,"smileDsp_specScaleTransfInv: SPECTSCALE_BARK_SPEEX: inversion not yet implemented");
    case SPECTSCALE_MEL :  // Mel scale according to: L.L. Beranek (1949) Acoustic Measurements, New York: Wiley. 
      return 700.0*(exp(x/1127.0)-1.0);       
    case SPECTSCALE_LINEAR:
    default:
      return x;
  }
  return x;
}


  /*======= window functions ==========*/

/* sinc function (modified) : (sin 2pi*x) / x */
double smileDsp_lcSinc(double x)
{
  double y = M_PI * x;
  return sin(y)/(y);
}

/* sinc function : (sin x) / x  */
double smileDsp_sinc(double x)
{
  return sin(x)/(x);
}

/* rectangular window */
double * smileDsp_winRec(long _N)
{
  int i;
  double * ret = (double *)malloc(sizeof(double)*_N);
  double * x = ret;
  for (i=0; i<_N; i++) {
    *x = 1.0; x++;
  }
  return ret;
}

/* triangular window (non-zero endpoints) */
double * smileDsp_winTri(long _N)
{
  long i;
  double * ret = (double *)malloc(sizeof(double)*_N);
  double * x = ret;
  for (i=0; i<_N/2; i++) {
    *x = 2.0*(double)(i+1)/(double)_N;
    x++;
  }
  for (i=_N/2; i<_N; i++) {
    *x = 2.0*(double)(_N-i)/(double)_N;
    x++;
  }
  return ret;
}

/* powered triangular window (non-zero endpoints) */
double * smileDsp_winTrP(long N)
{
  double *w = smileDsp_winTri(N);
  double *x = w;
  long n; 
  for (n = 0; n < N; n++) {
    *x = (*x) * (*x);
    x++;
  }
  return w;
}

/* bartlett (triangular) window (zero endpoints) */
double * smileDsp_winBar(long N)
{
  long i;
  double * ret = (double *)malloc(sizeof(double) * N);
  double * x = ret;
  for (i = 0; i < N/2; i++) {
    *x = 2.0 * (double)(i) / (double)(N - 1);
    x++;
  }
  for (i= N/2; i < N; i++) {
    *x = 2.0 * (double)(N - 1 - i) / (double)(N - 1);
    x++;
  }
  return ret;
}

/* hann(ing) window */
double * smileDsp_winHan(long N)
{
  double i;
  double * ret = (double *)malloc(sizeof(double) * N);
  double * x = ret;
  double NN = (double)N;
  for (i=0.0; i<NN; i += 1.0) {
    *x = 0.5*(1.0-cos( (2.0*M_PI*i)/(NN-1.0) ));
    x++;
  }
  return ret;
}

/* hamming window */
double * smileDsp_winHam(long N)
{
  double i;
  double * ret = (double *)malloc(sizeof(double) * N);
  double * x = ret;
  double NN = (double)N;
  for (i=0.0; i<NN; i += 1.0) {
/*    *x = 0.53836 - 0.46164 * cos( (2.0*M_PI*i)/(NN-1.0) ); */
    *x = 0.54 - 0.46 * cos( (2.0*M_PI*i)/(NN-1.0) );
    x++;
  }
  return ret;
}

/* half-wave sine window (cosine window) */
double * smileDsp_winSin(long N)
{
  double i;
  double * ret = (double *)malloc(sizeof(double) * N);
  double * x = ret;
  double NN = (double)N;
  for (i=0.0; i<NN; i += 1.0) {
    *x = sin( (1.0*M_PI*i)/(NN-1.0) );
    x++;
  }
  return ret;
}

/* Lanczos window */
double * smileDsp_winLac(long N)
{
  double i;
  double * ret = (double *)malloc(sizeof(double) * N);
  double * x = ret;
  double NN = (double)N;
  for (i=0.0; i<NN; i += 1.0) {
    *x = smileDsp_lcSinc( (2.0*i)/(NN-1.0) - 1.0 );
    x++;
  }
  return ret;
}

/* gaussian window ...??? */
double * smileDsp_winGau(long N, double sigma)
{
  double i;
  double * ret = (double *)malloc(sizeof(double) * N);
  double * x = ret;
  double NN = (double)N;
  double tmp;
  if (sigma <= 0.0) sigma = 0.01;
  if (sigma > 0.5) sigma = 0.5;
  for (i=0.0; i<NN; i += 1.0) {
    tmp = (i-(NN-1.0)/2.0)/(sigma*(NN-1.0)/2.0);
    *x = exp( -0.5 * ( tmp*tmp ) );
    x++;
  }
  return ret;
}

/* Blackman window */
double * smileDsp_winBla(long N, double alpha0, 
  double alpha1, double alpha2)
{
  double i;
  double * ret = (double *)malloc(sizeof(double) * N);
  double * x = ret;
  double NN = (double)N;
  double tmp;
  for (i=0.0; i<NN; i += 1.0) {
    tmp = (2.0*M_PI*i)/(NN-1.0);
    *x = alpha0 - alpha1 * cos( tmp ) + alpha2 * cos( 2.0*tmp );
    x++;
  }
  return ret;

}

/* Bartlett-Hann window */
double * smileDsp_winBaH(long N, double alpha0, 
  double alpha1, double alpha2)
{
  double i;
  double * ret = (double *)malloc(sizeof(double) * N);
  double * x = ret;
  double NN = (double)N;
  for (i=0.0; i<NN; i += 1.0) {
    *x = alpha0 - alpha1 * fabs( i/(NN-1.0) - 0.5 ) 
          - alpha2 * cos( (2.0*M_PI*i)/(NN-1.0) );
    x++;
  }
  return ret;
}

/* Blackman-Harris window */
double * smileDsp_winBlH(long N, double alpha0, 
  double alpha1, double alpha2, double alpha3)
{
  double i;
  double * ret = (double *)malloc(sizeof(double) * N);
  double * x = ret;
  double NN = (double)N;
  double tmp;
  for (i=0.0; i<NN; i += 1.0) {
    tmp = (2.0*M_PI*i)/(NN-1.0);
    *x = alpha0 - alpha1 * cos( tmp ) + alpha2 * cos( 2.0*tmp ) 
          - alpha3 * cos( 3.0*tmp );
    x++;
  }
  return ret;
}

int winFuncToInt(const char * winF)
{
  int winFunc;
  if ((!strcmp(winF,"Han"))||(!strcmp(winF,"han"))||(!strcmp(winF,"Hanning"))||(!strcmp(winF,"hanning"))||(!strcmp(winF,"hann"))||(!strcmp(winF,"Hann")))
    { winFunc = WINF_HANNING; }
  else if ((!strcmp(winF,"Ham"))||(!strcmp(winF,"ham"))||(!strcmp(winF,"Hamming"))||(!strcmp(winF,"hamming")))
    { winFunc = WINF_HAMMING; }
  else if ((!strcmp(winF,"Rec"))||(!strcmp(winF,"rec"))||(!strcmp(winF,"Rectangular"))||(!strcmp(winF,"rectangular"))||(!strcmp(winF,"none"))||(!strcmp(winF,"None")))
    { winFunc = WINF_RECTANGLE; }
  else if ((!strcmp(winF,"Gau"))||(!strcmp(winF,"gau"))||(!strcmp(winF,"Gauss"))||(!strcmp(winF,"gauss"))||(!strcmp(winF,"Gaussian"))||(!strcmp(winF,"gaussian")))
    { winFunc = WINF_GAUSS; }
  else if ((!strcmp(winF,"Sin"))||(!strcmp(winF,"sin"))||(!strcmp(winF,"Sine"))||(!strcmp(winF,"sine"))||(!strcmp(winF,"cosine"))||(!strcmp(winF,"Cosine"))||(!strcmp(winF,"Cos"))||(!strcmp(winF,"cos")))
    { winFunc = WINF_SINE; }
  else if ((!strcmp(winF,"Tri"))||(!strcmp(winF,"tri"))||(!strcmp(winF,"Triangle"))||(!strcmp(winF,"triangle")))
    { winFunc = WINF_TRIANGLE; }
  else if ((!strcmp(winF,"Bla"))||(!strcmp(winF,"bla"))||(!strcmp(winF,"Blackman"))||(!strcmp(winF,"blackman")))
    { winFunc = WINF_BLACKMAN; }
  else if ((!strcmp(winF,"BlH"))||(!strcmp(winF,"blh"))||(!strcmp(winF,"Blackman-Harris"))||(!strcmp(winF,"blackman-harris")))
    { winFunc = WINF_BLACKHARR; }
  else if ((!strcmp(winF,"Bar"))||(!strcmp(winF,"bar"))||(!strcmp(winF,"Bartlett"))||(!strcmp(winF,"bartlett")))
    { winFunc = WINF_BARTLETT; }
  else if ((!strcmp(winF,"BaH"))||(!strcmp(winF,"bah"))||(!strcmp(winF,"Bartlett-Hann"))||(!strcmp(winF,"bartlett-hann"))||(!strcmp(winF,"Bartlett-Hanning"))||(!strcmp(winF,"bartlett-hanning")))
    { winFunc = WINF_BARTHANN; }
  else if ((!strcmp(winF,"Lac"))||(!strcmp(winF,"lac"))||(!strcmp(winF,"Lanczos"))||(!strcmp(winF,"lanczos")))
    { winFunc = WINF_LANCZOS; }
  else {
    winFunc = WINF_UNKNOWN;
  }
  return winFunc;
}

/****** FFT related dsp functions *****/

void smileDsp_fftPhaseUnwrap(FLOAT_DMEM * phaseRadians, long N) {
  FLOAT_DMEM correction = 0.0;
  FLOAT_DMEM prevPhase = phaseRadians[0];
  long i;
  for (i = 1; i < N; i++) {
    FLOAT_DMEM delta = phaseRadians[i] - prevPhase;
    prevPhase = phaseRadians[i];
    if (delta > M_PI) {
      correction -= 2.0 * M_PI;
    }
    else if (delta < -M_PI) {
      correction += 2.0 * M_PI;
    }
    phaseRadians[i] += correction;
  }
}

// important: caller must ensure the following:
// mag is of size N+1
// phase is of size N+1
long smileDsp_fftComputeMagPhase(const FLOAT_DMEM *complex, long N,
    FLOAT_DMEM *mag, FLOAT_DMEM *phase, int normaliseFft)
{
  long n;
  if (phase != NULL && mag != NULL) {
    mag[0] = fabs(complex[0]);
    phase[0] = (complex[0] >= 0) ? 0 : M_PI;
    for (n = 2; n < N; n += 2) {
      mag[n >> 1] = sqrt(complex[n] * complex[n]
                         + complex[n + 1] * complex[n + 1]);
      phase[n >> 1] = atan2(complex[n + 1], complex[n]);
    }
    mag[N / 2] = complex[1];
    phase[N / 2] = (complex[1] >= 0) ? 0 : M_PI;
  }
  else if (mag != NULL) {
    mag[0] = fabs(complex[0]);
    for (n = 2; n < N; n += 2) {
      mag[n >> 1] = sqrt(complex[n] * complex[n]
                         + complex[n + 1] * complex[n + 1]);
    }
    mag[N / 2] = complex[1];
  }
  else if (phase != NULL) {
    phase[0] = (complex[0] >= 0) ? 0 : M_PI;
    for (n = 2; n < N; n += 2) {
      phase[n >> 1] = atan2(complex[n + 1], complex[n]);
    }
    phase[N / 2] = (complex[1] >= 0) ? 0 : M_PI;
  }
  if (normaliseFft && mag != NULL) {
    for (n = 0; n <= N / 2; n++) {
      mag[n] /= ((FLOAT_DMEM)N * 2.0);
    }
  }
  return N / 2 + 1;
}

/****** other dsp functions *******/

/* compute harmonic product spectrum from a linear scale magnitude spectrum, use up to maxMul down-scaled spectra */
long smileDsp_harmonicProductLin(const FLOAT_DMEM *src, long Nsrc, FLOAT_DMEM *dst, long Ndst, int maxMul)
{
  long i; int m;
  long oLen = Ndst;
  if (Nsrc < oLen) oLen = Nsrc;
  for (i=0; i<oLen; i++) {
    dst[i] = src[i];
    for (m=2; m<=maxMul; m++) {
      long idx = i*m;
      if (idx < Nsrc) dst[i] *= src[idx];
    }
  }
  return oLen;
}

/* compute harmonic sum spectrum from a linear scale magnitude spectrum, use up to maxMul down-scaled spectra */
long smileDsp_harmonicSumLin(const FLOAT_DMEM *src, long Nsrc, FLOAT_DMEM *dst, long Ndst, int maxMul)
{
  long i; int m;
  long oLen = Ndst;
  if (Nsrc < oLen) oLen = Nsrc;
  for (i=0; i<oLen; i++) {
    dst[i] = src[i];
    for (m=2; m<=maxMul; m++) {
      long idx = i*m;
      if (idx < Nsrc) dst[i] += src[idx]/(FLOAT_DMEM)m;
    }
  }
  return oLen;
}


/* convert lp coefficients to cepstra 
   HTKbook , equation 5.11
 */
FLOAT_DMEM smileDsp_lpToCeps(const FLOAT_DMEM *lp, int nLp, FLOAT_DMEM lpGain, FLOAT_DMEM *ceps, int firstCC, int lastCC) 
{
  // CHECK: nCeps <= nLp !
  int i;
  int n;
  int nCeps;
  
  if (firstCC < 1) 
    firstCC = 1;
  if (lastCC > nLp) 
    lastCC = nLp;
  nCeps = lastCC - firstCC + 1;

  for (n = firstCC; n <= lastCC; n++) {
    double sum = 0;
    for (i = 1; i < n; i++) { 
      sum += (n - i) * lp[i - 1] * ceps[n - i - 1]; 
    }
    ceps[n - firstCC] = -(lp[n - firstCC] + (FLOAT_DMEM)(sum / (double)n));
  }
  if (lpGain <= 0.0) { 
    lpGain = (FLOAT_DMEM)1.0; 
  }
  return (FLOAT_DMEM)(-log(1.0/(double)lpGain));
}


/* Autocorrelation in the time domain (for ACF LPC method) */
void smileDsp_autoCorr(const FLOAT_DMEM *x, const int n, FLOAT_DMEM *outp, int lag)
{
  int i;
  while (lag) {
    outp[--lag] = 0.0;
    for (i=lag; i < n; i++) {
      outp[lag] += x[i] * x[i-lag];
    }
  }
}

/* LPC analysis via acf (=implementation of Durbin recursion)*/
int smileDsp_calcLpcAcf(FLOAT_DMEM * r, FLOAT_DMEM *a, int p, FLOAT_DMEM *gain, FLOAT_DMEM *k)
{
  int i, m = 0;
  FLOAT_DMEM e;
  int errF = 1;
  FLOAT_DMEM k_m;
  FLOAT_DMEM *al;

  if (a == NULL) return 0;
  if (r == NULL) return 0;

  if ((r[0] == 0.0)||(r[0] == -0.0)) {
    for (i=0; i < p; i++) { a[i] = 0.0; }
    return 0;
  }

  al = (FLOAT_DMEM*)malloc(sizeof(FLOAT_DMEM)*(p));
  
  // Initialisation, Gl. 158
  e = r[0];
  if (e==0.0) {
    for (i=0; i<=p; i++) {
      a[i] = 0.0;
      if (k!=NULL) k[m] = 0.0;
    }
  } else {
    // Iterations: m = 1... p, Gl. 159
    for (m=1; m<=p; m++) {
      // Gl. 159 (a) 
      FLOAT_DMEM sum = (FLOAT_DMEM)1.0 * r[m];
      for (i=1; i<m; i++) {
        sum += a[i-1] * r[m-i];
      }
      k_m = ( (FLOAT_DMEM)-1.0 / e ) * sum;
      // copy refl. coeff.
      if (k != NULL) k[m-1] = k_m;
      // Gl. 159 (b)
      a[m-1] = k_m;
      for (i=1; i<=m/2; i++) {
        FLOAT_DMEM x = a[i-1];
        a[i-1] += k_m * a[m-i-1];
        if ((i < (m/2))||((m&1)==1)) a[m-i-1] += k_m * x;
      }
      // update error
      e *= ((FLOAT_DMEM)1.0-k_m*k_m);
      if (e==0.0) {
        for (i=m; i<=p; i++) {
          a[i] = 0.0;
          if (k!=NULL) k[m] = 0.0;
        }
        break;
      }
    }
  }
  free(al);

  if (gain != NULL) *gain=e;
  return 1;
}

int smileDsp_calcLpcBurg(const FLOAT_DMEM *samples, long N,
    FLOAT_DMEM *coeffs, int M, FLOAT_DMEM *lpcgain,
    FLOAT_DMEM **gbb, FLOAT_DMEM **gb2, FLOAT_DMEM **gaa)
{
  FLOAT_DMEM order_p = 0.0;
  FLOAT_DMEM tmp_x = 0.0;
  FLOAT_DMEM *tmp_aa = NULL;
  FLOAT_DMEM *tmp_b1 = NULL;
  FLOAT_DMEM *tmp_b2 = NULL;
  long i = 1;
  long j = 0;
  int burgStatus = 0;

  if ((N < M) || (M <= 0)) {
    return 0;
  }
  if (samples == NULL) {
    return 0;
  }
  if (coeffs == NULL) {
    return 0;
  }

  if (gaa != NULL) {
    tmp_aa = *gaa;
  }
  if (tmp_aa == NULL) {
    tmp_aa = (FLOAT_DMEM*)calloc(1, sizeof(FLOAT_DMEM)*M);
  }
  if (gbb != NULL) {
    tmp_b1 = *gbb;
  }
  if (tmp_b1 == NULL) {
    tmp_b1 = (FLOAT_DMEM*)calloc(1, sizeof(FLOAT_DMEM)*N);
  }
  if (gb2 != NULL) {
    tmp_b2 = *gb2;
  }
  if (tmp_b2 == NULL) {
    tmp_b2 = (FLOAT_DMEM*)calloc(1, sizeof(FLOAT_DMEM)*N);
  }

  for (j = 0; j < N; j++) {
    order_p += samples[j] * samples[j];
  }
  tmp_x = order_p / N;
  if (tmp_x > 0) {
    tmp_b1[0] = samples[0];
    tmp_b2[N - 2] = samples[N-1];
    for (j = 1; j < N - 1; j++) {
      tmp_b1[j] = tmp_b2[j - 1] = samples[j];
    }
    for (i = 0; i < M; i++) {
      FLOAT_DMEM nominator = 0.0;
      FLOAT_DMEM denominator = 0.0;
      for (j = 0; j < N - i - 1; j++) {
        nominator += tmp_b1[j] * tmp_b2[j];
        denominator += tmp_b1[j] * tmp_b1[j] + tmp_b2[j] * tmp_b2[j];
      }
      if (denominator <= 0) {
        burgStatus = -1;
        break;
      }
      coeffs[i] = (FLOAT_DMEM)2.0 * nominator / denominator;
      tmp_x *= (FLOAT_DMEM)1.0 - coeffs[i] * coeffs[i];
      for (j = 0; j < i; j++) {
        coeffs[j] = tmp_aa[j] - coeffs[i] * tmp_aa[i - j - 1];
      }
      if (i < M - 1) {
        // Algorithm note: i --> i + 1
        for (j = 0; j <= i; j++) {
          tmp_aa[j] = coeffs[j];
        }
        for (j = 0; j < N - i - 2; j++) {
          tmp_b1[j] -= tmp_aa[i] * tmp_b2[j];
          tmp_b2[j] = tmp_b2[j + 1] - tmp_aa[i] * tmp_b1[j + 1];
        }
      }
    }
    if (burgStatus == 0) {
      burgStatus = 1;
    }
  }
  // Free the temporary data
  if (gaa != NULL) {
    *gaa = tmp_aa;
  } else if (tmp_aa != NULL) {
    free(tmp_aa);
  }
  if (gbb != NULL) {
    *gbb = tmp_b1;
  } else if (tmp_b1 != NULL) {
    free(tmp_b1);
  }
  if (gb2 != NULL) {
    *gb2 = tmp_b2;
  } else if (tmp_b2 != NULL) {
    free(tmp_b2);
  }

  // Coefficients are inverted here to maintain compatibility with
  // the ACF method for estimating LPC coefficients
  for (j = 0; j < i; j++) {
    coeffs[j] = -coeffs[j];
  }
  // The remaining coefficients are padded with zeros
  for (j = i; j < M; j++) {
    coeffs[j] = 0.0;
  }
  // Finally, the gain is adjusted and returned:
  if (lpcgain != NULL) {
    *lpcgain = tmp_x * (FLOAT_DMEM)N;
  }
  return burgStatus;
}

// initialise a inverse RDFT work area
// K is number of frequency bins (input)
// I is the number of actual output samples to compute
// nI is the denominator of the sin/cos functions (usually =I) 
sDftWork * smileDsp_initIrdft(long K, long I, double nI, int antialias)
{
  long i,k;
  double pi2 = 2.0*M_PI;
  sDftWork * w = (sDftWork *)malloc(sizeof(sDftWork));

  w->K = K;
  w->I = I;
  if (antialias) {
    w->kMax = K>I ? I : K; // MIN(K,I);
  } else {
    w->kMax = K;
  }
  if (w->kMax & 1) { w->kMax--; } // "round" down to even number

  w->antiAlias = antialias;

  /* fill cos and sin tables: */
  w->costable = (FLOAT_DMEM*)malloc(sizeof(FLOAT_DMEM)*(w->kMax/2)*I);
  w->sintable = (FLOAT_DMEM*)malloc(sizeof(FLOAT_DMEM)*(w->kMax/2)*I);

  for (i=0; i<I; i++) {
    long i_n = i*w->kMax/2 - 1;
    if (I>=K)
      w->costable[i_n+K/2] = (FLOAT_DMEM)cos((pi2*(double)((K/2)*i))/nI) ; /* Nyquist */
    
    for (k=2; k<w->kMax; k+=2) {
      double kn = pi2 * (double)(k/2 * i) / nI;
      w->costable[i_n + k/2] = (FLOAT_DMEM)cos(kn); /*real*/
      w->sintable[i_n + k/2] = (FLOAT_DMEM)sin(kn); /*imaginary*/
    }
  }

  return w;
}

// free a DFT work area
sDftWork * smileDsp_freeDftwork(sDftWork * w)
{
  if (w != NULL) {
    if (w->costable != NULL) free(w->costable);
    if (w->sintable != NULL) free(w->sintable);
    free(w);
  }
  return NULL;
}

// perform an arbitrary inverse RDFT (slow version)
void smileDsp_irdft(const FLOAT_DMEM * inData, FLOAT_DMEM *out, sDftWork *w) 
{
  FLOAT_DMEM * costable = w->costable - 1;
  FLOAT_DMEM * sintable = w->sintable - 1;
  long i,k;
  for (i=0; i<w->I; i++) {
    //long i_n = i*w->kMax/2 - 1;
    out[i] = inData[0]; /* DC */
    if (w->I >= w->K) 
      out[i] += inData[1] * costable[w->K/2]; /* Nyquist */

    for (k=2; k<w->kMax; k+=2) {
      long k2 = k>>1;
      out[i] += inData[k] * costable[k2]; /*real*/
      out[i] += inData[k+1] * sintable[k2]; /*imaginary*/
    }
    out[i] /= (FLOAT_DMEM)(w->K/2);
    costable += w->kMax/2;
    sintable += w->kMax/2;
  }
}


sResampleWork * smileDsp_resampleWorkFree(sResampleWork * work)
{
  if (work != NULL) {
    if (work->winF != NULL) free(work->winF);
    if (work->winFo != NULL) free(work->winFo);
    if (work->_w != NULL) free(work->_w);
    if (work->_ip != NULL) free(work->_ip);
    if (work->x != NULL) free(work->x);
    smileDsp_freeDftwork(work->irdftWork);
    free(work);
  }
  return NULL;
}

sResampleWork * smileDsp_resampleWorkInit(long Nin)
{
  sResampleWork * ret = (sResampleWork *)calloc(1,sizeof(sResampleWork));
  ret->x = (FLOAT_DMEM*)malloc(sizeof(FLOAT_DMEM)*Nin);
  return ret;
}

int smileDsp_doResample(FLOAT_TYPE_FFT *x, long Nin, FLOAT_DMEM *y, long Nout, double nd, sResampleWork ** _work)
{
  long i;
  sResampleWork *work;

  if (x==NULL) return 0;
  if (y==NULL) return 0;
  if (_work == NULL) return 0;
  
  if (*_work==NULL) {
    *_work = smileDsp_resampleWorkInit(Nin);
  }
  work = *_work;

  // window the frame
  if (work->winF == NULL) {
    work->winF = smileDsp_winHan(Nin);
    smileMath_vectorRootD(work->winF,Nin);
  }
  for (i=0; i<Nin; i++) {
    x[i] *= (FLOAT_DMEM)(work->winF[i]);
  }
  // fft the frame
  if (work->_ip == NULL) { work->_ip = (int *)calloc(1,sizeof(int)*(Nin+2)); }
  if (work->_w==NULL) work->_w = (FLOAT_TYPE_FFT *)calloc(1,sizeof(FLOAT_TYPE_FFT)*(Nin*5)/4+2);
  rdft(Nin, 1, x, work->_ip, work->_w);

  // convert from FLOAT_TYPE_FFT to FLOAT_DMEM if necessary
#if FLOAT_FFT_NUM != FLOAT_DMEM_NUM
  for (i=0; i<Nin; i++) {
    work->x[i] = (FLOAT_DMEM)x[i];
  }
  // inverse dft to new sample rate:
  if (work->irdftWork == NULL) { work->irdftWork = smileDsp_initIrdft(Nin, Nout, nd, 1); } 
  smileDsp_irdft(work->x, y, work->irdftWork);
#else
  // inverse dft to new sample rate:
  if (work->irdftWork == NULL) { work->irdftWork = smileDsp_initIrdft(Nin, Nout, nd, 1); } 
  smileDsp_irdft(x, y, work->irdftWork);

  // window again
  if (work->winFo == NULL) {
    work->winFo = smileDsp_winHan(Nout);
    smileMath_vectorRootD(work->winFo,Nout);
  }
  for (i=0; i<Nout; i++) {
    y[i] *= (FLOAT_DMEM)(work->winFo[i]);
  }

#endif
  return 1;
}


/* Implementation of a lattice filter, processes a single value per call */
FLOAT_DMEM smileDsp_lattice(FLOAT_DMEM *k, FLOAT_DMEM *b, int M, FLOAT_DMEM in, FLOAT_DMEM *bM)
{
  int i;
  FLOAT_DMEM f0,f1,b0,b1;
  /* initialisation */
  b0 = f0 = in;
  for (i=0; i<M; i++) {
    f1 = f0 + k[i] * b[i];
    b1 = k[i] * f0 + b[i];
    b[i] = b0; // store b[n-1]
    // save coefficients for next iteration:
    f0 = f1;
    b0 = b1;
  }
  /* return b (optional) */
  if (bM != NULL) *bM = b1;
  /* return f */
  return f1;
}

/* Implementation of a lattice filter for an array *in of N floats, NOTE: *bM is an array of size N here! */
void smileDsp_latticeArray(FLOAT_DMEM *k, FLOAT_DMEM *b, int M, FLOAT_DMEM *in, FLOAT_DMEM *out, FLOAT_DMEM *bM, int N)
{
  int i,n;
  FLOAT_DMEM f0,f1,b0,b1;

  for (n=0; n<N; n++) {

    /* initialisation */
    b0 = f0 = in[n];
    for (i=0; i<M; i++) {
      f1 = f0 + k[i] * b[i]; 
      b1 = k[i] * f0 + b[i]; 
      b[i] = b0; 
      f0 = f1; 
      b0 = b1; 
    }
    /* return b (optional) */
    if (bM != NULL) bM[n] = b1;
    /* return f */
    out[n] = f1;

  }
}

/* Implementation of an inverse lattice filter, processes a single value per call */
FLOAT_DMEM smileDsp_invLattice(FLOAT_DMEM *k, FLOAT_DMEM *b, int M, FLOAT_DMEM out)
{
  int i;
  FLOAT_DMEM fM;
  FLOAT_DMEM last = b[M-1];
  /* initialisation */
  fM = out;
  for (i=M-1; i>0; i--) {
    fM -= k[i]*b[i-1];
    b[i] = k[i]*fM + b[i-1];
  }
  b[M-1] = last;
  fM = fM - k[0]*b[M-1];
  b[0] = k[0]*fM + b[M-1];
  b[M-1] = fM;
  /* return fM */
  return fM;
}

/* peak enhancement in a linear magnitude spectrum */
int smileDsp_specEnhanceSHS (double *a, long n)
{
  long i = 0;
  long j = 0;
  long nmax = 0;
  long *posmax = (long *)calloc(1, sizeof(long)*( (n + 1) / 2 + 1));
  if ((n < 2) || (posmax == NULL)) {
    return 0;
  }
  if (a[0] > a[1]) {
    posmax[nmax++] = 0;
  }
  for (i = 1; i < n - 1; i++) {
    if (a[i] > a[i - 1] && a[i] >= a[i + 1]) {
      posmax[nmax++] = i;
    }
  }
  if (a[n - 1] > a[n - 2]) {
    posmax[nmax++] = n - 1;
  }
  if (nmax == 1) {
    for (j = 0; j <= posmax[1] - 3; j++) {
      a[j] = 0;
    }
    for (j = posmax[1] + 3; j < n; j++) {
      a[j] = 0;
    }
  } else {
    for (i = 1; i < nmax; i++) {
      for (j = posmax[i - 1] + 3; j <= posmax[i] - 3; j++) {
        a[j] = 0;
      }
    }
  }
  free(posmax);
  return 1;
}

/* smooth a magnitude spectrum (linear) */
void smileDsp_specSmoothSHS (double *a, long n)
{
  double ai = 0.0;
  double aim1 = 0.0;
  long i = 0;
  for ( ; i < n - 1; i++) {
    ai = a[i];
    a[i] = (aim1 + 2.0 * ai + a[i + 1]) / 4.0;
    aim1 = ai;
  }
}

/* computes formant frequencies and bandwidths from lpc polynomial roots 
    return value: number of valid formants detected from given lpc polynomial roots
 */
int smileDsp_lpcrootsToFormants(double *r, int nR, double *fc, double *bc, int nF, double samplePeriod, double fLow, double fHigh)
{
  double f;
  int i;
  int n = nF;
  int nFormants = 0;
  double spPi = samplePeriod * M_PI;  // (1/sp*2*pi)
  double spPi2 = spPi * 2.0;
  if ((fHigh < fLow) || (fHigh > 1.0/samplePeriod)) {
    fHigh = 0.5 / samplePeriod - fLow;
  }
  for (i = 0; i < nR; i++) {
    int iRe = i * 2;
    int iIm = iRe + 1;
    // only use complex poles, real-valued only is only spectrum slope
    if (r[iIm] < 0) {
      continue;
    }
    // compute formant frequency (= the phase of the complex number)
    f = fabs (atan2 (r[iIm], r[iRe])) / (spPi2);
    if ((f >= fLow) && (f <= fHigh)) { // if frequency is in range, compute the bandwidth
      if (bc != NULL) {
        bc[nFormants] = -log(smileMath_complexAbs(r[iRe],r[iIm])) / spPi;
      }
      fc[nFormants] = f;
      nFormants++;
      if (nFormants >= nF) break;
    }
  }
  for (i = nFormants; i < nF; i++) {
    fc[i] = 0.0; 
    if (bc != NULL) bc[i] = 0.0;
  }
  return nFormants;
}

/* converts an amplitude ratio to decibel (dB) 
     using the equation: 20*log10(a)
   a must be > 0
 */ 
double smileDsp_amplitudeRatioToDB(double a)
{
  if (a > 10e-50) {
    return 20.0 * log(a) / log(10.0);
  } else {
    return -1000.0;
  }
}



/*******************************************************************************************
 ***********************=====   Statistics functions   ===== *******************************
 *******************************************************************************************/

/*
Note: the entropy functions compute entropy from a PMF, thus a sequence of values must be converted to a PMF beforehand!
For spectral entropy the normalised spectrum is assumed to be a PMF, thus it is not converted...
*/

const double entropy_floor = 0.0000001;

/* compute entropy of normalised values (the values will be normalised to represent probabilities by this function) */
FLOAT_DMEM smileStat_entropy(const FLOAT_DMEM *vals, long N)
{
  double e = 0.0;
  int i;
  double dn = 0.0;
  FLOAT_DMEM min=0.0;
  double l2 = (double)log(2.0);
  // get sum of values and minimum value
  for (i=0; i<N; i++) {
    dn += (double)vals[i];
    if (vals[i] < min) min = vals[i];
  }
  // add minimum to values, if minimum is < 0
  if (min < 0.0) {
    double mf = entropy_floor + min;
    for (i=0; i<N; i++) {
      //vals[i] -= min;
      if (vals[i] <= mf) {
        //vals[i] = (FLOAT_DMEM)entropy_floor;
        dn += mf - vals[i];
      }
      dn -= (double)min;
    }
  } else {
    min = 0.0;
  }
  if (dn < (FLOAT_DMEM)entropy_floor) {
    dn = (FLOAT_DMEM)entropy_floor;
  }
  // normalise sample values and compute entropy
  for (i=0; i<N; i++) {
    double v = vals[i] - min;
    double ln;
    if (v <= entropy_floor) {
      v = entropy_floor;
    }
    ln = v / dn;
    if (ln > 0.0) {
      e += ln * (double)log(ln) / l2;
    }
  }
  return (FLOAT_DMEM)(-e);
}

/* compute entropy of normalised values (the values will be normalised to represent probabilities by this function) */
// ??? difference to normal entropy ???
FLOAT_DMEM smileStat_relativeEntropy(const FLOAT_DMEM *vals, long N)
{
  double e = 0.0;
  int i;
  double dn = 0.0;
  FLOAT_DMEM min=0.0;
  double lnorm = (double)log((double)N);
  // get sum of values and minimum value
  for (i=0; i<N; i++) {
    dn += (double)vals[i];
    if (vals[i] < min) min = vals[i];
  }
  // add minimum to values, if minimum is < 0
  if (min < 0.0) {
    double mf = entropy_floor + min;
    for (i=0; i<N; i++) {
      //vals[i] -= min;
      if (vals[i] <= mf) {
        //vals[i] = (FLOAT_DMEM)entropy_floor;
        dn += mf - vals[i];
      }
      dn -= (double)min;
    }
  } else {
    min = 0.0;
  }
  if (dn < (FLOAT_DMEM)entropy_floor) {
    dn = (FLOAT_DMEM)entropy_floor;
  }
  // normalise sample values and compute entropy
  for (i=0; i<N; i++) {
    double v = vals[i] - min;
    double ln;
    if (v <= entropy_floor) {
      v = entropy_floor;
    }
    ln = v / dn;
    if (ln > 0.0) 
      e += ln * (double)log(ln) / lnorm;
  }
  return (FLOAT_DMEM)(-e);
}


/* TODO:
compute a PMF from a sample sequence using a histogram sampling method

if a valid pointer is given in h->bins, then the histogram will be added to the existing data,
the resulting histogram will then be unnormalised, you will have to call smileMath_vectorNormEuc in the end..
--TODO: this is not the correct way of handling things....! We must normalise both histograms, add and divide by 2
--actually the input histogram is normalised.. so we only divide by 2

if h->bins is NULL the histogram will be initialised with zeros and the output will be normalised.

the memory pointed to by *h must be initialised with 0s! (at least h->bins must be NULL...)

set range to 0.0 to automatically have it determined from the data (max-min)
*/
void smileStat_getPMF(FLOAT_DMEM *_vals, long N, sHistogram *h)
{
  long i; long x; int renorm=0;
  //double sum=(double)_vals[0];
  FLOAT_DMEM min=_vals[0];
  FLOAT_DMEM max=_vals[0];
  FLOAT_DMEM p = (FLOAT_DMEM)(1.0/(double)N);

  if (h == NULL) return;

  if (h->Nbins == 0) h->Nbins = 1000;
  if (h->bins == NULL) {
    h->bins = (FLOAT_DMEM*)calloc(1,sizeof(FLOAT_DMEM)*h->Nbins);
    if ((h->min == 0.0) && (h->max == 0.0)) {
      // get sum of values and min/max value
      for (i=1; i<N; i++) {
        //sum += (double)_vals[i];
        if (_vals[i] < min) min = _vals[i];
        if (_vals[i] > max) max = _vals[i];
      }
      h->max = max;
      h->min = min;
    }
    h->stepsize = (h->max - h->min) / (FLOAT_DMEM)(h->Nbins);
    //printf("stepsize = %f  min = %f  max=%f (_vals0=%f)\n",h->stepsize,h->min, h->max,_vals[0]);
  } else {
    renorm = 1;
  }
  
  if (renorm && (h->weight > 1.0)) {
    for (i=0; i<h->Nbins; i++) {
      h->bins[i] *= h->weight;
    }
    p = 1.0;
  }

  for (i=0; i<N; i++) {
    x = (long)floor((_vals[i] - h->min)/h->stepsize);
    if (x<0) x = 0;  /* range clipping*/
    else if (x>=h->Nbins) x=h->Nbins-1; /* range clipping*/
    h->bins[x] += p;
  }

  if (renorm) {
    if (h->weight > 1.0) {
      h->weight += (FLOAT_DMEM)N;
      for (i=0; i<h->Nbins; i++) {
        h->bins[i] /= h->weight;
      }
    } else {
      for (i=0; i<h->Nbins; i++) {
        h->bins[i] /= 2.0;
      }
    }
  } else {
    h->weight = (FLOAT_DMEM)N;
  }
}

/* Estimate the probability of a value x belonging to a given pmf h (the pmf must be normalised to sum 1!) */
FLOAT_DMEM smileStat_probEstim(FLOAT_DMEM x, sHistogram *h, FLOAT_DMEM probFloor)
{
  long i;
  FLOAT_DMEM ret;
  if (h==NULL) return 0.0;
  i = (long)floor((x - h->min)/h->stepsize);
  if (i>=h->Nbins) return 0.0;
  if (i<0) return 0.0;
  // TODO: interpolate actual value more accurately by considering neighbour probabilities
  ret = MAX(h->bins[i],probFloor);
  if (ret > 1.0) {
#ifdef DEBUG
    fprintf("WARNING: Clipped probability to 1.0 in smileStat_probEstim (%f) (probFloor = %f)\n",ret,probFloor);
#endif
    ret = 1.0; // perform clipping
  }
  return ret;
}

/* get a PMF vector from a data matrix (result is a pmf histogram for each row of the matrix) */
/* *h must point to an allocated array of R x sHistogram */
/* N: number of colums in matrix  , R: number of rows in the matrix (must match the size of *h) */
/* matrix in _vals is read rowwise.. cMatrix in openSMILE is read columnwise, so you must transpose here!*/
void smileStat_getPMFvec(FLOAT_DMEM *_vals, long N, long R, sHistogram *h)
{
  long i;
  if (h== NULL) return;
  if (_vals == NULL) return;
  for (i=0; i<R; i++) {
    smileStat_getPMF(_vals+(N*i), N, h+i);
  }
}

/* get a PMF vector from a data matrix (result is a pmf histogram for each row of the matrix) */
/* *h must point to an allocated array of R x sHistogram */
/* N: number of colums in matrix  , R: number of rows in the matrix (must match the size of *h) */
/* matrix in _vals is read columnwise.. use this for compatiblity with openSMILE cMatrix!*/
void smileStat_getPMFvecT(FLOAT_DMEM *_vals, long N, long R, sHistogram *h)
{
  long i,n;
  FLOAT_DMEM *_row;
  if (h== NULL) return;
  if (_vals == NULL) return;
  _row = (FLOAT_DMEM *)malloc(sizeof(FLOAT_DMEM)*N);
  for (i=0; i<R; i++) {
    for (n=0; n<N; n++) {
      _row[n] = _vals[n*R+i];
    }
    smileStat_getPMF(_row, N, h+i);
  }
}

/* estimate probability of a vector belonging to a pmf array */
void smileStat_probEstimVec(FLOAT_DMEM *x, sHistogram *h, FLOAT_DMEM **p, long R, FLOAT_DMEM probFloor)
{
  long i;
  if (x==NULL) return;
  if (h==NULL) return;
  if (p==NULL) return;
  
  if (*p==NULL) 
     *p = (FLOAT_DMEM*)calloc(1,sizeof(FLOAT_DMEM)*R);

  for (i=0; i<R; i++) {
    (*p)[i] = smileStat_probEstim(x[i], h+i, probFloor);
  }
}

/* estimate probability of a vector belonging to a pmf array */
FLOAT_DMEM smileStat_probEstimVecLin(FLOAT_DMEM *x, sHistogram *h, long R, FLOAT_DMEM probFloor)
{
  FLOAT_DMEM *p = NULL;
  FLOAT_DMEM P = 1.0;
  long i;
  smileStat_probEstimVec(x,h,&p,R,probFloor);
  for (i=0; i<R; i++) {
    P *= p[i];
  }
  //P/=(FLOAT_DMEM)R;
  if (p!=NULL) free(p);
  return MAX(P,probFloor);
}

/* estimate probability of a vector belonging to a pmf array */
FLOAT_DMEM smileStat_probEstimVecLog(FLOAT_DMEM *x, sHistogram *h, long R, FLOAT_DMEM probFloorLog)
{
  FLOAT_DMEM *p = NULL;
  FLOAT_DMEM P = 1.0;
  long i;
  smileStat_probEstimVec(x,h,&p,R,0.0);
  for (i=0; i<R; i++) {
    if (p[i] < exp(probFloorLog)) P += probFloorLog;
    else P += (FLOAT_DMEM)log(p[i]);
  }
  if (p!=NULL) free(p);
  return MAX(P,probFloorLog);
}

/*******************************************************************************************
 ***********************=====   WAVE file I/O   ===== **************************************
 *******************************************************************************************/


typedef struct {
  uint32_t	Riff;    /* Must be little endian 0x46464952 (RIFF) */
  uint32_t	FileSize;
  uint32_t	Format;  /* Must be little endian 0x45564157 (WAVE) */
} sRiffPcmWaveHeader;

typedef struct {
  uint32_t  ChunkID;
  uint32_t  ChunkSize;
} sRiffChunkHeader;

typedef struct {
  uint16_t	AudioFormat;
  uint16_t	NumChannels;
  uint32_t	SampleRate;
  uint32_t	ByteRate;
  uint16_t	BlockAlign;
  uint16_t	BitsPerSample;
} sWavFmtChunk;

#define WAVE_FORMAT_PCM 1
#define WAVE_FORMAT_IEEE_FLOAT 3


/* parse wave header from in-memory wave file pointed to by *raw */
int smilePcm_parseWaveHeader(void *raw, long long N, sWaveParameters *pcmParam)
{
  // not implemented yet
  return 0;
}

// filename is optional and can be NULL ! It is used only for log messages.
int smilePcm_readWaveHeader(FILE *filehandle, sWaveParameters *pcmParam, const char *filename)
{
  if (filename == NULL) filename = "unknown";

  if ((filehandle == NULL)||(pcmParam==NULL)) return 0;

  int nRead;
  sRiffPcmWaveHeader head;
  sRiffChunkHeader chunkhead;
  sWavFmtChunk fmtchunk;

  fseek(filehandle, 0, SEEK_SET);
  nRead = (int)fread(&head, 1, sizeof(head), filehandle);
  if (nRead != sizeof(head)) {
    fprintf(stderr,"smilePcm: Error reading %zu bytes (header) from beginning of wave file '%s'! File too short??\n",sizeof(head),filename);
    return 0;
  }

  /* Check for valid header, TODO: support other endianness */
  if ((head.Riff != 0x46464952) ||
      (head.Format != 0x45564157)) {
    fprintf(stderr,"smilePcm:  Riff: %x\n  Format: %x\n", head.Riff, head.Format);
    fprintf(stderr,"smilePcm: bogus wave/riff header or file in wrong format ('%s')!)\n",filename);
    return 0;
  }

  /* Read fmt sub-chunk header */
  nRead = (int)fread(&chunkhead, 1, sizeof(chunkhead), filehandle);
  if (nRead != sizeof(chunkhead)) {
    fprintf(stderr,"smilePcm: less bytes read (%i) from wave file '%s' than there should be (%zu) while reading sub-chunk header! File seems broken!\n",nRead,filename,sizeof(chunkhead));
    return 0;
  }

  // in some cases, the fmt chunk may not be the first sub-chunk in the file
  // we assume, however, that it will always come before the data sub-chunk
  while (chunkhead.ChunkID != 0x20746D66 /* "fmt " */) {
    int padding = chunkhead.ChunkSize % 2;
    fseek(filehandle, chunkhead.ChunkSize + padding, SEEK_CUR);

    /* Read "fmt " chunk header */
    nRead = (int)fread(&chunkhead, 1, sizeof(chunkhead), filehandle);
    if (nRead != sizeof(chunkhead)) {
      fprintf(stderr,"smilePcm: less bytes read (%i) from wave file '%s' than there should be (%zu) while reading fmt chunk header! File seems broken!\n",nRead,filename,sizeof(chunkhead));
      return 0;
    }
  }

  if (chunkhead.ChunkSize != 16 && chunkhead.ChunkSize != 18 && chunkhead.ChunkSize != 40) {
    fprintf(stderr,"smilePcm:  chunk ID: %x\n  chunk size: %x\n", chunkhead.ChunkID, chunkhead.ChunkSize);
    fprintf(stderr,"smilePcm: first sub-chunk of RIFF chunk could not be parsed ('%s')!\n",filename);
    return 0;
  }

  /* Read "fmt " chunk */
  nRead = (int)fread(&fmtchunk, 1, sizeof(fmtchunk), filehandle);
  if (nRead != sizeof(fmtchunk)) {
    fprintf(stderr,"smilePcm: less bytes read (%i) from wave file '%s' than there should be (%zu) while reading fmt chunk! File seems broken!\n",nRead,filename,sizeof(chunkhead));
    return 0;
  }
  /* If the "fmt " chunk is in the extended format, skip the additional extension data */
  if (chunkhead.ChunkSize > sizeof(fmtchunk)) {
    fseek(filehandle, chunkhead.ChunkSize - sizeof(fmtchunk), SEEK_CUR);
  }
  /* Check for supported audio format */
  if (fmtchunk.AudioFormat != WAVE_FORMAT_PCM && 
      fmtchunk.AudioFormat != WAVE_FORMAT_IEEE_FLOAT) { 
    fprintf(stderr,"smilePcm: Wave format %x of file '%s' unsupported. Only PCM and IEEE Float are supported.\n",fmtchunk.AudioFormat,filename);
    return 0;
  }

  /* Read "data" chunk header */
  nRead = (int)fread(&chunkhead, 1, sizeof(chunkhead), filehandle);
  if (nRead != sizeof(chunkhead)) {
    fprintf(stderr,"smilePcm: less bytes read (%i) from wave file '%s' than there should be (%zu) while reading data chunk header! File seems broken!\n",nRead,filename,sizeof(chunkhead));
    return 0;
  }

  while (chunkhead.ChunkID != 0x61746164 /* "data" */) {
    int padding = chunkhead.ChunkSize % 2;
    fseek(filehandle, chunkhead.ChunkSize + padding, SEEK_CUR);

    /* Read "data" chunk header */
    nRead = (int)fread(&chunkhead, 1, sizeof(chunkhead), filehandle);
    if (nRead != sizeof(chunkhead)) {
      fprintf(stderr,"smilePcm: less bytes read (%i) from wave file '%s' than there should be (%zu) while reading data chunk header! File seems broken!\n",nRead,filename,sizeof(chunkhead));
      return 0;
    }
  }

  pcmParam->sampleType = fmtchunk.AudioFormat;
  pcmParam->sampleRate = fmtchunk.SampleRate;
  pcmParam->nChan = fmtchunk.NumChannels;
  pcmParam->nBPS = fmtchunk.BlockAlign/fmtchunk.NumChannels;
  pcmParam->nBits = fmtchunk.BitsPerSample;
  pcmParam->nBlocks = chunkhead.ChunkSize / fmtchunk.BlockAlign;
  pcmParam->blockSize = fmtchunk.BlockAlign;
  pcmParam->byteOrder = BYTEORDER_LE;
  pcmParam->memOrga = MEMORGA_INTERLV;
  pcmParam->headerOffset = ftell(filehandle);
  return 1;
}

int smilePcm_numberBytesToNumberSamples(int nBytes, const sWaveParameters *pcmParam) {
  int nSamples = nBytes / (pcmParam->nChan * pcmParam->nBPS);
  if (smilePcm_numberSamplesToNumberBytes(nSamples, pcmParam) != nBytes) {
    fprintf(stderr,
      "smilePcm: ERROR: number of bytes in audio buffer is not divisible by sample blocksize!\n");
  }
  return nSamples;
}

int smilePcm_numberSamplesToNumberBytes(int nSamples, const sWaveParameters *pcmParam) {
  int nBytes = nSamples * (pcmParam->nChan * pcmParam->nBPS);
  return nBytes;
}

// Converts integer PCM sample values to float sample values in [-1,+1].
// nChan is the number of channels allocated in *a.
// Requires nBits, nBPS, and nChan in pcmParam
int smilePcm_convertSamples(const void *buf, const sWaveParameters *pcmParam,
    float *a, int nChan, int nSamples, int monoMixdown)
{
  int i,c;
  #define smilePcm_setFMatrix(xx_a,xx_nChan,xx_c,xx_s,xx_val) xx_a[xx_nChan * xx_s + xx_c] = xx_val;

  const int8_t *b8=(const int8_t*)buf;
  const uint8_t *bu8=(const uint8_t*)buf;
  const int16_t *b16=(const int16_t*)buf;
  const int32_t *b32=(const int32_t*)buf;

  if (a==NULL) return 0;
  if (pcmParam==NULL) return 0;
  if (buf==NULL) return 0;

  // TODO: add selectChannel option to select a single channel from multi-channel instead of monoMixdown
  if (monoMixdown) {
    switch(pcmParam->nBPS) {
      case 1: // 8-bit int
        for (i=0; i<nSamples; i++) {
          float tmp = 0.0;
          for (c=0; c<pcmParam->nChan; c++) {
            tmp += (float)b8[i*pcmParam->nChan+c];
          }
          smilePcm_setFMatrix(a,nChan,0,i,(tmp/(float)pcmParam->nChan)/(float)127.0);
        }
        break;
      case 2: // 16-bit int
        for (i=0; i<nSamples; i++) {
          float tmp = 0.0;
          for (c=0; c<pcmParam->nChan; c++) {
            tmp += (float)b16[i*pcmParam->nChan+c];
          }
          smilePcm_setFMatrix(a,nChan,0,i,(tmp/(float)pcmParam->nChan)/(float)32767.0);
        }
        break;
      case 3: // 24-bit int
        for (i=0; i<nSamples; i++) {
          float tmp = 0.0;
          for (c=0; c<pcmParam->nChan; c++) {
            // the only known file with 3bytes was exported by matlab
            // a byte order conversion was necessary here.. is that always the case?? FIXME!
            uint32_t is=0;
            int32_t * iis;
            is |= (uint32_t)(bu8[(i*pcmParam->nChan+c)*3])<<8;
            is |= (uint32_t)(bu8[(i*pcmParam->nChan+c)*3+1])<<16;
            is |= (uint32_t)(bu8[(i*pcmParam->nChan+c)*3+2])<<24;
            iis = (int32_t*)&is;
            tmp += (float)(*iis >> 8);
          }
          smilePcm_setFMatrix(a,nChan,0,i,(tmp/(float)pcmParam->nChan)/(float)(32767.0*256.0));
        }
        break;
      case 4: // 32-bit int or 24-bit packed int
        if (pcmParam->nBits == 24) {
          for (i=0; i<nSamples; i++) {
            float tmp = 0.0;
            for (c=0; c<pcmParam->nChan; c++) {
              tmp += (float)(b32[i*pcmParam->nChan+c]&0xFFFFFF);
            }
            smilePcm_setFMatrix(a,nChan,0,i,(tmp/(float)pcmParam->nChan)/(float)(32767.0*256.0));
          } break;
        } else if (pcmParam->nBits == 32) {
          for (i=0; i<nSamples; i++) {
            float tmp = 0.0;
            for (c=0; c<pcmParam->nChan; c++) {
              tmp += (float)(b32[i*pcmParam->nChan+c]);
            }
            smilePcm_setFMatrix(a,nChan,0,i,(tmp/(float)pcmParam->nChan)/(float)2147483647.0);
          } break;
        } // no break here, as we use warning below for unknown format!       
      default:
        fprintf(stderr,"smilePcm: readData: cannot convert unknown sample format to float! (nBPS=%i, nBits=%i)\n",pcmParam->nBPS,pcmParam->nBits);
        fflush(stderr);
        break;
    }

  } else { // no mixdown, multi-channel matrix output
    if (nChan != pcmParam->nChan) {
      fprintf(stderr, "ERROR: smilePcm: if not using monomixdown option, the number of channels in the wave file (pcmData.nChan) must match the number of channels in the data matrix (nChan)!\n");
      return 0;
    }
    switch(pcmParam->nBPS) {
      case 1: // 8-bit int
        for (i=0; i<nSamples; i++) for (c=0; c<pcmParam->nChan && c < nChan; c++) {
          smilePcm_setFMatrix(a,nChan,c,i,((float)b8[i*pcmParam->nChan+c])/(float)127.0);
        } break;
      case 2: // 16-bit int
        for (i=0; i<nSamples; i++) for (c=0; c<pcmParam->nChan && c < nChan; c++) {
          smilePcm_setFMatrix(a,nChan,c,i,((float)b16[i*pcmParam->nChan+c])/(float)32767.0);
        } break;
      case 3: // 24-bit int
        for (i=0; i<nSamples; i++) {
          float tmp = 0.0;
          for (c=0; c<pcmParam->nChan && c < nChan; c++) {
            // the only known file with 3bytes was exported by matlab
            // a byte order conversion was necessary here.. is that always the case?? FIXME!
            uint32_t is=0;
            int32_t * iis;
            is |= (uint32_t)(bu8[(i*pcmParam->nChan+c)*3])<<8;
            is |= (uint32_t)(bu8[(i*pcmParam->nChan+c)*3+1])<<16;
            is |= (uint32_t)(bu8[(i*pcmParam->nChan+c)*3+2])<<24;
            iis = (int32_t*)&is;
            tmp = (float)(*iis >> 8);
            smilePcm_setFMatrix(a,nChan,c,i,(tmp)/(float)(32767.0*256.0));
          }
        }
        break;
      case 4: // 32-bit int or 24-bit packed int
        if (pcmParam->nBits == 24) {
          for (i=0; i<nSamples; i++) for (c=0; c<pcmParam->nChan && c < nChan; c++) {
            smilePcm_setFMatrix(a,nChan,c,i,((float)(b32[i*pcmParam->nChan+c]&0xFFFFFF))
                / (float)(32767.0*256.0));
          } break;
        } else if (pcmParam->nBits == 32) {
          for (i=0; i<nSamples; i++) for (c=0; c<pcmParam->nChan && c < nChan; c++) {
            smilePcm_setFMatrix(a,nChan,c,i,((float)b32[i*pcmParam->nChan+c])
                / (float)2147483647.0);
          } break;
        }  // no break here, as we use warning below for unknown format!
      default:
        fprintf(stderr,"smilePcm: readData: cannot convert unknown sample format to float! (nBPS=%i, nBits=%i)\n",
            pcmParam->nBPS, pcmParam->nBits);
        fflush(stderr);
    }
  }
  return nSamples;
}

int smilePcm_convertFloatSamples(const void *buf, const sWaveParameters *pcmParam,
    float *a, int nChan, int nSamples, int monoMixdown)
{
  int i,c;
  #define smilePcm_setFMatrix(xx_a,xx_nChan,xx_c,xx_s,xx_val) xx_a[xx_nChan * xx_s + xx_c] = xx_val;

  float *bf32=(float*)buf;

  if (sizeof(float) != 4) {
    fprintf(stderr, "ERROR: smilePcm: IEEE Float format only supported on platforms where sizeof(float) is 4 bytes\n");
    return 0;
  }

  if (a==NULL) return 0;
  if (pcmParam==NULL) return 0;
  if (buf==NULL) return 0;
  if (pcmParam->sampleType != WAVE_FORMAT_IEEE_FLOAT) {
    fprintf(stderr, "ERROR: smilePcm: smilePcm_convertFloatSamples can only handle the IEEE Float sample type!\n");
    return 0;
  }

  // TODO: add selectChannel option to select a single channel from multi-channel instead of monoMixdown
  if (monoMixdown) {
    switch(pcmParam->nBPS) {
      case 4: // 32-bit IEEE float
        if (pcmParam->nBits == 32) {
          for (i=0; i<nSamples; i++) {
            float tmp = 0.0;
            for (c=0; c<pcmParam->nChan; c++) {
              tmp += bf32[i*pcmParam->nChan+c];
            }
            smilePcm_setFMatrix(a,nChan,0,i,tmp/(float)pcmParam->nChan);
          } break;
        } // no break here, as we use warning below for unknown format!        
      default:
        fprintf(stderr,"smilePcm: readData: cannot convert unknown sample format to float! (nBPS=%i, nBits=%i)\n",pcmParam->nBPS,pcmParam->nBits);
        fflush(stderr);
        break;
    }

  } else { // no mixdown, multi-channel matrix output
    if (nChan != pcmParam->nChan) {
      fprintf(stderr, "ERROR: smilePcm: if not using monomixdown option, the number of channels in the wave file (pcmData.nChan) must match the number of channels in the data matrix (nChan)!\n");
      return 0;
    }
    switch(pcmParam->nBPS) {      
      case 4: // 32-bit IEEE float
        if (pcmParam->nBits == 32) {
          for (i=0; i<nSamples; i++) for (c=0; c<pcmParam->nChan && c < nChan; c++) {
            smilePcm_setFMatrix(a,nChan,c,i,bf32[i*pcmParam->nChan+c]);
          } break;
        } // no break here, as we use warning below for unknown format!
      default:
        fprintf(stderr,"smilePcm: readData: cannot convert unknown sample format to float! (nBPS=%i, nBits=%i)\n",
            pcmParam->nBPS, pcmParam->nBits);
        fflush(stderr);
    }
  }
  return nSamples;
}

// Reads pcm data from a filehandle and converts it to float array with value range -1 to +1.
// Return value: -1 eof, 0 error, > 0 num samples read
int smilePcm_readSamples(FILE **filehandle, sWaveParameters *pcmParam,
    float *a, int nChan, int nSamples, int monoMixdown)
{
  // reads data into matix m, size is determined by m, also performs format conversion to float samples and matrix format
  int bs, nRead;
  uint8_t *buf;

  if (filehandle==NULL) return -1;
  if (*filehandle==NULL) return 0;
  if (a==NULL) return 0;
  if (pcmParam==NULL) return 0;
  if (feof(*filehandle)) {
    return -1;
  }
  
  bs = pcmParam->blockSize * nSamples;
  buf = (uint8_t *)malloc(bs);
  if (buf==NULL) return 0;

  nRead = (int)fread(buf, 1, bs, *filehandle);
  if (nRead != bs) {
    nSamples = nRead / pcmParam->blockSize; // int div rounds down..!? it should better do so...
    // Close files, as we assume EOF here...
    fclose(*filehandle); *filehandle = NULL;
  }
  if (nRead > 0) {
    if (pcmParam->sampleType != WAVE_FORMAT_IEEE_FLOAT) {
      nSamples = smilePcm_convertSamples(buf, pcmParam, a, nChan, nSamples, monoMixdown);
    } else {
      nSamples = smilePcm_convertFloatSamples(buf, pcmParam, a, nChan, nSamples, monoMixdown);
    }
  }
  free(buf);
  return nSamples;
}

/*void smilePcm_setFMatrix(float* m, int nChan, int c, int s, float val)
{
  m[nChan*s+c]=val;
}*/

/*******************************************************************************************
 *******************=====   Vector save/load debug helpers   ===== *************************
 *******************************************************************************************/

/* these functions are not safe and should only be used for data output during debugging ! */

#include <stdio.h>

void saveDoubleVector_csv(const char * filename, double * vec, long N, int append)
{
  FILE * f = NULL;
  if (append)
    f = fopen(filename,"a");
  else
    f = fopen(filename,"w");

  if (f!=NULL) {
    long i;
    for (i=0; i<N-1; i++) 
      fprintf(f, "%f,", vec[i]);
    fprintf(f, "%f\n", vec[i]);
    fclose(f);
  }
}

void saveFloatVector_csv(const char * filename, float * vec, long N, int append)
{
  FILE * f = NULL;
  if (append)
    f = fopen(filename,"ab");
  else
    f = fopen(filename,"wb");

  if (f!=NULL) {
    long i;
    for (i=0; i<N-1; i++) 
      fprintf(f, "%f,", vec[i]);
    fprintf(f, "%f\n", vec[i]);
    fclose(f);
  }
}

void saveFloatDmemVector_csv(const char * filename, FLOAT_DMEM * vec, long N, int append)
{
  FILE * f = NULL;
  if (append)
    f = fopen(filename,"ab");
  else
    f = fopen(filename,"wb");

  if (f!=NULL) {
    long i;
    for (i=0; i<N-1; i++) 
      fprintf(f, "%f,", vec[i]);
    fprintf(f, "%f\n", vec[i]);
    fclose(f);
  }
}


void saveDoubleVector_bin(const char * filename, double * vec, long N, int append)
{
  FILE * f = NULL;
  if (append)
    f = fopen(filename,"ab");
  else
    f = fopen(filename,"wb");

  if (f!=NULL) {
    fwrite(vec, sizeof(double)*N, 1, f);
    fclose(f);
  }
}

void saveFloatVector_bin(const char * filename, float * vec, long N, int append)
{
  FILE * f = NULL;
  if (append)
    f = fopen(filename,"ab");
  else
    f = fopen(filename,"wb");

  if (f!=NULL) {
    fwrite(vec, sizeof(float)*N, 1, f);
    fclose(f);
  }
}

void saveFloatDmemVector_bin(const char * filename, FLOAT_DMEM * vec, long N, int append)
{
  FILE * f = NULL;
  if (append)
    f = fopen(filename,"ab");
  else
    f = fopen(filename,"wb");

  if (f!=NULL) {
    fwrite(vec, sizeof(FLOAT_DMEM)*N, 1, f);
    fclose(f);
  }
}

void saveFloatDmemVectorWlen_bin(const char * filename, FLOAT_DMEM * vec, long N, int append)
{
  FILE * f = NULL;
  if (append)
    f = fopen(filename,"ab");
  else
    f = fopen(filename,"wb");

  if (f!=NULL) {
    FLOAT_DMEM Nf = (FLOAT_DMEM)N;
    fwrite(&Nf, sizeof(FLOAT_DMEM), 1, f);
    fwrite(vec, sizeof(FLOAT_DMEM)*N, 1, f);
    fclose(f);
  }
}

//** HTK 

static int smileHtk_vax = 0;

static __inline void smileHtk_Swap32 ( uint32_t *p )
{
  uint8_t temp,*q;
  q = (uint8_t*) p;
  temp = *q; *q = *( q + 3 ); *( q + 3 ) = temp;
  temp = *( q + 1 ); *( q + 1 ) = *( q + 2 ); *( q + 2 ) = temp;
}

static __inline void smileHtk_Swap16 ( uint16_t *p ) 
{
  uint8_t temp,*q;
  q = (uint8_t*) p;
  temp = *q; *q = *( q + 1 ); *( q + 1 ) = temp;
}

void smileHtk_SwapFloat( float *p )
{
  uint8_t temp,*q;
  q = (uint8_t*) p;
  temp = *q; *q = *( q + 3 ); *( q + 3 ) = temp;
  temp = *( q + 1 ); *( q + 1 ) = *( q + 2 ); *( q + 2 ) = temp;
}

void smileHtk_prepareHeader( sHTKheader *h )
{
  if ( smileHtk_vax ) {
    smileHtk_Swap32 ( &(h->nSamples) );
    smileHtk_Swap32 ( &(h->samplePeriod) );
    smileHtk_Swap16 ( &(h->sampleSize) );
    smileHtk_Swap16 ( &(h->parmKind) );
  }
}

int smileHtk_readHeader(FILE *filehandle, sHTKheader *head)
{
  if (filehandle==NULL) return 0;
  if (!fread(head, sizeof(sHTKheader), 1, filehandle)) {
    fprintf(stderr,"error reading HTK header from file.");
    return 0;
  }
  smileHtk_prepareHeader(head); // convert to host byte order
  return 1;
}

#include <string.h>

int smileHtk_writeHeader(FILE *filehandle, sHTKheader *_head)
{
  sHTKheader head;  // local copy, due to prepareHeader! we don't want to change 'header' variable!
  if (filehandle==NULL) return 0;

  // adjust endianness
  memcpy(&head, _head, sizeof(sHTKheader));
  smileHtk_prepareHeader(&head);

  // seek to beginning of file:
  fseek(filehandle, 0 , SEEK_SET);
  
  // write header:
  if (!fwrite(&head, sizeof(sHTKheader), 1, filehandle)) {
    fprintf(stderr,"Error writing to htk feature file!");
    return 0;
  }

  return 1;
}

int smileHtk_IsVAXOrder ()
{
  short x;
  unsigned char *pc;
  pc = (unsigned char *) &x;
  *pc = 1; *( pc + 1 ) = 0;			// store bytes 1 0
  smileHtk_vax = (x==1);	  		// does it read back as 1?
  return smileHtk_vax; 
}
