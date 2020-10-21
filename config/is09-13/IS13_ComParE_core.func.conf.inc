///////////////////////////////////////////////////////////////////////////////////////
///////// > openSMILE configuration file for ComParE <               //////////////////
/////////                                                            //////////////////
///////// (c) 2014 by audEERING                                      //////////////////
/////////     All rights reserved. See file COPYING for details.     //////////////////
///////////////////////////////////////////////////////////////////////////////////////


;;;;;;;;; functionals / statistics


[componentInstances:cComponentManager]
instance[is13_functionalsA].type=cFunctionals
instance[is13_functionalsB].type=cFunctionals
instance[is13_functionalsF0].type=cFunctionals
instance[is13_functionalsNz].type=cFunctionals
; shared functionals for LLD
instance[is13_functionalsLLD].type=cFunctionals
; shared functionals for Delta LLD
instance[is13_functionalsDelta].type=cFunctionals


; functionals for energy related lld
[is13_functionalsA:cFunctionals]
reader.dmLevel = is13_lldA_smo;is13_lldA_smo_de
writer.dmLevel = is13_functionalsA
// nameAppend = 
copyInputName = 1
\{\cm[bufferModeRbConf]}
\{\cm[frameModeFunctionalsConf{../shared/FrameModeFunctionals.conf.inc}:path to included config to set frame mode for all functionals]}
functionalsEnabled = Extremes ; Percentiles ; Moments ; Segments ; Times ;  Lpc
Extremes.max = 0
Extremes.min = 0
Extremes.maxpos = 1
Extremes.minpos = 1
Extremes.maxameandist = 0
Extremes.minameandist = 0
Segments.maxNumSeg = 100
Segments.segmentationAlgorithm = relTh
Segments.thresholds = 0.25 ; 0.75
Segments.ravgLng = 3
Segments.numSegments = 0
Segments.meanSegLen = 1
Segments.maxSegLen = 1
Segments.minSegLen = 1
Segments.segLenStddev = 1
Segments.norm = second
Moments.doRatioLimit = 0
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
Times.upleveltime75 = 1
Times.downleveltime75 = 0
Times.upleveltime90 = 1
Times.downleveltime90 = 0
Times.risetime = 1
Times.falltime = 0
Times.leftctime = 1
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

; functionals for spectrum related lld
[is13_functionalsB:cFunctionals]
reader.dmLevel = is13_lldB_smo;is13_lldB_smo_de
writer.dmLevel = is13_functionalsB
// nameAppend = 
copyInputName = 1
\{\cm[bufferModeRbConf]}
\{\cm[frameModeFunctionalsConf]}
functionalsEnabled = Extremes ; Percentiles ; Moments ; Segments ; Times ;  Lpc
Extremes.max = 0
Extremes.min = 0
Extremes.maxpos = 1
Extremes.minpos = 1
Extremes.maxameandist = 0
Extremes.minameandist = 0
Segments.maxNumSeg = 100
Segments.segmentationAlgorithm = relTh
Segments.thresholds = 0.25 ; 0.75
Segments.rangeRelThreshold = 0.200000
Segments.numSegments = 0
Segments.meanSegLen = 1
Segments.maxSegLen = 1
Segments.minSegLen = 1
Segments.segLenStddev = 1
Segments.norm = second
Moments.doRatioLimit = 0
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
Times.upleveltime75 = 1
Times.downleveltime75 = 0
Times.upleveltime90 = 1
Times.downleveltime90 = 0
Times.risetime = 1
Times.falltime = 0
Times.leftctime = 1
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

