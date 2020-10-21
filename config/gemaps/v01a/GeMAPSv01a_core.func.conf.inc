///////////////////////////////////////////////////////////////////////////////////////
///////// > openSMILE configuration file, Geneva feature set <       //////////////////
/////////                                                            //////////////////
///////// (c) 2014 by audEERING                                      //////////////////
/////////     All rights reserved. See file COPYING for details.     //////////////////
///////////////////////////////////////////////////////////////////////////////////////

;;;;;;;;;;;;;;;; temporal statistics: ;;;;;;;;;;;;;;;;;;;;;;

[componentInstances:cComponentManager]
instance[gemapsv01a_smoF0].type=cContourSmoother
instance[gemapsv01a_smoLoudness].type=cContourSmoother
instance[gemapsv01a_temporalF0].type=cFunctionals
instance[gemapsv01a_temporalF0p].type=cFunctionals
instance[gemapsv01a_temporalLoudness].type=cFunctionals
instance[gemapsv01a_temporalSetNames].type=cDataSelector

;;smoothing ;;
[gemapsv01a_smoF0:cContourSmoother]
reader.dmLevel = gemapsv01a_lld_single_logF0
writer.dmLevel = gemapsv01a_lld_single_logF0_smo
\{\cm[bufferModeConf]}
copyInputName = 1
nameAppend = sma3nz
noPostEOIprocessing = 0
smaWin = 3
noZeroSma = 1

[gemapsv01a_smoLoudness:cContourSmoother]
reader.dmLevel = gemapsv01a_loudness
writer.dmLevel = gemapsv01a_loudness_smo
\{\cm[bufferModeConf]}
nameAppend = sma3
copyInputName = 1
noPostEOIprocessing = 0
smaWin = 3
noZeroSma = 0

; functionals for pitch onsets/offsets
[gemapsv01a_temporalF0:cFunctionals]
reader.dmLevel = gemapsv01a_lld_single_logF0_smo
writer.dmLevel = gemapsv01a_temporalF0
\{\cm[bufferModeRbConf]}
//nameAppend = ff0
copyInputName = 1
\{\cm[frameModeFunctionalsConf{../../shared/FrameModeFunctionals.conf.inc}:path to included config to set frame mode for all functionals]}
functionalsEnabled = Segments 
Segments.maxNumSeg = 1000
Segments.segmentationAlgorithm = nonX
Segments.X = 0.0
Segments.numSegments = 1
Segments.meanSegLen = 1
Segments.maxSegLen = 0
Segments.minSegLen = 0
Segments.segLenStddev = 1
Segments.norm = second
nonZeroFuncts = 0
masterTimeNorm = second

// TODO: this is only unvoiced segments, combine with energy / loudness for true pauses!
; functionals for pitch onsets/offsets
[gemapsv01a_temporalF0p:cFunctionals]
reader.dmLevel = gemapsv01a_lld_single_logF0_smo
writer.dmLevel = gemapsv01a_temporalF0pause
\{\cm[bufferModeRbConf]}
copyInputName = 0
functNameAppend = f0pause
\{\cm[frameModeFunctionalsConf]}
functionalsEnabled = Segments 
Segments.maxNumSeg = 1000
Segments.segmentationAlgorithm = eqX
Segments.X = 0.0
Segments.numSegments = 0
Segments.meanSegLen = 1
Segments.maxSegLen = 0
Segments.minSegLen = 0
Segments.segLenStddev = 1
Segments.norm = second
nonZeroFuncts = 0
masterTimeNorm = second

