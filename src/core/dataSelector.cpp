/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


/*  openSMILE component:

selects data elements by element name

*/


#include <core/dataSelector.hpp>

#define MODULE "cDataSelector"

#define MAX_LINE_LENGTH 2048

SMILECOMPONENT_STATICS(cDataSelector)

SMILECOMPONENT_REGCOMP(cDataSelector)
{
  SMILECOMPONENT_REGCOMP_INIT

    scname = COMPONENT_NAME_CDATASELECTOR;
  sdescription = COMPONENT_DESCRIPTION_CDATASELECTOR;

  // we inherit cDataProcessor configType and extend it:
  SMILECOMPONENT_INHERIT_CONFIGTYPE("cDataProcessor")

    SMILECOMPONENT_IFNOTREGAGAIN(
    ct->setField("selFile","The name of the data selection file to load. The file is a text file containing one element name or range of indices per line of the elements which should be selected (case-sensitive!). (Note: the first two lines make up a header; the first line is either 'str' or 'idx', the second line is number of following lines in the file, next each line contains one feature name or index range of the features to select)",(const char*)NULL);
  ct->setField("selected","This is an alternative to loading 'selFile'. An array of exact (case-sensitive) names of features / data elements to select.",(const char *)NULL, ARRAY_TYPE);
  ct->setField("selectedRange","This is an alternative to loading 'selFile'. It specifies the selected elements(!) by element index ranges. Each array element contains one range in the format of <start>-<end>, where <start> and <end> are the indicies of the startt and end features to *select* in this range. '$' specifies the last element, and '1' indicates the first element. If the '-' is missing, only a single element will be selected, not a range. NOTE: the 'newNames' option does not work in conjunction with this element selection method!",(const char*)NULL,ARRAY_TYPE);
  ct->setField("newNames","An array of new names to assign to the selected features / data elements (optional). The order thereby corresponds to the order of data element names in the input.",(const char *)NULL, ARRAY_TYPE);
  ct->setField("elementMode","1 = select elements exactly as given in 'selected' array or in 'selFile' (in this case, only full element names are allowed (i.e. mfcc[1], mfcc[2] instead of mfcc, mfcc[], or mfcc[1-2]\n   0 = automatically copy arrays or partial arrays, e.g. if field[1-4] or only 'field' is given as name in the selction array/file, then the partial (1-4) or complete field will be copied to the output",1);
  ct->setField("dummyMode","1 = don't set up output level names. Use this option temporarily, to get a working set-up where you can read the input level names, to set up your selection list.",0);
  ct->setField("outputSingleField", "If this is set to a string, the output will be a single field (with the name set by this string) for all the elements in the input.", (const char *)NULL);
  )

    SMILECOMPONENT_MAKEINFO(cDataSelector);
}


SMILECOMPONENT_CREATE(cDataSelector)

//-----

cDataSelector::cDataSelector(const char *_name) :
cDataProcessor(_name),
  selFile(NULL),
  names(NULL),
  mapping(NULL),
  elementMode(0),
  selectionIsRange(0),
  idxSelected(NULL),
  vecO(NULL), outputSingleField(NULL)
{
}

