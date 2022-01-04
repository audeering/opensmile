/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


/*  openSMILE component:

example dataSink:
reads data from data memory and outputs it to console/logfile (via smileLogger)
this component is also useful for debugging

*/


#include <rnn/rnnSink.hpp>




#define MODULE "cRnnSink"

#ifdef BUILD_RNN

SMILECOMPONENT_STATICS(cRnnSink)

SMILECOMPONENT_REGCOMP(cRnnSink)
{
  SMILECOMPONENT_REGCOMP_INIT

  scname = COMPONENT_NAME_CRNNSINK;
  sdescription = COMPONENT_DESCRIPTION_CRNNSINK;

  // we inherit cDataSink configType and extend it:
  SMILECOMPONENT_INHERIT_CONFIGTYPE("cDataSink")
  
  SMILECOMPONENT_IFNOTREGAGAIN(
    ct->setField("netfile","The file which contains the trained network created by rnnlib",(const char *)NULL);
    ct->setField("actoutput","A text file to which the raw output activations will be saved. Each frame will be saved on a separate line (this is rnnlib's activation output format transposed).",(const char*)NULL);
    ct->setField("classoutput","A text file to which the winning class label will be saved (task = classification or transcription). The result for each frame will be saved on a separate line.",(const char*)NULL);
    //TODO: label output

    ct->setField("classlabels","Give a string of comma separated (NO spaces allowed!!) class names (e.g. class1,class2,class3) for a classification or transcription task",(const char*)NULL);
    ct->setField("ctcDecode","1/0 = yes/no : Do basic ctc (transcription) decoding, i.e. remove duplicate labels and (TODO: compute label alignments.)",1);
    ct->setField("printConnections","1/0 = yes/no : print human readable information on the network layers on connections",0);
    ct->setField("printInputStats","1/0 = yes/no : print input weight sums (can be used for feature selection...)",0);
    //ct->setField("lag","Output data <lag> frames behind",0,0,0);
  )

  SMILECOMPONENT_MAKEINFO(cRnnSink);
}

SMILECOMPONENT_CREATE(cRnnSink)



//-----

cRnnSink::cRnnSink(const char *_name) :
  cDataSink(_name),
  netfile(NULL), in(NULL), out(NULL), rnn(NULL), classlabelArr(NULL), 
  ctcDecode(0), actoutput(NULL), classlabels(NULL), nClasses(0),
  outfile(NULL), outfileC(NULL)
{
  // ...
}

void cRnnSink::myFetchConfig()
{
  cDataSink::myFetchConfig();
  
  netfile = getStr("netfile");
  SMILE_IDBG(2,"netfile = '%s'",netfile);

  actoutput = getStr("actoutput");
  SMILE_IDBG(2,"actoutput = '%s'",actoutput);

  classoutput = getStr("classoutput");
  SMILE_IDBG(2,"classoutput = '%s'",classoutput);

  const char * _classlabels = getStr("classlabels");
  SMILE_IDBG(2,"classlabels = '%s'",_classlabels);
  if (_classlabels != NULL) {
    classlabels = strdup(_classlabels);
    // count number of classes:
    nClasses = 1;
    char *cl = classlabels;
    while(1) {
      char *x = strchr(cl,',');
      if (x!=NULL) {
        cl=x+1;
        nClasses++;
      } else {
        break;
      }
    }

    // allocate memory for classes array
    classlabelArr = (const char **)calloc(1,sizeof(const char *)*nClasses);

    // parse class names
    long i=0;
    cl = classlabels;
    while(1) {
      char *x = strchr(cl,',');
      if (x!=NULL) {
        *x=0;
        classlabelArr[i] = (const char *)cl;
        if (*(x+1) == 0) { nClasses--; break; }
        cl = x+1;
        i++;
      } else {
        classlabelArr[i] = (const char *)cl;
        i++;
        break;
      }
    }
   
  }

  ctcDecode = getInt("ctcDecode");
  SMILE_IDBG(2,"ctcDecode = %i",ctcDecode);

  printConnections = getInt("printConnections");
  SMILE_IDBG(2,"printConnections = %i",printConnections);
}

