/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/

/*****************************************************************
  This code uses functions from the Speex codec library,
  which are distributed under the following license terms:

Copyright 2002-2008     Xiph.org Foundation
Copyright 2002-2008     Jean-Marc Valin
Copyright 2005-2007     Analog Devices Inc.
Copyright 2005-2008     Commonwealth Scientific and Industrial Research
                        Organisation (CSIRO)
Copyright 1993, 2002, 2006 David Rowe
Copyright 2003          EpicGames
Copyright 1992-1994     Jutta Degener, Carsten Bormann

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:

- Redistributions of source code must retain the above copyright
notice, this list of conditions and the following disclaimer.

- Redistributions in binary form must reproduce the above copyright
notice, this list of conditions and the following disclaimer in the
documentation and/or other materials provided with the distribution.

- Neither the name of the Xiph.org Foundation nor the names of its
contributors may be used to endorse or promote products derived from
this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

****************************/

/*  openSMILE component: cLsp
Computes LSP (line spectral pairs) from LPC coefficients
*/


#include <lld/lsp.hpp>
#include <math.h>

#define MODULE "cLsp"

SMILECOMPONENT_STATICS(cLsp)

SMILECOMPONENT_REGCOMP(cLsp)
{
  SMILECOMPONENT_REGCOMP_INIT
  scname = COMPONENT_NAME_CLSP;
  sdescription = COMPONENT_DESCRIPTION_CLSP;

  // we inherit cVectorProcessor configType and extend it:
  SMILECOMPONENT_INHERIT_CONFIGTYPE("cVectorProcessor")
  
  SMILECOMPONENT_IFNOTREGAGAIN(
    //ct->setField("p","order of predictor (= number of lpc coefficients)",8);
    ct->setField("processArrayFields",NULL,0);
  )

  SMILECOMPONENT_MAKEINFO(cLsp);
}

SMILECOMPONENT_CREATE(cLsp)

//-----

cLsp::cLsp(const char *_name) :
  cVectorProcessor(_name),
  nLpc(0),
  lpcIdx(-1)
{
}

void cLsp::myFetchConfig()
{
  cVectorProcessor::myFetchConfig();

  //p=getInt("p");
  //SMILE_IDBG(2,"predictor order p = %i",p); 
}

/*
int cLsp::setupNamesForField(int i, const char*name, long nEl)
{
  return cDataProcessor::setupNamesForField(i,name,nEl);
}
*/

/*
int cLsp::myFinaliseInstance()
{
//  frameSizeFrames = reader->getLevelN();
  return cVectorProcessor::myFinaliseInstance();
}
*/

FLOAT_DMEM cLsp::cheb_poly_eva(FLOAT_DMEM *coef, FLOAT_DMEM x, int m)
{
   int k;
   FLOAT_DMEM b0, b1, tmp;

   /* Initial conditions */
   b0=0; /* b_(m+1) */
   b1=0; /* b_(m+2) */
   x*=2;
   /* Calculate the b_(k) */
   for (k=m;k>0;k--) {
      tmp=b0;                           /* tmp holds the previous value of b0 */
      b0=x*b0-b1+coef[m-k];    /* b0 holds its new value based on b0 and b1 */
      b1=tmp;                           /* b1 holds the previous value of b0 */
   }
   return(-b1+(FLOAT_DMEM)0.5*x*b0+coef[m]);
}

#define SIGN_CHANGE(a,b) (((a)*(b))<0.0)
#define LPC_SCALING  1.f
#define FREQ_SCALE 1.0
#define X2ANGLE(x) (acos(x))

#define LSP_DELTA1 .2
#define LSP_DELTA2 .05

