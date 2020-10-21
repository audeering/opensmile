/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


/*

read in one ARFF file, compute mean and variance and standardize + save normadta 
compute mean and variance 

*/


#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <inttypes.h>
#include <math.h>



typedef struct {
  long dim;
  float *x;
} sVectorFloat;
typedef sVectorFloat * vectorFloat;

typedef struct {
  long dim;
  double *x;
} sVectorDouble;
typedef sVectorDouble * vectorDouble;

typedef struct {
  long dim;
  void *x;
} sVector;
typedef sVector * vector;

typedef struct {
  int nDim;
  long *dim;
  long long els;
  float *x;
} sMatrixFloat;
typedef sMatrixFloat * matrixFloat;

typedef struct {
  long rows, cols;
  long long els;
  float *x;
} sMatrix2DFloat;
typedef sMatrix2DFloat * matrix2DFloat;


inline vector vectorCreate(long dim, int elSize) // elSize : size of vector element in bytes
{
  vector ret = (vector)malloc(sizeof(sVector));
  if (ret == NULL) return NULL;
  ret->x = calloc(1,elSize*dim);
  ret->dim = dim;
  return ret;
}

inline vectorFloat vectorFloatCreate(long dim) 
{
  vectorFloat ret = (vectorFloat)malloc(sizeof(sVectorFloat));
  if (ret == NULL) return NULL;
  ret->x = (float *)calloc(1,sizeof(float)*dim);
  ret->dim = dim;
  return ret;
}

inline vectorDouble vectorDoubleCreate(long dim) 
{
  vectorDouble ret = (vectorDouble)malloc(sizeof(sVectorDouble));
  if (ret == NULL) return NULL;
  ret->x = (double *)calloc(1,sizeof(double)*dim);
  ret->dim = dim;
  return ret;
}

inline vectorFloat vectorFloatDestroy(vectorFloat vec)
{
  if (vec != NULL) {
    if (vec->x != NULL) free(vec->x);
    free(vec);
  }
  return NULL;       
}

inline vectorDouble vectorDoubleDestroy(vectorDouble vec)
{
  if (vec != NULL) {
    if (vec->x != NULL) free(vec->x);
    free(vec);
  }
  return NULL;       
}

matrixFloat matrixFloatCreate(int nDim, long *dim) 
{
  int i;
  matrixFloat ret = (matrixFloat)malloc(sizeof(sMatrixFloat));
  if (ret == NULL) return NULL;
  ret->els = 1;
  for (i=0; i<nDim; i++) {
    ret->els *= dim[i];    
  }
  ret->x = (float *)calloc(1,sizeof(float)*ret->els);
  ret->dim = dim;
  return ret;
}

matrix2DFloat matrix2DFloatCreate(long rows, long cols) 
{
  matrix2DFloat ret = (matrix2DFloat)malloc(sizeof(sMatrix2DFloat));
  if (ret == NULL) return NULL;
  ret->els = rows*cols;
  ret->rows = rows;
  ret->cols = cols;
  ret->x = (float *)calloc(1,sizeof(float)*ret->els);
  return ret;
}

matrix2DFloat matrix2DFloatDestroy(matrix2DFloat mat) 
{
  if (mat == NULL) return NULL;
  if (mat->x != NULL) free(mat->x);
  free(mat);
  return NULL;
}



inline float minFloat(float a, float b)
{
  if (a < b) return a;
  else return b;         
}

inline long minLong(long a, long b)
{
  if (a < b) return a;
  else return b;         
}

vectorFloat getMatrix2DFloatRow(matrix2DFloat mat, long row)
{
  if ((mat != NULL)&&(mat->x != NULL)) {
    vectorFloat r = vectorFloatCreate(mat->cols);
    if (r == NULL) return NULL;
    memcpy(r->x, mat->x + (row * (mat->cols)), (mat->cols) * sizeof(float));
    return r;
  }
  return NULL;
}

void setMatrix2DFloatRow(matrix2DFloat mat, long row, vectorFloat r)
{
  if ((mat != NULL)&&(mat->x != NULL)) {
    if (r == NULL) return;
    memcpy(mat->x + (row * (mat->cols)), r->x , (mat->cols) * sizeof(float));
  }
}