int cDataSelector::loadSelection()
{
  if (selFile != NULL) {
    if (strlen(selFile)<1) return 0;

    FILE *f = fopen(selFile,"r");
    if (f== NULL) {
      SMILE_IERR(2,"error opening feature selection file '%s' for reading! NOT using a feature selection!",selFile);
      return 0;
    }

    // read first line to determine filetype:
    char line[MAX_LINE_LENGTH+1];
    long nStr=0;
    fgets( line, 5, f);
    line[3] = 0;
    if (!strcmp(line,"str")) { // string list
      fselType = 2;
      SMILE_IDBG(5,"reading string list of features");
      fscanf( f, "%ld\n", &nStr);
      if (nStr < 1) { COMP_ERR("Error reading feature selection file, nFeatures < 1!"); }

      nSel = nStr;
      names = (char**)calloc(1,sizeof(char*) * nSel);

      int i=0; line[0] = 0;
      while(fgets(line,MAX_LINE_LENGTH,f) != NULL) {
        if (strlen( line ) > 1) { 
          if (i<nStr) {
            line[strlen( line ) - 1] = 0;
            names[i++] = strdup(line);
          } else { 
            SMILE_IERR(1,"excess line in feature selection file '%s' : '%s' (expected only %i lines with features)",selFile,line,nStr);
          }
        }
      }
      if (nSel > i) {
        SMILE_IWRN(1,"less feature names (only %i) in feature selection file '%s' than specified in the header (%i).",i,selFile,nSel);
        nSel = i;
      }
      /*
      while(fgets(line,MAX_LINE_LENGTH,f) != NULL) {
      if (strlen( line ) > 1) { 
      line[strlen( line ) - 1] = 0;
      names[i++] = strdup(line);
      }
      }*/
      SMILE_IDBG(5,"selected %i features",i);
      fclose(f);
      return 1;
    } else if (!strcmp(line,"idx")) { // index list      
      fselType = 1;
      selectionIsRange = 1;
      elementMode = 1;
      SMILE_IDBG(5,"reading index list of features");
      fscanf( f, "%ld\n", &nStr);
      if (nStr < 1) { COMP_ERR("Error reading feature selection file, nFeatures < 1!"); }

      nSel = nStr;
      names = (char**)calloc(1,sizeof(char*) * nSel);

      int i=0; line[0] = 0;
      while(fgets(line,MAX_LINE_LENGTH,f) != NULL) {
        if (strlen( line ) > 1) { 
          if (i<nStr) {
            line[strlen( line ) - 1] = 0;
            names[i++] = strdup(line);
          } else { 
            SMILE_IERR(1,"excess line in feature selection file '%s' : '%s' (expected only %i lines with features)",selFile,line,nStr);
          }
        }
      }
      if (nSel > i) {
        SMILE_IWRN(1,"less feature indices (only %i) in feature selection file '%s' than specified in the header (%i).",i,selFile,nSel);
        nSel = i;
      }
      /*
      while(fgets(line,MAX_LINE_LENGTH,f) != NULL) {
      if (strlen( line ) > 1) { 
      line[strlen( line ) - 1] = 0;
      names[i++] = strdup(line);
      }
      }*/
      SMILE_IDBG(5,"selected %i features",i);
      fclose(f);
      return 1;
    } else { // bogus file...
      //TODO: read raw list of names??
      fclose( f );
      COMP_ERR("error parsing fselection file '%s'. bogus header! expected 'str' or 'idx' at beginning. found '%s'.",selFile,line);
    }
  }
  return 0;
}

void cDataSelector::myFetchConfig()
{
  cDataProcessor::myFetchConfig();

  dummyMode = getInt("dummyMode");

  outputSingleField = getStr("outputSingleField");

  selFile = getStr("selFile");
  if (selFile == NULL) {

    // load names of selected features:
    nSel = getArraySize("selected");
    if (nSel>0) {

      names = (char**)calloc(1,sizeof(char*) * nSel);
      int i;
      for (i=0; i<nSel; i++) {
        names[i] = (char *)getStr_f(myvprint("selected[%i]",i));
        if (names[i] == NULL) {
          SMILE_IERR(1, "The %i-th element of the 'selected' names list in the config is empty or invalid. Please fix your config (check for double ; !).", i + 1);
          COMP_ERR("aborting");
        }
        SMILE_IDBG(2,"selected: '%s'",names[i]);
      }
    } else {
      // get feature selection by index ranges:
      nSel = getArraySize("selectedRange");
      if (nSel > 0) {
        names = (char**)calloc(1,sizeof(char*) * nSel);

        int i;
        for (i=0; i<nSel; i++) {
          names[i] = (char *)getStr_f(myvprint("selectedRange[%i]",i));
          SMILE_IDBG(2,"selectedRange: '%s'",names[i]);
        }
        selectionIsRange = 1;
      } else {
        SMILE_IERR(1,"no features selected, this does not make sense!");
        COMP_ERR("stopping here");
      }
    }
    if (selectionIsRange) { elementMode = 1; }
    else {
      elementMode = getInt("elementMode");
      if (elementMode) { SMILE_IDBG(2,"exact element name matching enabled"); }
    } 
  } else {
    SMILE_IDBG(2,"exact element name matching enabled");
    SMILE_IDBG(2,"loading feature selection from file '%s' (this forces elementMode=1 !).",selFile);
    elementMode = 1;
    loadSelection();
  }

}

/*
int cDataSelector::myConfigureInstance()
{
int ret=1;
// if you would like to change the write level parameters... do so HERE:

//
// .. or override configureWriter() to do so, after the reader config is available!
//
ret *= cDataProcessor::myConfigureInstance();
return ret;
}
*/

/*
int cDataSelector::configureWriter(const sDmLevelConfig *c)
{

// we must return 1, in order to indicate configure success (0 indicates failure)
return 1;
}
*/

