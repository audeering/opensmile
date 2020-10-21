/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/

#ifndef __SMILEUTIL_ZEROSOLVER_H
#define __SMILEUTIL_ZEROSOLVER_H

#include <stdlib.h>

typedef struct {
  long double val[2];
} tZerosolverComplexlDouble;

typedef struct {
  double val[2];
} tZerosolverComplex;

typedef struct {
  float val[2];
} tZerosolverComplexFloat;

typedef double *  tZerosolverComplexNumber;
typedef float * tZerosolverComplexNumberFloat;
typedef long double * tZerosolverComplexNumberlDouble;

typedef const double * tZerosolverConstComplexNumber;
typedef const float * tZerosolverConstComplexNumberFloat;
typedef const long double * tZerosolverConstComplexNumberlDouble;

typedef tZerosolverComplexNumber tZerosolverComplexNumberArray;
typedef tZerosolverComplexNumberFloat tZerosolverComplexNumberFloatArray;
typedef tZerosolverComplexNumberlDouble  tZerosolverComplexNumberlDoubleArray;

typedef const double * tZerosolverConstComplexNumberArray;
typedef const float * tZerosolverConstComplexNumberArrayFloat;
typedef const long double * tZerosolverConstComplexNumberArraylDouble;

typedef tZerosolverComplexNumber tZerosolverComplexNumberPointer;
typedef tZerosolverComplexNumberFloat tZerosolverComplexNumberFloatPointer;
typedef tZerosolverComplexNumberlDouble tZerosolverComplexNumberlDoublePointer;

typedef const double * tZerosolverConstComplexNumberPointer;
typedef const float * tZerosolverConstComplexNumberFloatPointer;
typedef const long double * tZerosolverConstComplexNumberlDoublePointer;


typedef struct {
  long nCol;
  double * mat;
} sZerosolverPolynomialComplexWs;

#define ZEROSOLVER_REAL(z)     ((z).val[0])
#define ZEROSOLVER_IMAG(z)     ((z).val[1])
#define ZEROSOLVER_COMPLEX_POINTER(zp) ((zp)->val)
#define ZEROSOLVER_COMPLEX_POINTER_REAL(zp)  ((zp)->val[0])
#define ZEROSOLVER_COMPLEX_POINTER_IMAG(zp)  ((zp)->val[1])
#define ZEROSOLVER_COMPLEX_EQUAL(z1,z2) (((z1).val[0] == (z2).val[0]) && ((z1).val[1] == (z2).val[1]))
#define ZEROSOLVER_SET_COMPLEX(zptr,x,y) {(zptr)->val[0] = (x); (zptr)->val[1] = (y); }
#define ZEROSOLVER_SET_REAL(zptr,x)      {(zptr)->val[0] = (x); }
#define ZEROSOLVER_SET_IMAG(zptr,y)      {(zptr)->val[1] = (y); }
#define ZEROSOLVER_SET_COMPLEX_NUMBER(zptr,n,x,y)   { *((zptr) + 2 * (n)) = (x); *((zptr) + (2 * (n) + 1)) = (y); }

#define ZEROSOLVER_DBL_EPSILON 2.2204460492503131e-16

// Evaluates the polynomial
// a[0] + a[1] * x + a[2] * x^2 + ... + a[N - 1] * x^(N-1)
__inline double zerosolverEvaluatePolynomial(const double a[], const int N, const double x)
{
  double res = a[N-1];
  int i = N - 1;
  for (; i > 0; i--)
    res = a[i-1] + x * res;
  return res;
}

__inline tZerosolverComplex zerosolverEvaluateComplexPolynomialComplex(const tZerosolverComplex a[],
    const int N, const tZerosolverComplex x)
{
  tZerosolverComplex res = a[N-1];
  int i = N - 1;
  for (; i > 0; i--) {
    double tmp = ZEROSOLVER_REAL(a[i-1]) + ZEROSOLVER_REAL(x) * ZEROSOLVER_REAL(res)
        - ZEROSOLVER_IMAG(x) * ZEROSOLVER_IMAG(res);
    ZEROSOLVER_SET_IMAG(&res, ZEROSOLVER_IMAG(a[i - 1]) + ZEROSOLVER_IMAG(x) * ZEROSOLVER_REAL(res)
        + ZEROSOLVER_REAL(x) * ZEROSOLVER_IMAG(res));
    ZEROSOLVER_SET_REAL(&res, tmp);
  }
  return res;
}

__inline tZerosolverComplex zerosolverEvaluatePolynomialComplex(const double a[],
    const int N, const tZerosolverComplex x)
{
  tZerosolverComplex res;
  ZEROSOLVER_SET_COMPLEX (&res, a[N-1], 0.0);
  int i = N - 1;
  for(; i > 0; i--) {
    double tmp = a[i - 1] + ZEROSOLVER_REAL(x) * ZEROSOLVER_REAL(res)
        - ZEROSOLVER_IMAG(x) * ZEROSOLVER_IMAG(res);
    ZEROSOLVER_SET_IMAG(&res, ZEROSOLVER_IMAG(x) * ZEROSOLVER_REAL(res)
        + ZEROSOLVER_REAL(x) * ZEROSOLVER_IMAG(res));
    ZEROSOLVER_SET_REAL(&res, tmp);
  }
  return res;
}

// Allocates the workspace
sZerosolverPolynomialComplexWs * zerosolverPolynomialComplexWorkspaceAllocate(long N);
// Frees the workspace
void zerosolverPolynomialComplexWorkspaceFree(sZerosolverPolynomialComplexWs * w);

// Solves for the complex roots of a real polynomial given by *a
int zerosolverPolynomialComplexSolve (const double * a, long N,
    sZerosolverPolynomialComplexWs * w, tZerosolverComplexNumberPointer x);


#endif  // __SMILEUTIL_ZEROSOLVER_H