#if 0
unsigned long parseLayerNumber(char *s, unsigned long *dir)
{
  char *x = strchr(s,'_');
  if (dir != NULL) {
    *dir = LAYER_DIR_FWD;
  }
  if (x != NULL) { // two numbers separated by _, e.g. 0_1
    if ((*(x+1) == '1')&&(dir != NULL)) {
        *dir = LAYER_DIR_RWD;
    }
    *x = 0;
    return (unsigned long)strtol(s,NULL,10);
  } else { // only one number
    return (unsigned long)strtol(s,NULL,10);
  }
  return -1;
}

cRnnWeightVector *cRnnSink::createWeightVectorFromLine(char *line)
{
  long i=0;
  char *lastl = line;
  char *curl=line;
  char *name = NULL;

  // get name
  while(1) {
    if ((*curl == ' ')||(*curl == 0)) { 
      if (*curl == 0) return NULL;
      *curl = 0; name = lastl; curl++; lastl = curl; break; 
    }
    curl++;
  }
  // check for "_weights"
  char *_to=NULL, *_from=NULL;
  int isPeep = 0;
  char *ww = strstr(name,"_weights");
  if (ww != NULL) {
    // truncate
    ww[0] = 0;
    ww = strstr(name,"_to_");
    if (ww != NULL) {
      ww[0] = 0;
      _to = ww+4;
      ww = strstr(_to,"_delay_1"); // remove the
      if (ww != NULL) ww[0] = 0;   // delay_1 suffix for recurrent connections...
      ww = strstr(_to,"_delay_-1"); // remove the
      if (ww != NULL) ww[0] = 0;   // delay_-1 suffix for recurrent connections...
      _from = name;
    } else { // peepholes?
      ww = strstr(name,"_peepholes");
      if (ww != NULL) { // yes, peephole weights
        ww[0] = 0;
        _from = name; // assign only from !
        isPeep = 1;
      } else { //syntax error
        return NULL;
      }
    }
  } else {
    // if not found (no weight vector!)
    return NULL;
  }

  // get number of weights
  char *numw=NULL;
  while(1) {
    if ((*curl == ' ')||(*curl == 0)) { 
      if (*curl == 0) return NULL;
      *curl = 0; numw = lastl; curl++; lastl = curl; break; 
    }
    curl++;
  }
  // convert numw to int
  long nWeights = strtol(numw,NULL,10);

  if (nWeights > 0) {
    if (isPeep) {
      SMILE_IMSG(3,"%i peephole weights in layer '%s'",nWeights,_from);
    } else {
      SMILE_IMSG(3,"%i weights : from '%s' to '%s'",nWeights,_from,_to);
    }
    // create weights vector
    cRnnWeightVector *l = new cRnnWeightVector(nWeights);
    // copy from and to names..
    if (_to != NULL) { strncpy(l->to,_to,100); }
    if (_from != NULL) { strncpy(l->from,_from,100); }
    // parse from and to names...?
    if (isPeep) { // peephole weights require only the "from" field (they are layer internal)
      l->F = 0; l->T=-1;
      if (!strncmp(l->from,"hidden_",7)) { l->F = LAYER_HIDDEN; }
      l->F |= LAYER_PEEPHOLES;
      // layer number and direction ...
      unsigned long dir=0;
      l->F |= parseLayerNumber(l->from+7,&dir);
      l->F |= dir;
    } else {
      l->F = 0; l->T=-1;
      if (!strncmp(l->from,"bias",4)) { l->F = LAYER_BIAS; }
      else if (!strncmp(l->from,"hidden_",7)) {  // || (!strncmp(l->from,"gather_",7))
        l->F = LAYER_HIDDEN; 
        // layer number and direction ...
        unsigned long dir=0;
        l->F |= parseLayerNumber(l->from+7,&dir);
        l->F |= dir;
      } else if (!strncmp(l->from,"gather_",7)) {  // || (!strncmp(l->from,"gather_",7))
        l->F = LAYER_HIDDEN_GATHER; 
        // layer number and direction ...
        //unsigned long dir=0;
        l->F |= parseLayerNumber(l->from+7,NULL);
        //l->F |= dir;
        printf("layer F: %i\n",l->F);
      }
      else if (!strncmp(l->from,"input",5)) { l->F = LAYER_INPUT; }

      if (!strncmp(l->to,"hidden_",7)) { 
        l->T = LAYER_HIDDEN;
        // layer number and direction ...
        unsigned long dir=0;
        l->T |= parseLayerNumber(l->to+7,&dir);
        l->T |= dir;
      }
      else if (!strncmp(l->to,"output",6)) { l->T = LAYER_OUTPUT; }
    }

    //XX//fprintf(stderr,"strL curl %i\n",strlen(curl));
    // copy weights to vector
    long curW = 0;
    char *w=NULL;
    while(1) {
      if (*curl == ' ') { 
        *curl=0; char *ep=NULL;
        FLOAT_NN wf = strtod(lastl,&ep);
        l->weights[curW++] = wf;
        if (nWeights == curW) {
          SMILE_IERR(1,"too many weights on line, expected only %i weights ('%s')",nWeights,lastl);
          break;
        }
        //XX//if (wf != 0.001) { fprintf(stderr,"curW %i of %i\n", curW,nWeights); }
        curl++; lastl = curl; 
      } else if ((*curl == '\n')||(*curl == '\r')||(*curl == 0)) { 
        *curl=0; char *ep=NULL;
        FLOAT_NN wf = strtod(lastl,&ep);
        l->weights[curW++] = wf;
        
        //curl++; lastl = curl; 
        break;
      } else {
        curl++;
      }
    }

    return l;
  }
  return NULL;
}


