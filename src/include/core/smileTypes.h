/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


/*  openSMILE type definitions */


#ifndef __SMILE_TYPES_H
#define __SMILE_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

#include <inttypes.h>
#include <float.h>

/* opensmile internal types */
#define FLOAT_DMEM_FLOAT  0
#define FLOAT_DMEM_DOUBLE  1


// this defines the float type used throughout the data memory, either 'float' or 'double'
typedef float FLOAT_DMEM;
#define FLOAT_DMEM_NUM  FLOAT_DMEM_FLOAT // this numeric constant MUST equal the float type set above ...
                                           // 0 = float, 1 = double:
#define FLOAT_DMEM_MAX FLT_MAX
#define FLOAT_DMEM_MIN FLT_MIN

#ifdef __cplusplus
}
#endif

#endif // __SMILE_TYPES_H