/*  float *a                    lpc coefficients                        */
/*  int lpcrdr                  order of LPC coefficients (10)          */
/*  float *freq                 LSP frequencies in the x domain         */
/*  int nb                      number of sub-intervals (4)             */
/*  float delta                 grid spacing interval (0.02)            */
int cLsp::lpc_to_lsp (const FLOAT_DMEM *a, int lpcrdr, FLOAT_DMEM *freq, int nb, FLOAT_DMEM delta)
{
    FLOAT_DMEM temp_xr,xl,xr,xm=0;
    FLOAT_DMEM psuml,psumr,psumm,temp_psumr /*,temp_qsumr*/;
    int i,j,m,flag,k;
    FLOAT_DMEM *Q = NULL;                   /* ptrs for memory allocation           */
    FLOAT_DMEM *P = NULL;
    FLOAT_DMEM *Q16 = NULL;         /* ptrs for memory allocation           */
    FLOAT_DMEM *P16 = NULL;
    FLOAT_DMEM *px;                   /* ptrs of respective P'(z) & Q'(z)     */
    FLOAT_DMEM *qx;
    FLOAT_DMEM *p;
    FLOAT_DMEM *q;
    FLOAT_DMEM *pt;                   /* ptr used for cheb_poly_eval()
                                whether P' or Q'                        */
    int roots=0;                /* DR 8/2/94: number of roots found     */
    flag = 1;                   /*  program is searching for a root when,
                                1 else has found one                    */
    m = lpcrdr/2;               /* order of P'(z) & Q'(z) polynomials   */

    /* Allocate memory space for polynomials */
    Q = (FLOAT_DMEM*) calloc(1,sizeof(FLOAT_DMEM)*(m+1));
    P = (FLOAT_DMEM*) calloc(1,sizeof(FLOAT_DMEM)*(m+1));

    /* determine P'(z)'s and Q'(z)'s coefficients where
      P'(z) = P(z)/(1 + z^(-1)) and Q'(z) = Q(z)/(1-z^(-1)) */

    px = P;                      /* initialise ptrs                     */
    qx = Q;
    p = px;
    q = qx;

    *px++ = LPC_SCALING;
    *qx++ = LPC_SCALING;
    for(i=0;i<m;i++) {
       *px++ = (a[i]+a[lpcrdr-1-i]) - *p++;
       *qx++ = (a[i]-a[lpcrdr-1-i]) + *q++;
    }
    px = P;
    qx = Q;
    for(i=0;i<m;i++) {
       *px = 2**px;
       *qx = 2**qx;
       px++;
       qx++;
    }

    px = P;                     /* re-initialise ptrs                   */
    qx = Q;
    /* now that we have computed P and Q convert to 16 bits to
       speed up cheb_poly_eval */
    P16 = P;
    Q16 = Q;

    /* Search for a zero in P'(z) polynomial first and then alternate to Q'(z).
    Keep alternating between the two polynomials as each zero is found  */

    xr = 0;                     /* initialise xr to zero                */
    xl = FREQ_SCALE;                    /* start at point xl = 1                */

    for(j=0;j<lpcrdr;j++){
        if(j&1)                 /* determines whether P' or Q' is eval. */
            pt = Q16;
        else
            pt = P16;

        psuml = cheb_poly_eva(pt,xl,m);   /* evals poly. at xl    */
        flag = 1;
        while(flag && (xr >= -FREQ_SCALE)){
           FLOAT_DMEM dd;
           /* Modified by JMV to provide smaller steps around x=+-1 */

           dd=delta*((FLOAT_DMEM)1.0-(FLOAT_DMEM)0.9*xl*xl);
           if (fabs(psuml)<.2)
              dd *= (FLOAT_DMEM)0.5;

           xr = xl - dd;                          /* interval spacing     */
            psumr = cheb_poly_eva(pt,xr,m);/* poly(xl-delta_x)    */
            temp_psumr = psumr;
            temp_xr = xr;

   /* if no sign change increment xr and re-evaluate poly(xr). Repeat til
    sign change.
    if a sign change has occurred the interval is bisected and then
    checked again for a sign change which determines in which
    interval the zero lies in.
    If there is no sign change between poly(xm) and poly(xl) set interval
    between xm and xr else set interval between xl and xr and repeat till
    root is located within the specified limits                         */

            if(SIGN_CHANGE(psumr,psuml))
            {
                roots++;

                psumm=psuml;
                for(k=0;k<=nb;k++){
                    xm = (FLOAT_DMEM)0.5*(xl+xr);            /* bisect the interval  */
                    psumm=cheb_poly_eva(pt,xm,m);
                    /*if(psumm*psuml>0.)*/
                    if(!SIGN_CHANGE(psumm,psuml))
                    {
                        psuml=psumm;
                        xl=xm;
                    } else {
                        psumr=psumm;
                        xr=xm;
                    }
                }

               /* once zero is found, reset initial interval to xr      */
               if (xm>1.0) { xm = 1.0; } /* <- fixed a possible NAN issue here...? */
               else if (xm<-1.0) { xm = -1.0; }
               freq[j] = X2ANGLE(xm);
               xl = xm;
               flag = 0;                /* reset flag for next search   */
            }
            else{
                psuml=temp_psumr;
                xl=temp_xr;
            }
        }
    }
    if (P!=NULL) free(P);
    if (Q!=NULL) free(Q);
    return(roots);
}

int cLsp::setupNewNames(long nEl)
{
  nLpc = 0;
  const FrameMetaInfo *fmeta = reader_->getFrameMetaInfo();
  if (fmeta != NULL) {
    long field = fmeta->findFieldByPartialName( "lpcCoeff" );
    if (field >= 0) {
      lpcIdx = fmeta->fieldToElementIdx( field );
      nLpc = fmeta->field[field].N;
      writer_->addField( "lspFreq", nLpc );
      namesAreSet_=1;
      return nLpc;
    }
  }
  SMILE_IERR(1,"could not find required field 'lpcCoeff' in input data!");
  return nLpc;
}

int cLsp::processVector(const FLOAT_DMEM *src, FLOAT_DMEM *dst, long Nsrc, long Ndst, int idxi) // idxi=input field index
{
  // (TODO:) check if Ndst >= Nsrc!
  if (Ndst < Nsrc ) return 0;

  if ((lpcIdx == -1)||(nLpc <= 0)) {
    //if (!warned) SMILE_IWRN(1,"field 'lpcCoeff' not found in input (is the read level an lpc level ??)");
    // warned = 1;
    return 0;
  }

  /* LPC to LSPs (x-domain) transform */
  int roots;
  roots = lpc_to_lsp (src+lpcIdx, nLpc, dst, 10, (FLOAT_DMEM)LSP_DELTA1);
  if (roots!=nLpc) {
    roots = lpc_to_lsp (src+lpcIdx, nLpc, dst, 10, (FLOAT_DMEM)LSP_DELTA2);  // nLpc was Nsrc
    if (roots!=nLpc) {
      int i;
      for (i=roots;i<nLpc;i++) {
        dst[i]=0.0;
      }
    }
  }

  return 1;
}


cLsp::~cLsp()
{
}