// TODO!!!!!!!!!!   -> setupNewNames instead of finaliseInstance

// field selection -> fields (arrays) will be added as arrays in output
// element selection -> every output field will be one element
// how to choose the mode?? 
// --> a) if array index is given -> element selection
// --> b) if no array index for an array is given (field selection, select full field)
// --> c) if array index contains "-", assume range -> field selection (partial)

int cDataSelector::setupNewNames(long nEl)
{
  if (dummyMode) {
    addNameAppendField(NULL,"dummy");
    namesAreSet_=1;
    return 1;
  }
  // match our list of selected names to the list of names obtained from the reader
  long i,j,n;
  if (elementMode) {
    long N = reader_->getLevelN();  
    nElSel = 0;
    if (selectionIsRange) {
      idxSelected = (long *)calloc(1,sizeof(long)*N);
      for (i=0; i<nSel; i++) {
        long start, end;
        char * sep = strchr(names[i],'-');
        char *eptr=NULL;
        if (sep != NULL) { //range
          *sep = 0; 
          start = strtol(names[i],&eptr,10); if (eptr == names[i]) { SMILE_IERR(1,"parse error in config option 'selectedRange[%i]' : '%s' ! Expected a range <start>-<end>! Negative numbers are not allowed!",i,names[i]); }
          if (*(sep+1) == '$') {
            end = N-1;
          } else {
            end = strtol(sep+1,&eptr,10); if (eptr == sep+1) { SMILE_IERR(1,"parse error in config option 'selectedRange[%i]' : '%s' ! Expected a range <start>-<end>! Negative numbers are not allowed!",i,names[i]); }
          }
        } else { // single number
          start = strtol(names[i],&eptr,10); if (eptr == names[i]) { SMILE_IERR(1,"parse error in config option 'selectedRange[%i]' : '%s' ! Expected a single integer number (or a range with '-')! Negative numbers are not allowed!",i,names[i]); }
          end = start;
        }
        if (start > end) { 
          SMILE_IERR(1,"start element index (%i) cannot be greater than end elemet index (%i) ! Setting start = %i",start,end,end);
          start = end;
        }
        if (end>=N) {
          SMILE_IERR(1,"end element index (%i) out of range! Must be < %i. Setting end = %i",end,N,N-1);
          end = N-1;
        }
        nElSel += end-start+1;
        for (j=start; j<=end; j++) {
          idxSelected[j] = 1;
        }
      }
      int nElem = 0;
      for (i=0; i<N; i++) {
        if (idxSelected[i]) {
          if (outputSingleField != NULL) {
            nElem++;
          } else {
            char * nnn = reader_->getElementName(i);
            if (nnn!=NULL) {
              SMILE_IMSG(5,"selected element %i, name '%s'\n",i,nnn);
              writer_->addField(nnn,1);
              free(nnn);
            }
          }
          // TODO: check for full array fields in range and add them as original array fields...??
        }
      }
      if (outputSingleField != NULL) {
        writer_->addField(outputSingleField, nElem);
      }
    } else {
      mapping = (sDataSelectorSelData *)calloc(1,sizeof(sDataSelectorSelData) * nSel);
      int *isFound = (int*)calloc(1,sizeof(int)*nSel);
      for (n=0; n<N; n++) {
        //  int found=0;
        //  SMILE_IDBG(4,"selstr %i of %i",i,outputSelStr.n);
        char * tmpname = reader_->getElementName(n);
        for (i=0; i<nSel; i++) {
          if (!isFound[i]) {
            if (!strcmp(tmpname,names[i])) {
              mapping[i].eIdx = n;
              nElSel++; isFound[i]=1;
              break;
            }
          }
        }
        free(tmpname);
        if (nElSel >= nSel) break;
      }


      if (nElSel < nSel) {
        SMILE_IERR(1,"%i elements which were requested in element selection file were not found in current input data! please check openSMILE config! These elements are:",nSel-nElSel);
        for (i=0; i<nSel; i++) {
          if (isFound[i]==0) {
            SMILE_IERR(1,"  element '%s' ",names[i]);
          }
        }
        const FrameMetaInfo * fmeta = reader_->getFrameMetaInfo();
        SMILE_IMSG(1, "Available field names are:");
        fmeta->printFieldNames();
        COMP_ERR("Your system will not run stable, due to data incompatibilities. Thus, we are aborting here!");
      } else {
        for (i=0; i<nSel; i++) {
          if (selFile == NULL) {
            const char *newname = getStr_f(myvprint("newNames[%i]",i));
            if (newname != NULL) {
              writer_->addField(newname);
            } else {
              if (nameAppend_ != NULL) {
                char * tmp = myvprint("%s_%s", names[i], nameAppend_);
                writer_->addField(tmp); free(tmp);
              } else {
                writer_->addField(names[i]);  // if no newName is given, add old name
              }
            }
          } else {
            if (nameAppend_ != NULL) {
              char * tmp = myvprint("%s_%s",names[i],nameAppend_);
              writer_->addField(tmp); free(tmp);
            } else {
              writer_->addField(names[i]);  // if no newName is given, add old name
            }
          }
        }
        SMILE_IDBG(3,"mapping computed successfully");
      }
      free(isFound);
    }
    /*
    for (j=0; j<nSel; j++) {
    int found = 0;
    for (i=0; i<_N; i++) {
    char * tmp = reader->getElementName(i);
    if (!strcmp(tmp,names[j])) {
    // we found a match...
    found = 1;
    mapping[nElSel++].eIdx = i;
    if (selFile == NULL) {
    const char *newname = getStr_f(myvprint("newNames[%i]",j));
    if (newname != NULL) {
    writer->addField(newname);
    } else {
    writer->addField(tmp);  // if no newName is given, add old name
    }
    } else {
    writer->addField(tmp);
    }
    }
    free(tmp);
    }
    if (found==0) {
    SMILE_IERR(1,"element '%s' requested in selection, but not found in input elements! please check input config!",names[j]);
    }
    }
    SMILE_IDBG(2,"selected %i elements of %i requested elements",nElSel,nSel);
    */
  } else {

    // in non-element mode, create a list of expanded element names, then use the same code as in element mode
    int N = reader_->getLevelNf();  
    nElSel = 0; nFSel = 0;
    mapping = (sDataSelectorSelData *)calloc(1,sizeof(sDataSelectorSelData) * nSel);
    /// !!!!!!!!!!!!!!!!!!! TODO !!!!!!!!!!!!!!!!!!!!!!!
    //for (i=0; i<_N; i++) {
    long dimensionSingleField = 0;
    for (j=0; j<nSel; j++) {

      // analyse type of field specified in selection, and extract fieldname (if array)
      const char *fieldname=NULL;
      int startIdx = 0, endIdx = 0, rangeGiven = 0;

      char *tmp2 = NULL;
      if (names[j] != NULL) {

        tmp2 = strdup(names[j]);
        char *ar = strchr(tmp2,'[');
        if (ar == NULL) { // is non-array field (no [] in name) or full field
          fieldname=names[j];
        } else {    // is array field ( with [] in name )
          *ar = 0;
          fieldname=tmp2;
          ar++;
          int len = (int)strlen(ar);

          if (len > 0) {
            ar[len-1] = 0;  // remove closing ]

            // look for element [x] OR range [x-y]
            char *rng = strchr(ar,'-');
            if (rng != NULL) { // is range
              *rng = 0; rng++;
              char *end=NULL;
              startIdx = strtol(ar, &end, 10); // convert to int
              if ((end != NULL)&&(end[0] != 0)) { // error, invalid chars
                SMILE_IERR(1,"invalid character in array start index in 'selected' field : '%s'\n",names[j]);
                COMP_ERR("stopping");
              }
              end=NULL;
              endIdx = strtol(rng, &end, 10); // convert to int
              if ((end != NULL)&&(end[0] != 0)) { // error, invalid chars
                SMILE_IERR(1,"invalid character in array end index in 'selected' field : '%s'\n",names[j]);
                COMP_ERR("stopping");
              }
              rangeGiven=1;
            } else { // is element in array field
              char *end=0;
              startIdx = strtol(ar, &end, 10); // convert to int
              if ((end != NULL)&&(end[0] != 0)) { // error, invalid chars
                SMILE_IERR(1,"invalid character in array index in 'selected' field : '%s'\n",names[j]);
                COMP_ERR("stopping");
              }
              endIdx = startIdx; rangeGiven=1;
            }

          }

        }
      }

      // check if field exists
      int __N=0;
      int arrNameOffset=0;
      int found = 0;
      int elIdx = 0;
      for (i=0; i<N; i++) { // for each field
        const char *tmp = reader_->getFieldName(i,&__N,&arrNameOffset);
        if (!strcmp(fieldname,tmp)) {
          // match found, ok
          found = 1;
          if (!rangeGiven)  { startIdx = 0; endIdx = __N-1; }
          else {
            startIdx -= arrNameOffset;
            endIdx -= arrNameOffset;
          }
          mapping[nFSel].fIdx = i;
          mapping[nFSel].eIdx = elIdx;
          break;
        }
        elIdx += __N;
      }



      // add to writer, if found
      if (!found) {
        // warn
        SMILE_IWRN(1,"field '%s' requested for selection, but not found in input!",names[j]);
      } else {
        // check range of start/end array index
        if (startIdx < 0) {
          SMILE_IWRN(1,"start index (%i) for selected field '%s' is out of range (must be > 0)! Setting start index to 0.",startIdx,names[j]);
          startIdx = 0;
        }
        else if (startIdx >= __N) {
          SMILE_IWRN(1,"start index (%i) for selected field '%s' is out of range (must be < %i)! Setting start index to %i.",startIdx,names[j],__N,__N-1);
          startIdx = __N-1;
        }

        if (endIdx < 0) {
          SMILE_IWRN(1,"end index (%i) for selected field '%s' is out of range (must be > 0)! Setting end index to 0.",endIdx,names[j]);
          endIdx = 0;
        }
        else if (endIdx >= __N) {
          SMILE_IWRN(1,"end index (%i) for selected field '%s' is out of range (must be < %i)! Setting end index to %i.",endIdx,names[j],__N,__N-1);
          endIdx = __N-1;
        }
        //add
        if (outputSingleField == NULL) {
          writer_->addField(fieldname, endIdx - startIdx + 1, arrNameOffset + startIdx);
        } else {
          dimensionSingleField += endIdx - startIdx + 1;
        }
        // set names in mapping, and set nElSel correct
        // TODO:::.
        //mapping[nElSel++].fIdx = i; // note: i is still valid from the last for() loop
        mapping[nFSel].aIdx = startIdx+mapping[nFSel].eIdx; // start index as element index
        mapping[nFSel++].N = endIdx-startIdx+1; // start index as element index
        nElSel += endIdx-startIdx+1;
      }

      if (tmp2 != NULL) free(tmp2);
    }
    if (outputSingleField != NULL) {
      writer_->addField(outputSingleField, dimensionSingleField);
    }
  }
  namesAreSet_ = 1;
  return nElSel;
}

