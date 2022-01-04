/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


/*  openSMILE component: cVectorTransform

this is a base class for vector transforms which require history data or precomputed data
the class provides support for 
 a) saving computed data at the end of processing (level or custom)
 a) saving computed data continuously (level or custom)
 b) applying pre-computed transformation data loaded from file, w/o adaptation
 c) applying pre-computed transformation data loaded from file, with on-line adaptation
 d) applying purely on-line adaptive transformation

different modes of operation are distinguished:

 a) pure analysis: transform data is computed, no output is generated (except for transformation data, if it is output to a level)
       (full input, or each turn)
 b) pure transform: apply pre-computed transform data
       (apply always, only during turn, switch models for turn/non-turn, switch models by external message)
 c) transform w. online adaptation: combines a) and b). transform output to a level is NOT possible
         (more analysis methods: fixed buffer)
*/


/*
File Format for storing transformations (will be auto-detected):

A) HTK CMN Format   (text)
  <MEAN> N
  Val1 Val2 Val3 ...

B) simple MVN matrix data format (deprecated!): Binary (2*N x double):  
   (N x double = means), 
   (N x double = stddevs)

C) advanced smile transformation format: Binary
   (note: this format is used interally)
  Header: 
    Magic     (int32)  0xEE 0x11 0x11 0x00
    N vectors (int32)
    N groups  (int32)
    N timeunits(int32)
    Vecsize   (int32)
    N userData(int32)
    TypeID    (int32)  [defined in the header file of the component implementing the transform]
    Reserved  (16 byte)
  User data : N userData (double)
  Matrix data: N vec x Vecsize (double, 8 byte)

When a file is loaded, the first 4 bytes are checked for:
  a) magic       0xEE 0x11 0x11 0x00   => FORMAT C
  b) HTK Header: <    M    E    A    (or leading spaces, which will be removed..?!)  => FORMAT A
  c) else => FORMAT B

*/


/* Current component status:

* AVG method working
* Exp method working
* BUF/FIX method working

TODO:  AVGI method possibly broken !

*/


#include <core/vectorTransform.hpp>

#define MODULE "cVectorTransform"


SMILECOMPONENT_STATICS(cVectorTransform)

SMILECOMPONENT_REGCOMP(cVectorTransform)
{
  SMILECOMPONENT_REGCOMP_INIT
  scname = COMPONENT_NAME_CVECTORTRANSFORM;
  sdescription = COMPONENT_DESCRIPTION_CVECTORTRANSFORM;

  // we inherit cVectorProcessor configType and extend it:
  SMILECOMPONENT_INHERIT_CONFIGTYPE("cVectorProcessor")
  
  SMILECOMPONENT_IFNOTREGAGAIN(
    ct->setField("nameAppend",(const char *)NULL,(const char *)NULL);
    ct->setField("mode","The mode of operation:\n   an(alysis) : analyse all incoming data and compute transform for later use\n   tr(ansformation) : use pre-computed transform loaded from initFile, no on-line adaptation\n   in(cremental) : use initFile as inital transform, if given, update transform on-line using 'updateMethod'","analysis");

    ct->setField("initFile","The file to load the (initial) transformation data from (see vectorTransform.cpp for documentation of the file format). Default (null) disables loading, also setting the value to '?' disables loading of the file (parameters are initialized with 0, use mode incremental or analysis in this case.)", (const char*)NULL);
    ct->setField("saveFile","The file to save transformation data to (see vectorTransform.cpp for documentation of the file format)",(const char*)NULL);
    ct->setField("saveFileInterval", "If set to > 0, saves transform data update only every N-th frame. For 0 it only saves at the end. Set to 1 to save for every frame (costly!!)", 0);

    ct->setField("updateMethod","Method to use for incrementally updating the transform. Can be one of the following:\n   'exp'  exponential: m1 = alpha * m0  + (1-alpha) * x\n   'fix'/'buf'  compute transform over history buffer of fixed length\n   'ifix'/'ibuf'  compute transform over history buffer of fixed length and use init data from last turn (see weight option)\n   'usr'  other update method (usually defined by a descendant component of cVectorTransform, look for customUpdateMethod option)\n   'avg'  cummulative average method with weighted fixed inital values\n   'iavg'  cummulative average method with weighted variable (from the last turn) inital values\n   NOTE: if 'resetOnTurn'==0 then 'avg' and 'iavg' are identical","iavg");

    ct->setField("alpha","The weighting factor alpha for exponential transform update",0.995);
    ct->setField("weight","The weighting factor for 'avg'/'avgI'/'bufI' update, i.e. the factor the initial transform parameters are weighted by when building the cummulative average", 100);
    ct->setField("fixedBuffer","The size of the fixed length buffer [in seconds] (for fixed buffer update method)",5.0);
    ct->setField("fixedBufferFrames","The size of the fixed length buffer [in frames] (for fixed buffer update method). Use this when the input level is aperiodic (e.g. functionals).", 10);

    ct->setField("turnOnlyUpdate","1 = perform transform update only during turns (between turnStart and turnEnd messages) (works for all methods)",0);
    ct->setField("invertTurn","1 = invert the turn state (i.e. changes a 'turnOnly...' option into 'notTurn' option)",0);
    ct->setField("resetOnTurn"," 1 = reset transform values at the beginning of each new turn (only in mode 'analysis' and 'incremental')",0);
//    ct->setField("nextTrOnTurn","save/load next set of transform values at the end/beginning of each new turn (only in mode 'analysis' and 'transform')",0);
    ct->setField("updateMaxSec", "If update enabled, only update on the first X seconds of data if the value is > 0", 0.0);
    ct->setField("turnOnlyNormalise","1 = apply the transform only to turns (in between data will pass through unmodified) ('invertTurn' will also invert this option)",0); // check this!!
    ct->setField("turnOnlyOutput","1 = output data to writer level only during a turn (this will implicitely set turnOnlyNormalise = 1) ('invertTurn' will also invert this option)",0); // check this!!

    ct->setField("htkcompatible","A flag that indicates (if set to 1) whether last coefficient in 'initFile' is loaded into means[0] (use this only when reading htk-compatible cmn init files, and NOT using htk-compatible mfccs)",0);
    ct->setField("invertMVNdata", "1 = invert the loaded MVN data to 'unstandardise' to these parameters. Currently only works with MVn text and old MVN binary data. mu' = -mu/sigma and sigma' = 1/sigma ; ", 0);
    ct->setField("turnStartMessage","You can use this option to define a custom message name for the turn start message, i.e. if you want to use voice activity start/end messages instead","turnStart");
    ct->setField("turnEndMessage","You can use this option to define a custom message name for the turn end message, i.e. if you want to use voice activity start/end messages instead","turnEnd");

    // OBSOLETE
    //ct->setField("skipNfirst","The number of frames(!) at the beginning of the input to skip (i.e. not to use for computation/update of transform parameters)",10);
  )

  SMILECOMPONENT_MAKEINFO(cVectorTransform);
}



