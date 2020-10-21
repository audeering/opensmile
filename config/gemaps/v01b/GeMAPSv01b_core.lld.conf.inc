///////////////////////////////////////////////////////////////////////////////////////
///////// > openSMILE configuration file, Geneva feature set <       //////////////////
/////////                                                            //////////////////
///////// (c) 2014, 2020 by audEERING                                //////////////////
/////////     All rights reserved. See file COPYING for details.     //////////////////
///////////////////////////////////////////////////////////////////////////////////////

;;;;;;;;;;;;;;;;;;;;;;

[componentInstances:cComponentManager]
instance[gemapsv01b_frame60].type=cFramer
instance[gemapsv01b_win60].type=cWindower
instance[gemapsv01b_fft60].type=cTransformFFT
instance[gemapsv01b_fftmp60].type=cFFTmagphase

[gemapsv01b_frame60:cFramer]
reader.dmLevel=wave
writer.dmLevel=gemapsv01b_frame60
\{\cm[bufferModeRbConf{../../shared/BufferModeRb.conf.inc}:path to included config to set the buffer mode for the standard ringbuffer levels]}
frameSize = 0.060
frameStep = 0.010
frameCenterSpecial = left

[gemapsv01b_win60:cWindower]
reader.dmLevel=gemapsv01b_frame60
writer.dmLevel=gemapsv01b_winG60
winFunc=gauss
gain=1.0
sigma=0.4

[gemapsv01b_fft60:cTransformFFT]
reader.dmLevel=gemapsv01b_winG60
writer.dmLevel=gemapsv01b_fftcG60

[gemapsv01b_fftmp60:cFFTmagphase]
reader.dmLevel=gemapsv01b_fftcG60
writer.dmLevel=gemapsv01b_fftmagG60
\{\cm[bufferModeRbLagConf{../../shared/BufferModeRbLag.conf.inc}:path to included config to set the buffer mode for levels which will be joint with Viterbi smoothed -lagged- F0]}


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

[componentInstances:cComponentManager]
instance[gemapsv01b_frame25].type=cFramer
instance[gemapsv01b_win25].type=cWindower
instance[gemapsv01b_fft25].type=cTransformFFT
instance[gemapsv01b_fftmp25].type=cFFTmagphase

[gemapsv01b_frame25:cFramer]
reader.dmLevel=wave
writer.dmLevel=gemapsv01b_frame25
\{\cm[bufferModeRbConf]}
frameSize = 0.020
frameStep = 0.010
frameCenterSpecial = left

[gemapsv01b_win25:cWindower]
reader.dmLevel=gemapsv01b_frame25
writer.dmLevel=gemapsv01b_winH25
winFunc=hamming

[gemapsv01b_fft25:cTransformFFT]
reader.dmLevel=gemapsv01b_winH25
writer.dmLevel=gemapsv01b_fftcH25

[gemapsv01b_fftmp25:cFFTmagphase]
reader.dmLevel=gemapsv01b_fftcH25
writer.dmLevel=gemapsv01b_fftmagH25
;;; CHECK!!!!!!!
;writer.levelconf.growDyn = 1
;writer.levelconf.isRb = 0


;;;;;;;;;;;;;;;;;;;; HPS pitch

[componentInstances:cComponentManager]
instance[gemapsv01b_scale].type=cSpecScale
instance[gemapsv01b_shs].type=cPitchShs

[gemapsv01b_scale:cSpecScale]
reader.dmLevel=gemapsv01b_fftmagG60
writer.dmLevel=gemapsv01b_hpsG60
\{\cm[bufferModeRbConf]}
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

[gemapsv01b_shs:cPitchShs]
reader.dmLevel=gemapsv01b_hpsG60
writer.dmLevel=gemapsv01b_pitchShsG60
\{\cm[bufferModeRbLagConf]}
copyInputName = 1
processArrayFields = 0
maxPitch = 1000
minPitch = 55
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
instance[gemapsv01b_energy60].type=cEnergy

[gemapsv01b_energy60:cEnergy]
reader.dmLevel=gemapsv01b_winG60
writer.dmLevel=gemapsv01b_e60
 ; This must be > than buffersize of viterbi smoother
 ; writer.levelconf.nT=100
