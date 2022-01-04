/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


/*  openSMILE component:

CSV (Comma seperated value) file reader. 
This component reads all columns as attributes into the data memory. One line thereby represents one frame or sample. The first line may contain a header with the feature names (see header=yes/no/auto option).

*/


#include <iocore/csvSource.hpp>



#define MODULE "cCsvSource"

/*Library:
sComponentInfo * registerMe(cConfigManager *_confman) {
  cDataSource::registerComponent(_confman);
}
*/
#define N_ALLOC_BLOCK 50

SMILECOMPONENT_STATICS(cCsvSource)

SMILECOMPONENT_REGCOMP(cCsvSource)
{
  SMILECOMPONENT_REGCOMP_INIT

  scname = COMPONENT_NAME_CCSVSOURCE;
  sdescription = COMPONENT_DESCRIPTION_CCSVSOURCE;

  // we inherit cDataSource configType and extend it:
  SMILECOMPONENT_INHERIT_CONFIGTYPE("cDataSource")

  SMILECOMPONENT_IFNOTREGAGAIN(
    ct->setField("filename","The CSV file to read","input.csv");
    // TODO: support reading a frame time field from the arff file...
    ct->setField("delimChar","The CSV delimiter character to use. Usually ',' or ';'.", ';');
    ct->setField("header","yes/no/auto : wether to read the first line of the CSV file as header (yes), or treat it as numeric data (no), or automatically determine from the first field in the first line whether to read the header or not (auto).","auto");
    ct->setField("start", "Start at line 'start', not counting the header line. The first line after the (optional) header line is line 0 (also the default).", 0);
    ct->setField("end", "Read up to line 'end'. The number of the line given here is the last line that will be read. 0 is the first line in the file (excluding header). The default '-1' refers to the last line in the file (this is also the default).", -1);
    ct->setField("readFrameTime", "1 = read frameTime from arff field 'frameTime'. The frameTime field is not loaded into dataMemory then.", 0);

    //ct->setField("selection","indicies of attributes to read (separate indicies by , and specify ranges with a-b e.g. 1-5,7,8). Default ('all') is to read all numeric/real attributes.","all");
  )

  SMILECOMPONENT_MAKEINFO(cCsvSource);
}

SMILECOMPONENT_CREATE(cCsvSource)

//-----

cCsvSource::cCsvSource(const char *_name) :
  cDataSource(_name),
  field(NULL),
  lineNr(0),
  eof(0),
  header(0),
  delimChar(','),
  frameTimeNr_(-1),
  readFrameTime_(false)
{
}

#define HEADER_AUTO  0
#define HEADER_FORCE 1
#define HEADER_OMIT  2

void cCsvSource::myFetchConfig()
{
  cDataSource::myFetchConfig();
  
  start = getInt("start");
  end = getInt("end");

  filename = getStr("filename");
  SMILE_IDBG(2,"filename = '%s'",filename);
  
  delimChar = getChar("delimChar");
  SMILE_IDBG(2,"delimChar = %c",delimChar);
  readFrameTime_ = (getInt("readFrameTime") == 1);

  const char *_header = getStr("header");
  if (!strncasecmp(_header,"auto",4)) {
    header = HEADER_AUTO;
  } else if(!strncasecmp(_header,"yes",3)) {
    header = HEADER_FORCE;
  } else if (!strncasecmp(_header,"no",2)) {
    header = HEADER_OMIT;
  } else {
    header = HEADER_AUTO;
    SMILE_IWRN(1,"unknon value for 'header' parameter: '%s'. Allowed values are: yes / no / auto .",_header);
  }
}

int cCsvSource::configureWriter(sDmLevelConfig &c) 
{
  // if readFrameTime is disabled, we don't store anything in tmeta and can disable it
  c.noTimeMeta = !readFrameTime_;
  return 1;
}

void cCsvSource::setGenericNames(int nDelim) 
{
  writer_->addField("csvdata", nDelim + 1);
}