// add vector b onto vector a
void vectorFloatAdd(vectorFloat a, vectorFloat b)
{
  if ((a!= NULL)&&(a->x != NULL)&&(b!= NULL)&&(b->x != NULL)) {
    long i;
    long lng = minLong(a->dim, b->dim);
    for (i=0; i<lng; i++) {
      a->x[i] += b->x[i];    
    }
  }
}

// subtract vector b from vector a
void vectorFloatSub(vectorFloat a, vectorFloat b)
{
  if ((a!= NULL)&&(a->x != NULL)&&(b!= NULL)&&(b->x != NULL)) {
    long i;
    long lng = minLong(a->dim, b->dim);
    for (i=0; i<lng; i++) {
      a->x[i] -= b->x[i];    
    }
  }
}

// add vector b onto vector a
void vectorDoubleAdd(vectorDouble a, vectorDouble b)
{
  if ((a!= NULL)&&(a->x != NULL)&&(b!= NULL)&&(b->x != NULL)) {
    long i;
    long lng = minLong(a->dim, b->dim);
    for (i=0; i<lng; i++) {
      a->x[i] += b->x[i];    
    }
  }
}

// subtract vector b from vector a
void vectorDoubleSub(vectorDouble a, vectorDouble b)
{
  if ((a!= NULL)&&(a->x != NULL)&&(b!= NULL)&&(b->x != NULL)) {
    long i;
    long lng = minLong(a->dim, b->dim);
    for (i=0; i<lng; i++) {
      a->x[i] -= b->x[i];    
    }
  }
}

// divide elements in vector a by corresponding elements in vector b, save in a
void vectorFloatElemDiv(vectorFloat a, vectorFloat b)
{
  if ((a!= NULL)&&(a->x != NULL)&&(b!= NULL)&&(b->x != NULL)) {
    long i;
    long lng = minLong(a->dim, b->dim);
    for (i=0; i<lng; i++) {
      a->x[i] /= b->x[i];    
    }
  }
}

// divide elements in vector a by corresponding elements in vector b, save in a
void vectorDoubleElemDiv(vectorDouble a, vectorDouble b)
{
  if ((a!= NULL)&&(a->x != NULL)&&(b!= NULL)&&(b->x != NULL)) {
    long i;
    long lng = minLong(a->dim, b->dim);
    for (i=0; i<lng; i++) {
      a->x[i] /= b->x[i];    
    }
  }
}

// divide elements in vector a by corresponding elements in vector b, save in a
// SAFE: do not dived by 0, result will then be 0
void vectorDoubleElemSafeDiv(vectorDouble a, vectorDouble b)
{
  if ((a!= NULL)&&(a->x != NULL)&&(b!= NULL)&&(b->x != NULL)) {
    long i;
    long lng = minLong(a->dim, b->dim);
    for (i=0; i<lng; i++) {
      if (b->x[i] != 0.0)
        a->x[i] /= b->x[i];    
      else 
        a->x[i] = 0.0;
    }
  }
}

vectorDouble vectorFloatToVectorDouble(vectorFloat a)
{
  if ((a!= NULL)&&(a->x != NULL)) {
    long i;
    vectorDouble d = vectorDoubleCreate(a->dim);
    if (d==NULL) return NULL;
    for (i=0; i<a->dim; i++) {
      d->x[i] = (double)(a->x[i]);
    }
    return d;
  }
  return NULL;
}

vectorFloat vectorDoubleToVectorFloat(vectorDouble a)
{
  if ((a!= NULL)&&(a->x != NULL)) {
    long i;
    vectorFloat d = vectorFloatCreate(a->dim);
    if (d==NULL) return NULL;
    for (i=0; i<a->dim; i++) {
      d->x[i] = (float)(a->x[i]);
    }
    return d;
  }
  return NULL;
}

void vectorDoubleScalarDiv(vectorDouble vec, long long n)
{
  if ((vec != NULL)&&(vec->x != NULL)) {
    long i;
    double nD = (double)n;
    for (i=0; i<vec->dim; i++) {
      vec->x[i] /= nD;
    }         
  }
}

