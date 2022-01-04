/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


/*  openSMILE component:

linear svm classifier

*/


#include <classifiers/svmSink.hpp>

#ifdef BUILD_COMPONENT_SvmSink
#ifdef BUILD_MODELCRYPT
#include <private/modelEncryption.hpp>
#endif 

#define MODULE "cSvmSink"

SMILECOMPONENT_STATICS(cSvmSink)

SMILECOMPONENT_REGCOMP(cSvmSink)
{
  SMILECOMPONENT_REGCOMP_INIT

  scname = COMPONENT_NAME_CSVMSINK;
  sdescription = COMPONENT_DESCRIPTION_CSVMSINK;

  // we inherit cDataSink configType and extend it:
  SMILECOMPONENT_INHERIT_CONFIGTYPE("cDataSink")
  
  SMILECOMPONENT_IFNOTREGAGAIN(
    ct->setField("model","The name of the model file.",(const char *)NULL);
    ct->setField("printParseableResult","1 = print easily parseable classification/regression result to stdout", 0);
    ct->setField("printResult","1 = print classification/regression result to log", 0);
    ct->setField("saveResult","filename of text file the result(s) will be saved to", (const char*)NULL);
        ct->setField("instanceName", "If set, print instance name field to CSV (first column) when saveResult=1, with the given value.", (const char*)NULL);
    ct->setField("append", "1 = append to CSV file, in case of saveResult=1, instead of overwriting the file (default).", 0);
    ct->setField("resultRecp","List of component(s) to send 'classificationResult' messages to (use , to separate multiple recepients), leave blank (NULL) to not send any messages",(const char *) NULL);
    ct->setField("resultMessageName","Freely defineable name that is sent with 'classificationResult' message","svm_result");
    ct->setField("showStatsDebug","1 = show internal values for debugging and sanity checks.",0);
    ct->setField("ignoreLogitModel", "1 = don't use a logistic model for probability estimates, if one is contained in the model.", 0);
	ct->setField("winningClassMethodName", "prob = use probabilties instead of votes to determine the winning class, if a logistic model is used. vote = use the standard majority voting based on the distance","vote");

  )

  SMILECOMPONENT_MAKEINFO(cSvmSink);
}

SMILECOMPONENT_CREATE(cSvmSink)

//-----

cSvmSink::cSvmSink(const char *_name) :
  cDataSink(_name), modelObj(NULL),
  saveResult(0), sendResult(0), printResult(0), printParseableResult(0),
  ignoreLogitModel(0), winningClassMethodName("vote")
{

}

void cSvmSink::myFetchConfig()
{
  cDataSink::myFetchConfig();
  
  model = getStr("model");
  resultRecp = getStr("resultRecp");
  resultMessageName = getStr("resultMessageName");
  resultFile = getStr("saveResult");
  if (resultFile != NULL) { 
	  saveResult = 1; 
  }
  instName_ = getStr("instanceName");
  appendSaveResult_ = (getInt("append") != 0);
  if (resultRecp != NULL) { 
	  sendResult = 1; 
  }
  printResult = getInt("printResult");
  printParseableResult = getInt("printParseableResult");
  ignoreLogitModel = getInt("ignoreLogitModel");
  winningClassMethodName = getStr("winningClassMethodName");
  if (!strncmp(winningClassMethodName, "vote", 2)) { 
	  winningClassMethod = SVMSINK_WINNING_CLASS_VOTE;
	  usePairwiseCoupling = 0;
	  //
  } else  if (!strncmp(winningClassMethodName, "prob", 2)) {
	  winningClassMethod = SVMSINK_WINNING_CLASS_PROB;
	  usePairwiseCoupling = 1;
  } else {
    SMILE_IERR(1, "unknown value for winningClassMethod: '%s'. Using default 'vote'.", winningClassMethodName);
    winningClassMethod = SVMSINK_WINNING_CLASS_VOTE;
 }
}


int smileSvmModel::prepareModelStruct()
{
  int l = 1;

  if (nPairs > 0) {
    SMILE_IMSG(2,"loading %i binary SVM model(s).",nPairs);
    binSvm = (struct smileBinarySvmModel *)calloc(1,sizeof(struct smileBinarySvmModel)*nPairs);
  } else {
    SMILE_IERR(1,"Number of binary SVM models < 1 (%i). At least one model must be present. Your model file '%s' seems to be corrupt.",nPairs,model);
    l=0;
  }

  if (vectorDim < 1) {
    SMILE_IERR(1,"Number of attributes (elements of feature vectors) in model is < 1 (%i). Your model file '%s' seems to be corrupt.",vectorDim,model);
    l=0;
  } else {
    attributeNames = (char **)calloc(1,sizeof(char *)*vectorDim);
  }

  return 1;
}