SMILECOMPONENT_CREATE_ABSTRACT(cVectorTransform)

//-----

cVectorTransform::cVectorTransform(const char *_name) :
  cVectorProcessor(_name),
  initFile(NULL), saveFile_(NULL),
  mode(0), err(0), flushed(0),
  alpha(0.0), weight(0.0),
  buffer(NULL), bufferNframes(NULL),
  nAvgBuffer(0), wPtr(0), rPtr(0),
  updateMethod(0),
  turnOnlyNormalise(0),
  invertTurn(0),
  resetOnTurn(0),
  turnOnlyUpdate(0),
  turnOnlyOutput(0),
  zeroThIsLast(0),
  skipNfirst(10),
  fixedBuffer(0.0),
  fixedBufferFrames(0),
  isTurn(0), resetMeans(0), nFrames(0),
  turnStartMessage(NULL), turnEndMessage(NULL),
  saveFileCounter_(0), updateMaxFrames_(0),
  saveFileInterval_(0)
{
  transform.userData = NULL;
  transform.vectors = NULL;
  transform.user = NULL;
  /* use the free method to initialize all important values to 0 */
  freeTransformData(&transform);
  transform0.userData = NULL;
  transform0.vectors = NULL;
  transform0.user = NULL;
  /* use the free method to initialize all important values to 0 */
  freeTransformData(&transform0);
}

void cVectorTransform::myFetchConfig()
{
  cVectorProcessor::myFetchConfig();
  
  initFile = getStr("initFile");
  if (initFile != NULL) {
    SMILE_IDBG(2,"initFile = '%s'",initFile);
  }

  const char * saveFile = getStr("saveFile");
  if (saveFile != NULL) {
    saveFile_ = strdup(saveFile);
    SMILE_IDBG(2,"saveFile_ = '%s'",saveFile_);
  }

  saveFileInterval_ = getInt("saveFileInterval");
  saveFileCounter_ = saveFileInterval_;
  updateMaxSec_ = getDouble("updateMaxSec");

  const char * _mode = getStr("mode");
  SMILE_IDBG(2,"mode = '%s'",_mode);
  if (!strncmp(_mode,"an",2)) {
    mode = MODE_ANALYSIS;
  } else if (!strncmp(_mode,"tr",2)) {
    mode = MODE_TRANSFORMATION;
  } else if (!strncmp(_mode,"in",2)) {
    mode = MODE_INCREMENTAL;
  } else {
    SMILE_IERR(1,"unknown mode '%s' (setting to 'in(cremental)' !)",_mode);
    mode = MODE_INCREMENTAL;
  }

  alpha = (FLOAT_DMEM)getDouble("alpha");
  SMILE_IDBG(2,"alpha = %f",alpha);

  weight = (FLOAT_DMEM)getDouble("weight");
  SMILE_IDBG(2,"weight = %f",weight);

  const char * updateMethodStr = getStr("updateMethod");
  SMILE_IDBG(2,"updateMethod = '%s'",updateMethodStr);
  if (!strncmp(updateMethodStr,"exp",3)) {
    updateMethod = UPDMETHOD_EXP;
  } else if (!strncmp(updateMethodStr,"fix",3)) {
    updateMethod = UPDMETHOD_FIX;
  } else if (!strncmp(updateMethodStr,"buf",3)) {
    updateMethod = UPDMETHOD_FIX;
  } else if (!strncmp(updateMethodStr,"usr",3)) {
    updateMethod = UPDMETHOD_USR;
  } else if (!strncmp(updateMethodStr,"avg",3)) {
    updateMethod = UPDMETHOD_AVG;
  } else if (!strncmp(updateMethodStr,"iavg",4)) {
    updateMethod = UPDMETHOD_AVGI;
  } else if (!strncmp(updateMethodStr,"ifix",4)) {
    updateMethod = UPDMETHOD_FIXI;
  } else if (!strncmp(updateMethodStr,"ibuf",4)) {
    updateMethod = UPDMETHOD_FIXI;
  } else {
    SMILE_IERR(1,"unknown update method '%s' (setting to 'usr' !)",updateMethodStr);
    updateMethod = UPDMETHOD_USR;
  }

  zeroThIsLast = getInt("htkcompatible");
  SMILE_IDBG(2,"zeroThIsLast (htkcompatible) = %i",zeroThIsLast);

//  skipNfirst = getInt("skipNfirst");
//  SMILE_IDBG(2,"skipNfirst = %i",skipNfirst);
  
  turnOnlyUpdate = getInt("turnOnlyUpdate");
  SMILE_IDBG(2,"turnOnlyUpdate = %i",turnOnlyUpdate);

  turnOnlyNormalise = getInt("turnOnlyNormalise");
  turnOnlyOutput = getInt("turnOnlyOutput");
  SMILE_IDBG(2,"turnOnlyOutput = %i",turnOnlyOutput);
  if (turnOnlyOutput) turnOnlyNormalise = 1;

  SMILE_IDBG(2,"turnOnlyNormalise = %i",turnOnlyNormalise);

  resetOnTurn = getInt("resetOnTurn");
  SMILE_IDBG(2,"resetOnTurn = %i",resetOnTurn);

  invertTurn = getInt("invertTurn");
  SMILE_IDBG(2,"invertTurn = %i",invertTurn);

  fixedBuffer = getDouble("fixedBuffer");
  SMILE_IDBG(2,"fixedBuffer = %f",fixedBuffer);
  if (isSet("fixedBufferFrames")) {
    fixedBufferFrames = getInt("fixedBufferFrames");
  } else {
    fixedBufferFrames = -1;
  }

  turnStartMessage = getStr("turnStartMessage");
  SMILE_IDBG(2,"turnStartMessage = '%s'",turnStartMessage);

  turnEndMessage = getStr("turnEndMessage");
  SMILE_IDBG(2,"turnEndMessage = '%s'",turnEndMessage);

  invertMVNdata = getInt("invertMVNdata");
}

