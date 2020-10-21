///////////////////////////////////////////////////////////////////////////////////////
///////// > openSMILE LSTM-RNN voice activity detector<              //////////////////
/////////                                                            //////////////////
///////// (c) audEERING UG GmbH                                      //////////////////
/////////     All rights reserved.                                  //////////////////
///////////////////////////////////////////////////////////////////////////////////////


This folder contains configuration files and models for a
prototype LSTM-RNN Voice Activity Detector.
This is meant as an example to show how to implement such
a VAD in openSMILE with the RNN components and to provide
a simple VAD for prototype development. The noise-robustness
of this VAD is *not* state-of-the-art, it has been built on
a small-scale research data-set, mainly for clean speech. 


This config implements a voice activity detector as described in:

  Eyben, F.; Weninger, F.; Squartini, S.; Schuller, B.:
    "Real-life voice activity detection with LSTM Recurrent Neural 
    Networks and an application to Hollywood movies," 
    Proc. of ICASSP 2013, pp. 483-487, 26-31 May 2013. 
    doi: 10.1109/ICASSP.2013.6637694

The models are not the same models as used in the paper, but a
preliminary version of the data presented in the paper.

If you need a more accurate and noise-robust VAD,
contact audEERING at info@audeering.com for a demo of
our latest commercial VAD technology.
We have extremely robust detectors, which are even capable
of reliably detecting sung speech (vocals) in single channel
polyphonic music recordings.


There are two main config files to start from here:
  vad_opensource.conf : Dumps vad activations per frame to CSV
  vad_segmenter.conf  : Creates wave files with voice segments
                        and optionally writes activations to CSV

Run 
  SMILExtract -ccmdHelp -C <configfile>
to see the commandline options supported by these config files.

If you want to include the VAD as a module in your own
configuration files, you can use:
  vad_opensource.conf.inc
  turnDetector.conf.inc

See the comments in these config files for level input/output
naming requirements. See the main config files for examples
of how to use the module includes.