void vectorDoubleElemSqrt(vectorDouble vec)
{
  if ((vec != NULL)&&(vec->x != NULL)) {
    long i;
    for (i=0; i<vec->dim; i++) {
      vec->x[i] = sqrt(vec->x[i]);
    }         
  }
}

void vectorDoubleElemSqr(vectorDouble vec)
{
  if ((vec != NULL)&&(vec->x != NULL)) {
    long i;
    for (i=0; i<vec->dim; i++) {
      vec->x[i] = vec->x[i] * vec->x[i];
    }         
  }
}


vectorFloat columnSum (matrix2DFloat mat)
{
  long i,j;
  if ((mat != NULL)&&(mat->x != NULL)) {
    vectorFloat v = getMatrix2DFloatRow(mat,0);
    for(i=1; i<mat->rows; i++) {
       vectorFloat r = getMatrix2DFloatRow(mat,i);
       vectorFloatAdd(v,r);
       vectorFloatDestroy(r);
    }
    return v;
  }       
  return NULL;
}

vectorDouble columnVarianceSum (matrix2DFloat mat, vectorDouble means)
{
  long i,j;
  if ((mat != NULL)&&(mat->x != NULL)) {
    vectorFloat vf = getMatrix2DFloatRow(mat,0);
    vectorDouble v = vectorFloatToVectorDouble(vf);

    vf = vectorFloatDestroy(vf);
    for(i=1; i<mat->rows; i++) {
       vectorFloat r = getMatrix2DFloatRow(mat,i);
       vectorDouble rD = vectorFloatToVectorDouble(r);
       vectorFloatDestroy(r);
       vectorDoubleSub(rD,means);
       vectorDoubleElemSqr(rD);
       vectorDoubleAdd(v,rD);
       vectorDoubleDestroy(rD);
    }
    return v;
  }       
  return NULL;
}

void matRowsNormaliseMean (matrix2DFloat mat, vectorFloat mean)
{
  long i,j;
  if ((mat != NULL)&&(mat->x != NULL)) {
    for(i=0; i<mat->rows; i++) {
       vectorFloat r = getMatrix2DFloatRow(mat,i);
       vectorFloatSub(r,mean);
       setMatrix2DFloatRow(mat,i,r);
       vectorFloatDestroy(r);
    }
  }       
}

void matRowsStandardiseVariance (matrix2DFloat mat, vectorFloat stddev)
{
  long i,j;
  if ((mat != NULL)&&(mat->x != NULL)) {
    for(i=0; i<mat->rows; i++) {
       vectorFloat r = getMatrix2DFloatRow(mat,i);
       vectorFloatElemDiv(r,stddev);
       setMatrix2DFloatRow(mat,i,r);
       vectorFloatDestroy(r);
    }
  }       
}

#include <stdarg.h>
#define MIN_LOG_STRLEN   255
/* allocate a string and expand vararg format string expression into it */
// WARNING: memory allocated by myvprint must freed by the code calling myvprint!!
char *myvprint(const char *fmt, ...) {
  char *s= (char *)malloc(sizeof(char)*(MIN_LOG_STRLEN+1));
  if (s==NULL) return NULL;
  va_list ap;
  va_start(ap, fmt);
  int len = vsnprintf(s,MIN_LOG_STRLEN+1, fmt, ap);
  if (len > MIN_LOG_STRLEN) {
    free(s);        
    s = (char *)malloc(sizeof(char)*len+2);
    va_start(ap, fmt);
    len = vsnprintf(s,len+1, fmt, ap);
  }
  va_end(ap);
  return s;
}

    //  matRowsNormaliseMean(data,mean);
    //  matRowsStandardiseVariance(data,stddev);