int cRnnSink::loadNet(const char *filename)
{
  int bidirectional = 1;
  net.nContext = 0;
  if (filename != NULL) {
    FILE *f = fopen(filename,"r");
    if (f != NULL) {
      char *line=NULL;
      size_t n=0;
      int ret;
      do { // read the net file line by line...
        ret = smile_getline(&line,&n,f);
        if ((ret > 1)&&(line!=NULL)) {
          if (!strncmp(line,"weightContainer_",16)) { // weight vector line...
            cRnnWeightVector *v = createWeightVectorFromLine(line+16);
            if (v != NULL) { // add to a collection..
              net.wv[net.nWeightVectors++] = v;
            }
          } else {
            // net config line
            // we are interested in reading 'hiddenSize', 'hiddenType', 'task' (output layer!), and possibly (?) 'actSmoothing' and 'actDecay'
            if (!strncmp(line,"hiddenSize ",11)) {
              // parse string with sizes of hidden layer, space separated
              long i=0;
              char *n1=line+11, *n2=line+11;
              while(1) {
                if ((*n2 == ',')||(*n2=='\n')||(*n2=='\r')||(*n2==0)) {
                  char myc = *n2;
                  *n2 = 0;
                  long l = strtol(n1,NULL,10);
                  net.hiddenSize[i++] = l;
                  if ((myc=='\n')||(myc=='\r')||(myc==0)) { break; }
                  n2++; n1=n2;
                } else { n2++; }
              }
              net.nHiddenLayers = i;
            } else if (!strncmp(line,"hiddenType ",11)) {
              // parse string with types of hidden layers (space separated?)
              long i=0;
              char *n1=line+11, *n2=line+11;
              while(1) {
                if ((*n2 == ',')||(*n2=='\n')||(*n2=='\r')||(*n2==0)) {
                  char myc = *n2;
                  *n2 = 0;
                  if (!strncmp(n1,"lstm",4)) {
                    net.hiddenType[i] = NNLAYERTYPE_LSTM;
                    net.hiddenActType[i] = NNLSTMACT_TANHTANHLOGI;
                    i++;
                  } else if (!strncmp(n1,"linear_lstm",11)) {
                    net.hiddenType[i] = NNLAYERTYPE_LSTM;
                    net.hiddenActType[i] = NNLSTMACT_TANHIDENLOGI;
                    i++;
                  } else if (!strncmp(n1,"tanh",4)) {
                    net.hiddenType[i] = NNLAYERTYPE_RNN;
                    net.hiddenActType[i] = NNACT_TANH;
                    i++;
                  } else if (!strncmp(n1,"linear",6)) {
                    net.hiddenType[i] = NNLAYERTYPE_RNN;
                    net.hiddenActType[i] = NNACT_IDEN;
                    i++;
                  } else if (!strncmp(n1,"logistic",8)) {
                    net.hiddenType[i] = NNLAYERTYPE_RNN;
                    net.hiddenActType[i] = NNACT_LOGI;
                    i++;
                  } else {
                    SMILE_IERR(1,"unsupported hidden layer type '%s' ('hiddenType' option) while reading '%s'.",n1,netfile);
                  }
                  if ((myc=='\n')||(myc=='\r')||(myc==0)) { break; }
                  n2++; n1=n2;
                } else { n2++; }
              }
              // duplicate hiddenType if # < nHiddenLayers:
             /* if (i<net.nHiddenLayers) {
                for(; i<net.nHiddenLayers; i++) {
                  net.hiddenActType[i] = net.hiddenActType[i-1];
                  net.hiddenType[i] = net.hiddenType[i-1];
                }
              }*/
              if (net.nHiddenLayers > i) { // if less types than layer sizes given, assume the following layers are of the same type as the last layer...?
                int tp = net.hiddenType[i-1];
                int ac = net.hiddenActType[i-1];
                for (; i<net.nHiddenLayers; i++) {
                  net.hiddenType[i] = tp;
                  net.hiddenActType[i] = ac;
                }
              }
            } else if (!strncmp(line,"task ",5)) {
              // net task
              if (!strncmp(line+5,"regression",10)) {
                net.task = NNTASK_REGRESSION;
              } else if (!strncmp(line+5,"classification",14)) {
                net.task = NNTASK_CLASSIFICATION;
              } else if (!strncmp(line+5,"transcription",13)) {
                net.task = NNTASK_TRANSCRIPTION;
                //SMILE_IERR(1,"CTC decoding (task = transcription) not yet supported!");
              }
            } else if (!strncmp(line,"bidirectional ",14)) {
              if (!strncmp(line+14,"false",5)) bidirectional = 0;
              else if (!strncmp(line+14,"true",4)) bidirectional = 1;
            } else if (!strncmp(line,"contextLength ",13)) { // <-- this option is new, it is not an rnnlib option!
              if (!strncmp(line+13,"inf",3)) net.nContext = 0;
              else {
                net.nContext = strtol(line+13,NULL,10);
              }
            }
          }
        } else { if (ret <= 0) break; }
      } while (1);
      fclose(f);
      //// now determine inputSize and outputSize from connection weights
      //// (the rnnlib determiens these sizes from the .nc files, which we don't have...)
      // 1. find bias_to_output to get output size...
      int i;
      for (i=0; i<net.nWeightVectors; i++) {
        if ((net.wv[i]->F == LAYER_BIAS) && (net.wv[i]->T == LAYER_OUTPUT)) { net.outputSize = net.wv[i]->nWeights; break; }
      }
      // 2. find input_to_hidden_0_x
      for (i=0; i<net.nWeightVectors; i++) {
        if ((net.wv[i]->F == LAYER_INPUT) && ( (net.wv[i]->T & LAYER_HIDDEN) == LAYER_HIDDEN)) { 
          long f = 1;
          if (net.hiddenType[0] == NNLAYERTYPE_LSTM) { f = 4; }
          net.inputSize = net.wv[i]->nWeights / (net.hiddenSize[0] * f); 
        }
      }
      net.cellsPerBlock = 1; // FIXME...!
      // test for bidirectional network topology
      net.bidirectional = bidirectional; // FIXME??
      /*for (i=0; i<net.nWeightVectors; i++) {  // do this as extra check ???
        if (net.wv[i]->F & LAYER_DIR_RWD == LAYER_DIR_RWD) net.bidirectional = 1;
      }*/

      // fix gather layers for the unidirectional case ....
/*      if (!bidirectional) {
        for (i=0; i<net.nWeightVectors; i++) {
          if (net.wv[i]->F == LAYER_HIDDEN_GATHER) net.wv[i]->F = LAYER_HIDDEN;
          // Note: the direction is FWD by default (0), so we don't need to set it here
        }
      }*/
      // TODO: gather layers for bidirectional case...??


      net.loaded = 1;
      
      return 1;
    } else {
      SMILE_IERR(1,"failed to open rnn net file '%s'.",filename);
    }
  } else {
    SMILE_IERR(1,"failed to open rnn net file, the filename is a NULL string.");
  }
  return 0;
}

