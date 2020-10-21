/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


/*  SmileUtilSpline
    =========

contains modular DSP functions for splines

*/

#include <smileutil/smileUtilSpline.h>

int smileMath_spline(const double *xval, const double *yval,
  long N, double y1p, double ynp, double *y2, 
  double **workarea)
{
  long i = 0;
  long j = 0;
  double p = 0.0;
  double qn = 0.0;
  double sigma = 0.0;
  double un = 0.0;
  double *u = NULL;

  if (workarea != NULL) {
    u = *workarea;
  }
  if (u == NULL) {
    u = (double*)calloc(1, sizeof(double) * (N - 1));
  }
  if (y1p > 0.99e30) {
    u[0] = 0.0;
    y2[0] = 0.0;
  } else {
    u[0] = (3.0 / (xval[1] - xval[0])) *
        ((yval[1] - yval[0]) / (xval[1] - xval[0]) - y1p);
    y2[0] = -0.5;
  }

  for (i = 1; i < N - 1; i++) {
    sigma = (xval[i] - xval[i - 1]) / (xval[i + 1] - xval[i - 1]);
    p = sigma * y2[i - 1] + 2.0;
    y2[i] = (sigma - 1.0) / p;
    u[i] = (yval[i + 1] - yval[i]) / (xval[i + 1] - xval[i])
        - (yval[i] - yval[i - 1]) / (xval[i] - xval[i - 1]);
    u[i] = 1.0 / p * (6.0 * u[i] / (xval[i + 1] - xval[i - 1])
        - sigma * u[i - 1]);
  }

  if (ynp > 0.99e30) {
    qn = 0.0; 
    un = 0.0;
  } else {
    un = (3.0 / (xval[N - 1] - xval[N - 2])) * (ynp - (yval[N - 1] - yval[N - 2]) 
           / (xval[N - 1] - xval[N - 2]));
    qn = 0.5;
  }

  y2[N - 1] = (un - qn * u[N - 2]) 
               / (qn * y2[N - 2] + 1.0);
  for (j = N - 2; j >= 0; j--) {
    y2[j] = y2[j] * y2[j + 1] + u[j];
  }

  if (workarea == NULL) {
    free(u);
  } else {
    *workarea = u;
  }
  return 1;
}

int smileMath_spline_FLOAT_DMEM(const FLOAT_DMEM *xval, const FLOAT_DMEM *yval,
  long N, FLOAT_DMEM y1p, FLOAT_DMEM ynp, FLOAT_DMEM *y2,
  FLOAT_DMEM **workarea)
{
  long i = 0;
  long j = 0;
  FLOAT_DMEM p = 0.0;
  FLOAT_DMEM qn = 0.0;
  FLOAT_DMEM sigma = 0.0;
  FLOAT_DMEM un = 0.0;
  FLOAT_DMEM *u = NULL;

  if (workarea != NULL) {
    u = *workarea;
  }
  if (u == NULL) {
    u = (FLOAT_DMEM*)calloc(1, sizeof(FLOAT_DMEM) * (N - 1));
  }
  if (y1p > (FLOAT_DMEM)0.99e30) {
    u[0] = (FLOAT_DMEM)0.0;
    y2[0] = (FLOAT_DMEM)0.0;
  } else {
    u[0] = ((FLOAT_DMEM)3.0 / (xval[1] - xval[0]))
        * ((yval[1] - yval[0]) / (xval[1] - xval[0]) - y1p);
    y2[0] = (FLOAT_DMEM)-0.5;
  }

  for (i = 1; i < N - 1; i++) {
    sigma = (xval[i] - xval[i - 1]) / (xval[i + 1] - xval[i - 1]);
    p = sigma * y2[i - 1] + (FLOAT_DMEM)2.0;
    y2[i] = (sigma - (FLOAT_DMEM)1.0) / p;
    u[i] = (yval[i + 1] - yval[i]) / (xval[i + 1] - xval[i])
        - (yval[i] - yval[i - 1]) / (xval[i] - xval[i - 1]);
    u[i] = (FLOAT_DMEM)1.0 / p * ((FLOAT_DMEM)6.0 * u[i] / (xval[i + 1] - xval[i - 1])
        - sigma * u[i - 1]);
  }

  if (ynp > (FLOAT_DMEM)0.99e30) {
    qn = (FLOAT_DMEM)0.0;
    un = (FLOAT_DMEM)0.0;
  } else {
    un = ((FLOAT_DMEM)3.0 / (xval[N - 1] - xval[N - 2])) * (ynp - (yval[N - 1] - yval[N - 2])
           / (xval[N - 1] - xval[N - 2]));
    qn = (FLOAT_DMEM)0.5;
  }

  y2[N - 1] = (un - qn * u[N - 2])
               / (qn * y2[N - 2] + (FLOAT_DMEM)1.0);
  for (j = N - 2; j >= 0; j--) {
    y2[j] = y2[j] * y2[j + 1] + u[j];
  }

  if (workarea == NULL) {
    free(u);
  } else {
    *workarea = u;
  }
  return 1;
}