/*
int cDataSelector::myFinaliseInstance()
{
return cDataProcessor::myFinaliseInstance();
}
*/

eTickResult cDataSelector::myTick(long long t)
{
  if (dummyMode) return TICK_INACTIVE;

  SMILE_IDBG(4,"tick # %i, processing value vector",t);
  
  int framesWritten;

  // we do not process in blocks using getNextMatrix because
  // tmeta information would be only associated with the first vector in
  // each matrix and all other frames would lack tmeta information
  for (framesWritten = 0; framesWritten < MIN(blocksizeR_, blocksizeW_); framesWritten++) {
    if (!writer_->checkWrite(1))
      return framesWritten > 0 ? TICK_SUCCESS : TICK_DEST_NO_SPACE;

    // get next frame from dataMemory
    cVector *vec = reader_->getNextFrame();
    if (vec == NULL)
      return framesWritten > 0 ? TICK_SUCCESS : TICK_SOURCE_NOT_AVAIL;

    if (vecO == NULL) vecO = new cVector(nElSel);
    long i;

    if (elementMode) {
      if (selectionIsRange) {
        long j=0;
        for (i=0; i<vec->N; i++) {
          if (idxSelected[i]) { vecO->data[j++] = vec->data[i]; }
        }
      } else {
        for (i=0; i<nElSel; i++) {
          vecO->data[i] = vec->data[mapping[i].eIdx];
        }
      }
    } else {
      int j; long n=0;
      for (i=0; i<nFSel; i++) {
        for (j=0; j<mapping[i].N; j++) {
          vecO->data[n++] = vec->data[mapping[i].aIdx+j];
        }
      }
    }

    vecO->setTimeMeta(vec->tmeta);

    // save to dataMemory
    writer_->setNextFrame(vecO);
  }
  
  return framesWritten > 0 ? TICK_SUCCESS : TICK_INACTIVE;
}


cDataSelector::~cDataSelector()
{
  int i;
  if (vecO != NULL) delete(vecO);
  if (mapping!=NULL) free(mapping);
  if (idxSelected!=NULL) free(idxSelected);
  if (names!=NULL) {
    if (selFile != NULL) {
      for (i=0; i<nSel; i++) {
        if (names[i] != NULL) free(names[i]);  // memory allocated via strdup()
      }
    } // else we don't free the names, since they are allocated in the config manager!
    free(names); 
  }
}

