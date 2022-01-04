/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


#ifndef __RNN_HPP
#define __RNN_HPP

#include <core/smileCommon.hpp>

#ifdef BUILD_RNN
#include <float.h>

#define MAX_LAYERS 100 // this is used during loading of net config...

#define NNLAYERTYPE_NN 0
#define NNLAYERTYPE_RNN 1
#define NNLAYERTYPE_LSTM 2

#define NNLSTMACT_TANHTANHLOGI 11
#define NNLSTMACT_TANHIDENLOGI 12
#define NNACT_TANH              1
#define NNACT_IDEN              2
#define NNACT_LOGI              3

#define NNTASK_REGRESSION 1
#define NNTASK_CLASSIFICATION 2
#define NNTASK_TRANSCRIPTION 3

#define FLOAT_NN float

#define RNN_ERR(level, ...) { fprintf(stderr, "RNN ERROR: "); fprintf(stderr, __VA_ARGS__); fprintf(stderr,"\n"); }



FLOAT_NN nnTf_logistic(FLOAT_NN x);

class cNnTf {
  // transfer function class
public:
  // return the value of f(x), where f is the transfer (squashing) function
  virtual FLOAT_NN f(FLOAT_NN x)=0;
  virtual ~cNnTf() {}
};

class cNnTfTanh : public cNnTf {
  // transfer function class for tanh transfer function
public:
  // return the value of f(x), where f is the transfer function
  virtual FLOAT_NN f(FLOAT_NN x) override {
    // tanh transfer function:
    return (FLOAT_NN)(2.0) * nnTf_logistic((FLOAT_NN)2.0 * x) - (FLOAT_NN)1.0;
  }
};

class cNnTfIdentity : public cNnTf {
  // transfer function class for tanh transfer function
public:
  // return the value of f(x), where f is the transfer function
  virtual FLOAT_NN f(FLOAT_NN x) override {
    // tanh transfer function:
    return x;
  }
};

class cNnTfLogistic : public cNnTf {
  // transfer function class for logistic transfer function
public:
  // return the value of f(x), where f is the transfer function
  virtual FLOAT_NN f(FLOAT_NN x) override {
    // logistic transfer function:
    return nnTf_logistic(x);
  }
};



class cNnCell {
protected:
  // each cell can have multiple inputs to different units
  // a standard NN cell will only have one input which is the weighted sum of all the cell inputs
  // the weighted sum is computed by the cConnection object

  long nInputs;

  // each cell has only one output... except for LSTM blocks with multiple internal cells
  long nOutputs;
  FLOAT_NN *cellOutputs;
  FLOAT_NN cellOutput;

  // the transfer (squashing) function object of this cell
  cNnTf *transfer;
  // this is relevant only for debug output messages
  long layerIndex, cellIndex;  

public:
  // if transfer == NULL, then no squashing functions will be applied to the cell/unit output(s)
  cNnCell(cNnTf *_transfer=NULL, long _cIdx=0, long _lIdx=0) :
      cellIndex(_cIdx), layerIndex(_lIdx), transfer(_transfer), nOutputs(1), nInputs(1),
        cellOutputs(NULL), cellOutput(0.0)
  {
    // create a cell with nInputs number of units
  }

  // return the number of inputs (units) in this cell
  virtual long getInputSize() {
    return nInputs;
  }
  // return the number of outputs of this cell
  virtual long getOutputSize() {
    return nOutputs;
  }
  
  virtual void reset() 
  {
    cellOutput = 0.0;
  }


  // feed data through the cell and compute the cell output(s) (return value)
  // note: N and the size of *x must match the value returned by getInputSize()
  virtual const FLOAT_NN *forward(const FLOAT_NN *x, long *N=NULL)=0;

  // get the current output activation
  virtual const FLOAT_NN *getOutput(long *N=NULL) { if(N!=NULL) *N=1; return &cellOutput; }

  // destroy the cell
  virtual ~cNnCell() {}
};


// standard feed-forward or recurrent unit, 1 input, 1 output
class cNnNNcell : public cNnCell 
{
private:

public:
  cNnNNcell(cNnTf *_transfer=NULL, long _cIdx=0, long _lIdx=0) :
    cNnCell(_transfer, _cIdx, _lIdx)
  {}

  // feed data through the cell and compute the cell output (return value)
  // note: N and the size of *x must match the value returned by getInputSize()
  virtual const FLOAT_NN *forward(const FLOAT_NN *x, long *N=NULL) override 
  {
    // the input should only be 1 value (weighted sum of inputs, handled by the connection)
    // thus we apply the transfer function and have the output value :)
    if (transfer != NULL) {
      cellOutput = transfer->f(*x);
    } else {
      cellOutput = *x;
    }
    // set the number of outputs..
    if (N!=NULL) *N = 1;

    return (const FLOAT_NN*)&cellOutput;
  }

};