void smileMath_cspline_init(const double *x, long N, 
    sSmileMath_splineCache *cache)
{
  double *sigma = (double*)malloc(sizeof(double)*N);
  double *diff1 = (double*)malloc(sizeof(double)*N);
  double *diff2 = (double*)malloc(sizeof(double)*N);
  int i;
  for (i=1; i < N - 1; i++) {
    sigma[i] = (x[i] - x[i - 1]) / (x[i + 1] - x[i - 1]);
    diff1[i] = (x[i + 1] - x[i]) * (x[i + 1] - x[i - 1]);
    diff2[i] = (x[i] - x[i - 1]) * (x[i + 1] - x[i - 1]);
  }
  cache->sigma = sigma;
  cache->diff1 = diff1;
  cache->diff2 = diff2;
}

int smileMath_cspline(const double *xval, const double *yval,
  long N, double y1p, double ynp, double *y2, 
  double **workarea, const sSmileMath_splineCache *cache)
{
  long i = 0;
  long j = 0;
  double p = 0.0;
  double qn = 0.0;
  double un = 0.0;
  double *u = NULL;

  if (workarea != NULL) {
    u = *workarea;
  }
  if (u == NULL) {
    u = (double*)calloc(1, sizeof(double) * (N - 1));
  }
  if (y1p > 0.99e30) {
    u[0] = 0.0;
    y2[0] = 0.0;
  } else {
    u[0] = (3.0 / (xval[1] - xval[0])) *
        ((yval[1] - yval[0]) / (xval[1] - xval[0]) - y1p);
    y2[0] = -0.5;
  }

  for (i = 1; i < N - 1; i++) {
    double sigma = cache->sigma[i];
    p = 1.0 / (sigma * y2[i - 1] + 2.0);
    y2[i] = (sigma - 1.0) * p;
    double ut = (yval[i + 1] - yval[i]) / cache->diff1[i] - (yval[i] - yval[i - 1]) / cache->diff2[i];
    u[i] = p * (6.0 * ut - sigma * u[i - 1]);
  }

  if (ynp > 0.99e30) {
    qn = 0.0; 
    un = 0.0;
  } else {
    un = (3.0 / (xval[N - 1] - xval[N - 2])) * (ynp - (yval[N - 1] - yval[N - 2]) 
           / (xval[N - 1] - xval[N - 2]));
    qn = 0.5;
  }

  y2[N - 1] = (un - qn * u[N - 2]) 
               / (qn * y2[N - 2] + 1.0);
  for (j = N - 2; j >= 0; j--) {
    y2[j] = y2[j] * y2[j + 1] + u[j];
  }

  if (workarea == NULL) {
    free(u);
  } else {
    *workarea = u;
  }

  return 1;
}

void smileMath_cspline_free(sSmileMath_splineCache *cache)
{
  free(cache->sigma);
  free(cache->diff1);
  free(cache->diff2);
}

