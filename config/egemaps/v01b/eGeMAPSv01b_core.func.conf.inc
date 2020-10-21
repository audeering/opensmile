///////////////////////////////////////////////////////////////////////////////////////
///////// > openSMILE configuration file, Geneva feature set <       //////////////////
/////////                                                            //////////////////
///////// (c) 2014, 2020 by audEERING                                //////////////////
/////////     All rights reserved. See file COPYING for details.     //////////////////
///////////////////////////////////////////////////////////////////////////////////////


[componentInstances:cComponentManager]
instance[egemapsv01b_leqLin].type= cFunctionals
instance[egemapsv01b_leq].type = cVectorOperation

;; LEq
[egemapsv01b_leqLin:cFunctionals]
reader.dmLevel = egemapsv01b_energyRMS
writer.dmLevel = egemapsv01b_leqLin
// nameAppend =
preserveFields = 1
\{\cm[bufferModeRbConf]}
copyInputName = 1
\{\cm[frameModeFunctionalsConf]}
noPostEOIprocessing = 0
functionalsEnabled = Means
Means.amean = 1
Means.posamean = 0
Means.absmean = 0
Means.qmean = 0
Means.nzamean = 0
Means.nzabsmean = 0
Means.nzqmean = 0
Means.nzgmean = 0
Means.nnz = 0

[egemapsv01b_leq:cVectorOperation]
reader.dmLevel = egemapsv01b_leqLin
writer.dmLevel = egemapsv01b_leq
nameBase = equivalentSoundLevel
copyInputName = 1
processArrayFields = 0
operation = dBp
appendOperationToName = 1



;;;;;;;;;;;;;;;;;;;;; functionals / summaries ;;;;;;;;;;;;;;;

[componentInstances:cComponentManager]
instance[egemapsv01b_functionalsMVR].type=cFunctionals
instance[egemapsv01b_functionalsMeanUV].type=cFunctionals
instance[egemapsv01b_functionalsMVRVoiced].type = cFunctionals

[egemapsv01b_functionalsMVR:cFunctionals]
reader.dmLevel = egemapsv01b_lldSetNoF0AndLoudnessZ_smo
writer.dmLevel = egemapsv01b_functionalsMeanStddevZ
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
nonZeroFuncts = 0
masterTimeNorm = segment

[egemapsv01b_functionalsMeanUV:cFunctionals]
reader.dmLevel = egemapsv01b_lldSetSpectralZ_smo
writer.dmLevel = egemapsv01b_functionalsMeanUnvoiced
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
nonZeroFuncts = 1
masterTimeNorm = segment

[egemapsv01b_functionalsMVRVoiced:cFunctionals]
reader.dmLevel = egemapsv01b_lldSetNoF0AndLoudnessNz_smo;egemapsv01b_lldSetSpectralNz_smo
writer.dmLevel = egemapsv01b_functionalsMeanStddevVoiced
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
nonZeroFuncts = 1
masterTimeNorm = segment