// LSTM block or cell
class cNnLSTMcell : public cNnNNcell {
private:
  // LSTM cells require the cell state to be saved:
  long nCells;
  FLOAT_NN *sc;  // cell state(s)


  // weights array for LSTM cells:
  // we have 4 weights for each input connection, 4*N total weights (N=nInput)
                                                   //NO!! 1...N inGate , 1...N forget Gate,  1..N outGate, 1...N cell input
  // in1, fg1, cellIn1, outgate1, in2, ..
   // for multiple cells per block: in1C1, in1C2, ..., fg1C1, fg1C2, ..., in2C1, in2C2, ...

  // the peep weights are in the following order:
  // inp1, forget1, out1, inp2, ...
  // for multiple cells per block, see above...

  // transfer functions for cell state output and gate ouputs:
  cNnTf * transferOut;
  cNnTf * transferGate;

  // peephole weights:
  long nPeeps; 
  FLOAT_NN *peep;

public:
  cNnLSTMcell(cNnTf *_transferIn, cNnTf *_transferOut, cNnTf *_transferGate, long _cellsPerBlock=1, long _cIdx=0, long _lIdx=0) :
      cNnNNcell(_transferIn, _cIdx, _lIdx), nCells(_cellsPerBlock), 
      peep(NULL),
      transferOut(_transferOut), transferGate(_transferGate)
  {
    // create a cell with nInputs to each unit
    nInputs = nCells+3;
    nOutputs = nCells;
    // create cell states...
    sc = (FLOAT_NN*)calloc(1,sizeof(FLOAT_NN)*nCells);
    // create cell outputs...
    cellOutputs = (FLOAT_NN*)calloc(1,sizeof(FLOAT_NN)*nCells);
  }

  void setPeepWeights(FLOAT_NN *x, long N, int copy);

  // reset the cell
  virtual void reset() override;

  // feed data through the cell and compute the cell output (return value)
  // note: for an lstm block with nCells > 1, the number of output values will be > 1... how to handle this??
  virtual const FLOAT_NN * forward(const FLOAT_NN *x, long *N=NULL) override;

  ~cNnLSTMcell() {
    //if (transferOut != NULL) delete transferOut;
    //if (transferGate != NULL) delete transferGate;
    if (sc != NULL) free(sc);
    if (cellOutputs != NULL) free(cellOutputs);
    if (peep != NULL) free(peep);
  }
};



/* a layer in the net */
class cNnLayer {
protected:
  int direction; // 0 == forward, 1 == reverse
  // a layer contains a set of cells or blocks which again contain cells
  long layerIdx; 
  long nCells; // nInputsTotal, nUnitsPerCell;
  long nContext;
  char *name;

  long nCellOutputs, nCellInputs;
  long nOutputs, nInputs;
  cNnCell **cell;
  // TODO. for on-line BRNN we require output to be 2D, buffering X past states..
  FLOAT_NN *output; // <-- cyclic buffer
  long curPtr;  // <- the current frame pointer (to the next uninitialised frame...)
  long nDelayed; // <- number of delayed output frames currently in buffer (0 if only the current frame is available)

  // this function creates the output buffers and does some initialisation, 
  //  after the cell objects have been created and we know the layer dimensions
  void afterCreateCells() 
  {
    if ((cell!=NULL) && (cell[0] != NULL)) {
      nCellInputs = cell[0]->getInputSize();
      nCellOutputs = cell[0]->getOutputSize();
    }
    if (nCells > 0) {
      nInputs = nCells * nCellInputs;
      nOutputs = nCells * nCellOutputs;
    }
    if (nOutputs > 0)
      output = (FLOAT_NN*)calloc(1,sizeof(FLOAT_NN)*nOutputs*(nContext+1));
    curPtr = 0;
    nDelayed = 0; // we init with 0 because always the first frame (all 0) is available
  }

public:
  // create a layer which has nInput connections for each cell
  cNnLayer(long _nCells, int _layerIdx=0, int _direction=0, long _nContext=0) : 
      nCells(_nCells), layerIdx(_layerIdx), cell(NULL), output(NULL), direction(_direction), nContext(_nContext), name(NULL)
  {
    if (_nCells > 0) 
      cell = (cNnCell**)calloc(1,sizeof(cNnCell*)*nCells);
    if (nContext < 0) nContext = 0;
  }

  int isReverse() { return direction; }
  