int cVectorTransform::loadHTKCMNdata(const char *filename, struct sTfData * tf)
{
  int ret;
  FILE *f = NULL;
  if (filename != NULL) {
    f = fopen(filename,"r");
  }
  freeTransformData(tf);

  if (f!=NULL) {
    int i = 0;
    int n = -1;
    float v;
    fscanf(f,"<MEAN> %i\n",&n);
    if (n < 1) {
      SMILE_IERR(1, "Number of means in HTK CMN file is < 1 (%i).", n);
      fclose(f);
      return 0;
    }
    tf->head.nVec = 1;
    tf->head.nGroups = 1;
    tf->head.vecSize = n;
    tf->head.typeID = TRFTYPE_MNN;
    tf->vectors = (double *)malloc(sizeof(double)*n);
    //printf("nMeans to load: %i\n",n);
    if (zeroThIsLast) { i=1; }
    for (; i<n; i++) {
      if (fscanf(f," %f",&v) > 0) {
        tf->vectors[i] = v;
        //printf("v(%i): %f\n",i,v);
      } else {
        tf->vectors[i] = 0.0;
        SMILE_IERR(1,"parse error during loading of HTK cepstral mean file: '%s'",filename);
      }
    }
    if (zeroThIsLast) {
      if (fscanf(f," %f",&v) > 0) {
        tf->vectors[0] = v;
        //printf("v(%i): %f\n",i,v);
      } else {
        tf->vectors[0] = 0.0;
        SMILE_IERR(1,"parse error during load of HTK cepstral mean file: '%s'\n",filename);
      }
    }
    fclose(f);
    ret = 1;
  } else {
    if (filename != NULL) {
      SMILE_IERR(1,"cannot open transform data initialisation file '%s' for reading it as HTK Ascii format",filename);
      ret = 0;
    } else {
      ret = 1;
    }
  }
  return ret;
}

void cVectorTransform::prepareUnstandardise(struct sTfData * tf)
{
  if (tf->head.nVec == 2 && tf->head.nGroups == 2 && tf->head.typeID == TRFTYPE_MVN) {
    double *m = tf->vectors;
    double *v = tf->vectors + tf->head.vecSize;
    // means

    for (int i = 0; i < tf->head.vecSize; i++) {
      if (*v != 0.0) {
        *m = - (*m) / (*v);
      } else {
        *m = - (*m);
      }
      m++;
      v++;
    }
    v = tf->vectors + tf->head.vecSize;
    for (int i = 0; i < tf->head.vecSize; i++) {
      if (*v != 0.0) {
        *v = 1.0 / *v;
      }
      v++;
    }
    SMILE_IMSG(3, "'Unstandardised' (=inverted) this MVN transformation!");
  } else {
    SMILE_IERR(2, "This transform does not seem to be MVN type, not applying unstandardise!");
  }
}

