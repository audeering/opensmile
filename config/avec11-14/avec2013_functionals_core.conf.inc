///////////////////////////////////////////////////////////////////////////////////////
///////// > openSMILE configuration file                             //////////////////
/////////   for AVEC 2013                                            //////////////////
/////////   based on avec2011.conf                                   //////////////////
/////////  * written 2013 by Florian Eyben *                         //////////////////
/////////                                                            //////////////////
///////// (c) 2013-2016 by audEERING.                                //////////////////
/////////     All rights reserved. See file COPYING for details.     //////////////////
///////////////////////////////////////////////////////////////////////////////////////



;;;;;;;;; functionals / statistics


[componentInstances:cComponentManager]
instance[functionalsA].type=cFunctionals
instance[functionalsAde].type=cFunctionals
instance[functionalsF0v].type=cFunctionals
instance[functionalsF0p].type=cFunctionals
instance[functionalsNz].type=cFunctionals
instance[functionalsNzDe].type=cFunctionals
instance[functionalsDur].type=cFunctionals

; functionals for energy and spectral related lld
[functionalsA:cFunctionals]
reader.dmLevel = lldA_smo;lldB_smo
writer.dmLevel = functionalsA
// nameAppend = 
copyInputName = 1
\{\cm[frameModeConf{avec2013_functionals_frame_mode.conf.inc}:functionals frame mode config]}

functionalsEnabled = Percentiles ; Means ; Moments ; Peaks2 ; Segments ; Regression ; Times ; Lpc

Means.amean = 1
Means.absmean = 0
Means.rqmean = 1
Means.flatness = 1
Means.qmean = 0
Means.nzamean = 0
Means.nzabsmean = 0
Means.nzqmean = 0
Means.nzgmean = 0
Means.nnz = 0
Means.norm = frames

Peaks2.doRatioLimit = 0
Peaks2.numPeaks = 0
Peaks2.meanPeakDist = 1
Peaks2.meanPeakDistDelta = 0
Peaks2.peakDistStddev = 1
Peaks2.peakRangeAbs = 0
Peaks2.peakRangeRel = 1
Peaks2.peakMeanAbs = 0
Peaks2.peakMeanMeanDist = 0
Peaks2.peakMeanRel = 1
Peaks2.ptpAmpMeanAbs = 0
Peaks2.ptpAmpMeanRel = 0
Peaks2.ptpAmpStddevAbs = 0
Peaks2.ptpAmpStddevRel = 0
Peaks2.minRangeAbs = 0
Peaks2.minRangeRel = 1
Peaks2.minMeanAbs = 0
Peaks2.minMeanMeanDist = 0
Peaks2.minMeanRel = 0
Peaks2.mtmAmpMeanAbs = 0
Peaks2.mtmAmpMeanRel = 0
Peaks2.mtmAmpStddevAbs = 0
Peaks2.mtmAmpStddevRel = 0
Peaks2.meanRisingSlope = 1
Peaks2.maxRisingSlope = 0
Peaks2.minRisingSlope = 0
Peaks2.stddevRisingSlope = 1
Peaks2.meanFallingSlope = 1
Peaks2.maxFallingSlope = 0
Peaks2.minFallingSlope = 0
Peaks2.stddevFallingSlope = 1
Peaks2.norm = seconds
Peaks2.relThresh = 0.100000
Peaks2.dynRelThresh = 0
;Peaks2.posDbgOutp = minmax.txt
Peaks2.posDbgAppend = 0
Peaks2.consoleDbg = 0

Segments.maxNumSeg = 100
Segments.segmentationAlgorithm = NArelTh
Segments.thresholds = 0.25
Segments.rangeRelThreshold = 0.200000
Segments.numSegments = 0
Segments.meanSegLen = 1
Segments.maxSegLen = 1
Segments.minSegLen = 1
Segments.segLenStddev = 1
Segments.norm = second

Moments.variance = 0
Moments.stddev = 1
Moments.skewness = 1
Moments.kurtosis = 1
Moments.amean = 0
Moments.doRatioLimit = 0

Percentiles.quartiles = 1
Percentiles.iqr = 1
Percentiles.percentile[0] = 0.01
Percentiles.percentile[1] = 0.99
Percentiles.pctlrange[0] = 0-1
Percentiles.interp = 1

Regression.centroidUseAbsValues = 0
Regression.centroidRatioLimit = 0
Regression.doRatioLimit = 0
Regression.linregc1 = 1
Regression.linregc2 = 0
Regression.linregerrA = 1
Regression.linregerrQ = 0
Regression.qregc1 = 1
Regression.qregc2 = 0
Regression.qregc3 = 0
Regression.qregerrA = 1
Regression.qregerrQ = 0
Regression.oldBuggyQerr = 0
Regression.centroid = 0
Regression.normRegCoeff = 1
Regression.normInputs = 1