cNnLSTMlayer *cRnnSink::createLstmLayer(int i, int idx, int dir)
{
  // create the forward layer
  cNnLSTMlayer * l = new cNnLSTMlayer(net.hiddenSize[i],idx,dir,net.nContext);
  // create cells with activation functions
  cNnTf * tfI=NULL, * tfO=NULL, *tfG=NULL;
  if (net.hiddenActType[i] == NNLSTMACT_TANHTANHLOGI) {
    tfI = new cNnTfTanh();
    tfO = new cNnTfTanh();
    tfG = new cNnTfLogistic();
  } else if (net.hiddenActType[i] == NNLSTMACT_TANHIDENLOGI) {
    tfI = new cNnTfTanh();
    tfO = new cNnTfIdentity();
    tfG = new cNnTfLogistic();
  } else {
    COMP_ERR("unknown hiddenActType[%i] %i while creating an LSTM layer!",i,net.hiddenActType[i]);
  }
  l->createCells(tfI,tfO,tfG , net.cellsPerBlock);

  return l;
}

int cRnnSink::findPeepWeights(unsigned long id) 
{
  int j;
  for (j=0; j<net.nWeightVectors; j++) {
    if ( net.wv[j]->F == (id|LAYER_PEEPHOLES) ) {
      // return the weightVector index..
      return j;
    }
  }
  return -1;
}

