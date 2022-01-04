/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


#ifndef __SMILE_UTIL_SPLINE_H
#define __SMILE_UTIL_SPLINE_H

/* you may remove this include if you are using smileUtil outside of openSMILE */
//#include <smileCommon.hpp>
/* --------------------------------------------------------------------------- */
#ifndef __SMILE_COMMON_H
#define __SMILE_COMMON_H

#include <stdlib.h>
#include <stdio.h>

#endif  // __SMILE_COMMON_H


#include <core/smileTypes.h>


/****** spline functions ******/

/* smileMath_spline: evaluates a spline function:
    Given two arrays x[1..n] and y[1..n] containing a tabulated function, that is
    y[i] = f(x[i]), with x[1] < x[2] < ... < x[n], and the values yp1 and
    ypn for the first derivative of the interpolating function at points
    1 and n, respectively, this function returns an array y2[1..n] which
    contains the second derivative of the interpolating function at the
    point x.
    If yp1 and / or ypn are greater or equal than 10^30,
    the corresponding boundary condition for a natural spline is set, with
    a zero second derivative on that boundary.
    u is an optional pointer to a workspace area (smileMath_spline will
    allocate a vector there if the pointer pointed to is NULL).
    The calling code is responsible of freeing this memory with free() at any
    later time which seems convenient (i.e. at the end of all calculations).
*/
int smileMath_spline(const double *x, const double *y,
    long n, double yp1, double ypn, double *y2, double **workspace);

int smileMath_spline_FLOAT_DMEM(const FLOAT_DMEM *x, const FLOAT_DMEM *y,
    long n, FLOAT_DMEM yp1, FLOAT_DMEM ypn, FLOAT_DMEM *y2, FLOAT_DMEM **workspace);

/* smileMath_cspline: cached variant of smileMath_spline:
    If smileMath_spline needs to be evaluated multiple times with
    constant x but varying y, this variant should be used instead for better
    performance. It caches internal computations that only depend on x so that 
    the results can be reused.
    Initialize the cache by calling smileMath_cspline_init, then call
    smileMath_cspline an arbitrary number of times (x must be the same for all
    evaluations), finally clean up by calling smileMath_cspline_free.
*/

typedef struct {
    double *sigma;
    double *diff1;
    double *diff2;
} sSmileMath_splineCache;

void smileMath_cspline_init(const double *x, long n, 
    sSmileMath_splineCache *cache);

int smileMath_cspline(const double *x, const double *y,
    long n, double yp1, double ypn, double *y2, double **workspace,
    const sSmileMath_splineCache *cache);

void smileMath_cspline_free(sSmileMath_splineCache *cache);

/* smileMath_splint:
    Does spline interpolation of y for a given x value of function ya=f(xa).

    xa[1..n] and ya[1..n] contain the function to be interpolated,
    i.e., ya[i] = f(xa[i]), with xa[1] < xa[2] < ... < xa[n].
    y2a[1..n] is the output of smileMath_spline(...) above,
    containing the second derivatives.
    n contains the length of xa and ya
    x holds the position at which to interpolate *y = f(x).
*/
int smileMath_splint(const double *xa, const double *ya,
    const double *y2a, long n, double x, double *y);

int smileMath_splint_FLOAT_DMEM(const FLOAT_DMEM *xa, const FLOAT_DMEM *ya,
    const FLOAT_DMEM *y2a, long n, FLOAT_DMEM x, FLOAT_DMEM *y);

/* smileMath_csplint:
    Optimized implementation of smileMath_splint for cases where multiple splines have
    to be constructed and evaluated at the same points (xa and x).
    Initialize the cache by calling smileMath_csplint_init, passing xa and x. 
    Then, perform an arbitrary number of spline interpolations by calling 
    smileMath_csplint. Finally, clean up by calling smileMath_csplint_free. 
*/
typedef struct {
    long nx;
    long *k;
    double *coeffs;
} sSmileMath_splintCache;

int smileMath_csplint_init(const double *xa, long n, const double *x, 
    long nx, sSmileMath_splintCache *cache);

void smileMath_csplint(const double *ya, const double *y2a,
    const sSmileMath_splintCache *cache, double *y);

void smileMath_csplint_free(sSmileMath_splintCache *cache);

#endif // __SMILE_UTIL_SPLINE_H