/* smileMath_splint: 
    Does spline interpolation of y value for a given x value of function ya=f(xa).

    xorig[1..n] and yorig[1..n] contain the function f to be interpolated,
     --> yorig[i] = f(xorig[i]), where xorig[1] < xorig[2] < ... < xorig[n]
    y2[1..n] is the output of smileMath_spline(...) above, which contains the second order derivatives.
    N contains the length of both xorig and yorig arrays.
    'x' holds the x position at which to interpolate *y = f(x)
*/
int smileMath_splint(const double *xorig, const double *yorig, const double *y2, long N,
    double x, double *y)
{
  long klower;
  long kupper;
  long kcurrent;
  double range;
  double b;
  double a;

  // binary search over full xorig array
  klower = 0;
  kupper = N - 1;
  while (kupper - klower > 1) {
    kcurrent = (kupper + klower) / 2;
    if (xorig[kcurrent] > x) {
      kupper = kcurrent;
    } else {
      klower = kcurrent;
    }
  }    
  range = xorig[kupper] - xorig[klower];
  if (range == 0.0) {
    printf("smileMath_splint(): bad input (range == 0)!\n");
    return 0;
  }
  a = (xorig[kupper] - x) / range;  
  b = 1.0 - a;
  *y = a * yorig[klower] + b * yorig[kupper] + ((a * a * a - a) * y2[klower] +
    (b * b * b - b) * y2[kupper]) * (range * range) / 6.0;
  return 1;
}

int smileMath_splint_FLOAT_DMEM(const FLOAT_DMEM *xorig, const FLOAT_DMEM *yorig,
    const FLOAT_DMEM *y2, long N, FLOAT_DMEM x, FLOAT_DMEM *y)
{
  long klower;
  long kupper;
  long kcurrent;
  FLOAT_DMEM range;
  FLOAT_DMEM b;
  FLOAT_DMEM a;

  // binary search over full xorig array
  klower = 0;
  kupper = N - 1;
  while (kupper - klower > 1) {
    kcurrent = (kupper + klower) / 2;
    if (xorig[kcurrent] > x) {
      kupper = kcurrent;
    } else {
      klower = kcurrent;
    }
  }
  range = xorig[kupper] - xorig[klower];
  if (range == 0.0) {
    printf("smileMath_splint(): bad input (range == 0)!\n");
    return 0;
  }
  a = (xorig[kupper] - x) / range;
  b = (FLOAT_DMEM)1.0 - a;
  *y = a * yorig[klower] + b * yorig[kupper] + ((a * a * a - a) * y2[klower] +
    (b * b * b - b) * y2[kupper]) * (range * range) / (FLOAT_DMEM)6.0;
  return 1;
}

int smileMath_csplint_init(const double *xorig, long N, const double *x, long Nx, 
    sSmileMath_splintCache *cache)
{
  int i;
  long kupper = 1;
  long *k = (long*)malloc(sizeof(long) * Nx);
  double *coeffs = (double*)malloc(sizeof(double) * Nx * 3);
  if (x[0] < xorig[0] || x[Nx - 1] > xorig[N - 1]) {
    printf("smileMath_csplint_init(): x out of range!\n");
    goto error;
  }
  for (i = 0; i < Nx; i++) {
    long klower;
    // linear search starting from last kupper
    while (kupper < N && xorig[kupper] < x[i]) {
      kupper++;
    }    
    if (kupper == N) {
      //kupper = N - 1;
      printf("smileMath_csplint_init(): x out of range!\n");
      goto error;
    }
    klower = kupper - 1;
    k[i] = klower;

    double range = xorig[kupper] - xorig[klower];
    if (range == 0.0) {
      printf("smileMath_csplint_init(): bad input (range == 0)!\n");
      goto error;
    }
    double a = (xorig[kupper] - x[i]) / range;
    double b = 1.0 - a;
    double range2 = range * range / 6.0;
    double c = (a * a * a - a) * range2;
    double d = (b * b * b - b) * range2;    
    coeffs[i * 3] = a;
    coeffs[i * 3 + 1] = c;
    coeffs[i * 3 + 2] = d;
  }
  cache->nx = Nx;
  cache->k = k;
  cache->coeffs = coeffs;
  return 1;
error:
  free(k);
  free(coeffs);
  return 0;
}

void smileMath_csplint(const double *yorig, const double *y2, 
  const sSmileMath_splintCache *cache, double *y)
{
  int i;
  const long *k = cache->k;
  const double *coeffs = cache->coeffs;
  for (i = 0; i < cache->nx; i++) {
    double a = coeffs[i * 3];
    double b = 1.0 - a;
    double c = coeffs[i * 3 + 1];
    double d = coeffs[i * 3 + 2];
    y[i] = a * yorig[k[i]] + b * yorig[k[i] + 1] + c * y2[k[i]] + d * y2[k[i] + 1];
  }
}

void smileMath_csplint_free(sSmileMath_splintCache *cache)
{
  free(cache->k);
  free(cache->coeffs);
}