int smileSvmModel::parseKernelType(char *line)
{
  if (!strncmp(line,"Linear Kernel",13)) {
    return SMILESVM_KERNEL_LINEAR;
  } 
  if (!strncmp(line,"RBF Kernel",10)) {
    return SMILESVM_KERNEL_RBF;
  }
  if (!strncmp(line,"Polynomial Kernel",17)) {
    return SMILESVM_KERNEL_POLYNOMIAL;
  }
  
  return SMILESVM_KERNEL_UNKNOWN;
}




int smileSvmModel::parseNClasses(char * line)
{
  char * c = strchr(line, ',');
  if (c!=NULL) *c = 0;
  char * eptr = NULL;
  long l = strtol(line,&eptr,10);
  if (c!=NULL) *c = ',';
  return l;
}

char ** smileSvmModel::parseClassnames(char * line, int nClasses)
{
  char ** cln = NULL;
  char * c = strchr(line, ',');
  char * ln;
  int i;

  if (c!=NULL) {
    smileUtil_stripline(&c);
    ln = strdup(c+1);
  }
  
  if (nClasses > 0) {
    cln = (char **)calloc(1,sizeof(char *) * nClasses);
    cln[0] = ln;
    for (i=1; i<nClasses; i++) {
      char * x = strchr(ln,',');
      if (x!=NULL) {
        *x = 0;
        smileUtil_stripline(&ln); // strip end of previous token
        x++;
        smileUtil_stripline(&x); // strip beginning of current token
        cln[i] = x;
        ln = x;
      } else {
        SMILE_IERR(1,"expected more class names (%i) than actually found! (Class names must be separated by ',')",nClasses);
      }
    }
    /*for (i=0; i<nClasses; i++) {
      cln[i] = strdup(cln[i]);
    }*/
  }
  return cln;
}

int smileSvmModel::parseInt(char * line)
{
  char * eptr = NULL;
  long l = strtol(line,&eptr,10);
  if (eptr != NULL && *eptr != 0) {
    SMILE_IERR(1,"excess characters ('%s') on a line where only a single integer number is expected (line nr. %i : '%s').",eptr,lineNr,line);
    return 0;
  }
  return l;
}

int smileSvmModel::getClassIndex(const char * c) 
{
  int i;
  if (c==NULL) { SMILE_IERR(1,"Class name cannot be NULL in getClassIndex()."); return 0; }
  for (i=0; i<nClasses; i++) {
    if (!strcmp(c,classnames[i])) {
      return i;
    }   
  }
  SMILE_IERR(1,"Class index for class '%s' cannot be determined (nClasses=%i). This seems to be a bug!",c,nClasses);
  return 0;
}

int smileSvmModel::parseClassPair(struct smileBinarySvmModel *_s, char *inp)
{
  char * x = strchr(inp,',');
  if (x != NULL) {
    *x = 0;
    _s->c1 = strdup(inp);
    _s->c2 = strdup(x+1);
    _s->ic1 = getClassIndex(_s->c1);
    _s->ic2 = getClassIndex(_s->c2);
  } else {
    SMILE_IERR(1,"Error parsing binary SVM class pair. There should be two names separated by a comma. Offending line: '%s'",inp);
    return 0;
  }
  return 1;
}

int smileSvmModel::parseLogisticModel(struct smileBinarySvmModel *_s, char *inp)
{
  char * x = strchr(inp,';');
  if (x != NULL) {
    char *eptr = NULL;
    *x = 0;
    double a = strtod(x+1,&eptr);
    //if (eptr != NULL && eptr != x+1) { SMILE_IERR(1,"Parse error while parsing logistic model of binary svm model (coeff1), token '%s'",x+1); }
    _s->logit_coeff1 = a;
    eptr = NULL;
    a = strtod(inp,&eptr);
    //if (eptr != NULL && eptr != x+1) { SMILE_IERR(1,"Parse error while parsing logistic model of binary svm model (coeff0), token '%s'",inp); }
    _s->logit_intercept = a;
    _s->has_logit = 1;
  } else {
    SMILE_IERR(1,"Error parsing binary SVM logistic model. There should be two coefficients separated by ';'. Offending line: '%s'",inp);
    return 0;
  }
  return 1;
}

