/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/

#include <stdio.h>
#include <math.h>
#include <smileutil/zerosolve.h>
#include <core/smileCommon.hpp>

#define MODULE "zerosolve"

// matrix element access helpers
#define MATC(m,i,j,n) ((m)[(i)*(n) + (j)])
#define MATF(m,i,j,n) ((m)[((i)-1)*(n) + ((j)-1)])

const double radix = 2.0;
const double radix2 = radix * radix;

void zerosolveBalanceCmatrix(double *cMat, long nCol)
{
  int converged = 0;
  double normedRow = 0;
  double normedCol = 0;
  while (!converged) {
    double tmp1;
    double tmp2;
    double tmp3;
    converged = 1;
    for (long i = 0; i < nCol; i++) {
      // normalise columns, exclude diagonal elements
      if (i != nCol - 1) {
        normedCol = fabs(MATC(cMat, i + 1, i, nCol));
      } else {
        normedCol = 0.0;
        for (long j = 0; j < nCol - 1; j++) {
          normedCol += fabs(MATC(cMat, j, nCol - 1, nCol));
        }
      }
      // normalise rows, exclude diagonal elements
      if (i == 0) {
        normedRow = fabs(MATC(cMat, 0, nCol - 1, nCol));
      } else if (i == nCol - 1) {
        normedRow = fabs(MATC(cMat, i, i - 1, nCol));
      } else {
        normedRow = (fabs(MATC(cMat, i, i - 1, nCol))
            + fabs(MATC(cMat, i, nCol - 1, nCol)));
      }
      if (normedCol == 0.0 || normedRow == 0.0)
        continue;

      tmp2 = 1.0;
      tmp1 = normedRow / radix;
      tmp3 = normedCol + normedRow;
      while (normedCol < tmp1) {
        tmp2 *= radix;
        normedCol *= radix2;
      }
      tmp1 = normedRow * radix;
      while (normedCol > tmp1) {
        tmp2 /= radix;
        normedCol /= radix2;
      }
      if ((normedRow + normedCol) < 0.95 * tmp3 * tmp2) {
        converged = 0;
        tmp1 = 1.0 / tmp2;
        if (i == 0) {
          MATC(cMat, 0, nCol - 1, nCol) *= tmp1;
        } else {
          MATC(cMat, i, i - 1, nCol) *= tmp1;
          MATC(cMat, i, nCol - 1, nCol) *= tmp1;
        }
        if (i == nCol - 1) {
          for (long j = 0; j < nCol; j++) {
            MATC(cMat, j, i, nCol) *= tmp2;
          }
        } else {
          MATC(cMat, i + 1, i, nCol) *= tmp2;
        }
      }
    }
  }
}

void zerosolveSetCmatrix(const double *a, long nCol, double *cMat)
{
  for (long i = 0; i < nCol; i++) {
    for (long j = 0; j < nCol; j++) {
      MATC(cMat, i, j, nCol) = 0.0;
    }
  }
  for (long i = 1; i < nCol; i++) {
    MATC(cMat, i, i - 1, nCol) = 1.0;
  }
  for (long i = 0; i < nCol; i++) {
    MATC(cMat, i, nCol - 1, nCol) = -a[i] / a[nCol];
  }
}

