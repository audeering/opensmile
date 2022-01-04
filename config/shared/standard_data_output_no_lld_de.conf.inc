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

   It requires the main extractor configuration file to provide the following
   data memory levels:  lld and func

   It is identical to standard_data_output.conf.inc, apart from the fact that it
   does not expect an lld_de level.
 */

;;;;;;;;; output LLD features to CSV ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

[componentInstances:cComponentManager]
instance[lldsink].type=cCsvSink
instance[lldhtksink].type=cHtkSink
instance[lldarffsink].type=cArffSink
instance[arffsink].type=cArffSink
instance[csvsink].type=cCsvSink
instance[htksink].type=cHtkSink

[lldsink:cCsvSink]
reader.dmLevel = lld
filename=\cm[lldcsvoutput(D){?}:output csv file for LLD, disabled by default ?, only written if filename given]
instanceName=\cm[instname(N){unknown}:instance name]
append = \cm[appendcsvlld{0}:set to 1 to append to the LLD output csv file, default is not to append]
timestamp = \cm[timestampcsvlld{1}:set to 0 to suppress timestamp column, default is 1, i.e. to show timestamp in second column]
number = 0
printHeader = \cm[headercsvlld{1}:set to 0 to suppress header line with feature names, default is 1, i.e. to show header line]
errorOnNoOutput = 1

[lldhtksink:cHtkSink]
reader.dmLevel = lld
filename=\cm[lldhtkoutput{?}:output HTK binary file for LLD, disabled by default ?, only written if filename given]
append = \cm[appendhtklld{0}:set to 1 to append to the LLD output htk file, default is not to append]
  ; this is broken for HTK sink...
errorOnNoOutput = 0  
parmKind = 9

[lldarffsink:cArffSink]
reader.dmLevel = lld
frameIndex = 0
frameTime = \cm[timestamparfflld{1}:set to 0 to suppress timestamp column, default is 1, i.e. to show timestamp in second column]
filename=\cm[lldarffoutput{?}:name of WEKA Arff output file, set to a valid filename to enable this output sink]
relation=\cm[relation{openSMILE_features}:arff relation attribute, feature set name and/or corpus name]
instanceName=\cm[instname]
 ;; use this line instead of the above to always set the instance name to the
 ;; name of the input wave file
 ;instanceName=\cm[inputfile]
\{\cm[lldarfftargetsfile{arff_targets.conf.inc}:name of arff targets include file]}
append=\cm[appendarfflld{1}:set to 0 to disable appending to an existing arff parameter summary file, given by the arffoutput option]
errorOnNoOutput = 1

[arffsink:cArffSink]
reader.dmLevel=func
frameIndex = 0
frameTime = \cm[timestamparff{0}:set to 1 to include timestamp column, default is 0, i.e. to not save timestamp in second column]
frameTimeAdd=\cm[frameTimeAdd{0}:frame time offset -start of segment in seconds- used by avec2013]
filename=\cm[output(O){?}:name of WEKA Arff output file, set to a valid filename to enable this output sink]
relation=\cm[relation{openSMILE_features}:arff relation attribute, feature set name and/or corpus name]
instanceName=\cm[instname]
 ;; use this line instead of the above to always set the instance name to the
 ;; name of the input wave file
 ;instanceName=\cm[inputfile]
\{\cm[arfftargetsfile{arff_targets.conf.inc}:name of arff targets include file]}
append=\cm[appendarff{1}:set to 0 to disable appending to an existing arff parameter summary file, given by the arffoutput option]
errorOnNoOutput = 1

[csvsink:cCsvSink]
reader.dmLevel = func
filename=\cm[csvoutput{?}:output CSV file for summarised parameters, set to a valid filename to enable this output sink, data is appended if file exists]
append=\cm[appendcsv{1}:set to 0 to disable appending to an existing csv parameter summary file, given by the csvoutput option]
frameIndex=0
frameTime=\cm[timestampcsv{1}:set to 0 to suppress timestamp column, default is 1, i.e. to show timestamp in second column]
instanceName=\cm[instname]
errorOnNoOutput = 1
printHeader = \cm[headercsv{1}:set to 0 to suppress header line with feature names, default is 1, i.e. to show header line]

  ; TODO: output without a frame period does not work so that HTK (HList) can read it!
[htksink:cHtkSink]
reader.dmLevel = func
filename=\cm[htkoutput{?}:output HTK file for summarised parameters, set to a valid filename to enable this output sink, no append by default, use -appendstatichtk option to enable]
append=\cm[appendhtk{0}:set to 1 to enable appending to an existing HTK parameter summary file, given by the htkoutput option]
parmKind = 9
  ; this is broken for HTK sink...
errorOnNoOutput = 0
 ; avoid broken files which HTK cannot read for period approx. > 0.06 (int16 overflow)
forcePeriod = 0.01