int smileSvmModel::parseSVs(FLOAT_DMEM **_SV, int *_nSV, char * line, int idx)
{
  if (idx == 0 && *_SV == NULL) {
    // read nSV
    *_nSV = parseInt(line);
    *_SV = (FLOAT_DMEM*)calloc(1,sizeof(FLOAT_DMEM)* (*_nSV) * vectorDim);
  } else {
    int i=0;
    char * x = strchr(line,',');
    while(x != NULL) {
      *x = 0;
      (*_SV)[i*vectorDim+idx] = parseDouble(line);
      line = x;
      x = strchr(line,',');
      i++;
      if (i >= *_nSV-1) { break; }
    }
    (*_SV)[i*vectorDim+idx] = parseDouble(line);
    idx++;
  }
  return idx;
}

FLOAT_DMEM smileSvmModel::parseDouble(char * line)
{
  char * eptr = NULL;
  double d = strtod(line,&eptr);
  if (eptr != NULL && *eptr != 0) {
    SMILE_IERR(1,"excess characters ('%s') on a line where only a single floating point value is expected (line nr. %i : '').",eptr,lineNr,line);
    return 0;
  }
  return (FLOAT_DMEM)d;
}

FLOAT_DMEM smileSvmModel::parseBias(char * line)
{
  if (!strncmp(line,"BIAS: ",6)) { 
    line += 6;
    smileUtil_stripline(&line);
    return parseDouble(line);
  } else {
    SMILE_IERR(1,"Expected BIAS: on line # %i in '%s' (found: '%s').",lineNr,model,line);
    return 0.0;
  }
}

int smileSvmModel::parseEnd(char * line) 
{
  if (!strncmp(line,"++END++",7)) {
    return 1;
  } else { 
    return 0; 
  }
}


int smileSvmModel::load()
{
  FILE *m = fopen(model,"rb");
  if (m==NULL) {
    SMILE_IERR(1,"failed to open SVM model file '%s' for reading.",model);
    return 0;
  }

  int expectAttrNames = 0;
  int expectHeader = 0;
  int nAttrLoaded = 0;
  int expectEnd = 0;
  int properEnd = 0;
  int encryptedFile = 0;
  #ifdef BUILD_MODELCRYPT
  cSmileModelcrypt crypt;
  #endif
  int magicNumber = fgetc(m);
  if (magicNumber == 1) {
    encryptedFile = 1;
    SMILE_MSG(2, "SVM file format: 2");
    // ignore magic number
  } else {
    SMILE_MSG(2, "SVM file format: 1");
    fseek(m, 0, 0);
    // first byte is part of text ...
  }
  
  long read;
  lineNr = 0;
  
  size_t lineLen=0;
  char * line=NULL;

  char * origline = NULL;
  int l=1;
  int binsvmIdx = -1;
  int svIdx = 0;

  int len=0;
  int i=0, ncnt=0;
  // get next non-empty line
  do {
    // read the next line from the text file
    #ifdef BUILD_MODELCRYPT
    read = crypt.getLine(&origline, &lineLen, m, encryptedFile);
    #else 
    if (encryptedFile) {
      SMILE_ERR(1, "This model file (%s) type is not supported by this release of openSMILE.");
      fclose(m);
      return 0;
    } else {
      read = smile_getline(&origline, &lineLen, m);
    }
    #endif
    //origline = line;
    line = origline;

    if ((read > 0)&&(line!=NULL)) {
      lineNr++;
      len = smileUtil_stripline(&line);
      if (len > 0) {
        if (lineNr < 6) {

          if ((lineNr == 1)&&((len<15)||(strncmp(line,"SMILE SVM MODEL",15)))) { // file magic
            SMILE_IERR(1,"The file '%s' does not seem to be a valid smile SVM model file. This first line of the file does not contain the identifier 'SMILE SVM MODEL'.",model);
            l = 0; 
          }
          if (lineNr == 2) { // kernel type
            kernelType = parseKernelType(line);
          }
          if (lineNr == 3) { // classes
            nClasses = parseNClasses(line);
            classnames = parseClassnames(line,nClasses);
          }
          if (lineNr == 4) { // number of pairwise binary SVMs
            nPairs = parseInt(line);
          }
          if (lineNr == 5) { // number of attributes
            vectorDim = parseInt(line);
            //allocate memory
            l = prepareModelStruct();
            expectAttrNames = 1;
          }

        } else {
          if (expectAttrNames) {
            attributeNames[nAttrLoaded++] = strdup(line);
            if (nAttrLoaded >= vectorDim) {
              expectHeader = 2;
              expectAttrNames = 0;
            }
          } else if (expectEnd) {
            if (parseEnd(line)) {
              properEnd = 1;
              l=0;
            } else if (!strncmp(line,"##++LOGISTIC;",13)) {
              // parse logistic regression model for probability estimates
              parseLogisticModel(&(binSvm[binsvmIdx]),line+13);
            }
            else if ((expectHeader == 2) && strncmp(line,"##",2)) { // ignore all lines beginning with ## to allow for extra model parameters
              SMILE_IERR(1,"Error reading model file '%s' on line %i. The next model was expected on this line, but the identifier '++BINARYSVM++: ' was not found!",model,lineNr);
              l=0;
            }
          } else if (expectHeader) {
            if (!strncmp(line,"++BINARYSVM++: ",15)) {
              // class names for this pair ...
              binsvmIdx++;
              l = parseClassPair(&(binSvm[binsvmIdx]),line+15);
              expectHeader = 0; svIdx = 0;
            } 
            else if (!strncmp(line,"##++LOGISTIC;",13)) {
              // parse logistic regression model for probability estimates
              parseLogisticModel(&(binSvm[binsvmIdx]),line+13);
            }
            else if ((expectHeader == 2) && strncmp(line,"##",2)) { // ignore all lines beginning with ## to allow for extra model parameters
              SMILE_IERR(1,"Error reading model file '%s' on line %i. The next model was expected on this line, but the identifier '++BINARYSVM++: ' was not found!",model,lineNr);
              l=0;
            }
          } else if (svIdx < vectorDim) {
            // read weights/SVs
            svIdx = parseSVs(&(binSvm[binsvmIdx].SV),&(binSvm[binsvmIdx].nSV),line,svIdx);
          } else {
            // read SV weights & bias  // TODO: SV weights!
            binSvm[binsvmIdx].bias = parseBias(line);
            expectHeader = 2;

            if (binsvmIdx == nPairs-1) { expectEnd = 1; }
            else { expectEnd = 0; }
          }
        }
      }
    } else { 
      l=0; // EOF
    }

  } while (l);

  if (!properEnd) {
    SMILE_IERR(1,"Premature End-of-file in model file '%s'. The file end marker line '++END++' was not found in %i lines read.",model,lineNr);
    l=0;
  }

  if (origline != NULL) free(origline);

  fclose(m);

  return properEnd;
}

