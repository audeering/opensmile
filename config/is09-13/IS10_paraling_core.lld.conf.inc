
///////////////////////////////////////////////////////////////////////////////////////
///////// > openSMILE configuration file for IS10 features <         //////////////////
/////////                                                            //////////////////
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
instance[is10_fr40].type=cFramer
  ;; 40 ms frames features
instance[is10_w40].type=cWindower
instance[is10_fft40].type=cTransformFFT
instance[is10_fftmagphase40].type=cFFTmagphase
instance[is10_scale].type=cSpecScale
instance[is10_pitchShs].type=cPitchShs
instance[is10_pitchSmooth].type=cPitchSmoother
instance[is10_pitchJitter].type=cPitchJitter
instance[is10_pitchSmooth2].type=cPitchSmoother
instance[is10_res].type=cSpecResample
 ;;; 25 ms frames features:
instance[is10_fr25].type=cFramer
instance[is10_pe].type=cVectorPreemphasis
instance[is10_win].type=cWindower
instance[is10_fft].type=cTransformFFT
instance[is10_fftmagphase].type=cFFTmagphase
 ; mfcc
instance[is10_mspec].type=cMelspec
instance[is10_mfcc].type=cMfcc
 ; log mel frequency bands (mfb)
instance[is10_mspec2].type=cMelspec
instance[is10_vo].type=cVectorOperation
instance[is10_lpc].type=cLpc
 ; Line Spectral Frequencies
instance[is10_lsp].type=cLsp
 ; Loudness (narrow-band approximation)
instance[is10_intens].type=cIntensity
 ;;; all LLD concattenated and smoothed using a moving average filter
instance[is10_lld].type=cContourSmoother
instance[is10_lld2].type=cContourSmoother
 ; delta coefficients of LLD
instance[is10_delta1].type=cDeltaRegression
instance[is10_delta2].type=cDeltaRegression


[is10_fr40:cFramer]
reader.dmLevel=wave
writer.dmLevel=is10_frames40
\{\cm[bufferModeRbConf{../shared/BufferModeRb.conf.inc}:path to included config to set the buffer mode for the standard ringbuffer levels]}
frameMode = fixed
frameSize = 0.060
frameStep = 0.010
frameCenterSpecial = left
noPostEOIprocessing = 1

[is10_w40:cWindower]
reader.dmLevel=is10_frames40
writer.dmLevel=is10_win40frame
winFunc = gauss
sigma = 0.25
gain = 1.0

[is10_fft40:cTransformFFT]
reader.dmLevel=is10_win40frame
writer.dmLevel=is10_fftc40

[is10_fftmagphase40:cFFTmagphase]
reader.dmLevel=is10_fftc40
writer.dmLevel=is10_fftmag40
magnitude = 1
phase = 0

[is10_scale:cSpecScale]
reader.dmLevel=is10_fftmag40
writer.dmLevel=is10_hps
scale=octave
sourceScale = lin
specSmooth = 0
auditoryWeighting = 0
specEnhance = 0
minF = 20
maxF = -1
nPointsTarget = 0
specSmooth = 1
specEnhance = 1
auditoryWeighting = 1
interpMethod = spline

[is10_pitchShs:cPitchShs]
reader.dmLevel=is10_hps
writer.dmLevel=is10_pitchShs
inputFieldSearch = fftMag_octScale
F0raw = 0
voicingClip = 0
voicingC1=0
scores=1
voicing=1
nCandidates = 6
octaveCorrection = 0
greedyPeakAlgo = 1
compressionFactor = 0.85
nHarmonics = 15
voicingCutoff = 0.70
maxPitch = 620
minPitch = 52

[is10_pitchSmooth:cPitchSmoother]
reader.dmLevel=is10_pitchShs
writer.dmLevel=is10_pitch
F0raw = 0
F0final = 0
F0finalEnv = 1
voicingFinalUnclipped = 1
medianFilter0 = 0
postSmoothingMethod = simple
;simple
octaveCorrection = 0
writer.levelconf.nT=10
;writer.levelconf.noHang=2
writer.levelconf.isRb=0
writer.levelconf.growDyn=1

[is10_pitchSmooth2:cPitchSmoother]
reader.dmLevel=is10_pitchShs
writer.dmLevel=is10_pitchF
F0raw = 0
F0final = 1
F0finalEnv = 0
voicingFinalUnclipped = 0
medianFilter0 = 0
postSmoothingMethod = simple
octaveCorrection = 0
writer.levelconf.nT=10
;writer.levelconf.noHang=2
writer.levelconf.isRb=0
writer.levelconf.growDyn=1

 ;;;; default (template) configuration section for component 'cPitchJitter' ;;;;
