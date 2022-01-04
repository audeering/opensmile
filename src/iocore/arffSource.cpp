/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


/*  openSMILE component:

example dataSource
writes data to data memory...

*/


#include <iocore/arffSource.hpp>



#define MODULE "cArffSource"

/*Library:
sComponentInfo * registerMe(cConfigManager *_confman) {
  cDataSource::registerComponent(_confman);
}
*/
#define N_ALLOC_BLOCK 50

SMILECOMPONENT_STATICS(cArffSource)

//sComponentInfo * cArffSource::registerComponent(cConfigManager *_confman)
SMILECOMPONENT_REGCOMP(cArffSource)
{
  SMILECOMPONENT_REGCOMP_INIT
/*  if (_confman == NULL) return NULL;
  int rA = 0;
*/
  scname = COMPONENT_NAME_CARFFSOURCE;
  sdescription = COMPONENT_DESCRIPTION_CARFFSOURCE;

  // we inherit cDataSource configType and extend it:
  SMILECOMPONENT_INHERIT_CONFIGTYPE("cDataSource")
/*
  ConfigType *ct = new ConfigType( *(_confman->getTypeObj("cDataSource")) , scname );
  if (ct == NULL) {
    SMILE_IWRN(4,"cDataSource config Type not found!");
    rA=1;
  }*/
  
  SMILECOMPONENT_IFNOTREGAGAIN(
    ct->setField("filename","The filename of the ARFF file to read","input.arff");
    // TODO: support reading a frame time field from the arff file...
    ct->setField("skipFirst","the number of numeric(!) attributes to skip at the beginning",0);
    ct->setField("skipClasses","The number of numeric(!) (or real) attributes (values) at end of each instance to skip (Note: nominal and string attributes are ignored anyway, this option only applies to the last numeric attributes, even if they are followed by string or nominal attributes). To have more fine-grained control over selecting attributes, please use the component cDataSelector!",0);
    ct->setField("saveInstanceIdAsMetadata","1/0 = on/off : save the first string attribute of the arff file as instance ID string in the vector metadata (e.g. for use by the winToVecProcessor component in the frameMode=meta mode).",0);
    ct->setField("saveTargetsAsMetadata","1/0 = on/off : save everything after the last numeric attribute as metadata string. This can be read by arffSink and appended to the instances instead of reading individual targets from the config.",0);
    ct->setField("readFrameTime", "1 = read frameTime from arff field 'frameTime'. The field frameTime is not automatically skipped, use the skipFirst option to skip the first N elements.", 0);
    ct->setField("readFrameLength", "1 = read frameLength from arff field 'frameLength'. The field framelength is not automatically skipped, use the skipFirst option to skip the first N elements.", 0);
    ct->setField("frameTimeIndex", "if set to >= 0, specifies the index of the attribute that is the frameTime attribute. The default (-1) will enable autodetection based on the attribute name 'frameTime'. If readFrameTime==0, then this option has no effect.", -1);
    ct->setField("frameLengthIndex", "if set to >= 0, specifies the index of the attribute that is the frameLength attribute. The default (-1) will enable autodetection based on the attribute name 'frameLength'. If readFrameLength==0, then this option has no effect.", -1);
    // TODO: add classes as custom meta data...?

    //ct->setField("selection","indicies of attributes to read (separate indicies by , and specify ranges with a-b e.g. 1-5,7,8). Default ('all') is to read all numeric/real attributes.","all");
  )

/*
    ConfigInstance *Tdflt = new ConfigInstance( scname, ct, 1 );
    _confman->registerType(Tdflt);
  } else {
    if (ct!=NULL) delete ct;
  }
*/
  SMILECOMPONENT_MAKEINFO(cArffSource);
}

SMILECOMPONENT_CREATE(cArffSource)

//-----

