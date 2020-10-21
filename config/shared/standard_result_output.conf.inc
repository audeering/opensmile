///////////////////////////////////////////////////////////////////////////////////////
///////// > openSMILE configuration file for data output <           //////////////////
/////////                                                            //////////////////
///////// (c) 2014-2016 audEERING,                                   //////////////////
/////////     All rights reserved. See file COPYING for details     //////////////////
///////////////////////////////////////////////////////////////////////////////////////


/*
   This file can be included as data output file for standard feature
   extraction configuration files. It provides commandline options
   for the batch extraction GUI, and supports LLD and Functionals (summaries)
   saving.

   It requires the main extrator configuration file to provide the following
   data memory levels:  lld, lld_de, and func
 */

;;;;;;;;; output LLD features to CSV ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

[componentInstances:cComponentManager]
instance[predarffsink].type=cArffSink
instance[predcsvsink].type=cCsvSink
instance[predhtksink].type=cHtkSink

[predarffsink:cArffSink]
reader.dmLevel=model_predictions
frameIndex = 0
frameTime = \cm[predtimestamparff{0}:set to 1 to include timestamp column, default is 0, i.e. to not save timestamp in second column]
frameTimeAdd=\cm[predframeTimeAdd{0}:frame time offset -start of segment in seconds- used by avec2013]
filename=\cm[predarffoutput{?}:name of WEKA Arff output file, set to a valid filename to enable this output sink]
relation=\cm[predrelation{openSMILE_predictions}:arff relation attribute, feature set name and/or corpus name]
instanceName=\cm[instname]
 ;; use this line instead of the above to always set the instance name to the
 ;; name of the input wave file
 ;instanceName=\cm[inputfile]
\{\cm[predarfftargetsfile{arff_targets.conf.inc}:name of arff targets include file]}
append=\cm[predappendarff{1}:set to 0 to disable appending to an existing arff parameter summary file, given by the arffoutput option]
errorOnNoOutput = 1

[predcsvsink:cCsvSink]
reader.dmLevel=model_predictions
filename=\cm[predcsvoutput{?}:output CSV file for summarised parameters, set to a valid filename to enable this output sink, data is appended if file exists]
append=\cm[predappendcsv{1}:set to 0 to disable appending to an existing csv parameter summary file, given by the csvoutput option]
frameIndex=0
frameTime=\cm[predtimestampcsv{1}:set to 0 to suppress timestamp column, default is 1, i.e. to show timestamp in second column]
instanceName=\cm[instname]
errorOnNoOutput = 1
printHeader = \cm[predheadercsv{1}:set to 0 to suppress header line with feature names, default is 1, i.e. to show header line]

  ; TODO: output without a frame period does not work so that HTK (HList) can read it!
[predhtksink:cHtkSink]
reader.dmLevel=model_predictions
filename=\cm[predhtkoutput{?}:output HTK file for summarised parameters, set to a valid filename to enable this output sink, no append by default, use -appendstatichtk option to enable]
append=\cm[predappendhtk{0}:set to 1 to enable appending to an existing HTK parameter summary file, given by the htkoutput option]
parmKind = 9
  ; this is broken for HTK sink...
errorOnNoOutput = 0
 ; avoid broken files which HTK cannot read for period approx. > 0.06 (int16 overflow)
forcePeriod = 0.01