int cVectorTransform::loadMVNtextdata(const char *filename, struct sTfData * tf)
{
  int ret;
  FILE *f = NULL;
  if (filename != NULL) {
    f = fopen(filename,"r");
  }
  freeTransformData(tf);

  if (f!=NULL) {
    int i=0,n; float v;
    fscanf(f,"<MVN> %i\n",&n);
    tf->head.nVec = 2;
    tf->head.nGroups = 2;
    tf->head.vecSize = n;
    tf->head.typeID = TRFTYPE_MVN;
    tf->vectors = (double *)malloc(sizeof(double)*n*2);
    for (int line = 0; line < 2; line++) {
      double * vectors = tf->vectors + line * n;
      i = 0;
      if (zeroThIsLast) {
        i = 1;
      }
      for (; i < n - 1; i++) {
        if (fscanf(f,"%f ",&v) > 0) {
          vectors[i] = (double)v;
          //printf("v(%i): %f\n",i,v);
        } else {
          vectors[i] = 0.0;
          SMILE_IERR(1,"parse error during loading of text MVN file (line %i, element: %i, zeroThIsLast: %i): '%s'", line, i, zeroThIsLast, filename);
        }
      }
      if (zeroThIsLast) {
        i = 0;
      }
      const char *match = "%f\n";
      if (line == 1) {
        match = "%f";
      }
      if (fscanf(f, match, &v) > 0) {
        vectors[i] = (double)v;
      } else {
        vectors[i] = 0.0;
        SMILE_IERR(1,"parse error during loading of text MVN file (line: %i, element: %i, zeroThIsLast: %i): '%s'", line, i, zeroThIsLast, filename);
      }
    }
    fclose(f);
    if (invertMVNdata) {
      prepareUnstandardise(tf);
    }
    ret = 1;
  } else {
    if (filename != NULL) {
      SMILE_IERR(1,"cannot open transform data initialisation file '%s' for reading (mvn text format).", filename);
      ret = 0;
    } else {
      ret = 1;
    }
  }
  return ret;
}


int cVectorTransform::loadMVNdata(const char *filename, struct sTfData * tf)
{
  int ret;
  FILE *f = NULL;
  if (filename != NULL) {
    f = fopen(filename,"rb");
  }
  freeTransformData(tf);

  if (f!=NULL) {
    // get file size:
    fseek(f, 0L, SEEK_END);
    long len = ftell(f);
    fseek(f, 0L, SEEK_SET);
    tf->head.nVec = 2;
    tf->head.nGroups = 2;
    tf->head.typeID = TRFTYPE_MVN;
    tf->head.vecSize = len / ( sizeof(double) * 2 );
    tf->vectors = (double*)malloc( sizeof(double) * tf->head.vecSize * tf->head.nVec);
    double * const means = tf->vectors;
    double * const stddev = tf->vectors + tf->head.vecSize; 

    if (!fread(means, sizeof(double)*tf->head.vecSize, 1, f))
      SMILE_IERR(1,"error reading data from file '%s', encountered EOF before it was expected\n",filename);
    if (!fread(stddev, sizeof(double)*tf->head.vecSize, 1, f))
      SMILE_IERR(1,"error reading data from file '%s', encountered EOF before it was expected\n",filename);

    double floor = 0.0000001;
    //for (i=0; i<tf->head.vecSize; i++) {
      //if (stddev[i] <= 0.0) {
      //  stddev[i] = floor;
      //  SMILE_IWRN(2,"some values of stddev in MVN file '%s' are <= 0.0 ! Flooring them to %f",f,floor);
      //}
    //}
    fclose(f);
    if (invertMVNdata) {
      prepareUnstandardise(tf);
    }
    ret = 1;
  } else {
    if (filename != NULL) {
      SMILE_IERR(1,"cannot open transform data initialisation file '%s' for reading it as MVN binary format",filename);
      ret = 0;
    } else {
      ret = 1;
    }
  }
  return ret;
}

int cVectorTransform::loadSMILEtfData(const char *filename, struct sTfData * tf)
{
  int ret;
  FILE *f = NULL;
  if (filename != NULL) {
    f = fopen(filename,"rb");
  }
  freeTransformData(tf);

  if (f!=NULL) {
    if (!fread(&(tf->head), sizeof(struct sTfHeader), 1, f)) 
      SMILE_IERR(1,"error reading header from file '%s', encountered EOF before it was expected\n",filename);
    if (tf->head.nUserdata > 0.0) {
      tf->userData = (double *)malloc(sizeof(double)*tf->head.nUserdata);
      if (!fread(tf->userData, sizeof(double)*tf->head.nUserdata, 1, f)) 
        SMILE_IERR(1,"error reading userData from file '%s', encountered EOF before it was expected\n",filename);
    }
    if ( (tf->head.nVec > 0.0)&&(tf->head.vecSize > 0.0) ) {
      tf->vectors = (double *)malloc(sizeof(double)*tf->head.nVec*tf->head.vecSize);
      if (!fread(tf->vectors, sizeof(double)*tf->head.nVec*tf->head.vecSize, 1, f)) 
        SMILE_IERR(1,"error reading vectors from file '%s', encountered EOF before it was expected\n",filename);
    }
    fclose(f);
    if (invertMVNdata) {
      prepareUnstandardise(tf);
    }
    // this is done later in initTransform
    //if (tf->head.nTimeunits > 0)
    //  nFrames = (long)(tf->head.nTimeunits);
    ret = 1;
  } else {
    if (filename != NULL) {
      SMILE_IERR(1,"cannot open transform data initialisation file '%s' for reading it as SMILEtf binary format",filename);
      ret = 0;
    } else {
      ret = 1;
    }
  }
  return ret;
}