\{\cm[bufferModeRbLagConf]}
rms=1
log=0

[componentInstances:cComponentManager]
instance[gemapsv01b_pitchSmoothViterbi].type=cPitchSmootherViterbi

[gemapsv01b_pitchSmoothViterbi:cPitchSmootherViterbi]
reader.dmLevel=gemapsv01b_pitchShsG60
reader2.dmLevel=gemapsv01b_pitchShsG60
writer.dmLevel=gemapsv01b_logPitchRaw
copyInputName = 1
bufferLength=40
F0final = 1
F0finalLog = 1
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
instance[gemapsv01b_volmerge].type = cValbasedSelector

[gemapsv01b_volmerge:cValbasedSelector]
reader.dmLevel = gemapsv01b_e60;gemapsv01b_logPitchRaw
writer.dmLevel = gemapsv01b_logPitch
\{\cm[bufferModeRbLagConf]}
idx=0
threshold=0.001
removeIdx=1
zeroVec=1
outputVal=0.0

;;;;;;;;;;;;;;;;;;; Voice Quality (VQ)

[componentInstances:cComponentManager]
instance[gemapsv01b_pitchJitter].type=cPitchJitter

[gemapsv01b_pitchJitter:cPitchJitter]
reader.dmLevel = wave
writer.dmLevel = gemapsv01b_jitterShimmer
\{\cm[bufferModeRbLagConf]}
copyInputName = 1
F0reader.dmLevel = gemapsv01b_logPitch
F0field = F0final
searchRangeRel = 0.100000
jitterLocal = 1
jitterDDP = 0
jitterLocalEnv = 0
jitterDDPEnv = 0
shimmerLocal = 0
shimmerLocalDB = 1
shimmerLocalEnv = 0
onlyVoiced = 0
logHNR = 0
 ; This must be larger than the viterbi pitch smoother lag
inputMaxDelaySec = 2.5
minNumPeriods = 2
minCC = 0.5
useBrokenJitterThresh = 0

;;;;;;;;;;;;;;;;;;;;; Energy / loudness


[componentInstances:cComponentManager]
instance[gemapsv01b_melspec1].type=cMelspec
instance[gemapsv01b_audspec].type=cPlp
instance[gemapsv01b_audspecSum].type=cVectorOperation

[gemapsv01b_melspec1:cMelspec]
reader.dmLevel=gemapsv01b_fftmagH25
writer.dmLevel=gemapsv01b_melspec1
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
[gemapsv01b_audspec:cPlp]
reader.dmLevel=gemapsv01b_melspec1
writer.dmLevel=gemapsv01b_audspec
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

[gemapsv01b_audspecSum:cVectorOperation]
reader.dmLevel = gemapsv01b_audspec
writer.dmLevel = gemapsv01b_loudness
; This must be larger than the F0 viterbi buffer length
; since audspecSum and F0 envelope are joint later!
\{\cm[bufferModeRbLagConf]}
nameAppend = loudness
copyInputName = 0
processArrayFields = 0
operation = ll1
nameBase = loudness

;;;;;;;;;;;;;; Formants ;;;;;;;;;;;;;;;;;;;;;;
[componentInstances:cComponentManager]
instance[gemapsv01b_resampLpc].type=cSpecResample
instance[gemapsv01b_lpc].type=cLpc
instance[gemapsv01b_formantLpc].type=cFormantLpc

[gemapsv01b_resampLpc:cSpecResample]
// use fftcG60 ?  H25 has faster resampling
reader.dmLevel=gemapsv01b_fftcH25 
writer.dmLevel=gemapsv01b_outpR
targetFs = 11000

[gemapsv01b_lpc:cLpc]
reader.dmLevel=gemapsv01b_outpR
writer.dmLevel=gemapsv01b_lpc
p=11
method=acf
lpGain=0
saveLPCoeff=1
residual=0
forwardFilter=0
lpSpectrum=0

