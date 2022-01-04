/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/

#include <core/smileCommon.hpp>
#include <rnn/rnn.hpp>
#include <limits>
#include <string>
#include <smileutil/JsonClasses.hpp>
#include <fstream>

#ifdef BUILD_MODELCRYPT
#include <private/modelEncryption.hpp>
#endif

#define MODULE "smileRnn"

#ifdef BUILD_RNN

static const FLOAT_NN expLim = log(FLT_MAX);

FLOAT_NN nnTf_logistic(FLOAT_NN x)
{
  if (x > expLim) { return 1.0; }
  else if (x < -expLim) { return 0.0; }
  return (FLOAT_NN)(1.0 / (1.0 + exp(-x)));
}

/**************************** cNnLSTMcell *******************************************/

void cNnLSTMcell::setPeepWeights(FLOAT_NN *x, long N, int copy=1)
{ // function currently works for 1 cell per block only..
  if (N != 3*nCells) { // 3 is the number of gates... thus the number of peephole connections per cell
    RNN_ERR(1, "cNnLSTMcell: number of peep weights in cell %i.%i (%i) does not match expected number (%i x %i (=cellsPerBlock) = %i)!",
            (int)layerIndex, (int)cellIndex, (int)N, 3, (int)nCells, 3*(int)nCells);
  }
  if (copy) {
    peep = (FLOAT_NN*)malloc(sizeof(FLOAT_NN)*N);
    memcpy(peep, x, N*sizeof(FLOAT_NN));
  } else {
    peep = x;
  }
  nPeeps = N;
}

// reset the cell
void cNnLSTMcell::reset() 
{
  // reset parent cell object
  cNnNNcell::reset();
  // reset cell state
  long i;
  for (i=0; i<nCells; i++) { 
    sc[i] = 0.0;
  }
  // reset outputs
  if ((cellOutputs != NULL) && (nOutputs > 1)) {
    long i; for (i=0; i<nCells; i++) { cellOutputs[i] = 0.0; }
  }
}


// feed data through the cell and compute the cell output (return value)
// note: for an lstm block with nCells > 1, the number of output values will be > 1... how to handle this??
const FLOAT_NN * cNnLSTMcell::forward(const FLOAT_NN *x, long *N) 
{
  // x[0] IG input
  // x[1] FG input
  // x[2] cell input to cell 1
  //( x[3] cell input to cell 2...)
  //( x[4] cell input to cell 3...)
  // ...
  // x[N] OG input

  FLOAT_NN actIG, actFG, actOG;

  if (nCells == 1) { // quick version for one cell per block

    // input Gate
    actIG = transferGate->f( x[0] + (*sc)*peep[0] );
    // forget Gate
    actFG = transferGate->f( x[1] + (*sc)*peep[1] );
    // cell
    *sc = actIG * *cNnNNcell::forward(x+2) + (*sc)*actFG;
    // output Gate
    actOG = transferGate->f( x[3] + (*sc)*peep[2] );
    // cell output

    cellOutput =  actOG * transferOut->f( *sc );
    //XX//fprintf(stderr,"pg: %f --> cellOutput %f   actOG %f   ag: %f\n",*sc,cellOutput,actOG,transferOut->f( *sc ));

    if (N!=NULL) *N=1;
    return &cellOutput;

  } else { // long version
    long i;
    FLOAT_NN sum=0.0;

    // input Gate
    for (i=0; i<nCells; i++) {  // TODO: check the order of the peephole weights
      sum += (sc[i])*peep[i];
    }
    actIG = transferGate->f( x[0] + sum );

    // forget Gate // TODO: add multi-dim. forget gate acts...? "num_seq_dims"
    sum = 0.0;
    for (i=0; i<nCells; i++) {  // TODO: check the order of the peephole weights
      sum += (sc[i])*peep[i+nCells];
    }
    actFG = transferGate->f( x[1] + sum );

    // cell
    for (i=0; i<nCells; i++) {  
      sc[i] = actIG * *cNnNNcell::forward(x+2+i) + (sc[i])*actFG;
      // TODO: add multi-dim. forget gate acts...? "num_seq_dims"
    }

    // output Gate
    sum = 0.0;
    for (i=0; i<nCells; i++) {  // TODO: check the order of the peephole weights
      sum += (sc[i])*peep[i+2*nCells];
    }
    actOG = transferGate->f( x[3] + sum );

    // cell output
    for (i=0; i<nCells; i++) {  
      cellOutputs[i] = actOG * transferOut->f(sc[i]);
    }

    if (N!=NULL) *N=nOutputs;
    return (const FLOAT_NN*)cellOutputs;
  }

}


/**************************** cNnLayer *******************************************/


// feed data forward, N must match the layer's input size (nInputs)
void cNnLayer::forward(FLOAT_NN *x, long N) 
{
  long i;
  FLOAT_NN * curoutp = output + curPtr * nOutputs;
  for (i=0; i<nCells; i++) {
    long n = nCellInputs;
    const FLOAT_NN *tmpout = cell[i]->forward(x,&n);
    x += nCellInputs;
    // add tmpout to our output vector
    memcpy(curoutp+i*nCellOutputs,tmpout,sizeof(FLOAT_NN)*nCellOutputs);
  }
  if (nContext > 0) {
    curPtr++; 
    if (curPtr > nContext) curPtr = 0;  /* NOTE: the buffersize of the output buffer is nContext + 1 */
    else nDelayed++;
  } /* else {
    memcpy(output+i*nCellOutputs,tmpout,sizeof(FLOAT_NN)*nCellOutputs);
    }*/

}

void cNnLayer::resetLayer() 
{
  // reset the output activation buffer
  nDelayed = 0;
  curPtr = 0;
  bzero(output , nContext*nOutputs*sizeof(FLOAT_NN));
  // reset the cells
  long i;
  for (i=0; i<nCells; i++) {
    cell[i]->reset();
  }
}


/**************************** cNnSoftmaxLayer *******************************************/

void cNnSoftmaxLayer::forward(FLOAT_NN *x, long N) 
{
  long i; 
  double sum = 0.0;
  //TODO: handle curPtr and context buffered output..!? OR: ensure nContext always == 1
  if (N > MIN(nInputs,nOutputs)) { N = MIN(nInputs,nOutputs); }
  for (i=0; i<N; i++) {
    if (x[i] == -std::numeric_limits<FLOAT_NN>::infinity()) { output[i] = 0.0; }
    else if (x[i] > expLim) { output[i] = FLT_MAX; }
    else output[i] = exp(x[i]);
    sum += output[i];
  }
  if (sum != 0.0) {
    for (i=0; i<N; i++) {
      output[i] = (FLOAT_NN)((double)output[i] / sum); 
    }
  }
}