; functionals for pitch onsets/offsets
[is13_functionalsF0:cFunctionals]
reader.dmLevel = is13_lld_f0_nzsmo
writer.dmLevel = is13_functionalsF0
//nameAppend = ff0
copyInputName = 1
\{\cm[bufferModeRbConf]}
\{\cm[frameModeFunctionalsConf]}
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
Segments.maxNumSeg = 100
Segments.segmentationAlgorithm = nonX
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
[is13_functionalsNz:cFunctionals]
reader.dmLevel = is13_lld_nzsmo;is13_lld_nzsmo_de
writer.dmLevel = is13_functionalsNz
// nameAppend = 
copyInputName = 1
\{\cm[bufferModeRbConf]}
\{\cm[frameModeFunctionalsConf]}
functionalsEnabled = Means ; Extremes ; Regression ; Percentiles ; Moments ; Times ; Lpc
Means.amean = 1
Means.posamean = 1
Means.absmean = 0
Means.qmean = 0
Means.rqmean = 1
Means.nzamean = 0
Means.nzabsmean = 0
Means.nzqmean = 0
Means.posrqmean = 0
Means.nzgmean = 0
Means.nnz = 0
Means.flatness = 1
Means.norm = frames
Extremes.max = 0
Extremes.min = 0
Extremes.maxpos = 1
Extremes.minpos = 1
Extremes.maxameandist = 0
Extremes.minameandist = 0
Moments.doRatioLimit = 0
Moments.variance = 0
Moments.stddev = 1
Moments.skewness = 1
Moments.kurtosis = 1
Moments.amean = 0
Regression.centroidUseAbsValues = 0
Regression.centroidRatioLimit = 0
Regression.doRatioLimit = 0
Regression.linregc1 = 1
Regression.linregc2 = 1
Regression.linregerrA = 0
Regression.linregerrQ = 1
Regression.qregc1 = 1
Regression.qregc2 = 1
Regression.qregc3 = 1
Regression.qregerrA = 0
Regression.qregerrQ = 1
Regression.oldBuggyQerr = 0
Regression.centroid = 1
Regression.normRegCoeff = 0
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
Times.upleveltime75 = 1
Times.downleveltime75 = 0
Times.upleveltime90 = 1
Times.downleveltime90 = 0
Times.risetime = 1
Times.falltime = 0
Times.leftctime = 1
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


[is13_functionalsLLD:cFunctionals]
reader.dmLevel = is13_lldA_smo;is13_lldB_smo
writer.dmLevel = is13_functionalsLLD
copyInputName = 1
\{\cm[bufferModeRbConf]}
\{\cm[frameModeFunctionalsConf]}
functionalsEnabled = Means ; Peaks2 ; Regression
Means.amean = 1
Means.posamean = 0
Means.absmean = 0
Means.qmean = 0
Means.rqmean = 1
Means.nzamean = 0
Means.nzabsmean = 0
Means.nzqmean = 0
Means.posrqmean = 0
Means.nzgmean = 0
Means.nnz = 0
Means.flatness = 1
Means.norm = frames
Regression.centroidUseAbsValues = 0
Regression.centroidRatioLimit = 0
Regression.doRatioLimit = 0
Regression.linregc1 = 1
Regression.linregc2 = 1
Regression.linregerrA = 0
Regression.linregerrQ = 1
Regression.qregc1 = 1
Regression.qregc2 = 1
Regression.qregc3 = 1
Regression.qregerrA = 0
Regression.qregerrQ = 1
Regression.oldBuggyQerr = 0
Regression.centroid = 1
Regression.normRegCoeff = 0
Peaks2.doRatioLimit = 0
Peaks2.numPeaks = 0
Peaks2.meanPeakDist = 1
Peaks2.meanPeakDistDelta = 0
Peaks2.peakDistStddev = 1
Peaks2.peakRangeAbs = 1
Peaks2.peakRangeRel = 1
Peaks2.peakMeanAbs = 1
Peaks2.peakMeanMeanDist = 1
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


[is13_functionalsDelta:cFunctionals]
reader.dmLevel = is13_lldA_smo_de;is13_lldB_smo_de
writer.dmLevel = is13_functionalsDelta
copyInputName = 1
\{\cm[bufferModeRbConf]}
\{\cm[frameModeFunctionalsConf]}
functionalsEnabled = Means ; Peaks2
Means.amean = 0
Means.posamean = 1
Means.absmean = 0
Means.qmean = 0
Means.rqmean = 1
Means.nzamean = 0
Means.nzabsmean = 0
Means.nzqmean = 0
Means.posrqmean = 0
Means.nzgmean = 0
Means.nnz = 0
Means.flatness = 1
Means.norm = frames
Peaks2.doRatioLimit = 0
Peaks2.numPeaks = 0
Peaks2.meanPeakDist = 1
Peaks2.meanPeakDistDelta = 0
Peaks2.peakDistStddev = 1
Peaks2.peakRangeAbs = 1
Peaks2.peakRangeRel = 1
Peaks2.peakMeanAbs = 1
Peaks2.peakMeanMeanDist = 1
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



