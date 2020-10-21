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


#include <other/bowProducer.hpp>
#define MODULE "cBowProducer"

SMILECOMPONENT_STATICS(cBowProducer)

SMILECOMPONENT_REGCOMP(cBowProducer)
{
  SMILECOMPONENT_REGCOMP_INIT
  scname = COMPONENT_NAME_CBOWPRODUCER;
  sdescription = COMPONENT_DESCRIPTION_CBOWPRODUCER;

  // we inherit cDataSource configType and extend it:
  SMILECOMPONENT_INHERIT_CONFIGTYPE("cDataSource")
  
  SMILECOMPONENT_IFNOTREGAGAIN(
    ct->setField("kwList","text file with list of keywords (one word per line) to use for BoW",(const char*)NULL);
    ct->setField("kwListPrefixFilter","keywords in kwList file are expected to have a prefix 'prefix'. Only those keywords will be loaded from the list, everything else will be ignored (i.e. line not beginning with 'prefix').",0);
    ct->setField("prefix","prefix to append to keywords to create feature names","BOW_");
    ct->setField("count","1=count frequency of keyword in input / 0=binary output only (keyword present/not present)",0);
    ct->setField("textfile","A file that contains sentences (words separated by spaces), which will be converted to BOW vectors. You can use this feature to use this component in offline mode (in online mode the text strings will be received as smile messages).",(const char *)NULL);
    ct->setField("singleSentence","A single sentence to be converted to a BoW vector. Words must be separated by spaces.",(const char *)NULL);
    ct->setField("syncWithAudio","If set to 1, wait for a 'turnFrameTime' message before writing the BoW vector to the output level. This applies only in message-based mode, i.e. this option has no effect if either 'textfile' or 'singleSentence' are used.",1);
    //ct->setField("useConf","1=use confidence instead of count or binary word presence",0);
  )

  SMILECOMPONENT_MAKEINFO(cBowProducer);
}

SMILECOMPONENT_CREATE(cBowProducer)

//-----

cBowProducer::cBowProducer(const char *_name) :
  cDataSource(_name),kwList(NULL),count(0),numKw(0),
  keywords(NULL), prefix(NULL), dataFlag(0), filehandle(NULL), 
  lineNr(0), line(NULL), line_n(0), txtEof(0)
{
}

void cBowProducer::myFetchConfig()
{
  cDataSource::myFetchConfig();
  
  kwList = getStr("kwList");
  SMILE_IDBG(2,"kwList = '%s'",kwList);
  kwListPrefixFilter = getInt("kwListPrefixFilter");
  SMILE_IDBG(2,"kwListPrefixFilter = '%i'",kwListPrefixFilter);

  count = getInt("count");
  SMILE_IDBG(2,"count = %i",count);
  prefix = getStr("prefix");
  SMILE_IDBG(2,"prefix = %s",prefix);
  textfile = getStr("textfile");
  SMILE_IDBG(2,"textfile = %s",textfile);
  singleSentence = getStr("singleSentence");
  SMILE_IDBG(2,"singleSentence = %s",singleSentence);

  syncWithAudio = getInt("syncWithAudio");
}

/*
int cBowProducer::myConfigureInstance()
{
  int ret = cDataSource::myConfigureInstance();
  // ...

  return ret;
}
*/

int cBowProducer::configureWriter(sDmLevelConfig &c)
{
  // configure your writer by setting values in &c
  c.T = 0; /* a-periodic level */
  return 1;
}

/*
int isEmptyLine(const char *line, int n)
{
  int i;
  int empty=1;
  if ((n==0)||(n>strlen(line))) n=strlen(line);
  for (i=0; i<n; i++) {
    if ( (line[i] != ' ')&&(line[i] != '\t')&&(line[i] != '\n') ) {
      empty = 0; break;
    }
  }
  return empty;
}
*/