Times.upleveltime25 = 1
Times.downleveltime25 = 0
Times.upleveltime50 = 1
Times.downleveltime50 = 0
Times.upleveltime75 = 0
Times.downleveltime75 = 0
Times.upleveltime90 = 1
Times.downleveltime90 = 0
Times.risetime = 1
Times.falltime = 0
Times.leftctime = 0
Times.rightctime = 0
Times.duration = 0
Times.buggySecNorm = 0
Times.norm = segment

Lpc.lpGain = 1
Lpc.lpc = 1
Lpc.firstCoeff = 0
Lpc.order = 5

nonZeroFuncts = 0
masterTimeNorm = segment


; functionals for energy and spectral related lld (deltas)
[functionalsAde:cFunctionals]
reader.dmLevel = lldA_smo_de;lldB_smo_de
writer.dmLevel = functionalsAde
// nameAppend = 
copyInputName = 1
\{\cm[frameModeConf]}

functionalsEnabled = Percentiles ; Means ; Moments ; Times 
Means.amean = 0
Means.posamean = 1
Means.absmean = 0
Means.rqmean = 1
Means.flatness = 1
Means.qmean = 0
Means.nzamean = 0
Means.nzabsmean = 0
Means.nzqmean = 0
Means.nzgmean = 0
Means.nnz = 0
Means.norm = frames

Moments.variance = 0
Moments.stddev = 1
Moments.skewness = 1
Moments.kurtosis = 1
Moments.amean = 0
Moments.doRatioLimit = 0

Percentiles.quartiles = 1
Percentiles.iqr = 1
Percentiles.percentile[0] = 0.01
Percentiles.percentile[1] = 0.99
Percentiles.pctlrange[0] = 0-1
Percentiles.interp = 1

Times.upleveltime25 = 1
Times.downleveltime25 = 0
Times.upleveltime50 = 1
Times.downleveltime50 = 0
Times.upleveltime75 = 0
Times.downleveltime75 = 0
Times.upleveltime90 = 1
Times.downleveltime90 = 0
Times.risetime = 1
Times.falltime = 0
Times.leftctime = 0
Times.rightctime = 0
Times.duration = 0
Times.buggySecNorm = 0
Times.norm = segment

nonZeroFuncts = 0
masterTimeNorm = segment




; functionals for voiced segments and pitch onsets
[functionalsF0v:cFunctionals]
reader.dmLevel = lld_f0v_nzsmo
  ;lld_f0_nzsmo_de
writer.dmLevel = functionalsF0v
//nameAppend = ff0
copyInputName = 1
\{\cm[frameModeConf]}
functionalsEnabled = Means ; Segments
Means.amean = 0
Means.absmean = 0
Means.qmean = 0
Means.nzamean = 0
Means.nzabsmean = 0
Means.nzqmean = 0
Means.nzgmean = 0
Means.nnz = 1
Means.norm = segment
Segments.maxNumSeg = 999
Segments.segmentationAlgorithm = nonX
Segments.X = 0.0
Segments.numSegments = 0
Segments.meanSegLen = 1
Segments.maxSegLen = 1
Segments.minSegLen = 1
Segments.segLenStddev = 1
Segments.norm = second
Onset.threshold = 1
Onset.onsetRate = 1
Onset.norm = second
nonZeroFuncts = 0
masterTimeNorm = segment


; functionals for unvoiced segments
[functionalsF0p:cFunctionals]
reader.dmLevel = lld_f0p_nzsmo
writer.dmLevel = functionalsF0p
//nameAppend = pauses
copyInputName = 0
\{\cm[frameModeConf]}
frameCenterSpecial = left
functionalsEnabled = Segments
Segments.maxNumSeg = 999
Segments.segmentationAlgorithm = eqX
Segments.X = 0.0
Segments.numSegments = 0
Segments.meanSegLen = 1
Segments.maxSegLen = 1
Segments.minSegLen = 1
Segments.segLenStddev = 1
Segments.norm = second
nonZeroFuncts = 0
masterTimeNorm = segment



