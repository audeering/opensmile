///////////////////////////////////////////////////////////////////////////////////////
///////// > openSMILE configuration file for ComParE <               //////////////////
/////////                                                            //////////////////
///////// (c) 2014 by audEERING                                      //////////////////
/////////     All rights reserved. See file COPYING for details.     //////////////////
///////////////////////////////////////////////////////////////////////////////////////



[componentInstances:cComponentManager]
instance[is13_frame60].type=cFramer
instance[is13_win60].type=cWindower
instance[is13_fft60].type=cTransformFFT
instance[is13_fftmp60].type=cFFTmagphase

[is13_frame60:cFramer]
reader.dmLevel=wave
writer.dmLevel=is13_frame60
\{\cm[bufferModeRbConf{../shared/BufferModeRb.conf.inc}:path to included config to set the buffer mode for the standard ringbuffer levels]}
frameSize = 0.060
frameStep = 0.010
frameCenterSpecial = left

[is13_win60:cWindower]
reader.dmLevel=is13_frame60
writer.dmLevel=is13_winG60
winFunc=gauss
gain=1.0
sigma=0.4

[is13_fft60:cTransformFFT]
reader.dmLevel=is13_winG60
writer.dmLevel=is13_fftcG60
 ; for compatibility with 2.2.0 and older versions
zeroPadSymmetric = 0

[is13_fftmp60:cFFTmagphase]
reader.dmLevel=is13_fftcG60
writer.dmLevel=is13_fftmagG60


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

[componentInstances:cComponentManager]
instance[is13_frame25].type=cFramer
instance[is13_win25].type=cWindower
instance[is13_fft25].type=cTransformFFT
instance[is13_fftmp25].type=cFFTmagphase

[is13_frame25:cFramer]
reader.dmLevel=wave
writer.dmLevel=is13_frame25
\{\cm[bufferModeRbConf]}
frameSize = 0.020
frameStep = 0.010
frameCenterSpecial = left

[is13_win25:cWindower]
reader.dmLevel=is13_frame25
writer.dmLevel=is13_winH25
winFunc=hamming

[is13_fft25:cTransformFFT]
reader.dmLevel=is13_winH25
writer.dmLevel=is13_fftcH25
 ; for compatibility with 2.2.0 and older versions
zeroPadSymmetric = 0

[is13_fftmp25:cFFTmagphase]
reader.dmLevel=is13_fftcH25
writer.dmLevel=is13_fftmagH25



;;;;;;;;;;;;;;;;;;;; HPS pitch

[componentInstances:cComponentManager]
instance[is13_scale].type=cSpecScale
instance[is13_shs].type=cPitchShs

[is13_scale:cSpecScale]
reader.dmLevel=is13_fftmagG60
writer.dmLevel=is13_hpsG60
copyInputName = 1
processArrayFields = 0
scale=octave
sourceScale = lin
interpMethod = spline
minF = 25
maxF = -1
nPointsTarget = 0
specSmooth = 1
specEnhance = 1
auditoryWeighting = 1 

[is13_shs:cPitchShs]
reader.dmLevel=is13_hpsG60
writer.dmLevel=is13_pitchShsG60
\{\cm[bufferModeRbLagConf{../shared/BufferModeRbLag.conf.inc}:path to included config to set the buffer mode for levels which will be joint with Viterbi smoothed -lagged- F0]}
copyInputName = 1
processArrayFields = 0
maxPitch = 620
minPitch = 52
nCandidates = 6
scores = 1
voicing = 1
F0C1 = 0
voicingC1 = 0
F0raw = 1
voicingClip = 1
voicingCutoff = 0.700000
inputFieldSearch = Mag_octScale
octaveCorrection = 0
nHarmonics = 15
compressionFactor = 0.850000
greedyPeakAlgo = 1

;;;;; Pitch with Viterbi smoother
[componentInstances:cComponentManager]
instance[is13_energy60].type=cEnergy

[is13_energy60:cEnergy]
reader.dmLevel=is13_winG60
writer.dmLevel=is13_e60
 ; This must be > than buffersize of viterbi smoother
\{\cm[bufferModeRbLagConf]}
rms=1
log=0

[componentInstances:cComponentManager]
instance[is13_pitchSmoothViterbi].type=cPitchSmootherViterbi