cArffSource::cArffSource(const char *_name) :
  cDataSource(_name),
  field(NULL),
  fieldNalloc(0),
  lineNr(0),
  eof(0),
  lastNumeric(-1),
  strField(-1),
  filehandle(NULL),
  origline(NULL), lineLen(0)
{
}

void cArffSource::myFetchConfig()
{
  cDataSource::myFetchConfig();
  filename = getStr("filename");
  skipClasses = getInt("skipClasses");
  skipFirst = getInt("skipFirst");
  useInstanceID = getInt("saveInstanceIdAsMetadata");
  saveClassesAsMetadata = getInt("saveTargetsAsMetadata");
  readFrameTime_ = (getInt("readFrameTime") == 1);
  readFrameLength_ = (getInt("readFrameLength") == 1);
  frameTimeNr_ = getInt("frameTimeIndex");
  frameLengthNr_ = getInt("frameLengthIndex");
}

/*
int cArffSource::myConfigureInstance()
{
    // call writer->setConfig here to set the dataMemory level configuration and override config file and defaults...
//  double T = 1.0 / (double)(pcmParam.sampleRate);
//  writer->setConfig(1,2*buffersize,T, 0.0, 0.0 , 0, DMEM_FLOAT);  // lenSec = (double)(2*buffersize)*T

  int ret = 1;
  ret *= cDataSource::myConfigureInstance();
  // ...
  return ret;
}
*/

int cArffSource::configureWriter(sDmLevelConfig &c) 
{
  // if these options are disabled, we don't store anything in tmeta and can disable it
  c.noTimeMeta = !useInstanceID && !saveClassesAsMetadata && !readFrameTime_ && !readFrameLength_;
  return 1;
}