; functionals for pitch onsets/offsets
[gemapsv01a_temporalLoudness:cFunctionals]
reader.dmLevel = gemapsv01a_loudness_smo
writer.dmLevel = gemapsv01a_temporalLoudness
\{\cm[bufferModeRbConf]}
//nameAppend = ff0
copyInputName = 1
\{\cm[frameModeFunctionalsConf]}
functionalsEnabled = Peaks2 
Peaks2.numPeaks = 1
Peaks2.meanPeakDist = 0
Peaks2.meanPeakDistDelta = 0
Peaks2.peakDistStddev = 0
Peaks2.peakRangeAbs = 0
Peaks2.peakRangeRel = 0
Peaks2.peakMeanAbs = 0
Peaks2.peakMeanMeanDist = 0
Peaks2.peakMeanRel = 0
Peaks2.ptpAmpMeanAbs = 0
Peaks2.ptpAmpMeanRel = 0
Peaks2.ptpAmpStddevAbs = 0
Peaks2.ptpAmpStddevRel = 0
Peaks2.minRangeAbs = 0
Peaks2.minRangeRel = 0
Peaks2.minMeanAbs = 0
Peaks2.minMeanMeanDist = 0
Peaks2.minMeanRel = 0
Peaks2.mtmAmpMeanAbs = 0
Peaks2.mtmAmpMeanRel = 0
Peaks2.mtmAmpStddevAbs = 0
Peaks2.mtmAmpStddevRel = 0
Peaks2.meanRisingSlope = 0
Peaks2.maxRisingSlope = 0
Peaks2.minRisingSlope = 0
Peaks2.stddevRisingSlope = 0
Peaks2.meanFallingSlope = 0
Peaks2.maxFallingSlope = 0
Peaks2.minFallingSlope = 0
Peaks2.stddevFallingSlope = 0
Peaks2.norm = seconds
Peaks2.relThresh = 0.100000
Peaks2.dynRelThresh = 0
;Peaks2.posDbgOutp = minmax.txt
Peaks2.posDbgAppend = 0
Peaks2.consoleDbg = 0
nonZeroFuncts = 0
masterTimeNorm = second

;;;;;;;;;;;;;;;;;; filtering and renaming of names ;;;;;;;;;;;

[gemapsv01a_temporalSetNames:cDataSelector]
reader.dmLevel = gemapsv01a_temporalLoudness;gemapsv01a_temporalF0;gemapsv01a_temporalF0pause
writer.dmLevel = gemapsv01a_temporalSet
\{\cm[bufferModeRbConf]}
selected = loudness_sma3_numPeaks;F0semitoneFrom27.5Hz_sma3nz_numSegments;F0semitoneFrom27.5Hz_sma3nz_meanSegLen;F0semitoneFrom27.5Hz_sma3nz_segLenStddev;F0semitoneFrom27.5Hz_sma3nz__f0pause_meanSegLen;F0semitoneFrom27.5Hz_sma3nz__f0pause_segLenStddev
newNames = loudnessPeaksPerSec;VoicedSegmentsPerSec;MeanVoicedSegmentLengthSec;StddevVoicedSegmentLengthSec;MeanUnvoicedSegmentLength;StddevUnvoicedSegmentLength

;;;;;;;;;;;;;;;;;;;;; functionals / summaries ;;;;;;;;;;;;;;;

[componentInstances:cComponentManager]
instance[gemapsv01a_functionalsF0].type=cFunctionals
instance[gemapsv01a_functionalsLoudness].type=cFunctionals
instance[gemapsv01a_functionalsMeanSpectralUV].type=cFunctionals
instance[gemapsv01a_functionalsMVRVoiced].type=cFunctionals