[gemapsv01b_formantLpc:cFormantLpc]
reader.dmLevel=gemapsv01b_lpc
writer.dmLevel=gemapsv01b_formants
\{\cm[bufferModeRbLagConf]}
saveIntensity=0
saveBandwidths=1
maxF=5450.0
minF=50.0
nFormants=5
useLpSpec=0
medianFilter=0
octaveCorrection=0

;;;;;;;;;;;;; Harmonics ;;;;;;;;;;;;;;;;;;;;
[componentInstances:cComponentManager]
instance[gemapsv01b_harmonics].type = cHarmonics

  ;;;; default (template) configuration section for component 'cHarmonics' ;;;;
[gemapsv01b_harmonics:cHarmonics]
reader.dmLevel = gemapsv01b_logPitch;gemapsv01b_formants;gemapsv01b_fftmagG60
writer.dmLevel = gemapsv01b_harmonics
\{\cm[bufferModeRbLagConf]}
copyInputName = 0
processArrayFields = 0
includeSingleElementFields = 1
preserveFieldNames = 0
formantFrequencyFieldName = formantFreqLpc
formantFrequencyFieldNameIsFull = 1
formantBandwidthFieldName = formantBandwidthLpc
formantBandwidthFieldNameIsFull = 1
f0ElementName = F0final
f0ElementNameIsFull = 1
magSpecFieldName = pcm_fftMag
magSpecFieldNameIsFull = 1
nHarmonics = 100
harmonicDifferences = H1-H2;H1-A3
harmonicDifferencesLog = 1
nHarmonicMagnitudes = 0
firstHarmonicMagnitude = 1
outputLogRelMagnitudes = 1
formantAmplitudes=1
formantAmplitudesLogRel = 1
formantAmplitudesStart = 1
formantAmplitudesEnd = 3
computeAcfHnrLogdB = 1

;;;;;;;;;;;;;;; spectral
[componentInstances:cComponentManager]
instance[gemapsv01b_logSpectral].type=cSpectral

[gemapsv01b_logSpectral:cSpectral]
reader.dmLevel=gemapsv01b_fftmagH25
writer.dmLevel=gemapsv01b_logSpectral
\{\cm[bufferModeRbLagConf]}
flux = 0
centroid = 0
maxPos=0
minPos=0
entropy = 0
flatness = 0
harmonicity = 0
sharpness = 0
variance=0
skewness=0
kurtosis=0
alphaRatio = 1
hammarbergIndex = 1
slope = 0
slopes[0] = 0-500
slopes[1] = 500-1500
  ; NOTE: added this to sync with eGeMAPS set, should have no effect.
normBandEnergies = 1
squareInput = 1
useLogSpectrum = 1
freqRange = 0-5000
oldSlopeScale = 0

;;;;;;;;;;;; collecting, filtering, and renaming ;;;;;;;;;;;;;;;;,

/*
logSpectral     Hammarberg, AlphaRatio, spectralSlope  0-500, 500-1500
harmonics   H1-H2, H1-A3, HNRlog
logPitch    F0finalLog
jitterShimmer   jitterLocal, shimmerLocal
loudness
formants   F1-3 freq, F1 bandw (check!)
harmonics  F1-3 level relative
*/

[componentInstances:cComponentManager]
instance[gemapsv01b_lldSetSelectorE].type=cDataSelector
instance[gemapsv01b_lldSetSelectorF].type=cDataSelector
instance[gemapsv01b_lldSetSelectorLogF0].type=cDataSelector
instance[gemapsv01b_formantVoiced].type = cValbasedSelector
instance[gemapsv01b_lldSetSelectorNoF0LoudnNz].type = cDataSelector
instance[gemapsv01b_logSpectralVoiced].type = cValbasedSelector
instance[gemapsv01b_logSpectralUnvoiced].type = cValbasedSelector
instance[gemapsv01b_lldSetSelectorSpectralNz].type=cDataSelector
instance[gemapsv01b_lldSetSelectorSpectralZ].type=cDataSelector