  // set the human readable name of this layer (required only for debug output)
  void setName(const char *_name ) 
  { 
    if (name != NULL) free(name);
    if (_name != NULL) name = strdup(_name); 
  }
  
  // get the human readable name of this layer
  const char *getName() { return (const char *)name; }

  // return the layer index assigned by the rnn net class when this layer was created
  long getLayerIdx() { return layerIdx; }

  // feed data forward, N must match the layer's input size (nInputs)
  virtual void forward(FLOAT_NN *x, long N);

  // get the output activations
  const FLOAT_NN * getOutput(long *N, long delay=0) 
  {
    if (N!=NULL) *N = nOutputs;
    if (delay == 0) {
      return (const FLOAT_NN*)output;
    } else {
      if (delay > nContext) delay = nContext;
      long idx = (nContext+curPtr-delay)%nContext;
      return ((const FLOAT_NN*)output)+idx;
    }
  }

  virtual void resetLayer();

  // check whether output is available
  // for forward layers this always returns 1 (true)
  // for reverse layers this only returns 1 (true) if the output (ring-)buffer is full
  int outputAvailable() {
    if (direction) {
//      printf("nD:%i nC:%i ",nDelayed,nContext);
      if (nDelayed >= nContext) return 1;
    } else {
      return 1;
    }
    return 0;
  }
  //TODO: we also require   a function to query the current context buffer state
  //long getNdelayed() { return nDelayed; }

  virtual long getOutputSize() { return nOutputs; }
  virtual long getInputSize() { return nInputs; }
  long getNContext() { return nContext; }

  virtual ~cNnLayer() {
    long i;
    if (cell != NULL) {
      for (i=0; i<nCells; i++) {
        if (cell[i] != NULL) delete(cell[i]);
      }
      free(cell);
    }
    if (output != NULL) free(output);
    if (name != NULL) free(name);
  }

};

/**************************** cNnNNlayer *******************************************/

class cNnNNlayer : public cNnLayer {
protected: 
  cNnTf *_tf;

public:
  cNnNNlayer(long _nCells, int _layerIdx=0, int _direction=0, long _nContext=0) : cNnLayer(_nCells, _layerIdx, _direction, _nContext), _tf(NULL) {}

  // create the cells for this layer
  void createCells(cNnTf *_transfer) {
    long i;
    for (i=0; i<nCells; i++) {
      cell[i] = new cNnNNcell(_transfer,i,layerIdx);
    }
    _tf = _transfer; // NOTE: the transfer function object will be freed by the layer, since it is allocated only once and the same pointer is passed to all cells...
    afterCreateCells();
  }

  ~cNnNNlayer() {
    if (_tf != NULL) delete _tf;
  }
};

/**************************** cNnSoftmaxLayer *******************************************/

class cNnSoftmaxLayer : public cNnLayer {
protected: 
  double expLim;

public:
  cNnSoftmaxLayer(int _nOutputs, int _layerIdx=0, int _direction=0, long _nContext=0) : cNnLayer(0, _layerIdx, _direction, _nContext), expLim(log(DBL_MAX)) 
  {
    nInputs = _nOutputs;
    nOutputs = _nOutputs; 
    afterCreateCells();
  }

  // create the cells for this layer, dummy function here...
  void createCells() {
    
  }

  virtual void forward(FLOAT_NN *x, long N) override;

  ~cNnSoftmaxLayer() {
  }
};

/**************************** cNnLSTMlayer *******************************************/

class cNnLSTMlayer : public cNnLayer {
protected: 
  cNnTf  *_tf, *_tfO, *_tfG;

public:
  cNnLSTMlayer(long _nCells, int _layerIdx=0, int _direction=0, long _nContext=0) : cNnLayer(_nCells, _layerIdx, _direction, _nContext), _tf(NULL), _tfO(NULL), _tfG(NULL) {}

  // create the cells for this layer
  void createCells(cNnTf *_transferIn, cNnTf *_transferOut, cNnTf *_transferGate, long _cellsPerBlock=1) {
    int i;
    for (i=0; i<nCells; i++) {
      cell[i] = new cNnLSTMcell(_transferIn, _transferOut, _transferGate, _cellsPerBlock, i, layerIdx);
    }
    _tf = _transferIn;
    _tfO = _transferOut;
    _tfG = _transferGate;
    afterCreateCells();
  }

  // set the LSTM cells' peephole weights: input is a vector with all weights for all cells in this layer
  virtual void setPeepWeights(FLOAT_NN *_peep, long N, int copy=1) 
  { 
    long i;
    for (i=0; i<nCells; i++) {
      ((cNnLSTMcell*)(cell[i]))->setPeepWeights(_peep+i*3,3,copy);
    }
  }