double smileSvmModel::evalBinSvm(FLOAT_DMEM *v, int index) 
{
  struct smileBinarySvmModel * m = &(binSvm[index]);
  double d = 0.0;
  int i;
  if (kernelType == SMILESVM_KERNEL_LINEAR) {
    if (ftSelMap == NULL) {
      for (i=0; i<vectorDim; i++) {
        d += v[i] * (m->SV[i]);
      }
    } else { // feature selection given in model (should be the default case...)
      for (i=0; i<nFtSel; i++) {
        d += v[ftSelMap[i]] * (m->SV[i]);
      }
    }
    d += m->bias;
  } else {
    // TODO:

  }
  return d;
}

// compute logit prob for class 0
// prob. for class 1 is (1-p(class0))
double smileSvmModel::evalLogit(int index, double d) 
{
  struct smileBinarySvmModel * m = &(binSvm[index]);
  // sigmoid probability estimate:
  if (m->has_logit) {
    double z = m->logit_intercept + m->logit_coeff1 * d;
    return 1.0 / (1.0 + exp(-z));
  } else { 
    return -1.0;
  }
}

// ps is array of probabilities for each pair
FLOAT_DMEM * smileSvmModel::getPairwiseProbabilities(double **ps)
{
  FLOAT_DMEM * probs;
  probs = (FLOAT_DMEM*)calloc(1,sizeof(FLOAT_DMEM)*nClasses);

  int i,j;
  //// compute sum of weights
  double **wsum = new double *[nClasses];
  for (i=0; i<nClasses; i++) {
    wsum[i] = new double[nClasses];
    for (j=0; j<nClasses; j++) {
      wsum[i][j] = 0.0;
    }
  }
  for (i=0; i<nPairs; i++) {
    struct smileBinarySvmModel * m = &(binSvm[i]);
    double w_sum = 0.0;
    if (kernelType == SMILESVM_KERNEL_LINEAR) {
      for (j=0; j<vectorDim; j++) {
        w_sum += (double)fabs(m->SV[j]);
      }
    }
	if (m->ic1 > m->ic2) {
		wsum[m->ic2][m->ic1] = w_sum;
	} else {
		wsum[m->ic1][m->ic2] = w_sum;
	}
  }

  //// pairwise coupling algorithm
  
  // init p and u arrays
  double * p = new double[nClasses];
  for (i=0; i<nClasses; i++) {
    p[i] = 1.0 / (double)nClasses;
  }
  double ** u;
  u = new double *[nClasses];
  for (i=0; i<nClasses; i++) {
    u[i] = new double[nClasses];
    for (j=i+1; j<nClasses; j++) {
      u[i][j] = 0.5;
    }
  }
  
  // initial sum:
  double * sum1 = new double[nClasses];
  for (i=0; i<nClasses; i++) { 
    sum1[i] = 0.0; 
  }
  for (i=0; i<nClasses; i++) {
    for (j=i+1; j<nClasses; j++) {  
      sum1[i] += wsum[i][j] * ps[i][j];
      sum1[j] += wsum[i][j] * (1.0 - ps[i][j]);
    }
  }
  
  // iterative optimization
  double * sum2 = new double[nClasses];
  int changed = 0;
  int nIterations = 0;
  do {
    changed = 0;
    for (i=0; i<nClasses; i++) { 
      sum2[i] = 0.0; 
    }
    for (i=0; i<nClasses; i++) {
      for (j=i+1; j<nClasses; j++) {  
        sum2[i] += wsum[i][j] * u[i][j]; 
        sum2[j] += wsum[i][j] * (1.0 - u[i][j]);
      }
    }

    for (i=0; i<nClasses; i++) {
      if ((sum1[i] == 0.0) || (sum2[i] == 0.0)) {
        if (p[i] > 0.0) { 
          changed = 1; 
        }
        p[i] = 0.0;
      } else {
        double f = sum1[i] / sum2[i];
        double pOld = p[i];
        p[i] *= f;
        if (fabs(pOld-p[i]) > 0.001) { 
          changed = 1; 
        }
      }
    }

    // normalise p
    double psum = 0;
    for (i=0; i<nClasses; i++) {
      psum += p[i];
    }
    for (i=0; i<nClasses; i++) {
      p[i] /= psum;
      // we only add this to avoid hangs if probs. converge to 1
      if (p[i] > 0.99999) { changed = 0; }
    }
    // update
    for (i=0; i<nClasses; i++) {
      for (j=i+1; j<nClasses; j++) {  
        u[i][j] = p[i] / (p[i]+p[j]);
      }
    }
    nIterations++;
  } while (changed && nIterations < 500);

  if (nIterations > 450) {
    // This shouldn't be... if it is, we warn the user
    SMILE_IWRN(3, "High number of iterations in pairwise coupling! Check if your model is correct or if the data has problems..?");
  }
  delete[] wsum;

  for (i=0; i<nClasses; i++) {
    delete[] u[i];
    probs[i] = (FLOAT_DMEM)p[i];
  }
  delete[] p;
  delete[] u;
  delete[] sum1;
  delete[] sum2;

  return probs;
}