/**************************************************************************************/
/**************************** cNnConnection *******************************************/
/**************************************************************************************/


void cNnConnection::initialise(long _bidir_bufsize)
{
  int i; int bidir=0;
  // get i/o size parameters from input layers:
  inputSize = 0;
  for (i=0; i<nInputs; i++) {
    inputStart[i] = inputSize;
    inputSize += input[i]->getOutputSize();
    if (input[i]->isReverse()) bidir = 1;
  }
  // get number of outputs:
  outputSize = output->getInputSize();
  // allocate weight memory for inputSize*outputSize weights:
  nWeights = inputSize*outputSize;
  //printf("iS: %i, oS: %i, nWeights:%i\n",inputSize,outputSize,nWeights); fflush(stdout);
  weight = (FLOAT_NN*)malloc(sizeof(FLOAT_NN)*nWeights);
  // allocate bias weight memory for outputSize weights:
  nBias = outputSize;
  bias = (FLOAT_NN*)malloc(sizeof(FLOAT_NN)*nBias);
  // allocate current output vector/buffer:
  /* we don't require the buffer in the connection object, it is in the layers
  if (bidir) {
  // bidirectional nets require buffered output for the "backward" pass
  //?? outputs = (FLOAT_NN*)calloc(1,sizeof(FLOAT_NN)*outputSize);
  } else {
  // caching of one output vector is sufficient for unidirectional (recurrent) nets
  */
  outputs = (FLOAT_NN*)calloc(1,sizeof(FLOAT_NN)*outputSize);
  //}
}

// print human readable connection information 
void cNnConnection::printInfo()
{
  int i;
  fprintf(stderr,"  from: ");
  for (i=0; i<nInputs; i++) {
    if (input[i] != NULL) 
      fprintf(stderr," '%s' (%li); ",input[i]->getName(),input[i]->getLayerIdx());
  }
  fprintf(stderr,"   -> to: '%s' (%li)\n",output->getName(),output->getLayerIdx());
}

// forward data (one timestep) from the input layers to the output layer
void cNnConnection::forward() 
{
  // the job of the connection class is:
  //   to read the layer outputs from the input layers (the first layer must initialised with forward() externally)
  //   apply the weights
  //   and build the input vectors for the output layer cells
  //   then call forward() on the output layer to "compute the next layer"
  int i;
  long j,n; 
  FLOAT_NN *wght = weight;
  FLOAT_NN *outp;

  for (j=0; j<outputSize; j++) {
    outputs[j] = bias[j];
  }

  /* handling of (buffered) bidirectional nets.... */
  /*

  check if output is reverse layer...
  if no:   
  forward data as usual, however only if all inputs from bidirectional input layers are available 
  if yes:
  forward data only (nContext times backward) if the input buffer is full (nDelayed == nContext)
  IMPORTANT: output only one next frame...?
  prob: the output activations get stored by the net..., so we only forward the top element of the activations...

  PROBLEM: theoretically every hidden layer in a bi-directional net introduces a delay
  */
  /****/

  // check if output is bidirectional reverse layer:
  int bidirOut = output->isReverse();

  // check for bidirectional inputs (if one is present we must delay processing)
  int bidirIn = 0;
  for (i=0; i<nInputs; i++) {
    if (input[i]->isReverse()) { bidirIn = 1; break; }
  }


  if (!bidirOut) { // "forward pass"
    //printf("fw ");
    // read input layer outputs, multiply with weights and sum to output
    for (i=0; i<nInputs; i++) {
      //printf("OAV(%i) = %i ",i,input[i]->outputAvailable());
      if (!(input[i]->outputAvailable())) { return; }

      long N=0;
      long _idx = 0;
      if (!input[i]->isReverse()) { _idx = input[i]->getNContext(); } // pull top index from reverse layers...?

      const FLOAT_NN *tmp = input[i]->getOutput(&N,_idx);
      long size = input[i]->getOutputSize();

      //XX//for (n=0; n<size; n++) { if (tmp[n] != 0.0) { fprintf(stderr,"input %i , tmp %i\n",i,n); } }

      // TODO: check _N == outputSize ??

      outp = outputs;
      for (j=0; j<outputSize; j++) {
        for (n=0; n<size; n++) {
          //XX//if (*wght != 0.0012) fprintf(stderr,"n  %i j %i\n",n,j);
          *outp += tmp[n]* (*(wght++));
        }
        outp++;
      }
      //XX//fprintf(stderr,"outputS   %i %i\n",outputSize,size);
    }

    // pass output to output layer
    //XX//for (n=0; n<outputSize; n++) { if (outputs[n] != 0.0) { fprintf(stderr,"FWoutput %i = %f\n",n,outputs[n]); } }
    output->forward(outputs,outputSize);

  } else { // "backward pass" over full buffer



    // we first must reset the layer, then gather the inputs at the end of the buffer (t=0)
    // and forward .. then loop through the buffer until we reach (t = -bufsize)
    // this is our first valid output of the output level 
    output->resetLayer();

    // check if input is available in all forward input layers... EXCLUDE self recurrent connections!
    for (i=0; i<nInputs; i++) {
      // we check if nContext samples are available in this input layer...
      if ((input[i]!=output)&&(!(input[i]->outputAvailable()))) { return; }
    }

    long nContext = output->getNContext();
    // Note: nContext in input an outout layers should be the same.. if not, we have to use a different value for nContext..? input context size?
    //printf("bw nCO=%i ",nContext);
    long c;
    for (c=0; c<nContext; c++) {

      wght = weight;
      // read input layer outputs, multiply with weights and sum to output
      for (i=0; i<nInputs; i++) {

        //printf("OAV(%i) = %i ",i,input[i]->outputAvailable()); fflush(stdout);
        // NOTE: backward recurrent connections must be forwarded disregarding the output available.... (we go through the data backwards anyways)


        long N=0;
        long _idx = 0;
        if (input[i]->isReverse() && (input[i] != output)) { _idx = c; } // TODO?: pull top index from reverse layers, except the self recurrent layers...?
        else { _idx = c ; }
        if (input[i] == output) { _idx = 0; }

        const FLOAT_NN *tmp = input[i]->getOutput(&N, _idx);
        long size = input[i]->getOutputSize();

        //XX//for (n=0; n<size; n++) { if (tmp[n] != 0.0) { fprintf(stderr,"input %i , tmp %i\n",i,n); } }

        // TODO: check _N == outputSize ??

        outp = outputs;
        //printf("size %i outputSize %i  (* = %i) nweights=%i\n",size,outputSize,size*outputSize,nWeights); fflush(stdout);
        for (j=0; j<outputSize; j++) {
          for (n=0; n<size; n++) {
            //XX//if (*wght != 0.0012) fprintf(stderr,"n  %i j %i\n",n,j);
            *outp += tmp[n]* (*(wght++));
          }
          outp++;
        }
        //XX//fprintf(stderr,"outputS   %i %i\n",outputSize,size);
      }

      // pass output to output layer
      //XX//for (n=0; n<outputSize; n++) { if (outputs[n] != 0.0) { fprintf(stderr,"FWoutput %i = %f\n",n,outputs[n]); } }
      output->forward(outputs,outputSize);

    }

  }
  //printf("\n");
}