[gemapsv01b_lldSetSelectorE:cDataSelector]
reader.dmLevel = gemapsv01b_loudness;gemapsv01b_logSpectral
writer.dmLevel = gemapsv01b_lldsetE
\{\cm[bufferModeRbConf]}
selected = loudness;pcm_fftMag_alphaRatioDB;pcm_fftMag_hammarbergIndexDB;pcm_fftMag_logSpectralSlopeOfBand0-500;pcm_fftMag_logSpectralSlopeOfBand500-1500
newNames = Loudness;alphaRatio;hammarbergIndex;slope0-500;slope500-1500

[gemapsv01b_lldSetSelectorF:cDataSelector]
reader.dmLevel = gemapsv01b_logPitch;gemapsv01b_jitterShimmer;gemapsv01b_harmonics;gemapsv01b_formants
writer.dmLevel = gemapsv01b_lldsetF
\{\cm[bufferModeRbConf]}
selected = F0finalLog;jitterLocal;shimmerLocalDB;HarmonicsToNoiseRatioACFLogdB;HarmonicDifferenceLogRelH1-H2;HarmonicDifferenceLogRelH1-A3;formantFreqLpc[1];formantBandwidthLpc[1];FormantAmplitudeByMaxHarmonicLogRelF0[1];formantFreqLpc[2];FormantAmplitudeByMaxHarmonicLogRelF0[2];formantFreqLpc[3];FormantAmplitudeByMaxHarmonicLogRelF0[3]
newNames = F0semitoneFrom27.5Hz;jitterLocal;shimmerLocaldB;HNRdBACF;logRelF0-H1-H2;logRelF0-H1-A3;F1frequency;F1bandwidth;F1amplitudeLogRelF0;F2frequency;F2amplitudeLogRelF0;F3frequency;F3amplitudeLogRelF0

[gemapsv01b_lldSetSelectorLogF0:cDataSelector]
reader.dmLevel = gemapsv01b_logPitch
writer.dmLevel = gemapsv01b_lld_single_logF0
\{\cm[bufferModeRbLagConf]}
selected = F0finalLog
newNames = F0semitoneFrom27.5Hz

// select only formants where F0 > 0 for functionals
// (in LLD csv we output all of them..!)
[gemapsv01b_formantVoiced:cValbasedSelector]
reader.dmLevel = gemapsv01b_lld_single_logF0;gemapsv01b_formants
writer.dmLevel = gemapsv01b_formantsNz
\{\cm[bufferModeRbLagConf]}
idx=0
threshold=0.000001
removeIdx=1
zeroVec=1
outputVal=0.0

[gemapsv01b_lldSetSelectorNoF0LoudnNz:cDataSelector]
reader.dmLevel = gemapsv01b_jitterShimmer;gemapsv01b_harmonics;gemapsv01b_formantsNz
writer.dmLevel = gemapsv01b_lldSetNoF0AndLoudnessNz
\{\cm[bufferModeRbConf]}
selected = jitterLocal;shimmerLocalDB;HarmonicsToNoiseRatioACFLogdB;HarmonicDifferenceLogRelH1-H2;HarmonicDifferenceLogRelH1-A3;formantFreqLpc[1];formantBandwidthLpc[1];FormantAmplitudeByMaxHarmonicLogRelF0[1];formantFreqLpc[2];FormantAmplitudeByMaxHarmonicLogRelF0[2];formantFreqLpc[3];FormantAmplitudeByMaxHarmonicLogRelF0[3]
newNames = jitterLocal;shimmerLocaldB;HNRdBACF;logRelF0-H1-H2;logRelF0-H1-A3;F1frequency;F1bandwidth;F1amplitudeLogRelF0;F2frequency;F2amplitudeLogRelF0;F3frequency;F3amplitudeLogRelF0

// select logspectral for voiced sounds
[gemapsv01b_logSpectralVoiced:cValbasedSelector]
reader.dmLevel = gemapsv01b_lld_single_logF0;gemapsv01b_logSpectral
writer.dmLevel = gemapsv01b_logSpectralVoiced
\{\cm[bufferModeRbLagConf]}
idx=0
threshold=0.000001
removeIdx=1
zeroVec=1
outputVal=0.0