int cBowProducer::loadKwList()
{
  if (kwList != NULL) {
    FILE * f = fopen(kwList,"r");
    if (f==NULL) {
      SMILE_IERR(1,"cannot open keyword list file '%s' (fopen failed)!",kwList);
      return 0;
    }
    int lineNr=0;
    int ret=1;
    char *line = NULL;
    size_t n,read; int len;

    do {
      read = smile_getline(&line, &n, f);
      if ((read != (size_t)-1)&&(line!=NULL)) {
        lineNr++;
        len=(int)strlen(line);
        if (len>0) { if (line[len-1] == '\n') { line[len-1] = 0; len--; } }
        if (len>0) { if (line[len-1] == '\r') { line[len-1] = 0; len--; } }
        while (((line[0] == ' ')||(line[0] == '\t'))&&(len>=0)) { line[0] = 0; line++; len--; }
        if (len > 0) {
          if ((!kwListPrefixFilter)||(prefix == NULL)||(!strncmp(line,prefix,MIN(strlen(prefix),strlen(line))))) {
            numKw++;
            keywords = (char**)realloc(keywords,numKw*sizeof(char*));
            keywords[numKw-1] = strdup(line);
            SMILE_IDBG(4,"read keyword '%s' from keyword list",keywords[numKw-1]);
          }
        }
      } else { break; }
    } while (read != (size_t)-1);
    if (line != NULL) free(line);

    SMILE_IDBG(4,"read %i keywords from keyword list '%s'",numKw,kwList);
    fclose(f);
    return 1;
  }
  return 0;
}

// NOTE: nEl is always 0 for dataSources....
int cBowProducer::setupNewNames(long nEl)
{
  /* load names from keyword list file */
  if (!loadKwList()) {
    SMILE_IERR(1,"failed loading keyword list '%s'",kwList);
    COMP_ERR("aborting!");
  }

  // if used in offline mode, check if textfile (input sentences) exists
  if (textfile != NULL) {
    filehandle = fopen(textfile,"r");
    if (filehandle == NULL) {
      SMILE_IERR(1,"could not open input textfile '%s' for reading!",textfile);
      COMP_ERR("aborting!");
    } 
  }

  // configure dateMemory level, names, etc.
  int i;
  for (i=0; i<numKw; i++) {
    char *tmp;
    if ((kwListPrefixFilter)&&(prefix != NULL)) {
      writer_->addField(keywords[i],1);
    } else {
      if (prefix != NULL) {
        tmp = myvprint("%s%s",prefix,keywords[i]);
      } else {
        tmp = myvprint("BOW_%s",keywords[i]);
      }
      writer_->addField(tmp,1);
      free(tmp);
    }
  }

  namesAreSet_=1;
  allocVec(numKw); //??
  return numKw;
}

/*
int cBowProducer::myFinaliseInstance()
{
  return cDataSource::myFinaliseInstance();
}
*/
#include <classifiers/julius/kwsjKresult.h>




int cBowProducer::buildBoW( cComponentMessage *_msg )
{
  // build bag of words vector in 'vec' based on keywords in message
  //Kresult *k = (Kresult *)(_msg->custData);
  juliusResult *k = (juliusResult *)(_msg->custData);
  int i,j;
    // find keyword in list
    // k->keyword[i]
    // set index in vector
  for (j=0; j<numKw; j++) {
    int found = 0;
    for (i=0; i<k->numW; i++) {
      // NOTE: keyword comparison is case INsensitive ! is that ok?
      if (!strcasecmp( k->word[i], keywords[j] )) {
        vec_->dataF[j] = 1.0; found=1;
        break;
      }
    }
    if (found==0) vec_->dataF[j] = 0.0;
  }
  return 1; // was: 0,  does 1 break anything??
}

int cBowProducer::buildBoW( const char * str )
{
  int i,j;

  if (str == NULL) return 0;

  // split string into words at whitespaces
  // and build bow vector...
  
  char * tmp = strdup(str);
  int len = smileUtil_stripline(&tmp);
  char * curWord = tmp;


  // initialise BoW vector to zero:
  
  for (j=0; j<numKw; j++) {
    vec_->dataF[j] = 0.0;
  }

  for (i=0; i<=len; i++) {

    if (tmp[i] == ' ' || tmp[i] == 0) {
      tmp[i] = 0;

      // look for current word in keyword list...
      //int found = 0;
      for (j=0; j<numKw; j++) {
        // NOTE: keyword comparison is case INsensitive ! is that ok?
        if (!strcasecmp( curWord, keywords[j] )) {
          vec_->dataF[j] = 1.0; //found=1;
          break; // this assumes each word in kwlist is unique...? TODO: warn if words are not unique!
        }
      }
  

      i++;
      while(tmp[i] == ' ' && i<len) { i++; }
      curWord = tmp+i;
    }

  }

  free(tmp);
  return 1;
}