void cCsvSource::setNamesFromCSVheader(char *line, int nDelim) 
{
  int i = 0;
  int N = 0;
  int len = (int)strlen(line);
  const char * start = line;
  for (i = 0; i < len; i++) {
    if ((line[i] == delimChar)||(line[i]==0)||(line[i]=='\n')||(line[i]=='\r')) {
      line[i] = 0;
      if (start != NULL && start != line+i) {
        if (frameTimeNr_ == -1 && readFrameTime_ && !strncmp(start, "frameTime", 9)) {
          frameTimeNr_ = N;
          SMILE_IMSG(2, "Found frameTime attribute at index %i (0 is first).", N);
        } else {
          writer_->addField(start, 1);
        }
        N++;
      }
      start = line+i+1;
    }
  }
  if (frameTimeNr_ == -1 && readFrameTime_)
    SMILE_IWRN(2, "readFrameTime == true, but no field called 'frameTime' found in input CSV! Perhaps a naming issue?");
}


int cCsvSource::setupNewNames(long nEl)
{
  // read header line and split names
  int quote1=0, quote2=0;
  char *line=NULL;
  size_t n,read;
  int nDelim = 0;
  read = smile_getline(&line, &n, filehandle);
  if ((read > 0)&&(line!=NULL)) {
    lineNr++;
    // count number of unquoted delim chars  (quoting via ' or ")
    int len = (int)strlen(line);
    char * firstEl = NULL;
    
    int i;
    for (i=0; i<len; i++) {
      if ((!quote2)&&(line[i] == '\'')) { quote1 = !quote1; }
      else if ((!quote1)&&(line[i] == '"')) { quote2 = !quote2; }
      if ((quote1+quote2 == 0)&&(line[i] == delimChar)) { 
        if (nDelim == 0) { line[i] = 0; firstEl = strdup(line); line[i] = delimChar; }
        nDelim++;
      }
    }

    // allocate field selection map:
    field = (int*)malloc(sizeof(int)*(nDelim+1));
    // select all by default:
    for (i=0; i<=nDelim; i++) field[i] = 1;

    // set names, either generic or from csv "pseudo" header (first line)
    if (header == HEADER_AUTO) {
      // check first Element
      int head = 1;
      char *eptr=NULL;
      if (firstEl == NULL) {
        SMILE_IWRN(2, "Only one element on first line of CSV file '%s'. Did you configure the right delimiter character!? delimChar = '%c'", filename, delimChar);
        strtol(line, &eptr, 10);
        // if the first field is not a pure numeric field, assume it is a header
        if ((line[0] != 0) && ((eptr == NULL) || (eptr[0] == 0))) {
          head = 0;
        }
      } else {
        strtol(firstEl, &eptr, 10);
        // if the first field is not a pure numeric field, assume it is a header
        if ((firstEl[0] != 0) && ((eptr == NULL) || (eptr[0] == 0))) {
          head = 0;
        }
      }

      if (head) { 
        SMILE_IMSG(3,"automatically detected first line of CSV input as HEADER!");
        setNamesFromCSVheader(line,nDelim); 
      } else { 
        SMILE_IMSG(3,"automatically detected first line of CSV input as NUMERIC DATA! (No header is present, generic element names will be used).");
        setGenericNames(nDelim); rewind(filehandle); lineNr=0; 
      }

    } else if (header == HEADER_FORCE) {
      setNamesFromCSVheader(line, nDelim);
    } else {
      setGenericNames(nDelim);
      rewind(filehandle);
      lineNr=0;
    }
    if (firstEl != NULL) free(firstEl);
    //return nDelim+1;
    if (nDelim == 0) {
        // this should only be a warning, not an error (only 1 feature in CSV --> no delimiters)
        SMILE_IWRN(1,"no delimiter chars ('%c') found in first line of the CSV file: \n  %s\n    Have you selected the correct delimiter character?",delimChar,line);
    }
  }
  
  // TODO: nFields will actually reflect the true number of selected fields, nCols will reflect the number of columns in the first line of the csv file
  if (readFrameTime_ && frameTimeNr_ != -1)
    nDelim--;
  nCols = nDelim + 1;
  nFields = nDelim + 1;
  if (line != NULL)
    free(line);
  allocVec(nFields);
  namesAreSet_=1;
  // exclude frameTime field from counting...

  return nDelim + 1;
}