/*
  probs will be a pointer to newly allocated space, which contains the probabilities for each class
  if the model has a logisitic (sigmoid) model for probability estimation.
  *conf will be set to a confidence value of the winning class (curently: average absolute distance to hyperplane)
*/
int smileSvmModel::evaluate(FLOAT_DMEM *v, long N, const char ** winningClass, FLOAT_DMEM ** probs, FLOAT_DMEM *conf, int winningClassMethod, int usePairwiseCoupling)
{
  int i,j;
  int hasLogit=1;
  double * dsum = new double[nClasses];
  double * probsum = new double[nClasses];
  int * result = new int[nClasses];
  double ** ps;
  ps = new double *[nClasses];
  for (i=0; i<nClasses; i++) {
    ps[i] = new double[nClasses];
    for (j=i+1; j<nClasses; j++) {
      ps[i][j] = 0.5;
    }
    dsum[i] = 0;
    result[i] = 0;
	  probsum[i] = 0;
  }

  for (i=0; i<nPairs; i++) {
    // get distance for each class pair
    double d = evalBinSvm(v, i);
    double p = evalLogit(i,d);
    if (p == -1.0) hasLogit = 0;

    // determine index of winning class
    int winner;

	
	if (d < 0.0) winner = binSvm[i].ic1;
	else winner = binSvm[i].ic2;

	if (binSvm[i].ic1 > binSvm[i].ic2) {
	  ps[binSvm[i].ic2][binSvm[i].ic1] = 1 - p;
	} else {
	  ps[binSvm[i].ic1][binSvm[i].ic2] = p;
	}

	//printf("pair %i: d=%f , winner = %i (%s)\n",i,d,winner,classnames[winner]);
	// add up to majority vote...
	result[winner]++;
	dsum[winner] += fabs(d);

    if (!usePairwiseCoupling) {
		if (p < 0.5) {
			winner = binSvm[i].ic2;
			p = 1.0 - p;
		} else {
			winner = binSvm[i].ic1;
		}
		probsum[winner] += p;
	} 

  }

  // pairwise coupling of probabilites....
  if (probs != NULL) {
	  if (hasLogit && ignoreLogitModel == 0) {
		  if (nPairs > 1 && usePairwiseCoupling) {
			  *probs = getPairwiseProbabilities(ps);
		  } else if (nPairs > 1) {
			  SMILE_IMSG(4,"\n  normalising probsum...");
			  // normalise the probs in probsum to have sum 1
			  FLOAT_DMEM sumOfProbs = 0;
			  for (i=0; i<nClasses; i++) {
				  sumOfProbs += (FLOAT_DMEM)probsum[i];
			  }
			  *probs = (FLOAT_DMEM*)calloc(1,sizeof(FLOAT_DMEM)*nClasses);
			  for (i=0; i<nClasses; i++){
				  (*probs)[i] = (FLOAT_DMEM)probsum[i] / sumOfProbs;
			  }
		  } else {
			  *probs = (FLOAT_DMEM*)calloc(1,sizeof(FLOAT_DMEM)*nClasses);
			  (*probs)[0] = (FLOAT_DMEM)ps[binSvm[0].ic1][binSvm[0].ic2];
			  (*probs)[1] = (FLOAT_DMEM)( 1.0-ps[binSvm[0].ic1][binSvm[0].ic2] );
		  }
	  } else {
		  // use normalised # votes
		  *probs = (FLOAT_DMEM*)calloc(1,sizeof(FLOAT_DMEM)*nClasses); 
		  for (i=0; i<nClasses; i++) {
			  (*probs)[i] = (FLOAT_DMEM)(result[i])/(FLOAT_DMEM)nPairs;
		  }
	  }
  }
  // find max

  int max = 0; int maxC = 0; FLOAT_DMEM max_prob = 0;
  if (winningClassMethod == SVMSINK_WINNING_CLASS_VOTE || probs == NULL || *probs == NULL) {
	  max = result[0];
	  for (i=1; i<nClasses; i++) {
		  
		  if (result[i] > max) { 
			  maxC = i;
			  max = result[i];
		  } 
		  // NOTE: weka does not do this part... WEKA choses only the first max. in case of evenness of votes
		  else if (result[i] == max) {
			  if (dsum[i] > dsum[maxC]) {
				  maxC = i;
				  max = result[i];
			  }
		  }
	  }
  } else if (winningClassMethod == SVMSINK_WINNING_CLASS_PROB) {
	  max_prob = (*probs)[0];
	  for (i=1; i<nClasses; i++) {
		  if ((*probs)[i] > max_prob) {
			  maxC = i;
			  max_prob = (*probs)[i]; 
		  } 
		  SMILE_IMSG(4,"\n  Step %i: max=%f, probs=%f, maxC=%i",i,max_prob,(*probs)[i],maxC);
	  }
  }
  if (showStatsDebug) {
	  // print result per class
	  for (i=0; i<nClasses; i++) {
		  if (probs != NULL && winningClassMethod == SVMSINK_WINNING_CLASS_PROB) { 
			  fprintf(stderr,"SVM result with probs, class %i/%i : probsum=%f , dsum=%f , prob=%f\n",i+1,nClasses,probsum[i],dsum[i],(*probs)[i]);
		  } else if (probs != NULL && winningClassMethod == SVMSINK_WINNING_CLASS_VOTE) {
			  fprintf(stderr,"SVM result, class %i/%i : nVotes=%i , dsum=%f , prob=%f\n",i+1,nClasses,result[i],dsum[i],(*probs)[i]);
		  } else {
			  fprintf(stderr,"SVM result, class %i/%i : nVotes=%i , dsum=%f\n",i+1,nClasses,result[i],dsum[i]);
		  }
	  }
  }

  SMILE_IMSG(4,"\n  Winning class: # %i (%s) (dsum=%f , #votes = %i).",maxC,classnames[maxC],dsum[maxC],max);
  if (conf != NULL) { *conf = (FLOAT_DMEM)fabs(dsum[maxC]); }

  // return result
  delete[] result;
  delete[] dsum;
  delete[] probsum;
  for (i=0; i<nClasses; i++) {
    if (ps[i] != NULL)
      delete[] ps[i];
  }
  delete[] ps;
  // delete pointers in ps!!??

  if (winningClass != NULL) {
    *winningClass = classnames[maxC];
  }
  return maxC;
}