; functionals for pitch and vq related lld in voiced regions
[functionalsNz:cFunctionals]
reader.dmLevel = lld_nzsmo
writer.dmLevel = functionalsNz
// nameAppend = 
copyInputName = 1
\{\cm[frameModeConf]}
functionalsEnabled = Percentiles ; Means ; Moments ; Peaks2 ; Regression ; Times 
Means.amean = 1
Means.absmean = 0
Means.rqmean = 1
Means.qmean = 0
Means.flatness = 1
Means.nzamean = 0
Means.nzabsmean = 0
Means.nzqmean = 0
Means.nzgmean = 0
Means.nnz = 0
Means.norm = frames
Peaks2.doRatioLimit = 0
Peaks2.numPeaks = 0
Peaks2.meanPeakDist = 1
Peaks2.meanPeakDistDelta = 0
Peaks2.peakDistStddev = 1
Peaks2.peakRangeAbs = 0
Peaks2.peakRangeRel = 1
Peaks2.peakMeanAbs = 0
Peaks2.peakMeanMeanDist = 0
Peaks2.peakMeanRel = 1
Peaks2.ptpAmpMeanAbs = 0
Peaks2.ptpAmpMeanRel = 0
Peaks2.ptpAmpStddevAbs = 0
Peaks2.ptpAmpStddevRel = 0
Peaks2.minRangeAbs = 0
Peaks2.minRangeRel = 1
Peaks2.minMeanAbs = 0
Peaks2.minMeanMeanDist = 0
Peaks2.minMeanRel = 0
Peaks2.mtmAmpMeanAbs = 0
Peaks2.mtmAmpMeanRel = 0
Peaks2.mtmAmpStddevAbs = 0
Peaks2.mtmAmpStddevRel = 0
Peaks2.meanRisingSlope = 1
Peaks2.maxRisingSlope = 0
Peaks2.minRisingSlope = 0
Peaks2.stddevRisingSlope = 1
Peaks2.meanFallingSlope = 1
Peaks2.maxFallingSlope = 0
Peaks2.minFallingSlope = 0
Peaks2.stddevFallingSlope = 1
Peaks2.norm = seconds
Peaks2.relThresh = 0.100000
Peaks2.dynRelThresh = 0
;Peaks2.posDbgOutp = minmax.txt
Peaks2.posDbgAppend = 0
Peaks2.consoleDbg = 0
Moments.variance = 0
Moments.stddev = 1
Moments.skewness = 1
Moments.kurtosis = 1
Moments.amean = 0
Percentiles.quartiles = 1
Percentiles.iqr = 1
Percentiles.percentile[0] = 0.01
Percentiles.percentile[1] = 0.99
Percentiles.pctlrange[0] = 0-1
Percentiles.interp = 1
Regression.centroidUseAbsValues = 0
Regression.centroidRatioLimit = 0
Regression.doRatioLimit = 0
Regression.linregc1 = 1
Regression.linregc2 = 0
Regression.linregerrA = 1
Regression.linregerrQ = 0
Regression.qregc1 = 1
Regression.qregc2 = 0
Regression.qregc3 = 0
Regression.qregerrA = 1
Regression.qregerrQ = 0
Regression.oldBuggyQerr = 0
Regression.centroid = 0
Regression.normRegCoeff = 1
Regression.normInputs = 1
Times.upleveltime25 = 1
Times.downleveltime25 = 0
Times.upleveltime50 = 1
Times.downleveltime50 = 0
Times.upleveltime75 = 0
Times.downleveltime75 = 0
Times.upleveltime90 = 1
Times.downleveltime90 = 0
Times.risetime = 1
Times.falltime = 0
Times.leftctime = 0
Times.rightctime = 0
Times.duration = 0
Times.buggySecNorm = 0
Times.norm = segment
Lpc.lpGain = 1
Lpc.lpc = 1
Lpc.firstCoeff = 0
Lpc.order = 5
nonZeroFuncts = 1
masterTimeNorm = segment


; functionals for pitch and vq related lld in voiced regions (deltas)
[functionalsNzDe:cFunctionals]
reader.dmLevel = lld_nzsmo_de
writer.dmLevel = functionalsNzDe
// nameAppend = 
copyInputName = 1
\{\cm[frameModeConf]}
functionalsEnabled = Percentiles ; Means ; Moments ; Times 
Means.amean = 0
Means.posamean = 1
Means.absmean = 0
Means.rqmean = 1
Means.qmean = 0
Means.flatness = 1
Means.nzamean = 0
Means.nzabsmean = 0
Means.nzqmean = 0
Means.nzgmean = 0
Means.nnz = 0
Means.norm = frames
Moments.variance = 0
Moments.stddev = 1
Moments.skewness = 1
Moments.kurtosis = 1
Moments.amean = 0
Percentiles.quartiles = 1
Percentiles.iqr = 1
Percentiles.percentile[0] = 0.01
Percentiles.percentile[1] = 0.99
Percentiles.pctlrange[0] = 0-1
Percentiles.interp = 1
Times.upleveltime25 = 1
Times.downleveltime25 = 0
Times.upleveltime50 = 1
Times.downleveltime50 = 0
Times.upleveltime75 = 0
Times.downleveltime75 = 0
Times.upleveltime90 = 1
Times.downleveltime90 = 0
Times.risetime = 1
Times.falltime = 0
Times.leftctime = 0
Times.rightctime = 0
Times.duration = 0
Times.buggySecNorm = 0
Times.norm = segment
Lpc.lpGain = 1
Lpc.lpc = 1
Lpc.firstCoeff = 0
Lpc.order = 5
nonZeroFuncts = 1
masterTimeNorm = segment


; functionals for input segment duration
[functionalsDur:cFunctionals]
reader.dmLevel = lld_f0v_nzsmo
writer.dmLevel = functionalsDur
// nameAppend =
copyInputName = 1
\{\cm[frameModeConf]}
noPostEOIprocessing=0
functionalsEnabled = Times
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
Times.buggySecNorm = 0
Times.norm = second
nonZeroFuncts = 0
masterTimeNorm = second