/**************************************************************************************/
/**************************** cNnRnn **************************************************/
/**************************************************************************************/

// feed data forward through the net and compute the next output activations vector
void cNnRnn::forward(FLOAT_NN *x, long N)
{
  long i;
  // feed input to first layer (input layer)
  layer[0]->forward(x,N);
  // iterate through connections...
  for (i=1; i<=nConnections; i++) {
    if (connection[i] != NULL) connection[i]->forward();
    else { RNN_ERR(1,"connection[%li] is NULL",i); }
  }
}


// print the connections in human readable format to the log
void cNnRnn::printConnections()
{
  int i;
  fprintf(stderr,"%i net layers:\n",nLayers);
  for (i=0; i<nLayers; i++) {
    fprintf(stderr,"  %i : '%s'\n",i,layer[i]->getName());
  }
  for (i=1; i<=nConnections; i++) {
    if (connection[i] != NULL) {
      fprintf(stderr,"--CONNECTION %i--\n",i);
      connection[i]->printInfo();
    }
  }
}


/********************************************* RNN init *******************************/


long smileRnn_parseLayerNumber(char *s, unsigned long *dir)
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

cRnnWeightVector *smileRnn_createWeightVectorFromLine(char *line)
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
      SMILE_MSG(3,"%i peephole weights in layer '%s'",nWeights,_from);
    } else {
      SMILE_MSG(3,"%i weights : from '%s' to '%s'",nWeights,_from,_to);
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
      l->F |= smileRnn_parseLayerNumber(l->from+7,&dir);
      l->F |= dir;
    } else {
      l->F = 0; l->T=-1;
      if (!strncmp(l->from,"bias",4)) { l->F = LAYER_BIAS; }
      else if (!strncmp(l->from,"hidden_",7)) {  // || (!strncmp(l->from,"gather_",7))
        l->F = LAYER_HIDDEN; 
        // layer number and direction ...
        unsigned long dir=0;
        l->F |= smileRnn_parseLayerNumber(l->from+7,&dir);
        l->F |= dir;
      } else if (!strncmp(l->from,"gather_",7)) {  // || (!strncmp(l->from,"gather_",7))
        l->F = LAYER_HIDDEN_GATHER; 
        // layer number and direction ...
        //unsigned long dir=0;
        l->F |= smileRnn_parseLayerNumber(l->from+7,NULL);
        //l->F |= dir;
        printf("layer F: %li\n",l->F);
      }
      else if (!strncmp(l->from,"input",5)) { l->F = LAYER_INPUT; }

      if (!strncmp(l->to,"hidden_",7)) { 
        l->T = LAYER_HIDDEN;
        // layer number and direction ...
        unsigned long dir=0;
        l->T |= smileRnn_parseLayerNumber(l->to+7,&dir);
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
        FLOAT_NN wf = (FLOAT_NN)strtod(lastl, &ep);
        l->weights[curW++] = wf;
        if (nWeights == curW) {
          SMILE_ERR(1,"too many weights on line, expected only %i weights ('%s')",nWeights,lastl);
          break;
        }
        //XX//if (wf != 0.001) { fprintf(stderr,"curW %i of %i\n", curW,nWeights); }
        curl++; lastl = curl; 
      } else if ((*curl == '\n')||(*curl == '\r')||(*curl == 0)) { 
        *curl=0; char *ep=NULL;
        FLOAT_NN wf = (FLOAT_NN)strtod(lastl,&ep);
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

//void smileRnn_readJsonFile(rapidjson::Document *doc, const std::string &filename);
int smileRnn_readJsonFile(rapidjson::Document *doc, const std::string &filename)
{
    // open the file
    std::ifstream ifs(filename.c_str(), std::ios::binary);
    if (!ifs.good()) {
      SMILE_ERR(1, "smileRnn_readJsonFile: Cannot open file '%s' for reading.", filename.c_str());
      return 0;
    }
    // calculate the file size in bytes
    ifs.seekg(0, std::ios::end);
    std::streamoff size = ifs.tellg();
    ifs.seekg(0, std::ios::beg);

    char magicNumber;
    ifs >> magicNumber;
    size_t net_size = (size_t)size;
    bool commercialModelFormat = false;
    if (magicNumber == 1) {
        commercialModelFormat = true;
        SMILE_MSG(2, "Net file format: 22");
        // pointer is at 1 where content starts
        net_size--;
    } else {
        SMILE_MSG(2, "Net file format: 21");
        // need to seek to 0 again
        ifs.seekg(0, std::ios::beg);
    }
    // read the file into a buffer
    char *buffer = new char[(long) net_size + 1];
    SMILE_MSG(3, "File size: %d bytes", size);
    SMILE_MSG(3, "Net size: %d bytes", net_size);
#ifdef BUILD_MODELCRYPT
    cSmileModelcrypt crypt;
    crypt.read(ifs, buffer, net_size, commercialModelFormat);
#else    
    if (commercialModelFormat) {
      SMILE_ERR(1, "This model file (%s) type is not supported by the open-source release of openSMILE. You need a commercial license to run it.");
      ifs.close();
      return 0;
    } else {
      ifs.read(buffer, net_size);
      buffer[net_size] = '\0';
    }
#endif
    std::string docStr(buffer);
    delete[] buffer;
    // extract the JSON tree
    if (doc->Parse<0>(docStr.c_str()).HasParseError()) {
      SMILE_ERR(1, "Json Parse error in smileRnn_readJsonFile: %s", doc->GetParseError());
      return 0;
    }
    return 1;
}

int smileRnn_parseHiddenType(const char *n1, int layerIdx, cRnnNetFile &net, const char *filename = NULL)
{
  if (!strncmp(n1,"lstm",4)) {
    net.hiddenType[layerIdx] = NNLAYERTYPE_LSTM;
    net.hiddenActType[layerIdx] = NNLSTMACT_TANHTANHLOGI;
    layerIdx++;
  } else if (!strncmp(n1,"blstm",5)) {
    net.hiddenType[layerIdx] = NNLAYERTYPE_LSTM;
    net.hiddenActType[layerIdx] = NNLSTMACT_TANHTANHLOGI;
    layerIdx++;
  } else if (!strncmp(n1,"linear_lstm",11)) {
    net.hiddenType[layerIdx] = NNLAYERTYPE_LSTM;
    net.hiddenActType[layerIdx] = NNLSTMACT_TANHIDENLOGI;
    layerIdx++;
  } else if (!strncmp(n1,"tanh",4)) {
    net.hiddenType[layerIdx] = NNLAYERTYPE_RNN;
    net.hiddenActType[layerIdx] = NNACT_TANH;
    layerIdx++;
  } else if (!strncmp(n1,"linear",6)) {
    net.hiddenType[layerIdx] = NNLAYERTYPE_RNN;
    net.hiddenActType[layerIdx] = NNACT_IDEN;
    layerIdx++;
  } else if (!strncmp(n1,"logistic",8)) {
    net.hiddenType[layerIdx] = NNLAYERTYPE_RNN;
    net.hiddenActType[layerIdx] = NNACT_LOGI;
    layerIdx++;
  } else if (!strncmp(n1,"feedforward_tanh", 16)) {  // currennt specific name
    net.hiddenType[layerIdx] = NNLAYERTYPE_NN;
    net.hiddenActType[layerIdx] = NNACT_TANH;
    layerIdx++;
  } else if (!strncmp(n1,"feedforward_linear", 18)) {  // currennt specific name
    net.hiddenType[layerIdx] = NNLAYERTYPE_NN;
    net.hiddenActType[layerIdx] = NNACT_IDEN;
    layerIdx++;
  } else if (!strncmp(n1,"feedforward_logistic", 20)) {  // currennt specific name ?
    net.hiddenType[layerIdx] = NNLAYERTYPE_NN;
    net.hiddenActType[layerIdx] = NNACT_LOGI;
    layerIdx++;
  } else if (!strncmp(n1,"recurrent_tanh", 14)) {  // currennt specific name
    net.hiddenType[layerIdx] = NNLAYERTYPE_RNN;
    net.hiddenActType[layerIdx] = NNACT_TANH;
    layerIdx++;
  } else if (!strncmp(n1,"recurrent_linear", 16)) {  // currennt specific name
    net.hiddenType[layerIdx] = NNLAYERTYPE_RNN;
    net.hiddenActType[layerIdx] = NNACT_IDEN;
    layerIdx++;
  } else if (!strncmp(n1,"recurrent_logistic", 18)) {  // currennt specific name ?
    net.hiddenType[layerIdx] = NNLAYERTYPE_RNN;
    net.hiddenActType[layerIdx] = NNACT_LOGI;
    layerIdx++;
  }
  else {
    SMILE_ERR(1,"unsupported hidden layer type '%s' ('hiddenType' option) while reading '%s'.", n1, filename);
  }
  return layerIdx;
}

int smileRnn_loadNetJson(const char *filename, cRnnNetFile &net) 
{
  rapidjson::Document netDoc;
  if (!smileRnn_readJsonFile(&netDoc, filename)) {
    return 0;
  }

  smileutil::json::JsonDocument jsonDoc(netDoc);
  // check the layers and weight sections
  if (!jsonDoc->HasMember("layers")) {
    SMILE_ERR(1, "smileRnn_loadNetJson: Missing section 'layers'. (file '%s')", filename);
    return 0;
  }
  rapidjson::Value &layersSection  = (*jsonDoc)["layers"];
  if (!layersSection.IsArray()) {
    SMILE_ERR(1, "smileRnn_loadNetJson: 'layers' is not an array. (file '%s')", filename);
    return 0;
  }

  smileutil::json::JsonValue weightsSection;
  if (jsonDoc->HasMember("weights")) {
    if (!(*jsonDoc)["weights"].IsObject()) {
      SMILE_ERR(1, "smileRnn_loadNetJson: Section 'weights' is not an object. (file '%s')", filename);
      return 0;
    }
    weightsSection = smileutil::json::JsonValue(&(*jsonDoc)["weights"]);
  }

  // extract the layers
  net.nWeightVectors = 0;
  net.bidirectional = 0;
  int max_hidden_layer_number = -1;
  int nLayers = 0;
  int nHiddenLayers = 0;
  const int nmap[4] = {2, 0, 1, 3};
  for (rapidjson::Value::ValueIterator layerChild = layersSection.Begin();
      layerChild != layersSection.End(); ++layerChild) {
    // check the layer child type
    if (!layerChild->IsObject()) {
      SMILE_ERR(1, "smileRnn_loadNetJson: A layer section in the 'layers' array is not an object. (file '%s')", filename);
      return 0;
    }
    // extract the layer type
    if (!layerChild->HasMember("type")) {
      SMILE_ERR(1, "smileRnn_loadNetJson: Missing value 'type' in layer description. (file '%s')", filename);
      return 0;
    }
    std::string layerType = (*layerChild)["type"].GetString();
    // extract the layer name
    if (!layerChild->HasMember("name")) {
      SMILE_ERR(1, "smileRnn_loadNetJson: Missing value 'name' in layer description. (file '%s')", filename);
      return 0;
    }
    std::string layerName = (*layerChild)["name"].GetString();
    int layerSize = (*layerChild)["size"].GetInt();

    if (weightsSection.isValid()) {
      if (!strncmp(layerName.c_str(), "input", 5)) {
        nLayers++;
        // no weight vector is needed, as they are in the weight sections of the hidden layers
        // we might only store the size here (as previous size) to verify the level_0 inputsize
        net.inputSize = layerSize;
        continue;
      }

      if (!weightsSection->HasMember(layerName.c_str())) {
        if (strncmp(layerName.c_str(), "postoutput", 10)) {
          SMILE_ERR(1, "smileRnn_loadNetJson: Missing weights section for layer '%s'. (file '%s')", layerName.c_str(), filename);
        }
        continue;
      }
      const rapidjson::Value &weightsChild = (*weightsSection)[layerName.c_str()];
      if (!weightsChild.IsObject()) {
        SMILE_ERR(1, "smileRnn_loadNetJson: Weights section for layer '%s' is not an object. (file '%s')", layerName.c_str(), filename);
        return 0;
      }

      if (!weightsChild.HasMember("input") || !weightsChild["input"].IsArray()) {
        SMILE_ERR(1, "smileRnn_loadNetJson: Missing array 'weights/%s/input'. (file '%s')", layerName.c_str(), filename);
        return 0;
      }
      if (!weightsChild.HasMember("bias") || !weightsChild["bias"].IsArray()) {
        SMILE_ERR(1, "smileRnn_loadNetJson: Missing array 'weights/%s/bias'. (file '%s')", layerName.c_str(), filename);
        return 0;
      }
      if (!weightsChild.HasMember("internal") || !weightsChild["internal"].IsArray()) {
        SMILE_ERR(1, "smileRnn_loadNetJson: Missing array 'weights/%s/internal'. (file '%s')", layerName.c_str(), filename);
        return 0;
      }
      const rapidjson::Value &inputWeightsChild    = weightsChild["input"];
      const rapidjson::Value &biasWeightsChild     = weightsChild["bias"];
      const rapidjson::Value &internalWeightsChild = weightsChild["internal"];

      /* TODO
            if (inputWeightsChild.Size() != this->size() * inputWeightsPerBlock * m_precedingLayer.size())
              throw std::runtime_error(std::string("Invalid number of input weights for layer '") + this->name() + "'");
            if (biasWeightsChild.Size() != this->size() * inputWeightsPerBlock)
              throw std::runtime_error(std::string("Invalid number of bias weights for layer '") + this->name() + "'");
            if (internalWeightsChild.Size() != this->size() * internalWeightsPerBlock)
              throw std::runtime_error(std::string("Invalid number of internal weights for layer '") + this->name() + "'");
       */

      // fill layer metadata
      // get layer category (input, hidden, output)
      long n = 0;
      if (!strncmp(layerName.c_str(), "output", 6)) {
        nLayers++;
        SMILE_MSG(3, "[rnn] output layer %i size %i", nHiddenLayers, layerSize);
        SMILE_MSG(3, "[rnn] output layer #%i w size %i", nHiddenLayers, (int)inputWeightsChild.Size());
        net.outputSize = layerSize;
        // output (input) weights
        cRnnWeightVector *v = new cRnnWeightVector(inputWeightsChild.Size());
        v->F = LAYER_HIDDEN;
        v->F |= max_hidden_layer_number;
        v->F |= LAYER_DIR_FWD;  // TODO: replace by variable like max_hidden_layer_dir
        v->T = LAYER_OUTPUT;
        // fill layer weights
        n = 0;
        for (rapidjson::Value::ConstValueIterator it = inputWeightsChild.Begin();
            it != inputWeightsChild.End(); ++it) {
          v->weights[n++] = (FLOAT_NN)(it->GetDouble());
        }
        // add to layer weight vector collection
        net.wv[net.nWeightVectors++] = v;
        // output bias:
        v = new cRnnWeightVector(biasWeightsChild.Size());
        v->F = LAYER_BIAS;
        v->T = LAYER_OUTPUT;
        // fill layer weights
        n = 0;
        for (rapidjson::Value::ConstValueIterator it = biasWeightsChild.Begin();
            it != biasWeightsChild.End(); ++it) {
          v->weights[n++] = (FLOAT_NN)(it->GetDouble());
        }
        // add to layer weight vector collection
        net.wv[net.nWeightVectors++] = v;

      } else if (!strncmp(layerType.c_str(), "lstm", 4) && !strncmp(layerName.c_str(), "lstm_level_", 11)) {
        //const char * layerNum = layerName.c_str() + 11;
        //char * eptr = NULL;
        //int layer_number = strtol(layerNum, &eptr, 10);
        smileRnn_parseHiddenType(layerType.c_str(), nHiddenLayers, net, filename);
        // NEW: override the parsed number to get the actual number!
        int layer_number = nHiddenLayers;
        if (layer_number > max_hidden_layer_number) {
          max_hidden_layer_number = layer_number;
        }
        net.hiddenSize[nHiddenLayers++] = layerSize;
        nLayers++;
        SMILE_MSG(3, "[rnn] lstm layer %i size %i", layer_number, layerSize);
        SMILE_MSG(3, "[rnn] lstm input %i w size %i", layer_number, (int)inputWeightsChild.Size());
        // create input weight vector
        cRnnWeightVector *v = new cRnnWeightVector(inputWeightsChild.Size());
        double *weightTmp = new double[inputWeightsChild.Size()];
        if (v != NULL) {
          v->T = LAYER_HIDDEN;
          v->T |= layer_number;
          v->T |= LAYER_DIR_FWD;
          if (layer_number == 0) {
            v->F = LAYER_INPUT;
          } else {
            v->F = LAYER_HIDDEN;
            v->F |= layer_number - 1;
          }
          // fill layer weights
          n = 0;
          for (rapidjson::Value::ConstValueIterator it = inputWeightsChild.Begin();
              it != inputWeightsChild.End(); ++it) {
            weightTmp[n++] = it->GetDouble();
          }
          // re-order layer weights from current to rnnlib/smile ordering
          int prevSize = inputWeightsChild.Size() / (layerSize * 4);
          double *weightCell = weightTmp;
          double *weightIg = weightTmp + prevSize * layerSize;
          double *weightFg = weightTmp + 2 * prevSize * layerSize;
          double *weightOg = weightTmp + 3 * prevSize * layerSize;
          for (int j = 0; j < layerSize; j++) {
            int jp4 = j * prevSize * 4;
            int jp = j * prevSize;
            for (int i = 0; i < prevSize; i++) {
              v->weights[jp4 + i] = (FLOAT_NN)weightIg[jp + i];
              v->weights[jp4 + prevSize + i] = (FLOAT_NN)weightFg[jp + i];
              v->weights[jp4 + prevSize * 2 + i] = (FLOAT_NN)weightCell[jp + i];
              v->weights[jp4 + prevSize * 3 + i] = (FLOAT_NN)weightOg[jp + i];
            }
          }
          // add to layer weight vector collection
          net.wv[net.nWeightVectors++] = v;
        }
        delete[] weightTmp;

        // create bias weight vector
        v = new cRnnWeightVector(biasWeightsChild.Size());
        weightTmp = new double[biasWeightsChild.Size()];
        if (v != NULL) {
          v->F = LAYER_BIAS;
          v->T = LAYER_HIDDEN;
          v->T |= LAYER_DIR_FWD;
          v->T |= layer_number;
          // fill layer weights
          n = 0;
          /*for (rapidjson::Value::ConstValueIterator it = biasWeightsChild.Begin();
              it != biasWeightsChild.End(); ++it) {
            //v->weights[n++] = (FLOAT_NN)(it->GetDouble());
            int nb = n / 4;
            int nl = nmap[n % 4];
            v->weights[nb + nl] = it->GetDouble();
            n++;
          }*/
          // fill layer weights
          n = 0;
          for (rapidjson::Value::ConstValueIterator it = biasWeightsChild.Begin();
              it != biasWeightsChild.End(); ++it) {
            weightTmp[n++] = it->GetDouble();
          }
          // re-order layer weights from current to rnnlib/smile ordering
          int prevSize = biasWeightsChild.Size() / (layerSize * 4);  // should be 1...
          double *weightCell = weightTmp;
          double *weightIg = weightTmp + prevSize * layerSize;
          double *weightFg = weightTmp + 2 * prevSize * layerSize;
          double *weightOg = weightTmp + 3 * prevSize * layerSize;
          for (int j = 0; j < layerSize; j++) {
            int jp4 = j * prevSize * 4;
            int jp = j * prevSize;
            for (int i = 0; i < prevSize; i++) {
              v->weights[jp4 + i] = (FLOAT_NN)weightIg[jp + i];
              v->weights[jp4 + prevSize + i] = (FLOAT_NN)weightFg[jp + i];
              v->weights[jp4 + prevSize * 2 + i] = (FLOAT_NN)weightCell[jp + i];
              v->weights[jp4 + prevSize * 3 + i] = (FLOAT_NN)weightOg[jp + i];
            }
          }
          // add to layer weight vector collection
          net.wv[net.nWeightVectors++] = v;
        }
        delete[] weightTmp;

        // check:
        if (4 * layerSize * layerSize + 3 * layerSize != internalWeightsChild.Size()) {
          SMILE_ERR(1, "There is something wrong with the size of the internal weight array on layer '%s' (num %i) (size %i).", layerName.c_str(), layer_number, layerSize);
          return 0;
        }

        // create internal (recurrent) weight vector
        v = new cRnnWeightVector(4 * layerSize * layerSize);
        weightTmp = new double[4 * layerSize * layerSize];
        if (v != NULL) {
          v->T = LAYER_HIDDEN;
          v->T |= layer_number;
          v->T |= LAYER_DIR_FWD;
          v->F = LAYER_HIDDEN;
          v->F |= layer_number;
          v->F |= LAYER_DIR_FWD;
          // fill layer weights
          n = 0;
          for (rapidjson::Value::ConstValueIterator it = internalWeightsChild.Begin();
              it != internalWeightsChild.Begin() + 4 * layerSize * layerSize; ++it) {
            weightTmp[n++] = it->GetDouble();
          }
          // re-order layer weights from current to rnnlib/smile ordering
          int prevSize = layerSize;
          double *weightCell = weightTmp;
          double *weightIg = weightTmp + prevSize * layerSize;
          double *weightFg = weightTmp + 2 * prevSize * layerSize;
          double *weightOg = weightTmp + 3 * prevSize * layerSize;
          for (int j = 0; j < layerSize; j++) {
            int jp4 = j * prevSize * 4;
            int jp = j * prevSize;
            for (int i = 0; i < prevSize; i++) {
              v->weights[jp4 + i] = (FLOAT_NN)weightIg[jp + i];
              v->weights[jp4 + prevSize + i] = (FLOAT_NN)weightFg[jp + i];
              v->weights[jp4 + prevSize * 2 + i] = (FLOAT_NN)weightCell[jp + i];
              v->weights[jp4 + prevSize * 3 + i] = (FLOAT_NN)weightOg[jp + i];
            }
          }
          // add to layer weight vector collection
          net.wv[net.nWeightVectors++] = v;
        }
        delete[] weightTmp;

        // create peep weight vector
        v = new cRnnWeightVector(3 * layerSize);
        if (v != NULL) {
          v->T = -1;
          v->F = LAYER_HIDDEN;
          v->F |= layer_number;
          v->F |= LAYER_PEEPHOLES;
          v->F |= LAYER_DIR_FWD;
          // fill layer weights
          n = 0;
          for (rapidjson::Value::ConstValueIterator it = internalWeightsChild.End() - 3 * layerSize;
              it != internalWeightsChild.End() - 2 * layerSize; ++it) {
            // input
            v->weights[n] = (FLOAT_NN)(it->GetDouble());
            n += 3;
          }
          n = 1;
          for (rapidjson::Value::ConstValueIterator it = internalWeightsChild.End() - 2 * layerSize;
              it != internalWeightsChild.End() - layerSize; ++it) {
            // input
            v->weights[n] = (FLOAT_NN)(it->GetDouble());
            n += 3;
          }
          n = 2;
          for (rapidjson::Value::ConstValueIterator it = internalWeightsChild.End() - layerSize;
              it != internalWeightsChild.End(); ++it) {
            // input
            v->weights[n] = (FLOAT_NN)(it->GetDouble());
            n += 3;
          }
          // add to layer weight vector collection
          net.wv[net.nWeightVectors++] = v;
        }

      } else if (!strncmp(layerType.c_str(), "blstm", 5) && !strncmp(layerName.c_str(), "blstm_level_", 12)) {
        const char * layerNum = layerName.c_str() + 11;
        char * eptr = NULL;
        long layer_number = strtol(layerNum, &eptr, 10);
        // TODO: need l->F = LAYER_HIDDEN_GATHER; somwhere!
        if (layer_number > max_hidden_layer_number) {
          max_hidden_layer_number = layer_number;
        }
        smileRnn_parseHiddenType(layerType.c_str(), nHiddenLayers, net, filename);
        net.hiddenSize[nHiddenLayers++] = layerSize / 2;
        nLayers++;
        net.bidirectional = 1;
        SMILE_ERR(1, "[rnn] BLSTM layers not yet supported!");
        SMILE_MSG(3, "[rnn] blstm layer %i size %i\n", layer_number, layerSize);
        SMILE_MSG(3, "[rnn] blstm input %i w size %i\n", layer_number, (int)inputWeightsChild.Size());
        //if (!strncmp(layerName.c_str(), "neuron_level_", xx)) {
      } else if (!strncmp(layerType.c_str(), "feedforward", 11) /*&& !strncmp(layerName.c_str(), "subsample_", 10)*/)  {
        //const char * layerNum = layerName.c_str() + 10;  // subsample_X_Y  (use X as layer number?) // UNUSED!
        //char * eptr = NULL;
        //int layer_number = strtol(layerNum, &eptr, 10);
        int layer_number = nHiddenLayers;
        if (layer_number > max_hidden_layer_number) {
          max_hidden_layer_number = layer_number;
        }
        smileRnn_parseHiddenType(layerType.c_str(), nHiddenLayers, net, filename);
        net.hiddenSize[nHiddenLayers++] = layerSize;
        nLayers++;
        SMILE_MSG(3, "[rnn] feedforward (nn) layer %i size %i", layer_number, layerSize);
        SMILE_MSG(3, "[rnn] feedforward (nn) input %i w size %i", layer_number, (int)inputWeightsChild.Size());

        // feedforward layer weights
        cRnnWeightVector *v = new cRnnWeightVector(inputWeightsChild.Size());
        if (v != NULL) {
          v->T = LAYER_HIDDEN; // + LAYER_SUBSAMPLE;
          v->T |= layer_number;
          v->T |= LAYER_DIR_FWD;
          if (layer_number == 0) {
            v->F = LAYER_INPUT;
          } else {
            v->F = LAYER_HIDDEN;
            v->F |= layer_number - 1;
          }
          // copy the weights
          n = 0;
          for (rapidjson::Value::ConstValueIterator it = inputWeightsChild.Begin();
              it != inputWeightsChild.End(); ++it) {
            v->weights[n++] = (FLOAT_NN)(it->GetDouble());
          }
          // add to layer weight vector collection
          net.wv[net.nWeightVectors++] = v;
        }
        // bias layer weights
        v = new cRnnWeightVector(biasWeightsChild.Size());
        if (v != NULL) {
          v->F = LAYER_BIAS;
          v->T = LAYER_HIDDEN;
          v->T |= LAYER_DIR_FWD;
          v->T |= layer_number;
          // copy the weights
          n = 0;
          for (rapidjson::Value::ConstValueIterator it = biasWeightsChild.Begin();
              it != biasWeightsChild.End(); ++it) {
            v->weights[n++] = (FLOAT_NN)(it->GetDouble());
          }
          // add to layer weight vector collection
          net.wv[net.nWeightVectors++] = v;
        }
      }
    }
  }

  // check if we have at least one input, one output and one post output layer
  if (nLayers < 3) {
    SMILE_ERR(1, "smileRnn_loadNetJson: Not enough layers defined. (file '%s')", filename);
    return 0;
  }

  // net task - set via config (TODO)
  const char *task = "regression";
  if (!strncmp(task,"regression",10)) {
    net.task = NNTASK_REGRESSION;
  } else if (!strncmp(task,"classification",14)) {
    net.task = NNTASK_CLASSIFICATION;
  } else if (!strncmp(task,"transcription",13)) {
    net.task = NNTASK_TRANSCRIPTION;
    //SMILE_IERR(1,"CTC decoding (task = transcription) not yet supported!");
  }
  // set these via config..?
  net.nContext = 0;  // context for online blstm
  net.loaded = 1;
  net.nHiddenLayers = nHiddenLayers;
  net.cellsPerBlock = 1; // FIXME...!
  return 1;
}



int smileRnn_loadNet(const char *filename, cRnnNetFile &net)
{
  int bidirectional = 1;
  bool encryptedFile = false;
  net.nContext = 0;
#ifdef BUILD_MODELCRYPT
  cSmileModelcrypt crypt;
#endif
  if (filename != NULL) {
    FILE *f = fopen(filename,"rb");
    if (f != NULL) {
      int magicNumber = fgetc(f);
      if (magicNumber == 1) {
        encryptedFile = true;
        SMILE_MSG(2, "Net file format: 2");
        // ignore magic number
      } else {
        SMILE_MSG(2, "Net file format: 1");
        fseek(f, 0, 0);
        // first byte is part of text ...
      }
      char *line=NULL;
      size_t n=0;
      int ret;
      do { // read the net file line by line...
#ifdef BUILD_MODELCRYPT
        ret = crypt.getLine(&line, &n, f, encryptedFile);
#else
        if (encryptedFile) {
          SMILE_ERR(1, "This model file (%s) type is not supported by this release of openSMILE.");
          fclose(f);
          return 0;
        } else {
          ret = smile_getline(&line, &n, f);
        }
#endif
//        printf("line = %s\n", line);
        if ((ret > 1)&&(line!=NULL)) {
          if (!strncmp(line,"weightContainer_",16)) { // weight vector line...
            cRnnWeightVector *v = smileRnn_createWeightVectorFromLine(line+16);
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
                  i = smileRnn_parseHiddenType(n1, i, net, filename);
                  /*
                   *
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
                    SMILE_ERR(1,"unsupported hidden layer type '%s' ('hiddenType' option) while reading '%s'.",n1,filename);
                  }
                  */
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
      free(line);
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
      SMILE_ERR(1,"failed to open rnn net file '%s'.",filename);
    }
  } else {
    SMILE_ERR(1,"failed to open rnn net file, the filename is a NULL string.");
  }
  return 0;
}

cNnLSTMlayer *smileRnn_createLstmLayer(int i, int idx, int dir, cRnnNetFile &net)
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

cNnNNlayer *smileRnn_createFeedforwardLayer(int i, int idx, int dir, cRnnNetFile &net)
{
  // create the forward layer
  cNnNNlayer * l = new cNnNNlayer(net.hiddenSize[i],idx,dir,net.nContext);
  // create cells with activation functions
  cNnTf * tf = NULL;
  if (net.hiddenActType[i] == NNACT_TANH) {
    tf = new cNnTfTanh();
  } else if (net.hiddenActType[i] == NNACT_IDEN) {
    tf = new cNnTfIdentity();
  } else if (net.hiddenActType[i] == NNACT_LOGI) {
    tf = new cNnTfLogistic();
  } else {
    COMP_ERR("unknown hiddenActType[%i] %i while creating a feedforward layer!", i, net.hiddenActType[i]);
  }
  l->createCells(tf);
  return l;
}

int smileRnn_findPeepWeights(unsigned long id, cRnnNetFile &net) 
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

int smileRnn_findWeights(unsigned long idFrom, unsigned long idTo, cRnnNetFile &net) 
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

int smileRnn_getInputSelection(cRnnNetFile &net, FLOAT_NN **weights)
{
  if (weights == NULL) return 0;

  int j = smileRnn_findWeights(LAYER_INPUT, LAYER_DIR_FWD | LAYER_HIDDEN | 0, net);
  
  // get number of input units
  *weights = (FLOAT_NN *)calloc(1,sizeof(FLOAT_NN)*net.inputSize);

  long nOut = net.wv[j]->nWeights / net.inputSize;

  // sum up input weights
  FLOAT_NN *w = net.wv[j]->weights;

  long i,n;
  for (i=0; i<net.inputSize; i++) {
    for (n=0; n<nOut; n++) {
      (*weights)[i] += fabs( *(w++) );
    }
    (*weights)[i] /= (FLOAT_NN)nOut;
  }

  // output weight vector:
  return net.inputSize;
}

// create a network from a successfully loaded net config file (loadNet function)
int smileRnn_createNet(cRnnNetFile &net, cNnRnn *&rnn, int printConnections)
{
  int i,j,idx=0;
  if (net.loaded) {
    int nlayers = net.nHiddenLayers+2;
    if (net.bidirectional) nlayers += net.nHiddenLayers;
    rnn = new cNnRnn(nlayers); // +1 input, +1 ouput layer

    // input layer...
    SMILE_DBG(2,"net input size: %i",net.inputSize);
    cNnNNlayer *nl = new cNnNNlayer(net.inputSize,0,0,net.nContext);
    cNnTf * tf = new cNnTfIdentity(); // identity at the inputs
    nl->createCells(tf);
    nl->setName("input");
    SMILE_MSG(3, "[rnn-c] adding input layer (%i)", idx);
    rnn->addLayer(nl, idx++);

    // hidden layers...
    unsigned long dir = LAYER_DIR_FWD;  // no blstm supported yet, or not needed?
    for (i=0; i<net.nHiddenLayers; i++) {
      cNnLayer *l = NULL;
      int recurrent = 0;

      ////////////// forward layers and backward layers (depends on odd/even iteration and net config)

      if (net.hiddenType[i] == NNLAYERTYPE_LSTM) {
        cNnLSTMlayer * ll = smileRnn_createLstmLayer(i,idx,(dir==LAYER_DIR_FWD?0:1),net);
        char *tmp = myvprint("hidden_lstm_%i_%c",i,(dir==LAYER_DIR_FWD?'f':'b'));
        ll->setName(tmp);
        free(tmp);
        //// peephole weights
        j = smileRnn_findPeepWeights(dir | LAYER_HIDDEN | i, net);
        if (j>=0) ll->setPeepWeights(net.wv[j]->weights, net.wv[j]->nWeights);
        l= (cNnLayer*)ll;
        recurrent = 1;
      } else if (net.hiddenType[i] == NNLAYERTYPE_NN) {
        cNnNNlayer * ll = smileRnn_createFeedforwardLayer(i,idx,(dir==LAYER_DIR_FWD?0:1),net);
        char *tmp = myvprint("hidden_feedforward_%i_%c",i,(dir==LAYER_DIR_FWD?'f':'b'));
        ll->setName(tmp);
        l = (cNnLayer*)ll;
      } else if (net.hiddenType[i] == NNLAYERTYPE_RNN) {
        SMILE_ERR(1, "[rnn] recurrent standard neuron (nn) layers are not yet supported!");
        l = new cNnNNlayer(net.hiddenSize[i],idx,(dir==LAYER_DIR_FWD?0:1),net.nContext);
        // TODO... weights, transfer functions, createCells, bias...

        char *_tmp;
          recurrent = 1; 
          _tmp = myvprint("hidden_rnn_%i_%c",i,(dir==LAYER_DIR_FWD?'f':'b'));
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
        SMILE_MSG(3, "[rnn-c] connecting layer %i from layer %i", idx, idx - 1);
      }
      if (recurrent) {
        // the recurrent connection
        rnn->connectFrom(idx,idx); 
      }

      // initialise this connection
      rnn->initialise(idx);

      //// set weights
      // the bias weights
      j = smileRnn_findWeights(LAYER_BIAS, dir | LAYER_HIDDEN | i, net);
      rnn->setBias(idx,net.wv[j]->weights, net.wv[j]->nWeights);


      // set connection weights
      if (net.bidirectional) {
        // bidirectional layers...
        // input: input layer? ?
        if (idx > 2) {
          // TODO: connect previous gather layer as input to both fwd and rwd layer
          j = smileRnn_findWeights(LAYER_HIDDEN_GATHER | i-1, dir | LAYER_HIDDEN | i, net);
        } else {
          // connect input layer to both fwd and rwd layers
          j = smileRnn_findWeights(LAYER_INPUT, dir | LAYER_HIDDEN | i , net);
        }
      } else if (idx > 1) {
        // previous hidden layer, non-bidirectional  (in the non-bidirectional case this is equivalent to the gather layer)
        j = smileRnn_findWeights(LAYER_HIDDEN_GATHER | i-1, dir | LAYER_HIDDEN | i, net);
        if (j < 0) {
          j = smileRnn_findWeights(LAYER_HIDDEN | i-1, dir | LAYER_HIDDEN | i, net);
        }
      } else {
        // input layer..
        j = smileRnn_findWeights(LAYER_INPUT, dir | LAYER_HIDDEN | i, net);
      }
      rnn->setWeights(0,idx,net.wv[j]->weights, net.wv[j]->nWeights);


      if (recurrent) {
        // the recurrent connection weights
        j = smileRnn_findWeights(dir | LAYER_HIDDEN | i, dir | LAYER_HIDDEN | i, net);
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
    SMILE_MSG(2,"[rnn] net-task: %i",net.task);
    SMILE_MSG(3,"[rnn] net output size: %i",net.outputSize);
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
      SMILE_ERR(1, "Bidirectional nets likely not yet supported!");
      rnn->connectTo(idx, 2);
      rnn->connectFrom(idx-1, idx);
      rnn->connectFrom(idx-2, idx);
    } else {
      SMILE_MSG(3, "[rnn-c] connecting layer %i (output layer) from layer %i", idx, idx - 1);
      rnn->connectTo(idx, 1);
      rnn->connectFrom(idx-1, idx);
    }

    rnn->initialise(idx);

    j = smileRnn_findWeights(LAYER_BIAS, LAYER_OUTPUT, net);
    rnn->setBias(idx,net.wv[j]->weights, net.wv[j]->nWeights);

    j = smileRnn_findWeights(LAYER_DIR_FWD | LAYER_HIDDEN | (net.nHiddenLayers-1), LAYER_OUTPUT, net);
    //    fprintf(stderr,"setW j=%i  %i  %i\n",j,idx,net.wv[j]->nWeights);
    if (j >= 0) {
      rnn->setWeights(0,idx,net.wv[j]->weights, net.wv[j]->nWeights);
    } else {
      SMILE_ERR(1, "No weights from hidden layer (forward) %i to output layer found!", (net.nHiddenLayers)); // +1 due to input layer
      return 0;
    }
    if (net.bidirectional) {
      j = smileRnn_findWeights(LAYER_DIR_RWD | LAYER_HIDDEN | (net.nHiddenLayers-1), LAYER_OUTPUT, net);
      if (j >= 0) {
        rnn->setWeights(1,idx,net.wv[j]->weights, net.wv[j]->nWeights);
      } else {
        SMILE_ERR(1, "No weights from hidden layer (backwards) %i to output layer found!", (net.nHiddenLayers-1));
        return 0;
      }
    }

    if (printConnections) rnn->printConnections();

    return 1;
  }
  return 0;
}

#endif // BUILD_RNN