int smileSvmModel::buildFtSelMap(int n, const char * name)
{
  if (name == NULL || n < 0) {
    return -1;
  }
  if (ftSelMap == NULL) {
    ftSelMap = (int *)malloc(sizeof(int) * vectorDim);
    for (int i = 0; i < vectorDim; i++) {
      ftSelMap[i] = -1;
    }
  }
  int i = n;
  int j = n - 1;
  if (j >= vectorDim) {
    j = vectorDim - 1;
  }
  do {
    if (i < vectorDim) {
      if (!strcmp(name, attributeNames[i])) {
        ftSelMap[i] = n;
        return i;
      }
      i++;
    } else {
      if (j < 0) { return -1; }
    }
    if (j >= 0) {
      if (!strcmp(name, attributeNames[j])) {
        ftSelMap[j] = n;
        return j;
      }
      j--;
    } else {
      if (i >= vectorDim) { return -1; }
    }
  } while(1);
  return -1;
}

// Checks the feature selection map for missing features,
// after it has been fully built.
int smileSvmModel::checkFtSelMap()
{
  for (int i = 0; i < vectorDim; i++) {
    if (ftSelMap[i] == -1) {
      SMILE_IERR(1, "The attribute # %i (%s) in the model file (%s) does not exist in the input data level to this component!", i, attributeNames[i], model);
      return 0;
    }
  }
  nFtSel = vectorDim;
  return 1;
}