[is10_pitchJitter:cPitchJitter]
reader.dmLevel = wave
writer.dmLevel = is10_jitter
\{\cm[bufferModeRbLagConf{../shared/BufferModeRb.conf.inc}:path to included config to set the buffer mode for levels which will be joint with Viterbi smoothed -lagged- F0]}
copyInputName = 1
F0reader.dmLevel = is10_pitchF
F0field = F0final
searchRangeRel = 0.200000
jitterLocal = 1
jitterDDP = 1
jitterLocalEnv = 0
jitterDDPEnv = 0
shimmerLocal = 1
shimmerLocalEnv = 0
onlyVoiced = 0
;periodLengths = 0
;periodStarts = 0
inputMaxDelaySec = 1
usePeakToPeakPeriodLength = 0
shimmerUseRmsAmplitude = 0
minCC = 0.5
minNumPeriods = 2

[is10_fr25:cFramer]
reader.dmLevel=wave
writer.dmLevel=is10_frames
\{\cm[bufferModeRbConf]}
frameSize = 0.025
frameStep = 0.010
frameCenterSpecial = left

[is10_pe:cVectorPreemphasis]
reader.dmLevel=is10_frames
writer.dmLevel=is10_framespe
k=0.97

[is10_win:cWindower]
reader.dmLevel=is10_framespe
writer.dmLevel=is10_winframe
winFunc = ham
gain = 1.0

[is10_fft:cTransformFFT]
reader.dmLevel=is10_winframe
writer.dmLevel=is10_fftc

[is10_fftmagphase:cFFTmagphase]
reader.dmLevel=is10_fftc
writer.dmLevel=is10_fftmag
magnitude = 1
phase = 0

[is10_mspec:cMelspec]
reader.dmLevel=is10_fftmag
writer.dmLevel=is10_mspec1
htkcompatible = 0
usePower = 1
lofreq = 20
hifreq = 8000
nBands=26
specScale = mel
bwMethod = lr

[is10_mfcc:cMfcc]
reader.dmLevel = is10_mspec1
writer.dmLevel = is10_mfcc
htkcompatible = 0
firstMfcc=0
lastMfcc=14
cepLifter=22
copyInputName = 0

[is10_mspec2:cMelspec]
reader.dmLevel=is10_fftmag
writer.dmLevel=is10_mspec2
htkcompatible = 0
usePower = 1
lofreq = 20
hifreq = 6500
nBands=8
specScale = mel
bwMethod = lr

[is10_vo:cVectorOperation]
reader.dmLevel=is10_mspec2
writer.dmLevel=is10_mspec2log
operation = log
copyInputName = 0
nameAppend=logMelFreqBand

[is10_res:cSpecResample]
reader.dmLevel=is10_fftc
writer.dmLevel=is10_outpR
targetFs = 11000

[is10_lpc:cLpc]
;reader.dmLevel=framespe
reader.dmLevel=is10_outpR
writer.dmLevel=is10_lpc
p=8
method = acf
saveLPCoeff = 1
lpGain = 0
saveRefCoeff = 0
residual = 0
forwardFilter = 0
lpSpectrum = 0

[is10_lsp:cLsp]
reader.dmLevel=is10_lpc
writer.dmLevel=is10_lsp

[is10_intens:cIntensity]
reader.dmLevel=is10_frames
writer.dmLevel=is10_intens
intensity=0
loudness=1

[is10_lld:cContourSmoother]
reader.dmLevel=is10_intens;is10_mfcc;is10_mspec2log;is10_lsp;is10_pitch
writer.dmLevel=is10_lld1
\{\cm[bufferModeConf{../shared/BufferMode.conf.inc}:path to included config to set the buffer mode for the levels before the functionals]}
smaWin = 3
; this level must grow to hold ALL the LLD of the full input

// ---- delta regression of LLD ----
[is10_delta1:cDeltaRegression]
reader.dmLevel=is10_lld1
writer.dmLevel=is10_lld1_de
\{\cm[bufferModeConf]}
deltawin=2
blocksize=1

[is10_lld2:cContourSmoother]
reader.dmLevel=is10_pitchF;is10_jitter
writer.dmLevel=is10_lld2
\{\cm[bufferModeConf]}
; this level must grow to hold ALL the LLD of the full input
smaWin = 3
noZeroSma = 1

// ---- delta regression of LLD ----
[is10_delta2:cDeltaRegression]
reader.dmLevel=is10_lld2
writer.dmLevel=is10_lld2_de
\{\cm[bufferModeConf]}
deltawin=2
blocksize=1
onlyInSegments = 1
zeroSegBound = 1


//////---------------------- END -------------------------///////
