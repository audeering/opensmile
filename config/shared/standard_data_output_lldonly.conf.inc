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
   data memory levels:  lld  (including, if available lld_de)
 */

;;;;;;;;; output LLD features to CSV ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

[componentInstances:cComponentManager]
instance[lldcsvsink].type=cCsvSink
instance[lldhtksink].type=cHtkSink
instance[lldarffsink].type=cArffSink

[lldcsvsink:cCsvSink]
reader.dmLevel = lld
filename=\cm[csvoutput{?}:output csv file for LLD, disabled by default ?, only written if filename given]
instanceName=\cm[instname(N){unknown}:instance name]
append = \cm[appendcsv{0}:set to 1 to append to the LLD output csv file, default is not to append]
timestamp = \cm[timestampcsv{1}:set to 0 to suppress timestamp column, default is 1, i.e. to show timestamp in second column] 
number = 0
printHeader = \cm[headercsv{1}:set to 0 to suppress header line with feature names, default is 1, i.e. to show header line]
errorOnNoOutput = 1

[lldhtksink:cHtkSink]
reader.dmLevel = lld
filename=\cm[output(O){output.htk}:output HTK binary file for LLD. Use ? as value to disable]
append = \cm[appendhtk{0}:set to 1 to append to the LLD output htk file, default is not to append]
; this is broken for HTK sink...:
errorOnNoOutput = 0  
parmKind = 9

[lldarffsink:cArffSink]
reader.dmLevel = lld
frameIndex = 0
frameTime = \cm[timestamparff{1}:set to 0 to suppress timestamp column, default is 1, i.e. to show timestamp in second column] 
filename=\cm[arffoutput{?}:name of WEKA Arff output file, set to a valid filename to enable this output sink]
relation=\cm[relation{openSMILE_features}:arff relation attribute, feature set name and/or corpus name]
instanceName=\cm[instname]
 ;; use this line instead of the above to always set the instance name to the
 ;; name of the input wave file
 ;instanceName=\cm[inputfile]
\{\cm[arfftargetsfile{arff_targets.conf.inc}:name of arff targets include file]}
append=\cm[appendarff{1}:set to 0 to disable appending to an existing arff parameter summary file, given by the arffoutput option]
errorOnNoOutput = 1