smileSvmModel::~smileSvmModel()
{
  int i;
  if (classnames != NULL) {
    if (classnames[0] != NULL) free(classnames[0]);
    free(classnames);
  }
  if (binSvm != NULL) {
    for (i=0; i<nPairs; i++) {
      if (binSvm[i].c1 != NULL) free(binSvm[i].c1);
      if (binSvm[i].c2 != NULL) free(binSvm[i].c2);
      if (binSvm[i].SV != NULL) free(binSvm[i].SV);
    }
    free(binSvm);
  }
  if (attributeNames != NULL) {
    for (i=0; i<vectorDim; i++) {
      if (attributeNames[i] != NULL) free(attributeNames[i]);
    }
    free(attributeNames);
  }
  if (ftSelMap != NULL)
    free(ftSelMap);
}



int cSvmSink::myFinaliseInstance()
{
  int ret = cDataSink::myFinaliseInstance();

  if ((ret)&&(model != NULL)) {
    modelObj = new smileSvmModel((cSmileComponent*)this, model);
    if (getInt("showStatsDebug")) { 
      modelObj->setStatsDebug(); 
    }
    if (ignoreLogitModel) {
      modelObj->setIgnoreLogitModel();
    }
    ret = modelObj->load();
    if (!ret) {
      delete modelObj;
      modelObj = NULL;
    } else {
      // build feature mapping table
      SMILE_IMSG(3, "Building feature index mapping table...");
      int N = reader_->getLevelN();
      for (int i = 0; i < N; i++) {
        char * name = reader_->getElementName(i);
        if (name == NULL) {
          SMILE_IERR(2, "Cannot get name from input level for element # %i! Thus, cannot compute feature mapping. Unless the features in the model file are in exact the same order as in the input level (and none are missing in the model), the result of SVMSINK WILL PROBABLY BE WRONG!!", i);
        } else {
          modelObj->buildFtSelMap(i, name);
          free(name);
        }
      }
      if (!modelObj->checkFtSelMap()) {
        delete modelObj;
        modelObj = NULL;
        ret = 0;
        SMILE_IERR(1, "Aborting due to missing attributes in input level!");
      }
      SMILE_IMSG(3, "Done with building feature index mapping table.");
    }
    
    if (saveResult) {
      bool doAppend = false;
      FILE *f = NULL;
      if (appendSaveResult_) {
        f = fopen(resultFile, "r");
        if (f != NULL) {
          doAppend = true;
          fclose(f);
        }
      }
      if (doAppend)
        f = fopen(resultFile, "a");
      else 
        f = fopen(resultFile, "w");
      if (f != NULL) {
        if (!doAppend) {
          if (instName_ != NULL) {
            fprintf(f, "name;");
          }
          fprintf(f, "time;length;classindex;classname;confidence");
          int i;
          for (i=0; i<modelObj->getNclasses(); i++) {
            fprintf(f, ";prob_class[%i=%s]", i, modelObj->className(i));
          }
          fprintf(f, "\n");
        }
        fclose(f);
      } else {
        SMILE_IERR(1,"cannot open result output file '%s' for writing (appending)! Disabling saving of classification result to file. No more errors will be shown.");
        saveResult = 0;
      }
    }
  }
  return ret;
}