int cRnnSink::findWeights(unsigned long idFrom, unsigned long idTo) 
{
  int j;
  for (j=0; j<net.nWeightVectors; j++) {
    if ((net.wv[j]->F == idFrom)&&(net.wv[j]->T == idTo)) {
      // return the weightVector index..
      return j;
    }
  }
  return -1;
}

// create a network from a successfully loaded net config file (loadNet function)
int cRnnSink::createNet()
{
  int i,j,idx=0;
  if (net.loaded) {
    int nlayers = net.nHiddenLayers+2;
    if (net.bidirectional) nlayers += net.nHiddenLayers;
    rnn = new cNnRnn(nlayers); // +1 input, +1 ouput layer

    // input layer...
    SMILE_IDBG(2,"net input size: %i",net.inputSize);
    cNnNNlayer *nl = new cNnNNlayer(net.inputSize,0,0,net.nContext);
    cNnTf * tf = new cNnTfIdentity(); // identity at the inputs
    nl->createCells(tf);
    nl->setName("input");
    rnn->addLayer(nl, idx++);
 
    
    // hidden layers...
    unsigned long dir = LAYER_DIR_FWD;
    for (i=0; i<net.nHiddenLayers; i++) {
      cNnLayer *l = NULL;
      int recurrent = 0;

      ////////////// forward layers and backward layers (depends on odd/even iteration and net config)

      if (net.hiddenType[i] == NNLAYERTYPE_LSTM) {
        cNnLSTMlayer * _l = createLstmLayer(i,idx,(dir==LAYER_DIR_FWD?0:1));
        char *_tmp = myvprint("hidden_lstm_%i_%c",i,(dir==LAYER_DIR_FWD?'f':'b'));
        _l->setName(_tmp);
        free(_tmp);
        //// peephole weights
        j = findPeepWeights(dir | LAYER_HIDDEN | i);
        if (j>=0) _l->setPeepWeights(net.wv[j]->weights, net.wv[j]->nWeights);
        l=(cNnLayer*)_l;
        recurrent = 1;
      } else if ((net.hiddenType[i] == NNLAYERTYPE_RNN)||(net.hiddenType[i] == NNLAYERTYPE_NN)) {
        l = new cNnNNlayer(net.hiddenSize[i],idx,(dir==LAYER_DIR_FWD?0:1),net.nContext);
        // TODO... weights, transfer functions, createCells, bias...

        char *_tmp;
        if (net.hiddenType[i] == NNLAYERTYPE_RNN) { 
          recurrent = 1; 
          _tmp = myvprint("hidden_rnn_%i_%c",i,(dir==LAYER_DIR_FWD?'f':'b'));
        } else {
          _tmp = myvprint("hidden_nn_%i_%c",i,(dir==LAYER_DIR_FWD?'f':'b'));
        }
        l->setName(_tmp);
        free(_tmp);
      }
      
      // add the new layer
      rnn->addLayer(l, idx);
      // connect hidden layer
      rnn->connectTo(idx,1+recurrent);
      // the input connection 
      if ((net.bidirectional)&&(idx>1)) {
        rnn->connectFrom(idx-2,idx); // TODO: consider gather layers...??
      } else {
        rnn->connectFrom(idx-1,idx); // previous layer only
      }
      if (recurrent) {
        // the recurrent connection
        rnn->connectFrom(idx,idx); 
      }

      // initialise this connection
      rnn->initialise(idx);


      //// set weights
      // the bias weights
      j = findWeights(LAYER_BIAS, dir | LAYER_HIDDEN | i);
      rnn->setBias(idx,net.wv[j]->weights, net.wv[j]->nWeights);

      
      // set connection weights
      if (net.bidirectional) {
        // bidirectional layers...
        // input: input layer? ?
        if (idx > 2) {
          // TODO: connect previous gather layer as input to both fwd and rwd layer
          j = findWeights(LAYER_HIDDEN_GATHER | i-1, dir | LAYER_HIDDEN | i);
        } else {
          // connect input layer to both fwd and rwd layers
          j = findWeights(LAYER_INPUT, dir | LAYER_HIDDEN | i);
        }
      } else if (idx > 1) {
        // previous hidden layer, non-bidirectional  (in the non-bidirectional case this is equivalent to the gather layer)
        j = findWeights(LAYER_HIDDEN_GATHER | i-1, dir | LAYER_HIDDEN | i);
      } else {
        // input layer..
        j = findWeights(LAYER_INPUT, dir | LAYER_HIDDEN | i);
      }
      rnn->setWeights(0,idx,net.wv[j]->weights, net.wv[j]->nWeights);
      

      if (recurrent) {
        // the recurrent connection weights
        j = findWeights(dir | LAYER_HIDDEN | i, dir | LAYER_HIDDEN | i);
        rnn->setWeights(1,idx,net.wv[j]->weights, net.wv[j]->nWeights);
      }

      idx++; // to next layer...

      if (net.bidirectional) { ///// add the backward layers in the next loop iteration ...
        if (dir == LAYER_DIR_FWD) { /* 0 = FWD */
          i--;
          dir = LAYER_DIR_RWD; /* 1 = reverse */
        } else {
          dir = LAYER_DIR_FWD;
        }
      }

    }

    // output layer... (TODO: other output layer types than regression...)
    SMILE_IMSG(2,"net-task: %i",net.task);
    SMILE_IDBG(2,"net output size: %i",net.outputSize);
    if (net.task == NNTASK_CLASSIFICATION) {
      cNnSoftmaxLayer * sfl = new cNnSoftmaxLayer(net.outputSize,idx);
      sfl->setName("softmax_output");
      rnn->addLayer(sfl, idx);
    } else if (net.task == NNTASK_TRANSCRIPTION) {
      cNnSoftmaxLayer * sfl = new cNnSoftmaxLayer(net.outputSize,idx);
      sfl->setName("CTCsoftmax_output");
      rnn->addLayer(sfl, idx);
    } else {
      nl = new cNnNNlayer(net.outputSize,idx);
      tf = new cNnTfIdentity(); // default sigmoid unit..? or identity at the inputs??
      nl->createCells(tf);
      nl->setName("output");
      rnn->addLayer(nl, idx);
      //rnn->addLayer(new cNnNNlayer(net.outputSize,idx), idx);
    }

    // connect output layer
    if (net.bidirectional) { // TODO.... memory!
      rnn->connectTo(idx, 2);
      rnn->connectFrom(idx-1, idx);
      rnn->connectFrom(idx-2, idx);
    } else {
      rnn->connectTo(idx, 1);
      rnn->connectFrom(idx-1, idx);
    }

    rnn->initialise(idx);

    j = findWeights(LAYER_BIAS, LAYER_OUTPUT);
    rnn->setBias(idx,net.wv[j]->weights, net.wv[j]->nWeights);

    j = findWeights(LAYER_DIR_FWD | LAYER_HIDDEN | (net.nHiddenLayers-1), LAYER_OUTPUT);
//    fprintf(stderr,"setW j=%i  %i  %i\n",j,idx,net.wv[j]->nWeights);
    rnn->setWeights(0,idx,net.wv[j]->weights, net.wv[j]->nWeights);
    if (net.bidirectional) {
      j = findWeights(LAYER_DIR_RWD | LAYER_HIDDEN | (net.nHiddenLayers-1), LAYER_OUTPUT);
      rnn->setWeights(1,idx,net.wv[j]->weights, net.wv[j]->nWeights);
    }

    if (printConnections) rnn->printConnections();

    return 1;
  }
  return 0;
}