int zerosolveQRhelper(double *h, long nc,
    tZerosolverComplexNumberPointer xRoot)
{
  long i = 0, j = 0, k = 0, m = 0;
  long e = 0;
  long nIterations = 0;
  long N = nc;
  double w = 0.0, s = 0.0;
  double x = 0.0, y = 0.0, z = 0.0;
  double p = 0.0;
  double q = 0.0;
  double r = 0.0;
  double t = 0.0;
  int isNotLast = 0;
  if (N == 0) {
    return 1;
  }

  while (1) {
    for (e = N; e >= 2; e--) {
      double a1 = fabs(MATF(h, e, e - 1, nc));
      double a2 = fabs(MATF(h, e - 1, e - 1, nc));
      double a3 = fabs(MATF(h, e, e, nc));
      if (a1 <= ZEROSOLVER_DBL_EPSILON * (a2 + a3)) {
        break;
      }
    }
    x = MATF(h, N, N, nc);
    if (e == N) {
      ZEROSOLVER_SET_COMPLEX_NUMBER(xRoot, N - 1, x + t, 0);
      N--;
      if (N == 0) {
        return 1;
      }
      nIterations = 0;
      continue;
    }
    y = MATF(h, N - 1, N - 1, nc);
    w = MATF(h, N - 1, N, nc) * MATF(h, N, N - 1, nc);
    if (e == N - 1) {
      p = (y - x) / 2;
      q = p * p + w;
      y = sqrt(fabs(q));
      x += t;
      if (q > 0) {
        if (p < 0) {
          y = -y;
        }
        y += p;
        ZEROSOLVER_SET_COMPLEX_NUMBER(xRoot, N-1, x - w / y, 0);
        ZEROSOLVER_SET_COMPLEX_NUMBER(xRoot, N-2, x + y, 0);
      } else {
        ZEROSOLVER_SET_COMPLEX_NUMBER(xRoot, N-1, x + p, -y);
        ZEROSOLVER_SET_COMPLEX_NUMBER(xRoot, N-2, x + p, y);
      }
      N -= 2;
      if (N == 0) {
        return 1;
      }
      nIterations = 0;
      continue;
    }

    // No other roots found, one more iteration though
    if (nIterations == 70) {
      return 0;
    }
    if (nIterations % 10 == 0 && nIterations > 0) {
      // shift roots
      t += x;
      for (i = 1; i <= N; i++) {
        MATF(h, i, i, nc) -= x;
      }
      s = fabs(MATF(h, N, N - 1, nc)) + fabs(MATF(h, N - 1, N - 2, nc));
      y = 3.0 / 4.0 * s;
      x = y;
      w = -0.4375 * s * s;
    }
    nIterations++;

    for (m = N - 2; m >= e; m--) {
      double a1, a2, a3;
      z = MATF(h, m, m, nc);
      r = x - z;
      s = y - z;
      p = MATF(h, m, m + 1, nc) + (r * s - w) / MATF(h, m + 1, m, nc);
      q = MATF(h, m + 1, m + 1, nc) - z - r - s;
      r = MATF(h, m + 2, m + 1, nc);
      s = fabs(p) + fabs(q) + fabs(r);
      p /= s;
      q /= s;
      r /= s;

      if (m == e) {
        break;
      }

      a1 = fabs(MATF(h, m, m - 1, nc));
      a2 = fabs(MATF(h, m - 1, m - 1, nc));
      a3 = fabs(MATF(h, m + 1, m + 1, nc));

      if (a1 * (fabs(q) + fabs(r)) <= ZEROSOLVER_DBL_EPSILON * fabs(p) * (a2 + a3)) {
        break;
      }
    }
    for (i = m + 2; i <= N; i++) {
      MATF(h, i, i - 2, nc) = 0;
    }
    for (i = m + 3; i <= N; i++) {
      MATF(h, i, i - 3, nc) = 0;
    }

    // A QR step (twice)
    for (k = m; k <= N - 1; k++) {
      isNotLast = (k != N - 1);
      if (k != m) {
        p = MATF (h, k, k - 1, nc);
        q = MATF (h, k + 1, k - 1, nc);
        if (isNotLast) {
          r = MATF(h, k + 2, k - 1, nc);
        } else {
          r = 0.0;
        }
        x = fabs(p) + fabs(q) + fabs(r);
        if (x == 0) {
          continue;
        }
        p /= x;
        q /= x;
        r /= x;
      }
      s = sqrt(p*p + q*q + r*r);
      if (p < 0) {
        s = -s;
      }
      if (k != m) {
        MATF(h, k, k - 1, nc) = -s * x;
      } else if (e != m) {
        MATF(h, k, k - 1, nc) *= -1;
      }
      p += s;
      z = r / s;
      y = q / s;
      x = p / s;
      r /= p;
      q /= p;

      // QR row modifications
      for (j = k; j <= N; j++) {
        p = MATF(h, k, j, nc) + q * MATF(h, k + 1, j, nc);
        if (isNotLast) {
          p += r * MATF(h, k + 2, j, nc);
          MATF(h, k + 2, j, nc) -= p * z;
        }
        MATF(h, k + 1, j, nc) -= p * y;
        MATF(h, k, j, nc) -= p * x;
      }
      if (k + 3 < N) {
        j = k + 3;
      } else {
        j = N;
      }
      // QR column modifications
      for (i = e; i <= j; i++) {
        p = x * MATF(h, i, k, nc) + y * MATF(h, i, k + 1, nc);
        if (isNotLast) {
          p += z * MATF(h, i, k + 2, nc);
          MATF(h, i, k + 2, nc) -= p * r;
        }
        MATF(h, i, k + 1, nc) -= p * q;
        MATF(h, i, k, nc) -= p;
      }
    }
  }
  return 0;
}

sZerosolverPolynomialComplexWs * zerosolverPolynomialComplexWorkspaceAllocate(long N)
{
  long nCol;
  sZerosolverPolynomialComplexWs * workspace;
  if (N <= 0) {
    SMILE_ERR(1, "zerosolve: matrix size N must be > 0");
    return NULL;
  }
  workspace = (sZerosolverPolynomialComplexWs *)malloc(sizeof(sZerosolverPolynomialComplexWs));
  if (workspace == NULL) {
    SMILE_ERR(1, "zerosolve: failed to allocate workspace memory");
    return NULL;
  }
  nCol = N - 1;
  workspace->nCol = nCol;
  workspace->mat = (double *)malloc(nCol * nCol * sizeof(double));
  if (workspace->mat == NULL) {
    free(workspace);
    SMILE_ERR(1, "zerosolve: failed to allocate workspace matrix array");
    return NULL;
  }
  return workspace;
}

void zerosolverPolynomialComplexWorkspaceFree(sZerosolverPolynomialComplexWs * workspace)
{
  if (workspace != NULL) {
    if (workspace->mat != NULL) {
      free(workspace->mat);
    }
    free(workspace);
  }
}

int zerosolverPolynomialComplexSolve(const double *a, long N,
      sZerosolverPolynomialComplexWs * w, tZerosolverComplexNumberPointer z)
{
  int status = 0;
  double *m = NULL;
  if (N == 0) {
    SMILE_ERR(1, "zerosolve: number of terms must be > 0!");
    return 0;
  }
  if (N == 1) {
    SMILE_ERR(1, "zerosolve: cannot solve for only a single term!");
    return 0;
  }
  if (a[N - 1] == 0.0) {
    SMILE_ERR(1, "zerosolve: first coefficient of polynomial must be != 0.0");
    return 0;
  }
  if (w->nCol != N - 1) {
    SMILE_ERR(1, "zerosolve: dimensionality of workspace does not match the number of polynomial coefficients!");
    return 0;
  }
  m = w->mat;
  zerosolveSetCmatrix(a, N - 1, m);
  zerosolveBalanceCmatrix(m, N - 1);
  status = zerosolveQRhelper(m, N - 1, z);
  if (!status) {
    SMILE_ERR(1, "zerosolve: the QR-method for root solving did not converge!");
    return 0;
  }
  return 1;
}

