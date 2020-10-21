
///////////////////////////////////////////////////////////////////////////////////////
///////// > openSMILE configuration file for IS10 features <         //////////////////
///////// > core features : functionals <                            //////////////////
///////// (c) 2014 by audEERING                                      //////////////////
/////////     All rights reserved. See file COPYING for details.     //////////////////
///////////////////////////////////////////////////////////////////////////////////////


// NOTE: This file is not compatible with old versions of openSMILE (< 2.1)
// and will not give the same features (numerically) 
// as the original IS10 features.
// It should be preferred in new studies / designes, however, as it contains
// some minor fixes.
// For compatible features, use the IS10_paraling_compat.conf

[componentInstances:cComponentManager]
 ;;; functionals over FULL input (e.g. turns)
instance[is10_functOnsets].type=cFunctionals
instance[is10_functL1].type=cFunctionals
instance[is10_functL1nz].type=cFunctionals

[is10_functOnsets:cFunctionals]
reader.dmLevel=is10_pitchF
writer.dmLevel=is10_functOnsets
\{\cm[bufferModeRbConf]}
 ; frameSize and frameStep = 0 => functionals over complete input
 ; (NOTE: buffersize of lld and lld_de levels must be large enough!!)
\{\cm[frameModeFunctionalsConf{../shared/FrameModeFunctionals.conf.inc}:path to included config to set frame mode for all functionals]}
copyInputName=0
functNameAppend=Turn
functionalsEnabled=Onset;Times
//noPostEOIprocessing = 0
Onset.threshold = 0
;Onset.thresholdOnset = 0
;Onset.thresholdOffset = 0
Onset.useAbsVal = 0
Onset.onsetPos = 0
Onset.offsetPos = 0
Onset.numOnsets = 0
Onset.numOffsets = 0
Onset.onsetRate = 1
Onset.norm = segment
Times.upleveltime25 = 0
Times.downleveltime25 = 0
Times.upleveltime50 = 0
Times.downleveltime50 = 0
Times.upleveltime75 = 0
Times.downleveltime75 = 0
Times.upleveltime90 = 0
Times.downleveltime90 = 0
Times.risetime = 0
Times.falltime = 0
Times.leftctime = 0
Times.rightctime = 0
Times.duration = 1
Times.norm = second

// statistical functionals
[is10_functL1:cFunctionals]
reader.dmLevel=is10_lld1;is10_lld1_de
writer.dmLevel=is10_funct
\{\cm[bufferModeRbConf]}
 ; frameSize and frameStep = 0 => functionals over complete input
 ; (NOTE: buffersize of lld and lld_de levels must be large enough!!)
\{\cm[frameModeFunctionalsConf]}
functionalsEnabled=Extremes;Regression;Moments;Percentiles;Times
Extremes.max = 0
Extremes.min = 0
Extremes.range = 0
Extremes.maxpos = 1
Extremes.minpos = 1
Extremes.amean = 1
Extremes.maxameandist=0
Extremes.minameandist=0
Extremes.norm = frame
Regression.linregc1 = 1
Regression.linregc2 = 1
Regression.linregerrA = 1
Regression.linregerrQ = 1
Regression.qregc1 = 0
Regression.qregc2 = 0
Regression.qregc3 = 0
Regression.qregerrA = 0
Regression.qregerrQ = 0
Regression.centroid = 0
Regression.oldBuggyQerr = 0
Regression.normInputs = 0
Regression.normRegCoeff = 0
Moments.variance = 0
Moments.stddev = 1
Moments.skewness = 1
Moments.kurtosis = 1
Moments.amean = 0
Percentiles.quartiles = 1
Percentiles.quartile1 = 0
Percentiles.quartile2 = 0
Percentiles.quartile3 = 0
Percentiles.iqr = 1
Percentiles.iqr12 = 0
Percentiles.iqr23 = 0
Percentiles.iqr13 = 0
Percentiles.interp = 1
Percentiles.percentile = 0.01;0.99
Percentiles.pctlrange=0-1
Times.upleveltime25 = 0
Times.downleveltime25 = 0
Times.upleveltime50 = 0
Times.downleveltime50 = 0
Times.upleveltime75 = 1
Times.downleveltime75 = 0
Times.upleveltime90 = 1
Times.downleveltime90 = 0
Times.risetime = 0
Times.falltime = 0
Times.leftctime = 0
Times.rightctime = 0
Times.duration = 0
Times.norm = segment
nonZeroFuncts = 0

// statistical functionals
[is10_functL1nz:cFunctionals]
reader.dmLevel=is10_lld2;is10_lld2_de
writer.dmLevel=is10_functNz
\{\cm[bufferModeRbConf]}
 ; frameSize and frameStep = 0 => functionals over complete input
 ; (NOTE: buffersize of lld and lld_de levels must be large enough!!)
\{\cm[frameModeFunctionalsConf]}
functionalsEnabled=Extremes;Regression;Moments;Percentiles;Times
Extremes.max = 0
Extremes.min = 0
Extremes.range = 0
Extremes.maxpos = 1
Extremes.minpos = 1
Extremes.amean = 1
Extremes.maxameandist=0
Extremes.minameandist=0
Extremes.norm = frame
Regression.linregc1 = 1
Regression.linregc2 = 1
Regression.linregerrA = 1
Regression.linregerrQ = 1
Regression.qregc1 = 0
Regression.qregc2 = 0
Regression.qregc3 = 0
Regression.qregerrA = 0
Regression.qregerrQ = 0
Regression.centroid = 0
Regression.oldBuggyQerr = 0
Regression.normInputs = 0
Regression.normRegCoeff = 0
Moments.variance = 0
Moments.stddev = 1
Moments.skewness = 1
Moments.kurtosis = 1
Moments.amean = 0
Percentiles.quartiles = 1
Percentiles.quartile1 = 0
Percentiles.quartile2 = 0
Percentiles.quartile3 = 0
Percentiles.iqr = 1
Percentiles.iqr12 = 0
Percentiles.iqr23 = 0
Percentiles.iqr13 = 0
Percentiles.interp = 1
Percentiles.percentile = 0.99
;Percentiles.pctlrange=0-1
Times.upleveltime25 = 0
Times.downleveltime25 = 0
Times.upleveltime50 = 0
Times.downleveltime50 = 0
Times.upleveltime75 = 1
Times.downleveltime75 = 0
Times.upleveltime90 = 1
Times.downleveltime90 = 0
Times.risetime = 0
Times.falltime = 0
Times.leftctime = 0
Times.rightctime = 0
Times.duration = 0
Times.norm = segment
nonZeroFuncts=1


//////---------------------- END -------------------------///////
