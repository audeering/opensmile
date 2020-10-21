
///////////////////////////////////////////////////////////////////////////////////////
///////// > openSMILE configuration file for IS09 emotion challenge< //////////////////
///////// > core features <                                          //////////////////
///////// (c) 2014 by audEERING                                      //////////////////
/////////     All rights reserved. See file COPYING for details.     //////////////////
///////////////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////////////
;
; This section is always required in openSMILE configuration files
;   it configures the componentManager and gives a list of all components which are to be loaded
; The order in which the components are listed should match 
;   the order of the data flow for most efficient processing
;
///////////////////////////////////////////////////////////////////////////////////////
[componentInstances:cComponentManager]
instance[is09_functL1].type=cFunctionals


[is09_functL1:cFunctionals]
reader.dmLevel=is09_lld;is09_lld_de
writer.dmLevel=is09_func
copyInputName = 1
\{\cm[bufferModeRbConf]}
\{\cm[frameModeFunctionalsConf{../shared/FrameModeFunctionals.conf.inc}:path to included config to set frame mode for all functionals]}
functionalsEnabled=Extremes;Regression;Moments
Extremes.max = 1
Extremes.min = 1
Extremes.range = 1
Extremes.maxpos = 1
Extremes.minpos = 1
Extremes.amean = 1
Extremes.maxameandist = 0
Extremes.minameandist = 0
 ; Note: the much better way to normalise the times of maxpos and minpos
 ; is 'turn', however for compatibility with old files the default 'frame'
 ; is kept here:
Extremes.norm = frame
Regression.linregc1 = 1
Regression.linregc2 = 1
Regression.linregerrA = 0
Regression.linregerrQ = 1
Regression.qregc1 = 0
Regression.qregc2 = 0
Regression.qregc3 = 0
Regression.qregerrA = 0
Regression.qregerrQ = 0
Regression.centroid = 0
Regression.oldBuggyQerr = 1
Regression.normInputs = 0
Regression.normRegCoeff = 0
Regression.centroidRatioLimit = 0
Regression.doRatioLimit = 0
Moments.doRatioLimit = 0
Moments.variance = 0
Moments.stddev = 1
Moments.skewness = 1
Moments.kurtosis = 1
Moments.amean = 0