[is13_pitchSmoothViterbi:cPitchSmootherViterbi]
reader.dmLevel=is13_pitchShsG60
reader2.dmLevel=is13_pitchShsG60
writer.dmLevel=is13_pitchG60_viterbi
\{\cm[bufferModeRbLagConf]}
copyInputName = 1
bufferLength=30
F0final = 1
F0finalEnv = 0
voicingFinalClipped = 0
voicingFinalUnclipped = 1
F0raw = 0
voicingC1 = 0
voicingClip = 0
wTvv =10.0
wTvvd= 5.0
wTvuv=10.0
wThr = 4.0
wTuu = 0.0
wLocal=2.0
wRange=1.0

[componentInstances:cComponentManager]
instance[is13_volmerge].type = cValbasedSelector

[is13_volmerge:cValbasedSelector]
reader.dmLevel = is13_e60;is13_pitchG60_viterbi
writer.dmLevel = is13_pitchG60
\{\cm[bufferModeRbLagConf]}
idx=0
threshold=0.001
removeIdx=1
zeroVec=1
outputVal=0.0

;;;;;;;;;;;;;;;;;;; Voice Quality (VQ)

[componentInstances:cComponentManager]
instance[is13_pitchJitter].type=cPitchJitter

[is13_pitchJitter:cPitchJitter]
reader.dmLevel = wave
writer.dmLevel = is13_jitterShimmer
\{\cm[bufferModeRbLagConf]}
copyInputName = 1
F0reader.dmLevel = is13_pitchG60
F0field = F0final
searchRangeRel = 0.250000
jitterLocal = 1
jitterDDP = 1
jitterLocalEnv = 0
jitterDDPEnv = 0
shimmerLocal = 1
shimmerLocalEnv = 0
onlyVoiced = 0
logHNR = 1
inputMaxDelaySec = 2.0
;periodLengths = 0
;periodStarts = 0
useBrokenJitterThresh = 1

;;;;;;;;;;;;;;;;;;;;; Energy / loudness


[componentInstances:cComponentManager]
instance[is13_energy].type=cEnergy
instance[is13_melspec1].type=cMelspec
instance[is13_audspec].type=cPlp
instance[is13_audspecRasta].type=cPlp
instance[is13_audspecSum].type=cVectorOperation
instance[is13_audspecRastaSum].type=cVectorOperation

[is13_energy:cEnergy]
reader.dmLevel = is13_frame25
writer.dmLevel = is13_energy
log=0
rms=1
; Enable this only for if quadratic energy is needed, otherwise it breaks ComParE feature set compatibility!!
;  energy2=1

[is13_melspec1:cMelspec]
reader.dmLevel=is13_fftmagH25
writer.dmLevel=is13_melspec1
; htk compatible sample value scaling
htkcompatible = 0
nBands = 26
; use power spectrum instead of magnitude spectrum
usePower = 1
lofreq = 20
hifreq = 8000
specScale = mel
showFbank = 0

; perform auditory weighting of spectrum
[is13_audspec:cPlp]
reader.dmLevel=is13_melspec1
writer.dmLevel=is13_audspec
firstCC = 0
lpOrder = 5
cepLifter = 22
compression = 0.33
htkcompatible = 0 
doIDFT = 0
doLpToCeps = 0
doLP = 0
doInvLog = 0
doAud = 1
doLog = 0
newRASTA=0
RASTA=0

; perform RASTA style filtering of auditory spectra
[is13_audspecRasta:cPlp]
reader.dmLevel=is13_melspec1
writer.dmLevel=is13_audspecRasta
nameAppend = Rfilt
firstCC = 0
lpOrder = 5
cepLifter = 22
compression = 0.33
htkcompatible = 0 
doIDFT = 0
doLpToCeps = 0
doLP = 0
doInvLog = 0
doAud = 1
doLog = 0
newRASTA=1
RASTA=0

[is13_audspecSum:cVectorOperation]
reader.dmLevel = is13_audspec
writer.dmLevel = is13_audspecSum
// nameAppend = 
copyInputName = 1
processArrayFields = 0
operation = ll1
nameBase = audspec

[is13_audspecRastaSum:cVectorOperation]
reader.dmLevel = is13_audspecRasta
writer.dmLevel = is13_audspecRastaSum
// nameAppend = 
copyInputName = 1
processArrayFields = 0
operation = ll1
nameBase = audspecRasta

;;;;;;;;;;;;;;; spectral

[componentInstances:cComponentManager]
instance[is13_spectral].type=cSpectral