/* loads data, the format (HTK, old MVN, smile native) is determined automatically */
int cVectorTransform::loadTransformData(const char *filename, struct sTfData * tf)
{
  if (tf == NULL) 
    return 0;
  if (filename == NULL || (filename[0] == '?' 
      && filename[1] == 0)) 
    return 0;
  FILE *f = fopen(filename,"rb");
  if (f != NULL) {
    int ret = 1;
    /* determine file format, and maybe reopen file as ascii, if ascii format... */
    unsigned char magic[4];
    fread(magic, 4, 1, f);
    /* check for SMILEtf format magic */
    if ( (magic[0] == smileMagic[0]) &&
         (magic[1] == smileMagic[1]) &&
         (magic[2] == smileMagic[2]) &&
         (magic[3] == smileMagic[3]) ) {
       /* load new format */
       fclose(f);
       SMILE_IMSG(4,"loading init file in SMILEtf binary format");
       ret = loadSMILEtfData(filename, tf);
    } else {
      /* check for HTK CMN ascii file (<MEAN> N)*/
      int i = 0;
        /* skip whitespace/newline characters at the beginning */
      while ((magic[i] == ' ')||(magic[i] == '\t')||(magic[i] == '\r')||(magic[i] == '\n')) {
        i++;
        if (i==4) {
          i=0;
          if (!fread(magic, 4, 1, f)) break; /* EOF */
        }
      }
      if (i) {
        int j;
        for (j=i; j<4; j++) magic[j-i] = magic[j];
        fread(&(magic[4-i]),i,1,f);
      }

      if ( (magic[0] == '<') &&
         ( (magic[1] == 'M') ) &&
         ( (magic[2] == 'E') ) &&
         ( (magic[3] == 'A') ) ) {
          /* load HTK CMN Ascii format file, close binary file handle first */
          fclose(f);
          SMILE_IMSG(2, "Loading transform init file in HTK CMN Ascii format");
          ret = loadHTKCMNdata(filename, tf);
      } else if ( (magic[0] == '<') &&
         ( (magic[1] == 'M') ) &&
         ( (magic[2] == 'V') ) &&
         ( (magic[3] == 'N') ) ) {
         fclose(f);
         SMILE_IMSG(2, "Loading init file in MVN text format");
         ret = loadMVNtextdata(filename, tf);
      } else {
         /* load old format MVN binary file */
         fclose(f);
         SMILE_IMSG(2, "Loading init file in old MVN binary format");
         ret = loadMVNdata(filename, tf);
      }
    }
    return ret;
  }
  SMILE_IERR(1, "could not open initial transform file (initFile) '%s' for reading, probably 'file not found'!", filename);
  return 0;
}
    
/* saves transform data in smile native format,
   other formats are not supported and will not be supported */
int cVectorTransform::saveTransformData(const char *filename, struct sTfData * tf)
{
  if (filename == NULL)
    return 0;
  if (saveFileInterval_ != 1 && !isEOI()) {
    if (saveFileInterval_ > 0) {
      saveFileCounter_--;
      if (saveFileCounter_ <= 0) {
        saveFileCounter_ = saveFileInterval_;
      } else {
        return 0;
      }
    } else {
      // if saveFileInterval_ <= 0, we never save
      return 0;
    }
  }
  if (tf == NULL)
    return 0;
  SMILE_IMSG(2, "saving transform data to file '%s'", filename);
  tf->head.nTimeunits = (int64_t)nFrames;
  FILE *f = fopen(filename, "wb");
  if (f != NULL) {
    tf->head.magic[0] = smileMagic[0]; 
    tf->head.magic[1] = smileMagic[1];
    tf->head.magic[2] = smileMagic[2];
    tf->head.magic[3] = smileMagic[3];
    if (!fwrite(&(tf->head), sizeof(struct sTfHeader), 1, f)) 
      SMILE_IERR(1,"error writing data to file '%s', fwrite returned 0",filename);
    if (tf->head.nUserdata > 0) {
      if (tf->userData == NULL) {
        double c = 0.0; int i;
        for (i=0; i<tf->head.nUserdata; i++) {
          if (!fwrite(&c, sizeof(double), 1, f)) 
            SMILE_IERR(1,"error writing data to file '%s', fwrite returned 0",filename);
        }
      } else {
        if (!fwrite(tf->userData, sizeof(double), tf->head.nUserdata, f)) 
          SMILE_IERR(1,"error writing data to file '%s', fwrite returned 0",filename);
      }
    }
    if ((tf->head.nVec > 0)&&(tf->head.vecSize > 0)) {
      if (tf->vectors == NULL) {
        double c = 0.0; int i;
        for (i=0; i<tf->head.vecSize*tf->head.nVec; i++) {
          if (!fwrite(&c, sizeof(double), 1, f)) 
            SMILE_IERR(1,"error writing data to file '%s', fwrite returned 0",filename);
        }
      } else {
        if (!fwrite(tf->vectors, tf->head.vecSize*sizeof(double)*tf->head.nVec, 1, f)) 
          SMILE_IERR(1,"error writing data to file '%s', fwrite returned 0",filename);
      }
    }
    fclose(f);
    return 1;
  }
  return 0;
}