int main(int argc, char *argv[])
{
  if (argc < 5) {
    fprintf(stderr, "USAGE: %s <input_arff_file> <output_arff_file(or'-')> <start ft#> <rel.end ft.#> [normdata.dat_file_to_load OR -normdata_file_to_saveTo]\n",argv[0]);
    return -1;
  }
  
  vectorDouble sum = NULL; 
  vectorDouble var = NULL; 
  vectorDouble mean = NULL;
  vectorDouble stddev = NULL;
  vectorFloat meanF = NULL;
  vectorFloat stddevF = NULL;
  long long n = 0;
  FILE *iarff;
  
  int startft = atoi(argv[3]); 
  int endft = atoi(argv[4]); 
  if ((argc == 5)||(argv[5][0]=='-')) { // no normdata file given, create normadata from input arff
 
  /*** mean ****/
  iarff = fopen(argv[1],"r");
  int read;
  char *line=NULL;
  char *nline=NULL;
  size_t len=0;
  int data = 0;
  int Natt = 0;
  while ((read = getline(&line, &len, iarff)) != -1) {
    int llen = strlen(line);
    if (llen <= 0) continue;
//printf("llen=%i\n",llen);
    if (line[llen-1] == '\n') {
      line[llen-1] = 0; // strip \n character
      llen--;
    }
    if (llen <= 0) continue;
    if (line[llen-1] == '\r') {
      line[llen-1] = 0; // strip \r character
      llen--;
    }
    if (llen <= 0) continue;
    // ignore non-data lines...
    if (data == 0) {
      if (llen >= 5) {
        data = 1;
        if (line[0] != '@') data = 0;
	else if (line[1] != 'd') data = 0;
	else if (line[2] != 'a') data = 0;
	else if (line[3] != 't') data = 0;
	else if (line[4] != 'a') data = 0;
//printf("data=%i line='%s'\n",data,line);
      }
      if (llen >= 10) {
        int att = 1;
        if (line[0] != '@') att = 0;
	else if (line[1] != 'a') att = 0;
	else if (line[2] != 't') att = 0;
	else if (line[3] != 't') att = 0;
	else if (line[4] != 'r') att = 0;
	else if (line[5] != 'i') att = 0;
	else if (line[6] != 'b') att = 0;
	else if (line[7] != 'u') att = 0;
	else if (line[8] != 't') att = 0;
	else if (line[9] != 'e') att = 0;
        if (att) Natt++;
      }
      nline = NULL;
    } else {
      nline = line;
      // remove spaces
      while ((llen>0)&&(nline[0]==' ')) { nline++; llen--; }
      while ((llen>0)&&(nline[llen-1]==' ')) { nline[llen-1]=0; llen--; }
      // convert line to vecotr
      char *el=NULL;
      int vlen = (Natt-endft)-startft+1;
      vectorDouble d = vectorDoubleCreate(vlen);
      if (sum == NULL) {
        sum = vectorDoubleCreate(vlen);
printf("create sum vlen=%i\n",vlen); fflush(stdout);
      }
      int fti=1;
      if (llen > 0) {
        do {
  	  el=strchr(nline,',');
          if (el!=NULL) 
  	    el[0] = 0;
	  if (fti>=startft) 
            if (fti<vlen+startft)
              d->x[fti-startft] = atof(nline);
          fti++;
	  if (el!=NULL) {
	    nline = el+1;             
          }
        } while (el!=NULL);
      }
      if (fti-startft < vlen) { printf("warning: less elements on line than expected!!\n"); }
      // add vector to sum...
      vectorDoubleAdd(sum,d);
      vectorDoubleDestroy(d);
      n++;
    }
    free(line); line=NULL;

  }
  fclose(iarff);

  // compute means:
  if (n>0)
    vectorDoubleScalarDiv(sum,n);
  mean = sum;
  
  /**** variance ****/
  vectorDouble var = NULL;
  iarff = fopen(argv[1],"r");
  read = 0;
  line=NULL;
  nline=NULL;
  len=0;
  data = 0;
  Natt = 0;
  while ((read = getline(&line, &len, iarff)) != -1) {
    int llen = strlen(line);
    if (llen <= 0) continue;
    if (line[llen-1] == '\n') {
      line[llen-1] = 0; // strip \n character
      llen--;
    }
    if (llen <= 0) continue;
    if (line[llen-1] == '\r') {
      line[llen-1] = 0; // strip \r character
      llen--;
    }
    if (llen <= 0) continue;
    // ignore non-data lines...
    if (data == 0) {
      if (llen >= 5) {
        data = 1;
        if (line[0] != '@') data = 0;
        else if (line[1] != 'd') data = 0;
        else if (line[2] != 'a') data = 0;
        else if (line[3] != 't') data = 0;
        else if (line[4] != 'a') data = 0;
      }
      if (llen >= 10) {
        int att = 1;
        if (line[0] != '@') att = 0;
        else if (line[1] != 'a') att = 0;
        else if (line[2] != 't') att = 0;
        else if (line[3] != 't') att = 0;
        else if (line[4] != 'r') att = 0;
        else if (line[5] != 'i') att = 0;
        else if (line[6] != 'b') att = 0;
        else if (line[7] != 'u') att = 0;
        else if (line[8] != 't') att = 0;
        else if (line[9] != 'e') att = 0;
        if (att) Natt++;
      }
      nline = NULL;
    } else {
      nline = line;
      // remove spaces
      while ((llen>0)&&(nline[0]==' ')) { nline++; llen--; }
      while ((llen>0)&&(nline[llen-1]==' ')) { nline[llen-1]=0; llen--; }
      // convert line to vecotr
      char *el=NULL;
      int vlen = (Natt-endft)-startft+1;
      vectorDouble d = vectorDoubleCreate(vlen);
      if (var == NULL) {
        var = vectorDoubleCreate(vlen);
      }
      int fti=1;
      if (llen > 0) {
        do {
          el=strchr(nline,',');
          if (el != NULL) 
            el[0] = 0;
          if (fti>=startft)
            if (fti<vlen+startft)
              d->x[fti-startft] = atof(nline);
          fti++;
          if (el!=NULL) {
            nline = el+1;
          }
        } while (el!=NULL);
      }
      if (fti-startft < vlen) { printf("warning: less elements on line than expected!!\n"); }
      // add vector var to variance sum...
//...
      vectorDoubleSub(d,mean);
      vectorDoubleElemSqr(d);
      vectorDoubleAdd(var,d);
      vectorDoubleDestroy(d); d=NULL;
      //n++;
    }
    free(line); line=NULL;

  }
  fclose(iarff);


  if (n>0)
    vectorDoubleScalarDiv(var,n);
  stddev = var;
  vectorDoubleElemSqrt(stddev);
  
  //printf("var0: %f (dim=%i)\n",var->x[0],var->dim);
  // save normdata file in current path 
  
  FILE *nd ;
  if ((argc>5)&&(argv[5][0]=='-')) {
    nd = fopen((argv[5]+1),"wb");
  } else {
    nd = fopen("normdata.dat","wb");
  }
  if (nd != NULL) {
    fwrite(mean->x, sizeof(double)*mean->dim, 1, nd);
    fwrite(stddev->x, sizeof(double)*stddev->dim, 1, nd);
    fclose(nd);  
  } else {
    fprintf(stderr,"ERROR saving normdata\n");       
  }

  //meanF = vectorDoubleToVectorFloat(mean);
  //stddevF = vectorDoubleToVectorFloat(stddev);

  }

/* do actual standardisation::: */
/***************************************************************************/

  FILE * oarff=NULL;
  iarff = fopen(argv[1],"r");
  if (!(argv[2][0] == '-')) {
    oarff = fopen(argv[2],"w");
  } else {
    char *tmp = myvprint("%s.norm.arff",argv[1]);
    oarff = fopen(tmp,"w");
    free(tmp);
  }
  int read = 0;
  char *line=NULL;
  char *nline=NULL;
  size_t len=0;
  int data = 0;
  int Natt = 0;
  while ((read = getline(&line, &len, iarff)) != -1) {
    int llen = strlen(line);
    if (line[llen-1] == '\n') {
      line[llen-1] = 0; // strip \n character
      llen--;
    }
    if (line[llen-1] == '\r') {
      line[llen-1] = 0; // strip \r character
      llen--;
    }
    // ignore non-data lines...
    if (data == 0) {
      if (llen >= 5) {
        data = 1;
        if (line[0] != '@') data = 0;
        else if (line[1] != 'd') data = 0;
        else if (line[2] != 'a') data = 0;
        else if (line[3] != 't') data = 0;
        else if (line[4] != 'a') data = 0;
      }
      if (llen >= 10) {
        int att = 1;
        if (line[0] != '@') att = 0;
        else if (line[1] != 'a') att = 0;
        else if (line[2] != 't') att = 0;
        else if (line[3] != 't') att = 0;
        else if (line[4] != 'r') att = 0;
        else if (line[5] != 'i') att = 0;
        else if (line[6] != 'b') att = 0;
        else if (line[7] != 'u') att = 0;
        else if (line[8] != 't') att = 0;
        else if (line[9] != 'e') att = 0;
        if (att) Natt++;
      }
      nline = NULL;
      fprintf(oarff,"%s\n",line);
    } else {
      int vlen = (Natt-endft)-startft+1;
      if ((mean == NULL)&&(argc > 5)) {
        mean = vectorDoubleCreate(vlen);
        stddev = vectorDoubleCreate(vlen);
//  FILE *nd = fopen("normdata.dat","rb");
        FILE *nd = fopen(argv[5],"rb");
        if (nd != NULL) {
          fread(mean->x, sizeof(double)*mean->dim, 1, nd);
          fread(stddev->x, sizeof(double)*stddev->dim, 1, nd);
          fclose(nd);  
        } else {
          fprintf(stderr,"ERROR loading normdata\n");       
        }
      }


      nline = line;
      // remove spaces
      while ((llen>0)&&(nline[0]==' ')) { nline++; llen--; }
      while ((llen>0)&&(nline[llen-1]==' ')) { nline[llen-1]=0; llen--; }
      // convert line to vecotr
      char *el=NULL;
      vectorDouble d = vectorDoubleCreate(vlen);
      if (var == NULL) {
        var = vectorDoubleCreate(vlen);
      }
      int fti=1;
      char *oline0=NULL;
      char *oline1=NULL;
      char *oline0e=NULL;
      char *oline1e=NULL;
      if (llen > 0) {
        do {
          el=strchr(nline,',');
          if (el!=NULL) 
            el[0] = 0;
          if (fti>=startft)
            if (fti<vlen+startft)
              d->x[fti-startft] = atof(nline);
            else {
              if (oline0e != NULL) {
                oline1e = myvprint("%s,%s",oline0e,nline);
                free(oline0e); oline0e=oline1e;
              } else {
                oline0e = strdup(nline);
              }
            }
          else {
            if (oline0 != NULL) {
              oline1 = myvprint("%s,%s",oline0,nline);
              free(oline0); oline0=oline1;
            } else {
              oline0 = strdup(nline);
            }
	  }
          fti++;
          if (el!=NULL) {
            nline = el+1;
          }
        } while (el!=NULL);
      if (fti-startft < vlen) { printf("warning: less elements on line than expected!!\n"); }
      // add vector var to variance sum...
//...
      vectorDoubleSub(d,mean);
      vectorDoubleElemSafeDiv(d,stddev);
      // rebuild line and write to file...
      int prev = 0;
      if (oline0 != NULL) {
        fprintf(oarff,"%s",oline0);
        free(oline0);
        prev = 1;
      }
      long i;
      for (i=0; i<vlen; i++) {
        if (prev)
          fprintf(oarff,",%f",d->x[i]);
        else
          fprintf(oarff,"%f",d->x[i]);
        prev = 1;
      }
      if (oline0e != NULL) {
        if (prev) fprintf(oarff,",");
        fprintf(oarff,"%s",oline0e);
        free(oline0e);
      }
      fprintf(oarff,"\n");

/*

      char *oline=strdup(oline0);
      int i;
      for (i=0; i<vlen; i++) {
        char *tmp = myvprint("%s,%f",oline,d->x[i]);
        free(oline); oline=tmp;
      }
      char *tmp = myvprint("%s,%s",oline,oline0e);
      free(oline); oline=tmp;
      fprintf(oarff,"%s\n",oline);*/
      //n++;
      }
    }
    free(line); line=NULL;

  }
  fclose(oarff);
  fclose(iarff);

 
 
  vectorDoubleDestroy(mean);  // at the end...
  vectorDoubleDestroy(stddev);  
  vectorFloatDestroy(meanF);  // at the end...
  vectorFloatDestroy(stddevF);  
  
  //printf("Hello World!\n");
}