#endif

int cRnnSink::myConfigureInstance()
{
  int ret = cDataSink::myConfigureInstance();
  if (ret) {
    if (actoutput != NULL) { 
      outfile = fopen(actoutput,"w");  // TODO: "append" option
      if (outfile == NULL) {
        SMILE_IERR(1,"cannot open output activations output file '%s' for writing! Check if the path etc. exists and is writeable, also check for free disk space!",actoutput);
        COMP_ERR("aborting");
      }
    }
    if (classoutput != NULL) {
      outfileC = fopen(classoutput,"w");  // TODO: "append" option
      if (outfileC == NULL) {
        SMILE_IERR(1,"cannot open class output file '%s' for writing! Check if the path etc. exists and is writeable, also check for free disk space!",classoutput);
        COMP_ERR("aborting");
      }
    }
    ret = smileRnn_loadNet(netfile,net);
  }
  return ret;
}


int cRnnSink::myFinaliseInstance()
{
  int ret = cDataSink::myFinaliseInstance();

  if (ret) {
    if (getInt("printInputStats")) {
      FLOAT_NN *wg = NULL;
      long N = smileRnn_getInputSelection(net, &wg);
      long i;
      if (wg != NULL) {
        SMILE_PRINT("input weighting:");
        for (i=0; i<N; i++) {
          SMILE_PRINT("%i: %f",i,wg[i]);
        }
      } else {
        SMILE_IERR(1,"input weighting information is not available");
      }
    }
    ret = smileRnn_createNet(net,rnn,printConnections);
    if (ret) {
      in = (FLOAT_NN*)malloc(sizeof(FLOAT_NN)*net.inputSize);
      out = (FLOAT_DMEM*)malloc(sizeof(FLOAT_DMEM)*net.outputSize);
    }
  }
  return ret;
}