[gemapsv01a_functionalsF0:cFunctionals]
reader.dmLevel = gemapsv01a_lld_single_logF0_smo
writer.dmLevel = gemapsv01a_functionalsF0
\{\cm[bufferModeRbConf]}
copyInputName = 1
\{\cm[frameModeFunctionalsConf]}
functionalsEnabled = Moments ; Percentiles ; Peaks2
Moments.variance = 0
Moments.stddev = 0
Moments.stddevNorm = 2
Moments.skewness = 0
Moments.kurtosis = 0
Moments.amean = 1
Moments.doRatioLimit = 0
Percentiles.quartiles = 0
Percentiles.iqr = 0
Percentiles.percentile[0] = 0.20
Percentiles.percentile[1] = 0.50
Percentiles.percentile[2] = 0.80
Percentiles.pctlrange[0] = 0-2
Percentiles.interp = 1
Peaks2.doRatioLimit = 0
Peaks2.numPeaks = 0
Peaks2.meanPeakDist = 0
Peaks2.meanPeakDistDelta = 0
Peaks2.peakDistStddev = 0
Peaks2.peakRangeAbs = 0
Peaks2.peakRangeRel = 0
Peaks2.peakMeanAbs = 0
Peaks2.peakMeanMeanDist = 0
Peaks2.peakMeanRel = 0
Peaks2.ptpAmpMeanAbs = 0
Peaks2.ptpAmpMeanRel = 0
Peaks2.ptpAmpStddevAbs = 0
Peaks2.ptpAmpStddevRel = 0
Peaks2.minRangeAbs = 0
Peaks2.minRangeRel = 0
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
nonZeroFuncts = 1
masterTimeNorm = segment

[gemapsv01a_functionalsLoudness:cFunctionals]
reader.dmLevel = gemapsv01a_loudness_smo
writer.dmLevel = gemapsv01a_functionalsLoudness
\{\cm[bufferModeRbConf]}
copyInputName = 1
\{\cm[frameModeFunctionalsConf]}
functionalsEnabled = Moments ; Percentiles ; Peaks2
Moments.variance = 0
Moments.stddev = 0
Moments.stddevNorm = 2
Moments.skewness = 0
Moments.kurtosis = 0
Moments.amean = 1
Moments.doRatioLimit = 0
Percentiles.quartiles = 0
Percentiles.iqr = 0
Percentiles.percentile[0] = 0.20
Percentiles.percentile[1] = 0.50
Percentiles.percentile[2] = 0.80
Percentiles.pctlrange[0] = 0-2
Percentiles.interp = 1
Peaks2.doRatioLimit = 0
Peaks2.numPeaks = 0
Peaks2.meanPeakDist = 0
Peaks2.meanPeakDistDelta = 0
Peaks2.peakDistStddev = 0
Peaks2.peakRangeAbs = 0
Peaks2.peakRangeRel = 0
Peaks2.peakMeanAbs = 0
Peaks2.peakMeanMeanDist = 0
Peaks2.peakMeanRel = 0
Peaks2.ptpAmpMeanAbs = 0
Peaks2.ptpAmpMeanRel = 0
Peaks2.ptpAmpStddevAbs = 0
Peaks2.ptpAmpStddevRel = 0
Peaks2.minRangeAbs = 0
Peaks2.minRangeRel = 0
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
nonZeroFuncts = 0
masterTimeNorm = segment

[gemapsv01a_functionalsMeanSpectralUV:cFunctionals]
reader.dmLevel = gemapsv01a_lldSetSpectralZ_smo
writer.dmLevel = gemapsv01a_functionalsMeanSpectralUnvoiced
\{\cm[bufferModeRbConf]}
copyInputName = 1
\{\cm[frameModeFunctionalsConf]}
functionalsEnabled = Moments
Moments.variance = 0
Moments.stddev = 0
Moments.stddevNorm = 0
Moments.skewness = 0
Moments.kurtosis = 0
Moments.amean = 1
Moments.doRatioLimit = 0
nonZeroFuncts = 1
masterTimeNorm = segment

[gemapsv01a_functionalsMVRVoiced:cFunctionals]
reader.dmLevel = gemapsv01a_lldSetNoF0AndLoudnessNz_smo;gemapsv01a_lldSetSpectralNz_smo;
writer.dmLevel = gemapsv01a_functionalsMeanStddevVoiced
\{\cm[bufferModeRbConf]}
copyInputName = 1
\{\cm[frameModeFunctionalsConf]}
functionalsEnabled = Moments
Moments.variance = 0
Moments.stddev = 0
Moments.stddevNorm = 2
Moments.skewness = 0
Moments.kurtosis = 0
Moments.amean = 1
Moments.doRatioLimit = 0
nonZeroFuncts = 1
masterTimeNorm = segment