eTickResult cSvmSink::myTick(long long t)
{
  SMILE_IDBG(4,"tick # %i, reading value vector:",t);
  
  cVector *vec= reader_->getNextFrame();
  if (vec == NULL) return TICK_SOURCE_NOT_AVAIL;
  
  int cls;
  const char * name;
  FLOAT_DMEM conf;
  FLOAT_DMEM *probs=NULL;
  cls = modelObj->evaluate(vec->data, vec->N, &name, &probs, &conf, winningClassMethod, usePairwiseCoupling);
  // cls contains the index of the winning class, name the name, and conf the confidence measure

  int isFinal = 0; // TODO: read this from meta-data
  int ID = 0;      // TODO: read this from meta-data

  if (printResult) {
    SMILE_IMSG(1, "\n  ~~> SmileSVM classification result: class %i (%s) (conf=%f) <~~",cls,name,conf); /*modelObj->className(cls)*/
    // print probabilites...
    if (probs != NULL) {
      int i;
      for (i = 0; i < modelObj->getNclasses(); i++) {
        SMILE_IMSG(1, "    prob class #%i (%s) = %f", i, modelObj->className(i), probs[i]);
      } 
    }
  }
  if (printParseableResult) { // standard parseable format
    printf("SMILE-RESULT::ORIGIN=svmsmo::TYPE=classification::VIDX=%ld::NAME=%s::CATEGORY_IDX=%i::CATEGORY=%s::CONFIDENCE=%f", vec->tmeta->vIdx, getInstName(), cls, name, conf);
    if (probs != NULL) {
      for (int i = 0; i < modelObj->getNclasses(); i++) {
        printf("::PROB=%i;%s:%f", i, modelObj->className(i), probs[i]);
      }
    }
    printf("\n");
  }

  FILE * fl = NULL;
  if (saveResult) {
    fl = fopen(resultFile,"a");
    if (fl==NULL) {
      SMILE_IERR(1,"cannot open result output file '%s' for writing (appending)! Disabling saving of classification result to file. No more errors will be shown.");
      saveResult = 0;
    } else {
      int i;
      // time;length;classindex/value;classname;confidence;probs...
      if (instName_ != NULL) {
        fprintf(fl, "%s;", instName_);
      }
      fprintf(fl,"%f;%f;%f;%s;%f",(float)vec->tmeta->time,(float)vec->tmeta->lengthSec,(float)cls,name,conf); /*modelObj->className(cls)*/
      if (probs != NULL) {
        for (i=0; i<modelObj->getNclasses(); i++) {
          fprintf(fl,";%f",probs[i]);
	  //use the following instead if you want classnames in the resultsfile
	  //fprintf(fl,";%s;%f",modelObj->className(i),probs[i]);
        } 
      }
      fprintf(fl, "\n");
      fclose(fl);
    }
  }

  if (sendResult) {
    cComponentMessage msg("classificationResult", resultMessageName);
    msg.floatData[0] = (FLOAT_DMEM)cls;
    msg.floatData[1] = conf;
    msg.intData[0]   = modelObj->getNclasses(); // models[modelIdx].nClasses;
    msg.intData[1]   = isFinal;
    msg.intData[2]   = 0; // modelIdx
    msg.intData[3]   = 0; // mmm; ??
    msg.intData[5]   = ID; // f->ID;
    if (name != NULL) {  // class name
      strncpy(msg.msgtext, name, CMSG_textLen);
    }
    msg.custData     = (void *)probs; //f->probEstim;   // TODO: support probabilities
    msg.custDataSize = modelObj->getNclasses() * sizeof(FLOAT_DMEM);
    msg.custDataType = CUSTDATA_FLOAT_DMEM;
    msg.custData2    = (void *) ( /*models[modelIdx].modelResultName*/ NULL ); // dangerous conversion....!
    msg.userTime1    = vec->tmeta->time;
    msg.userTime2    = vec->tmeta->time + vec->tmeta->lengthSec;
    sendComponentMessage( resultRecp, &msg );
    SMILE_IDBG(3,"sending 'classificationResult' message to '%s'",resultRecp);
  }
  if (probs != NULL) free(probs);

  return TICK_SUCCESS;
}


cSvmSink::~cSvmSink()
{
  if (modelObj != NULL) delete modelObj;
}

#endif // BUILD_SMOSVM