/* free the vectors and user data */
void cVectorTransform::freeTransformData(struct sTfData * tf)
{
  if (tf != NULL) {
    if (tf->user != NULL) {
      free(tf->user);
      tf->user = NULL;
    }
    if (tf->userData != NULL) {
      free(tf->userData);
      tf->userData = NULL;
    }
    if (tf->vectors != NULL)  {
      free(tf->vectors);
      tf->vectors = NULL;
    }
    /*tf->head.nVec = 0;
    tf->head.nGroups = 0;
    tf->head.nTimeunits = 0;
    tf->head.vecSize = 0;
    tf->head.nUserdata = 0;
    tf->head.typeID = 0;*/
    memset(&(tf->head), 0, sizeof(tf->head));
  }
}
    

int cVectorTransform::configureWriter(sDmLevelConfig &c)
{
  if (c.T != 0.0) {
    if (fixedBuffer > 0.0 && fixedBufferFrames <= 0) {
      fixedBufferFrames = (long)ceil(fixedBuffer / c.T);
      SMILE_IDBG(2,"fixedBufferFrames = %i\n",fixedBufferFrames);
      /*
      //reader->setBlocksize(fixedBufferFrames+1);
      auxReader->setComponentEnvironment(getCompMan(), -1, this);
      auxReader->registerInstance();
      auxReader->configureInstance();
      auxReader->finaliseInstance();
      auxReader->setBlocksize(fixedBufferFrames+1);
      */
    }
    if (updateMaxSec_ > 0.0) {
      updateMaxFrames_ = (long)ceil(updateMaxSec_ / c.T);
    }
  }
  if (fixedBufferFrames < 0) {
    fixedBufferFrames = 0;
  }
  return 1;
}


int cVectorTransform::myFinaliseInstance()
{
  int ret = cVectorProcessor::myFinaliseInstance();
  if (!ret) return 0;

  // load initialisation transform file:
  if (!loadTransformData(initFile, &transform0)) {
    // set initial transform weight to 0 if no data has been loaded
    weight = 0.0;
    if (initFile != NULL && (initFile[0] != '?' && initFile[1] != 0)) {
      return 0;
    }
  }
  modifyInitTransform(&transform0);

  if ( (updateMethod == UPDMETHOD_FIX) || (updateMethod == UPDMETHOD_AVGI) ) {
    // memory for ring-buffer is allocated during the first call to processVector
    if (fixedBufferFrames < 3) {
      SMILE_IWRN(1,"fixedBuffer (in frames) is < 3, setting to minimum value of 3!!");
      fixedBufferFrames = 3;
    } 
  }
  return ret;
}

int cVectorTransform::processComponentMessage( cComponentMessage *_msg )
{
  const char * endM = NULL;
  if (turnEndMessage == NULL) endM = "turnEnd";
  else endM = turnEndMessage;

  if (isMessageType(_msg,endM)) {
    if (invertTurn) {
      isTurn = 1; resetMeans = 1;
    } else {
      isTurn = 0;
    }
    SMILE_IDBG(3,"received turnEnd (%s)",endM);
    return 1;
  }

  const char * startM = NULL;
  if (turnStartMessage == NULL) startM = "turnStart";
  else startM = turnStartMessage;

  if (isMessageType(_msg,startM)) {
    if (invertTurn) {
      isTurn = 0;
    } else {
      isTurn = 1;
      resetMeans = 1;
    }
    SMILE_IDBG(3,"received turnStart (%s)",startM);
    return 1;
  }
  return 0;
}

/* this function will handle saving of transform data, if enabled */
int cVectorTransform::flushVector(FLOAT_DMEM *dst, long Nsrc, long Ndst, int idxi)
{
  if (!flushed) {
    /* call refactoring method: */
    if (mode != MODE_TRANSFORMATION) 
      computeFinalTransformData(&transform,idxi);
    /* save transform data to SMILEtf format binary file */
    saveFileCounter_ = 0;
    saveTransformData(saveFile_, &transform);
    flushed = 1;
  }
  return 0; /* no output data */
}