int cCsvSource::myFinaliseInstance()
{
  filehandle = fopen(filename, "r");
  if (filehandle == NULL) {
    COMP_ERR("Error opening file '%s' for reading (component instance '%s', type '%s')",filename, getInstName(), getTypeName());
  }

  int ret = cDataSource::myFinaliseInstance();

  if (ret == 0) {
    fclose(filehandle); filehandle = NULL;
  }
  return ret;
  
}


eTickResult cCsvSource::myTick(long long t)
{
  if (isEOI()) return TICK_INACTIVE;

  SMILE_IDBG(4,"tick # %i, reading value vector from CSV file",t);
  if (eof) {
    SMILE_IDBG(4,"EOF, no more data to read");
    return TICK_INACTIVE;
  }

  int ret = 0;

  // read # blocksizeW lines from file
  int b;
  for (b=0; b<blocksizeW_; b++) {

    // if there's not enough space to write this frame but we have already written some frames, return TICK_SUCCESS
    if (!(writer_->checkWrite(1))) return ret ? TICK_SUCCESS : TICK_DEST_NO_SPACE;

    size_t n,read;
    char *line=NULL;
    int l=1;
    int len=0;
    int i=0, ncnt=0;
    // get next non-empty line in range [start; end]
    do {
      read = smile_getline(&line, &n, filehandle);
      if ((read != -1)&&(line!=NULL)) {
        lineNr++;
        if (lineNr > start && (lineNr - 1 <= end || (end == -1))) {
          len=(int)strlen(line);
          if (len>0) { if (line[len-1] == '\n') { line[len-1] = 0; len--; } }
          if (len>0) { if (line[len-1] == '\r') { line[len-1] = 0; len--; } }
          while (((line[0] == ' ')||(line[0] == '\t'))&&(len>=0)) { line[0] = 0; line++; len--; }
          if (len > 0) {
            l=0; // <- termination of while loop, when a non-empty line was found
            char *x, *x0=line;
            int nDel=0;
            do {
              x = strchr(x0,delimChar);
              if (x!=NULL) {
                *(x++)=0;
                nDel++;
              }
              if (field[i]) { // if this field is selected for import
                // convert value in x0
                if (readFrameTime_ && frameTimeNr_ != -1 && i == frameTimeNr_) {
                  char *ep = NULL;
                  double val = strtod(x0, &ep);
                  if ((val==0.0)&&(ep==x0)) {
                    SMILE_IERR(1,"error parsing frameTime value in csv file '%s' (line %i), expected double value "
                        "(element %i).", filename, lineNr, i);
                  }
                  vec_->tmeta->time = val;
                  nDel--;
                } else {
                  if (ncnt < nFields) {
                    char *ep = NULL;
                    double val = strtod(x0, &ep);
                    if ((val==0.0)&&(ep==x0)) {
                      SMILE_IERR(1,"error parsing numeric value in CSV file '%s'"
                          " (line %i), expected float/int value (element %i).",
                          filename, lineNr, i);
                    }
                    vec_->data[ncnt++] = (FLOAT_DMEM)val;
                  } else {
                    SMILE_IWRN(2,"trying to import more fields than selected (%i>%i) on "
                        "line %i of CSV file '%s'. Ignoring the excess fields!",
                        ncnt, nFields, lineNr, filename);
                  }
                }
              }
              if (x!=NULL) {
                i++;
                x0=x;
              }
            } while (x!=NULL);
            if (nDel != nCols-1) {
              SMILE_IWRN(2,"numer of columns (%i) on line %i of CSV file '%s' does not"
                  " match the number of expected columns (%i) (read from first line or "
                  "file header)", nDel + 1, lineNr, filename, nCols);
            }
          }
        }
      } else {
        l = 0; // EOF....  signal EOF...???
        eof = 1;
      }
    } while (l);
    if (line != NULL)
      free(line);
    if (!eof) {
      if (ncnt < nFields) {
        SMILE_IWRN(1,"less elements than expected (%i < %i) on line %i of CSV file '%s'",
            ncnt, nFields, lineNr, filename);
      }
      writer_->setNextFrame(vec_);
      ret = 1;
    } else { break; }

  }

  return ret ? TICK_SUCCESS : TICK_INACTIVE;
}


cCsvSource::~cCsvSource()
{
  if (filehandle!=NULL) fclose(filehandle);
  if (field != NULL) free(field);
}