int cArffSource::setupNewNames(long nEl)
{
  // read header lines...
  int ret=1;
  long read;
  char *line;
  int head=1;
  int fnr = 0;
  int nnr = 0;
  // count attributes
  int nAttributes = 0;
  if (filehandle != NULL) {
    fclose(filehandle);
  }
  filehandle = fopen(filename, "r");
  if (filehandle == NULL) {
    COMP_ERR("Error opening file '%s' for reading (component instance '%s', type '%s')",filename, getInstName(), getTypeName());
  }
  do {
    read = smile_getline(&origline, &lineLen, filehandle);
    line = origline;
    if ((read > 0)&&(line!=NULL)) {
      lineNr++;
      if ( (!strncasecmp(line, "@attribute ", 11)) ) {
        char *name = line + 11;
        while (*name == ' ') name++;
        char *type = strchr(name,' ');
        if (type != NULL) {
          *(type++) = 0;
          while (*type == ' ') type++; // skip spaces here too
          if ((!strncasecmp(type, "real", 4))
              || (!strncasecmp(type, "numeric", 7))) {
            // add numeric attribute:
            nAttributes++;
          }
        }
      } else if ( (!strncasecmp(line, "@data", 5)) ) {
        head = 0;
      }
    } else {
      // ERROR: EOF in header!!!
      head = 0;
      eof = 1;
      SMILE_IERR(1,"incomplete arff file '%s', could not find '@data' line!",filename);
      ret = 0;
    }
  } while (head);
  if (ret == 0) {
    return 0;
  }
  SMILE_IMSG(3, "Arff file '%s' has %i numeric attributes.", filename, nAttributes);
  head = 1;
  fclose(filehandle);
  filehandle = fopen(filename, "r");
  if (filehandle == NULL) {
    COMP_ERR("Error opening file '%s' for reading (component instance '%s', type '%s')",
        filename, getInstName(), getTypeName());
  }
  do {
    // TODO: specify index-based selection of attributes!
    read = smile_getline(&origline, &lineLen, filehandle);
    line = origline;
    if ((read > 0)&&(line!=NULL)) {
      lineNr++;
      if ( (!strncasecmp(line,"@attribute ",11)) ) {
        char *name = line + 11;
        /* bugfix thanks to Ina: skip spaces between @attribute and name! */
        while (*name == ' ') name++;
        char *type = strchr(name,' ');
        if (type != NULL) {
          *(type++) = 0;
          while (*type == ' ') type++; // skip spaces here too
          if ((!strncasecmp(type,"real",4))||(!strncasecmp(type,"numeric",7))) { // add numeric attribute:
            // Look for frameTime and frameLength attributes, ignore frameIndex
            if (frameTimeNr_ == -1 && readFrameTime_ && !strncmp(name, "frameTime", 9)) {
              frameTimeNr_ = fnr;
              SMILE_IMSG(2, "Found frameTime attribute at index %i (0 ist first).", fnr);
            }
            if (frameLengthNr_ == -1 && readFrameLength_ && !strncmp(name, "frameLength", 9)) {
              frameLengthNr_ = fnr;
              SMILE_IMSG(2, "Found frameLength attribute at index %i (0 is first).", fnr);
            }
            if (nnr < nAttributes - skipClasses) {
              if (nnr >= skipFirst) {
                // convert [] to _ to avoid confusions with field indices in element names
                // (field names may not contain [] characters)
                // TODO: check for [] at end of name and accumulate names to add as array field??
                //   this would require a completely new arff header parsing structure
                //   possibly using a map of objects to organise the names and types, etc.
                for (int i = 0; i < strlen(name); ++i) {
                  if (name[i] == '[' || name[i] == ']')
                    name[i] = '_';
                }
                writer_->addField(name,1);
                if (fnr >= fieldNalloc) {
                  field = (int*)crealloc( field, sizeof(int)*(fieldNalloc+N_ALLOC_BLOCK),
                      sizeof(int)*(fieldNalloc) );
                  fieldNalloc += N_ALLOC_BLOCK;
                }
                field[fnr] = 1;
                lastNumeric = fnr;
              }
              nnr++;
            }
            // TODO: detect array fields [X]
          } else if (!strncasecmp(type,"string",6)) {
            if ((strField == -1)&&(useInstanceID))
              strField = fnr; // set strField to point to the FIRST string attribute in the arff file
          }
          fnr++;

        } else { // ERROR:...
          ret=0;
        }
      } else if ( (!strncasecmp(line,"@data",5)) ) {
        head = 0;
      }
    } else {
      head = 0; eof=1;
      SMILE_IERR(1,"incomplete arff file '%s', could not find '@data' line!",filename);
      ret=0;
    } // ERROR: EOF in header!!!
  } while (head);
  //if (line!=NULL) free(line);

  // skip 'skipClasses' numeric classes from end
  /*
  if (skipClasses) {
    int i;
    int s=skipClasses;
    for (i=fnr-1; i>=0; i++) {
      if (field[i]) { field[i]=0; s--; nnr--; }
      if (s<=0) break;
    }
  }*/
 
  nFields = fnr;
  nNumericFields = nnr-skipFirst;

  allocVec(nnr-skipFirst);
  if (useInstanceID || saveClassesAsMetadata) {
    if (vec_->tmeta->metadata == NULL) {
      vec_->tmeta->metadata = std::unique_ptr<cVectorMeta>(new cVectorMeta());
    }
  }

  namesAreSet_=1;
  return 1;
}

int cArffSource::myFinaliseInstance()
{
  int ret = cDataSource::myFinaliseInstance();
  if (ret == 0) {
    fclose(filehandle);
    filehandle = NULL;
  }
  return ret;
  
}


