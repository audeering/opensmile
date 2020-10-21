/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


#include <stdio.h>
#include <stdlib.h>

/*
  sample code to make a 1-nn model from mfcc feature files (bianry format.. ??)
*/

int main(int argc, char**argv) 
{
  if (argc < 3) {
    printf("Usage: a.out <input mfcc file(s) ...> <output model file>\n");
    return -1;
  }
  int i; long Nv=0;
  float N,nVec;
  float *means = NULL;

  for (i=1; i<argc-1; i++) {
 
  FILE *f= fopen(argv[i],"rb");
  if (f==NULL) { printf("could not open '%s', skipping.\n"); continue; }

  
  fread( &N, sizeof(float), 1, f );
  fread( &nVec, sizeof(float), 1, f );
  int i,j;
  if (means == NULL) means=calloc(1,sizeof(float)*(int)N);
  float *vec=calloc(1,sizeof(float)*(int)N);
  for (i=0; i<(int)nVec; i++) {
    fread( vec, sizeof(float), (int)N, f );
    for(j=0; j<(int)N; j++) {
      means[j] += vec[j];
    }
    Nv++;
  }
  fclose(f);
  free(vec);

  }

  int j;
  for(j=0; j<(int)N; j++) {
    means[j] /= (float)Nv;
  }

  FILE *f = fopen(argv[2],"wb");
  N--;
  fwrite( &N, sizeof(float), 1, f);
  nVec = (float)1.0;
  fwrite( &nVec, sizeof(float), 1, f);
  fwrite( (means+1), sizeof(float), (int)(N-1), f);
  fclose(f);

  free(means);
}