// copy transform data from tf0 to tf OR init tf with 0'ed data
void cVectorTransform::initTransform(struct sTfData *tf, struct sTfData *tf0)
{
  if (tf0 == NULL) { weight = 0.0; return; } /*||(tf0->head.nVec==0)*/
  if (tf != NULL) {
    if (tf0->head.typeID == 0) tf0->head.typeID = tf->head.typeID;
    if (tf->head.typeID != tf0->head.typeID) {
      SMILE_IERR(2,"initTransform: transform data typeID mismatch (tgt: %i != src: %i), I will not copy data.",tf->head.typeID, tf0->head.typeID);
      return;
    }

    // only copy data if valid data is present in tf0
    if ((tf0->userData!=NULL)&&(tf->userData != NULL)) {
      if (tf->head.nUserdata < tf0->head.nUserdata) {
        SMILE_IERR(2,"nUserdata (%i) in transform < nUserdata (%i) in transform0, this is a bug, or you have loaded an incompatible transform file!",tf->head.nUserdata,tf0->head.nUserdata);
      } else tf->head.nUserdata = tf0->head.nUserdata;
      // copy user data:
      memcpy(tf->userData, tf0->userData, sizeof(double) * tf->head.nUserdata);
    } else {
      // zero-init
      if ((tf->userData != NULL)&&(tf->head.nUserdata > 0)) {
        memset(tf->userData, 0, tf->head.nUserdata * sizeof(double));
      }
    }

    if ((tf0->vectors!=NULL)&&(tf->vectors!=NULL)&&(tf0->head.nVec > 0)&&(tf0->head.vecSize > 0)) {
      if (tf->head.nGroups != tf0->head.nGroups) {
        if (tf->head.nGroups == 0) tf->head.nGroups = tf0->head.nGroups;
        else {
          SMILE_IWRN(3,"nGroups (%i) in transform != nGroups (%i) in transform0, this might be a bug, or you might have loaded an incompatible transform file, or everything is ok ;-)",tf->head.nGroups,tf0->head.nGroups);
        }
      }

      
      if (tf->head.nVec != tf0->head.nVec) {
        SMILE_IWRN(3,"nVec (%i) in transform != nVec (%i) in transform0, this might be a bug, or you might have loaded an incompatible transform file, or everything is ok ;-)",tf->head.nVec,tf0->head.nVec);
        
      }

      if (tf->head.vecSize != tf0->head.vecSize) {
        SMILE_IERR(2,"vecSize (%i) in transform != vecSize (%i) in transform0, this is a bug, or you have loaded an incompatible transform file!",tf->head.vecSize,tf0->head.vecSize);
        return;
      }

      // copy vec data:
      tf->head.nTimeunits = tf0->head.nTimeunits;
      memcpy(tf->vectors, tf0->vectors, sizeof(double) * tf0->head.nVec * tf0->head.vecSize);
      
      // zero remaining:
      if (tf0->head.nVec < tf->head.nVec) {
        memset(tf->vectors+tf0->head.nVec * tf0->head.vecSize, 0, ((tf->head.nVec - tf0->head.nVec) * tf0->head.vecSize) * sizeof(double));
      }
    } else {
      weight = 0.0;
      // zero-init:
      if ((tf->vectors != NULL)&&(tf->head.nVec > 0)&&(tf->head.vecSize > 0)) {
        memset(tf->vectors, 0, tf->head.nVec * tf->head.vecSize * sizeof(double));
      }
    }
    nFrames = (long)tf->head.nTimeunits;
    SMILE_IMSG(2, "nFrames initialised to %ld", nFrames);
    transformInitDone(tf,tf0);
  }
  
}

int cVectorTransform::updateTransform(struct sTfData * tf, const FLOAT_DMEM *src, FLOAT_DMEM *buf,
    long * _bufferNframes, long Nbuf, long wrPtr, int idxi)
{
  if (tf == NULL) return 0;
  if (src == NULL) return 0;
  switch (updateMethod) {
    case UPDMETHOD_EXP: 
      return updateTransformExp(tf,src,idxi);
    case UPDMETHOD_FIX: 
    case UPDMETHOD_FIXI:
      return updateTransformBuf(tf,src,buf,Nbuf,wrPtr,idxi);
    case UPDMETHOD_AVG: 
      return updateTransformAvg(tf,src,idxi);
    case UPDMETHOD_AVGI: 
      return updateTransformAvgI(tf,src,buf,_bufferNframes,Nbuf,wrPtr,idxi);
    case UPDMETHOD_NUL:
      return 0;
    default:
      SMILE_IWRN(2,"unknown update method in updateTransform() : %i\n",updateMethod);
      return 0;
  }
}

void cVectorTransform::updateRingBuffer(const FLOAT_DMEM *src, long Nsrc) 
{
  long i;
  if (nAvgBuffer == fixedBufferFrames) {
    for (i=0; i<Nsrc; i++) {
      // store current input in ringbuffer at position of last input:
      buffer[i + Nsrc*wPtr] = src[i];
    }
  } else {
    for (i=0; i<Nsrc; i++) {
      // store current input in ringbuffer:
      buffer[i + Nsrc*wPtr] = src[i];
    }
    nAvgBuffer++;
  }
  wPtr++;
  if (wPtr >= fixedBufferFrames) wPtr = 0;
}