  ~cNnLSTMlayer() {
    if (_tf != NULL) delete _tf;
    if (_tfO != NULL) delete _tfO;
    if (_tfG != NULL) delete _tfG;
  }
};

/**************************************************************************************/
/**************************** cNnConnection *******************************************/
/**************************************************************************************/

class cNnConnection {
protected:
  int bidirectional;
  // a connection requires memory for recurrent connections , and connects output with inputs
  int nInputs; int inpCnt;
  cNnLayer **input;
  cNnLayer *output;
  // vector sizes:
  long inputSize;
  long *inputStart;
  long outputSize;

  // connection weights
  long nWeights;
  FLOAT_NN *weight;
  // bias weights
  long nBias;
  FLOAT_NN *bias;

  // current output vector:
  FLOAT_NN *outputs;

public:
  // create the connection, which forwards to the output layer _output, and which has _nInputs input layers
  cNnConnection(cNnLayer *_output, int _nInputs, int _bidirectional=0) :
      output(_output), nInputs(_nInputs), inpCnt(0), bias(NULL), weight(NULL), outputs(NULL), bidirectional(_bidirectional)
  {
    input = (cNnLayer**)calloc(1,sizeof(cNnLayer*)*nInputs);
    inputStart = (long*)malloc(sizeof(long*)*nInputs);
  }

  // connect an input layer to the connection, return layer index
  int connectLayer(cNnLayer *_input)
  {
    input[inpCnt] = _input;
    return inpCnt++;
  }

  // initialise connection parameters after all input layers have been added...
  void initialise(long _bidir_bufsize=0);

  // set the connection weights from one large array (which has been loaded from the net config file) for one input layer..
  // call only after initialise()
  void setWeights(FLOAT_NN *_weights, long N, int _layerIdx) {
    if (N!=input[_layerIdx]->getOutputSize()*outputSize) {
      RNN_ERR(1,"number of weights mismatch for layer %i in connection->setWeights: number weights = %li, expected %li",_layerIdx,N,input[_layerIdx]->getOutputSize()*outputSize);
    }
    //XX//fprintf(stderr,"setWW %i %i N=%i\n",(inputStart[_layerIdx]*outputSize),_layerIdx,N);
    memcpy(weight+(inputStart[_layerIdx]*outputSize), _weights, N*sizeof(FLOAT_NN));
  }

  virtual void setBias(FLOAT_NN *_bias, long N) 
  { 
    //XX//printf("outputSize:%i, nBias:%i, _N:%i  bias:%i  _bias:%i \n",output->getInputSize(),nBias,N,bias,_bias);
    memcpy(bias, _bias, N*sizeof(FLOAT_NN));
  }


  // forward data (one timestep) from the input layers to the output layer
  virtual void forward();

  // print human readable connection information 
  void printInfo();

  virtual ~cNnConnection()
  {
    if (inputStart!=NULL) free(inputStart);
    if (outputs != NULL) free(outputs);
    if (bias != NULL) free(bias);
    if (weight != NULL) free(weight);
    //if (output != NULL) delete output;
    if (input != NULL) {
      /*  // We do NOT free the input layers here, since an input layer may be connected to multiple connections and thus it may get freed multiple times
      int i;
      for (i=0; i<nInputs; i++) {
        if (input[i] != NULL) delete input[i];
      }*/
      free(input);
    }
  }
};



/**************************************************************************************/
/**************************** cNnRnn **************************************************/
/**************************************************************************************/


/* the RNN class */
class cNnRnn  {
protected:
  // a net consists of layers and connections
  int nLayers; 
  int curLidx;
  int nConnections;
  cNnLayer **layer;
  cNnConnection **connection;

public:
  cNnRnn(int _nLayers, int _nConnections=-1) : nLayers(_nLayers), nConnections(_nConnections), curLidx(0)
  {
    if (nConnections == -1) nConnections = nLayers-1;
    layer = (cNnLayer**)calloc(1,sizeof(cNnLayer*)*nLayers);
    connection = (cNnConnection**)calloc(1,sizeof(cNnConnection*)*(nConnections+1));
  }

  // add layers to the net....
  // idx == -1 : add as next layer, else add as layer with index 'idx' (0..nLayers-1)
  void addLayer(cNnLayer *_l, int idx=-1) 
  {
    if (idx < 0) idx = curLidx++;
    layer[idx] = _l;
  }