eTickResult cArffSource::myTick(long long t)
{
  if (isEOI()) return TICK_INACTIVE;
  
  SMILE_IDBG(4,"tick # %i, reading value vector from arff file",t);
  if (eof) {
    SMILE_IDBG(4,"EOF, no more data to read");
    return TICK_INACTIVE;
  }

  if (!(writer_->checkWrite(1))) return TICK_DEST_NO_SPACE;
  
  size_t n=0,read;
  char *line=NULL;
  int l=1;
  int len=0;
  int i=0, ncnt=0;
  // get next non-empty line
  do {
    read = smile_getline(&origline, &lineLen, filehandle);
    line = origline;
    if ((read != -1)&&(line!=NULL)) {
      lineNr++;
      len=(int)strlen(line);
      if (len>0) { if (line[len-1] == '\n') { line[len-1] = 0; len--; } }
      if (len>0) { if (line[len-1] == '\r') { line[len-1] = 0; len--; } }
      while (((line[0] == ' ')||(line[0] == '\t'))&&(len>=0)) { line[0] = 0; line++; len--; }
      if (len > 0) {
        l=0;
        char *x, *x0=line;
        do {
          x = strchr(x0,',');
          if (x!=NULL) {
            *(x++)=0;
          }
          if (field[i]) { // if this field is numeric/selected
            // convert value in x0
            char *ep=NULL;
            double val = strtod(x0, &ep);
            if ((val==0.0)&&(ep==x0)) { SMILE_IERR(1,"error parsing value in arff file '%s' (line %i), expected double value (element %i).",filename,lineNr,i); }
            if (ncnt < vec_->N) vec_->data[ncnt++] = (FLOAT_DMEM)val;
            else { SMILE_IERR(1,"more elements in field selection (%i) than allocated in vector (%i)!",ncnt,vec_->N); } // <- should never happen?
          }
          if (readFrameTime_ && i == frameTimeNr_) {
            char *ep=NULL;
            double val = strtod(x0, &ep);
            if ((val==0.0)&&(ep==x0)) {
              SMILE_IERR(1,"error parsing frameTime value in arff file '%s' (line %i), expected double value "
                  "(element %i).",filename,lineNr,i);
            }
            vec_->tmeta->time = val;
          }
          if (readFrameLength_ && i == frameLengthNr_) {
            char *ep=NULL;
            double val = strtod(x0, &ep);
            if ((val==0.0)&&(ep==x0)) {
              SMILE_IERR(1,"error parsing frameLength value in arff file '%s' (line %i), expected double value "
                  "(element %i).",filename,lineNr,i);
            }
            vec_->tmeta->lengthSec = val;
          }
          if (i==strField) { // if instance label string field .. add name to vector meta data
            if (vec_->tmeta->metadata->text != NULL) {
              free( vec_->tmeta->metadata->text);
            } 
            vec_->tmeta->metadata->text = strdup(x0);
          }
          if (x!=NULL) {
            // TODO: check for end of numeric attributes.... and add rest as "tmeta->metadata->custom pointer!"
            if ((saveClassesAsMetadata)&&(i>=lastNumeric)&&(lastNumeric>=1)) {
              // add the remaining string to the custom info
              vec_->tmeta->metadata->iData[1] = 1234;
              long ll = (long)(strlen(x)+1);
              if (ll > vec_->tmeta->metadata->customLength) {
                if (vec_->tmeta->metadata->custom != NULL) free(vec_->tmeta->metadata->custom);
                vec_->tmeta->metadata->custom = strdup(x);
                vec_->tmeta->metadata->customLength = ll;
              } else {
                memcpy(vec_->tmeta->metadata->custom, x, ll);
              }
              x = NULL;
            } else {
              i++;
              x0=x;
            }
          }
        } while (x!=NULL);
      }
      //free(line); line = NULL;
    } else {
      l=0; // EOF....  signal EOF...???
      eof=1;
    }
  } while (l);

  if (!eof) {
//    SMILE_IMSG(1,"metadata->text = '%s'",vec->tmeta->metadata->text);
    writer_->setNextFrame(vec_);
    return TICK_SUCCESS;
  }
  
  return TICK_INACTIVE;
}


cArffSource::~cArffSource()
{
  if (filehandle!=NULL) fclose(filehandle);
  if (field != NULL) free(field);
  if (origline != NULL) free(origline);
}