eTickResult cRnnSink::myTick(long long t)
{
  SMILE_IDBG(4,"tick # %i, reading value vector:",t);
  cVector *vec= reader_->getNextFrame();
  if (vec == NULL) return TICK_SOURCE_NOT_AVAIL;
  
  long vi = vec->tmeta->vIdx;
  double tm = vec->tmeta->time;
  
// pass current vector to net
  long i, N=0;
  for (i=0; i<MIN(vec->N,net.inputSize); i++) { in[i] = (FLOAT_NN)(vec->data[i]); }
  rnn->forward(in, /*vec->N*/ MIN(vec->N,net.inputSize));
  const FLOAT_NN *outp = rnn->getOutput(&N);
//printf("outp[0]= %f , vec->data[0] = %f\n",outp[0],vec->data[0]);  

  // for now, print the output vector to stdout...
  //printf("--- vi = %i ---\n",vi);
  double max=0.0; int maxi=-1;
  if (outfile) {
    for (i=0; i<N-1; i++) {
      fprintf(outfile,"%f ",outp[i]);
      if (outp[i]>max) { maxi=i; max=outp[i]; }
    }
    fprintf(outfile,"%f\n",outp[i]);
    if (outp[i]>max) { maxi=i; max=outp[i]; }
  } else {
    for (i=0; i<N; i++) {
      if (outp[i]>max) { maxi=i; max=outp[i]; }
    }
  }

  //printf("voice: %f\n",outp[1]);

  if (ctcDecode && (net.task == NNTASK_TRANSCRIPTION)) {

    if (lasti != maxi) {
      // TODO: output to file
      // TODO: send smileMessage of a new type to be created...
      if (classlabelArr != NULL) {
        if (maxi < nClasses) printf("%s ",classlabelArr[maxi]);
        if ((maxi > -1) &&(maxi < nClasses) && (outfileC != NULL)) {
          fprintf(outfileC,"%s\n",classlabelArr[maxi]);
	}
      } else { // fallback to 39 phone TIMIT set, alphabetical order ...
        switch(maxi){
      case 0: printf("ae "); break;
      case 1: printf("ah "); break;
      case 2: printf("ao "); break;
      case 3: printf("aw "); break;
      case 4: printf("ay "); break;
      case 5: printf("b "); break;
      case 6: printf("ch "); break;
      case 7: printf("d "); break;
      case 8: printf("dh "); break;
      case 9: printf("dx "); break;
      case 10: printf("eh "); break;
      case 11: printf("el "); break;
      case 12: printf("en "); break;
      case 13: printf("er "); break;
      case 14: printf("ey "); break;
      case 15: printf("f "); break;
      case 16: printf("g "); break;
      case 17: printf("h "); break;
      case 18: printf("hh "); break;
      case 19: printf("ih "); break;
      case 20: printf("iy "); break;
      case 21: printf("jh "); break;
      case 22: printf("k "); break;
      case 23: printf("m "); break;
      case 24: printf("ng "); break;
      case 25: printf("ow "); break;
      case 26: printf("oy "); break;
      case  27: printf("p "); break;
      case  28: printf("r "); break;
      case  29: printf("s "); break;
      case  30: printf("sh "); break;
        //case  31: printf("sil "); break;
      case  32: printf("t "); break;
      case  33: printf("th "); break;
      case  34: printf("uh "); break;
      case  35: printf("uw "); break;
      case  36: printf("v "); break;
      case  37: printf("w "); break;
      case  38: printf("y "); break;
      case  39: printf("z "); break;
        }
      }
      fflush(stdout);
      lasti = maxi;
    }

  } else if (net.task == NNTASK_TRANSCRIPTION || net.task == NNTASK_CLASSIFICATION) {
    // output of winning class
    if ((maxi > -1) &&(maxi < nClasses) && (outfileC != NULL)) {
      fprintf(outfileC,"%s\n",classlabelArr[maxi]);
    }
    // TODO: send classification result message!

  }

  // TODO: a cVectorProcessor RNN component...!
  // TODO: rnn functions in extra hpp/cpp file for more reusability!

  return TICK_SUCCESS;
}


cRnnSink::~cRnnSink()
{
  if (outfile != NULL) { fclose(outfile); }
  if (outfileC != NULL) { fclose(outfileC); }
  if (in != NULL) free(in);
  if (out != NULL) free(out);
  if (rnn != NULL) delete rnn;
  if (classlabels != NULL) free(classlabels);
  if (classlabelArr != NULL) free(classlabelArr);
}


#endif // BUILD_RNN