  //// create the connections *after* adding *all* layers!
  // create a connection to the inputs of a layer with index >1
  void connectTo(int toLayer, int lN) 
  {
    if (toLayer > 0) {
      connection[toLayer] = new cNnConnection(layer[toLayer], lN);
    }
  }

  // after calling connectTo (which creates the connection), call connectFrom '_N' times to connect the input layers
  // the return value is the input index (idxF_local), which must be used when setting weights for this connection!
  int connectFrom(int fromLayer, int toLayer) 
  {
    if (connection[toLayer] != NULL) {
      return connection[toLayer]->connectLayer(layer[fromLayer]);
    }
    return -1;
  }

  void initialise(int idx=-1) {
    if (idx < 0) {
      int i;
      for (i=0; i<nConnections; i++) connection[i]->initialise();
    } else {
      connection[idx]->initialise();
    }
  }

  void setWeights(int idxF_local, int idxT, FLOAT_NN *x, long N)
  {
    if ((idxT >= 0)&&(idxT<=nConnections)&&(idxF_local >= 0)&&(connection[idxT] != NULL))
      connection[idxT]->setWeights(x,N,idxF_local);
  }

  void setBias(int idxT, FLOAT_NN *x, long N)
  {
    if ((idxT >= 0)&&(idxT<=nConnections)&&(connection[idxT] != NULL))
      connection[idxT]->setBias(x,N);
  }

  // feed data forward through the net and compute the next output activations vector
  void forward(FLOAT_NN *x, long N);

  // read the output activations vector
  const FLOAT_NN * getOutput(long *N)
  {
    return layer[nLayers-1]->getOutput(N);
  }

  // print the connections in human readable format to the log
  void printConnections();

  virtual ~cNnRnn() {
    // delete the layers and the connections
    long i;
    if (layer != NULL) {
      for (i=0; i<nLayers; i++) {
        if (layer[i] != NULL) delete layer[i];
      }
      free(layer);
    }
    if (connection != NULL) {
      for (i=0; i<= nConnections; i++) {
        if (connection[i] != NULL) {
          delete connection[i];
        }
      }
      free(connection);
    }
  }
};



/***************************************************************************************/

// variables for layer type must be 32 bit unsigned long!
// lower bits < 1024 : layer number (0 for bias, etc.)
#define LAYER_BIAS    1024
#define LAYER_HIDDEN  2048
#define LAYER_DIR_FWD 4096
#define LAYER_DIR_RWD 8192
#define LAYER_INPUT   8192*2
#define LAYER_OUTPUT  8192*4
#define LAYER_PEEPHOLES 8192*8
#define LAYER_HIDDEN_GATHER  8192*16
#define LAYER_SUBSAMPLE  8192*32

class cRnnWeightVector {
public:
  long nWeights;
  FLOAT_NN *weights;
  char from[102];
  char to[102];
  long F,T; // From, To as int constants (or'ed)

  cRnnWeightVector(long N) : nWeights(N) {
    weights = (FLOAT_NN*)calloc(1,sizeof(FLOAT_NN)*N);
    from[0] = 0; to[0] = 0;
  }
  ~cRnnWeightVector() {
    free(weights);
  }
};

// data of parsed rnn net file
class cRnnNetFile {
public:
  int loaded;
  int nHiddenLayers; // the number of hidden layers!!
  long inputSize, outputSize;
  long hiddenSize[MAX_LAYERS];
  int hiddenType[MAX_LAYERS];
  int hiddenActType[MAX_LAYERS];
  int cellsPerBlock;
  int bidirectional;
  long nContext; // -1 : whole sequence (TODO...), 0 : no context (unidirectional only), >0 : number of future frames to buffer before output is generated
  int task;
  int nWeightVectors;
  cRnnWeightVector * wv[4*MAX_LAYERS];

  cRnnNetFile() : nWeightVectors(0), nHiddenLayers(0), loaded(0) {
    int i;
    for (i=0; i<4*MAX_LAYERS; i++) { wv[i] = NULL; }
  }
  ~cRnnNetFile() {
    int i;
    for (i=0; i<4*MAX_LAYERS; i++) { if (wv[i] != NULL) delete wv[i]; }
  }
};

// load and parse a saved network file
int smileRnn_loadNetJson(const char *filename, cRnnNetFile &net);

// load and parse a saved network file
int smileRnn_loadNet(const char *filename, cRnnNetFile &net);

//creat a cNnRnn class instance from a loaded network file
int smileRnn_createNet(cRnnNetFile &net, cNnRnn *&rnn, int printConnections=1);

// get the input weight statistics
int smileRnn_getInputSelection(cRnnNetFile &net, FLOAT_NN **weights);

#endif // BUILD_RNN
#endif // __RNN_HPP