// a derived class should override this method, in order to implement the actual processing
int cVectorTransform::processVector(const FLOAT_DMEM *src, FLOAT_DMEM *dst,
    long Nsrc, long Ndst, int idxi) // idxi=input field index
{
  int ret = 0;
  long i;

  if (src == NULL)
    return 0;
  if (Nsrc <= 0)
    return 0;
  
  if (mode != MODE_TRANSFORMATION) {
    if ((updateMethod == UPDMETHOD_FIX) || (updateMethod == UPDMETHOD_AVGI)) {
      if (buffer == NULL) {
        nAvgBuffer = 0;
        buffer = (FLOAT_DMEM*)malloc(sizeof(FLOAT_DMEM)*fixedBufferFrames*Nsrc);
      }
      if (updateMethod == UPDMETHOD_AVGI) {
        if (bufferNframes == NULL)
          bufferNframes = (long*)malloc(sizeof(long)*fixedBufferFrames);
      }
    }
  }

  // setup transform struct
  int init = 0;
  if ((transform.userData == NULL)&&(transform.vectors==NULL)) {
    transform.head.vecSize = Nsrc;
    /* hook for allocating vectors and userData in transform data,
         this hook should also set the typeID, etc. */
    allocTransformData(&transform, Ndst, idxi);
    /* copy initial transform (transform0) to current transform (transform) */
    initTransform(&transform, &transform0);
    //printf("nFrames %i\n",nFrames);
    init = 1;
  }

  // TODO : check mismatch between different idxi's and report on this separately?

  if (transform.head.vecSize != Nsrc) {
    if (!err) {
      SMILE_IERR(1,"vecSize of loaded transform data (from '%s') [vecSize=%i] "
          "does not match vecSize of source level and field (%i) [=%i]. I will not apply any "
          "transformation, the output (for this field) will be corrupt. This message "
          "will not be repeated anymore for the following frames.", initFile,
          transform.head.vecSize, idxi, Nsrc);
      err = 1;
    }
    return 0;
  }

  if ((mode == MODE_ANALYSIS)||(mode == MODE_INCREMENTAL)) {

    int _resetMeans = 0;
    lockMessageMemory();
    int _isTurn = isTurn;
    if (resetOnTurn) _resetMeans = resetMeans;
    unlockMessageMemory();

    if ((_resetMeans)&&(!init)) {
      /* copy initial transform (transform0) to current transform (transform) TODO: check this code!!!*/
      if (!transformResetNotify(&transform, &transform0)) {
        if (updateMethod != UPDMETHOD_AVGI) {
          initTransform(&transform, &transform0);
        }
      }
      // reset history buffers:
      if (updateMethod != UPDMETHOD_AVGI) nFrames=0;
      else nFrames = (long)transform.head.nTimeunits;
      wPtr=0; nAvgBuffer = 0;
      lockMessageMemory();
      resetMeans = 0;
      unlockMessageMemory();
    }

    if ((!turnOnlyUpdate)||(_isTurn)) {

      if (nFrames < updateMaxFrames_ || updateMaxFrames_ <= 0) {
        // nFrames is the number of frames which were so-far analysed
        nFrames++; // NOTE: nFrames includes the current frame, when updateTransform is being called!

        /* hook for updating the transform parameters */
        updateTransform(&transform, src, buffer, bufferNframes,
            nAvgBuffer, wPtr, idxi);


        if ((updateMethod == UPDMETHOD_FIX)||(updateMethod == UPDMETHOD_FIXI)) { // manage ring buffer history if fixedBuffer update method
          // add a sample to the ring-buffer
          updateRingBuffer(src,Nsrc);
        } else if (updateMethod == UPDMETHOD_AVGI) {
          if (nFrames > 0) {
            /* update buffer of sums... */
            for (i=0; i<Nsrc; i++) {
              buffer[i + Nsrc*wPtr] = src[i];
            }
            bufferNframes[wPtr] = nFrames;
            wPtr++;
            if (wPtr == fixedBufferFrames) wPtr = 0;
            if (nAvgBuffer < fixedBufferFrames) nAvgBuffer++;
          }
        }
        /* save transform data to SMILEtf format binary file */
        saveTransformData(saveFile_, &transform);
      }
    }

    if (mode == MODE_INCREMENTAL) {
      if ((Ndst <= 0) || (dst == NULL)) return 0;

      if ((!turnOnlyNormalise)||(_isTurn)) {
        // apply transform
        //TODO: check !! transform might be uninitialized !!
        int ret = transformData(&(transform), src, dst, Nsrc, Ndst, idxi);
        //fprintf(stderr,"XXX: instname: %s\n",getInstName());
        checkDstFinite(dst,ret);
        return ret;
      } else {
        // copy without processing, if enabled
        if (turnOnlyOutput) return 0;
        else {
          for (i=0; i<MIN(Ndst,Nsrc); i++) {
            dst[i] = src[i];
          }
          return MIN(Ndst,Nsrc);
        }
      }
    } else { 
      // TODO: output transform parameters to *dst ??
      // this also requires changes in myConfigure and myFinalise ...
      return 0;
    }
  } else {
    if (mode == MODE_TRANSFORMATION) {
      if ((Ndst <= 0) || (dst == NULL)) return 0;
      lockMessageMemory();
      int _isTurn = isTurn;
      unlockMessageMemory();
      /*if (transform.head.vecSize != Nsrc) {
        if (!err) {
          SMILE_IERR(1,"vecSize of loaded transform data (from '%s') [vecSize=%i] "
              "does not match vecSize of source level and field (%i) [=%i]. I will not apply any "
              "transformation, the output (for this field) will be corrupt. This message "
              "will not be repeated anymore for the following frames.", initFile,
              transform.head.vecSize, idxi, Nsrc);
          err = 1;
        }
        return 0;
      }*/
      if ((!turnOnlyNormalise)||(_isTurn)) {
        // apply transform
        int ret = transformData(&(transform), src, dst, Nsrc, Ndst, idxi);
        checkDstFinite(dst,ret);
        return ret;
      } else {
        // copy without processing, if enabled
        if (turnOnlyOutput) return 0;
        else {
          for (i=0; i<MIN(Ndst,Nsrc); i++) {
            dst[i] = src[i];
          }
          return MIN(Ndst,Nsrc);
        }
      }
    }
  }
  // unknown mode....
  return 0;
}

cVectorTransform::~cVectorTransform()
{
  saveFileInterval_ = 1;  // force saving in any case ...
  saveTransformData(saveFile_, &transform);
  if (saveFile_ != NULL)
    free(saveFile_);
  if (buffer != NULL)
    free(buffer);
  if (bufferNframes != NULL)
    free(bufferNframes);
  freeTransformData(&transform);
  freeTransformData(&transform0);
}