int cBowProducer::processComponentMessage( cComponentMessage *_msg )
{
  if (isMessageType(_msg,"turnFrameTime")) {
    // write current keyword status
    // the output is only written when a turn-frame time message is received (this ensures sync)
    SMILE_IDBG(3,"received 'turnFrameTime' message");
    if (syncWithAudio) {
      if (!writer_->setNextFrame(vec_)) {
        SMILE_IERR(2,"Could not write output to destination level, increase its buffer size.");
      }
      dataFlag = 1;
    }
    //return queNextFrameData(_msg->floatData[0], _msg->floatData[1], _msg->intData[0]);
  } else if (isMessageType(_msg,"asrKeywordOutput")) {  
    SMILE_IDBG(3,"received 'asrKeywordOutput' message");
    buildBoW(_msg);
    if (!syncWithAudio) {
      if (!writer_->setNextFrame(vec_)) {
        SMILE_IERR(2,"Could not write output to destination level, increase its buffer size.");
      }
      dataFlag = 1;
    }
    return 1;  // message was processed
  } else if (isMessageType(_msg,"textString")) {  
    SMILE_IDBG(3,"received 'textString' message");
    buildBoW((const char *)(_msg->custData));
    if (!syncWithAudio) {
      if (!writer_->setNextFrame(vec_)) {
        SMILE_IERR(2,"Could not write output to destination level, increase its buffer size.");
      }
      dataFlag = 1;
    }
    return 1;  // message was processed
  }

  return 0;
}


eTickResult cBowProducer::myTick(long long t)
{
  if (textfile != NULL) {
     
  /* TODO: offline bow mode:
     accept an array of sentences through the config and run buildBow during the first ticks for each sentence
     then write the resulting bow vectors to the output until all have been processed and saved
  */
    if (txtEof) return TICK_INACTIVE;

    if (filehandle == NULL) { 
      // open the input text file, if not yet opened.. (this should not happen actually as we open it during the init phase..)
      filehandle = fopen(textfile,"r");
      SMILE_IWRN(1,"re-opening input text file, will read input from 1st line");
      lineNr=0;
      if (filehandle == NULL) {
        SMILE_IERR(1,"error opening input text file for reading ('%s').\n",textfile);
      }
    }

    if (!writer_->checkWrite(1))
      return TICK_DEST_NO_SPACE;

    // read next line
    long read;
    read = (long)smile_getline(&line, &line_n, filehandle);
    if ((read != -1)&&(line!=NULL)) {
      lineNr++;
      // call buildBow
      if (buildBoW(line)) {
        writer_->setNextFrame(vec_);
        return TICK_SUCCESS;
      }

    } else {
      // if EOF, close file and filehandle=NULL;
      fclose(filehandle); filehandle=NULL;
      if (line != NULL) {
        free(line);
        line = NULL;
      }
      txtEof=1;
      if (lineNr <= 0) {
        SMILE_IWRN(1,"<= 0 lines read from text input file '%s'! The file seems to be empty..?",textfile);
      } else {
        SMILE_IMSG(3,"read %i sentences (lines) from text input file '%s'. EOF reached, no more BoW output will be generated now.",lineNr,textfile);
      }
    }

    
  } else if (singleSentence != NULL) { // single sentence from commandline
    
    if (txtEof) return TICK_INACTIVE;

    if (!writer_->checkWrite(1))
      return TICK_DEST_NO_SPACE;

    if (buildBoW(singleSentence)) {
      txtEof = 1;
      writer_->setNextFrame(vec_);
      return TICK_SUCCESS;
    } 

  } else { // on-line, message-based mode:

    if (dataFlag) {
      dataFlag = 0;
      return TICK_SUCCESS;
    } 

  }
  return TICK_INACTIVE;
}


cBowProducer::~cBowProducer()
{
  int i;
  if (keywords != NULL) {
    for (i=0; i<numKw; i++) {
      free(keywords[i]);
    } 
    free(keywords);
  }

  if (filehandle != NULL) fclose(filehandle);
  if (line != NULL) free(line);
}