[is13_spectral:cSpectral]
reader.dmLevel=is13_fftmagH25
writer.dmLevel=is13_spectral
bands[0]=250-650
bands[1]=1000-4000
rollOff[0] = 0.25
rollOff[1] = 0.50
rollOff[2] = 0.75
rollOff[3] = 0.90
flux=1
centroid=1
maxPos=0
minPos=0
entropy=1
variance=1
skewness=1
kurtosis=1
slope=1
harmonicity=1
sharpness=1


;;;;;;;;;;;;;;; mfcc

[componentInstances:cComponentManager]
instance[is13_melspecMfcc].type=cMelspec
instance[is13_mfcc].type=cMfcc

[is13_melspecMfcc:cMelspec]
reader.dmLevel=is13_fftmagH25
writer.dmLevel=is13_melspecMfcc
copyInputName = 1
processArrayFields = 1
; htk compatible sample value scaling
htkcompatible = 1
nBands = 26
; use power spectrum instead of magnitude spectrum
usePower = 1
lofreq = 20
hifreq = 8000
specScale = mel
inverse = 0

[is13_mfcc:cMfcc]
reader.dmLevel=is13_melspecMfcc
writer.dmLevel=is13_mfcc1_12
copyInputName = 0
processArrayFields = 1
firstMfcc = 1
lastMfcc  = 14
cepLifter = 22.0
htkcompatible = 1


;;;;;;;;;;;;;;;;  zcr

[componentInstances:cComponentManager]
instance[is13_mzcr].type=cMZcr

[is13_mzcr:cMZcr]
reader.dmLevel = is13_frame60
writer.dmLevel = is13_zcr
copyInputName = 1
processArrayFields = 1
zcr = 1
mcr = 0
amax = 0
maxmin = 0
dc = 0


;;;;;;;;;;;;;;;;;;;; smoothing

[componentInstances:cComponentManager]
instance[is13_smoNz].type=cContourSmoother
instance[is13_smoA].type=cContourSmoother
instance[is13_smoB].type=cContourSmoother
instance[is13_f0sel].type=cDataSelector

[is13_smoNz:cContourSmoother]
reader.dmLevel = is13_pitchG60;is13_jitterShimmer
writer.dmLevel = is13_lld_nzsmo
\{\cm[bufferModeConf{../shared/BufferMode.conf.inc}:path to included config to set the buffer mode for the levels before the functionals]}
nameAppend = sma
copyInputName = 1
noPostEOIprocessing = 0
smaWin = 3
noZeroSma = 1

[is13_f0sel:cDataSelector]
reader.dmLevel = is13_lld_nzsmo
writer.dmLevel = is13_lld_f0_nzsmo
\{\cm[bufferModeConf]}
nameAppend = ff0
selected = F0final_sma

[is13_smoA:cContourSmoother]
reader.dmLevel = is13_audspecSum;is13_audspecRastaSum;is13_energy;is13_zcr
writer.dmLevel = is13_lldA_smo
\{\cm[bufferModeConf]}
nameAppend = sma
copyInputName = 1
noPostEOIprocessing = 0
smaWin = 3

[is13_smoB:cContourSmoother]
reader.dmLevel = is13_audspecRasta;is13_spectral;is13_mfcc1_12
writer.dmLevel = is13_lldB_smo
\{\cm[bufferModeConf]}
nameAppend = sma
copyInputName = 1
noPostEOIprocessing = 0
smaWin = 3

;;;;;;;;; deltas
[componentInstances:cComponentManager]
instance[is13_deNz].type=cDeltaRegression
instance[is13_deA].type=cDeltaRegression
instance[is13_deB].type=cDeltaRegression
instance[is13_def0sel].type=cDeltaRegression

[is13_deNz:cDeltaRegression]
reader.dmLevel = is13_lld_nzsmo
writer.dmLevel = is13_lld_nzsmo_de
\{\cm[bufferModeConf]}
onlyInSegments = 1
zeroSegBound = 1

[is13_deA:cDeltaRegression]
reader.dmLevel = is13_lldA_smo
writer.dmLevel = is13_lldA_smo_de
\{\cm[bufferModeConf]}

[is13_deB:cDeltaRegression]
reader.dmLevel = is13_lldB_smo
writer.dmLevel = is13_lldB_smo_de
\{\cm[bufferModeConf]}

[is13_def0sel:cDeltaRegression]
reader.dmLevel = is13_lld_f0_nzsmo
writer.dmLevel = is13_lld_f0_nzsmo_de
\{\cm[bufferModeConf]}
onlyInSegments = 1
zeroSegBound = 1