// select logspectral for voiced sounds
[gemapsv01b_logSpectralUnvoiced:cValbasedSelector]
reader.dmLevel = gemapsv01b_lld_single_logF0;gemapsv01b_logSpectral
writer.dmLevel = gemapsv01b_logSpectralUnvoiced
\{\cm[bufferModeRbLagConf]}
idx=0
invert = 1
threshold = 0.000001
removeIdx=1
zeroVec=1
outputVal=0.0

[gemapsv01b_lldSetSelectorSpectralNz:cDataSelector]
reader.dmLevel = gemapsv01b_logSpectralVoiced
writer.dmLevel = gemapsv01b_lldSetSpectralNz
\{\cm[bufferModeRbConf]}
selected = pcm_fftMag_alphaRatioDB;pcm_fftMag_hammarbergIndexDB;pcm_fftMag_logSpectralSlopeOfBand0-500;pcm_fftMag_logSpectralSlopeOfBand500-1500
newNames = alphaRatioV;hammarbergIndexV;slopeV0-500;slopeV500-1500

[gemapsv01b_lldSetSelectorSpectralZ:cDataSelector]
reader.dmLevel = gemapsv01b_logSpectralUnvoiced
writer.dmLevel = gemapsv01b_lldSetSpectralZ
\{\cm[bufferModeRbConf]}
selected = pcm_fftMag_alphaRatioDB;pcm_fftMag_hammarbergIndexDB;pcm_fftMag_logSpectralSlopeOfBand0-500;pcm_fftMag_logSpectralSlopeOfBand500-1500
newNames = alphaRatioUV;hammarbergIndexUV;slopeUV0-500;slopeUV500-1500


;;;;;;;;;;;;;;;;  smoothing ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
[componentInstances:cComponentManager]
instance[gemapsv01b_smoFnz].type=cContourSmoother
instance[gemapsv01b_smoE].type=cContourSmoother
instance[gemapsv01b_smoNoF0andLoudnNz].type=cContourSmoother
instance[gemapsv01b_smoSpectralZ].type=cContourSmoother
instance[gemapsv01b_smoSpectralNz].type=cContourSmoother

[gemapsv01b_smoFnz:cContourSmoother]
reader.dmLevel = gemapsv01b_lldsetF
writer.dmLevel = gemapsv01b_lldsetF_smo
nameAppend = sma3nz
copyInputName = 1
noPostEOIprocessing = 0
smaWin = 3
noZeroSma = 1

[gemapsv01b_smoE:cContourSmoother]
reader.dmLevel = gemapsv01b_lldsetE
writer.dmLevel = gemapsv01b_lldsetE_smo
nameAppend = sma3
copyInputName = 1
noPostEOIprocessing = 0
smaWin = 3
noZeroSma = 0

[gemapsv01b_smoNoF0andLoudnNz:cContourSmoother]
reader.dmLevel = gemapsv01b_lldSetNoF0AndLoudnessNz
writer.dmLevel = gemapsv01b_lldSetNoF0AndLoudnessNz_smo
\{\cm[bufferModeConf{../../shared/BufferMode.conf.inc}:path to included config to set the buffer mode for the levels before the functionals]}
nameAppend = sma3nz
copyInputName = 1
noPostEOIprocessing = 0
smaWin = 3
noZeroSma = 1

[gemapsv01b_smoSpectralZ:cContourSmoother]
reader.dmLevel = gemapsv01b_lldSetSpectralZ
writer.dmLevel = gemapsv01b_lldSetSpectralZ_smo
\{\cm[bufferModeConf]}
nameAppend = sma3nz
copyInputName = 1
noPostEOIprocessing = 0
smaWin = 3
; non-zero SMA is ok here, as it is inverted with 0's for the voiced parts
noZeroSma = 1

[gemapsv01b_smoSpectralNz:cContourSmoother]
reader.dmLevel = gemapsv01b_lldSetSpectralNz
writer.dmLevel = gemapsv01b_lldSetSpectralNz_smo
\{\cm[bufferModeConf]}
nameAppend = sma3nz
copyInputName = 1
noPostEOIprocessing = 0
smaWin = 3
noZeroSma = 1






