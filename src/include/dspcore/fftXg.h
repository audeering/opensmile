/*
 * http://www.kurims.kyoto-u.ac.jp/~ooura/fft.html
 * Copyright Takuya OOURA, 1996-2001
 *
 * You may use, copy, modify and distribute this code for any purpose (include
 * commercial use) and without fee. Please refer to this package when you modify
 * this code.
 *
 * Changes:
 * Type modifications and ifdefs by the openSMILE authors.
 */

#ifndef __FFTXG_H
#define __FFTXG_H

#define FLOAT_TYPE_FFT float
#define FLOAT_FFT_NUM  0      // 0 = float, 1 = double

#ifdef __cplusplus
extern "C" 
{
#endif

  void cdft(int n, int isgn, FLOAT_TYPE_FFT *a, int *ip, FLOAT_TYPE_FFT *w);
  void rdft(int n, int isgn, FLOAT_TYPE_FFT *a, int *ip, FLOAT_TYPE_FFT *w);

#ifdef __cplusplus
}
#endif

#endif // __FFTXG_H
