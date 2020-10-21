///////////////////////////////////////////////////////////////////////////////////////
///////// > openSMILE configuration file                             //////////////////
/////////   for AVEC 2013                                            //////////////////
/////////   based on avec2011.conf                                   //////////////////
/////////  * written 2013 by Florian Eyben *                         //////////////////
/////////                                                            //////////////////
///////// (c) 2014 by audEERING UG (limited)                         //////////////////
/////////     All rights reserved                                    //////////////////
///////////////////////////////////////////////////////////////////////////////////////



;;;;;;; component list ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

[componentInstances:cComponentManager]
instance[dataMemory].type=cDataMemory
printLevelStats=0

\{../shared/standard_wave_input.conf.inc}

;;;;;;;;;;;;;;;;;;;;;;

[componentInstances:cComponentManager]
instance[frame60].type=cFramer
instance[win60].type=cWindower
instance[fft60].type=cTransformFFT
instance[fftmp60].type=cFFTmagphase

[frame60:cFramer]
reader.dmLevel=wave
writer.dmLevel=frame60
frameSize = 0.060
frameStep = 0.010
frameCenterSpecial = left


[win60:cWindower]
reader.dmLevel=frame60
writer.dmLevel=winG60
winFunc=gauss
gain=1.0
sigma=0.4

[fft60:cTransformFFT]
reader.dmLevel=winG60
writer.dmLevel=fftcG60
zeroPadSymmetric = 0

[fftmp60:cFFTmagphase]
reader.dmLevel=fftcG60
writer.dmLevel=fftmagG60


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

[componentInstances:cComponentManager]
instance[frame25].type=cFramer
instance[pe25].type=cVectorPreemphasis
instance[win25].type=cWindower
instance[fft25].type=cTransformFFT
instance[fftmp25].type=cFFTmagphase

[frame25:cFramer]
reader.dmLevel=wave
writer.dmLevel=frame25
frameSize = 0.025
frameStep = 0.010
frameCenterSpecial = left

[pe25:cVectorPreemphasis]
reader.dmLevel=frame25
writer.dmLevel=frame25pe
k=0.97
de=0

[win25:cWindower]
reader.dmLevel=frame25
writer.dmLevel=winH25
winFunc=hamming

[fft25:cTransformFFT]
reader.dmLevel=winH25
writer.dmLevel=fftcH25
zeroPadSymmetric = 0

[fftmp25:cFFTmagphase]
reader.dmLevel=fftcH25
writer.dmLevel=fftmagH25

;;;;;;;;;;;;;;;;;;;; HPS pitch

[componentInstances:cComponentManager]
instance[scale].type=cSpecScale
instance[shs].type=cPitchShs
instance[pitchSmooth].type=cPitchSmootherViterbi
instance[energy60].type=cEnergy
instance[volmerge].type=cValbasedSelector

[energy60:cEnergy]
reader.dmLevel=winG60
writer.dmLevel=e60
rms=1
log=0
writer.levelconf.nT=200


[scale:cSpecScale]
reader.dmLevel=fftmagG60
writer.dmLevel=hpsG60
copyInputName = 1
processArrayFields = 0
scale=octave
sourceScale = lin
// logScaleBase = 2
// logSourceScaleBase = 2
// firstNote = 55
interpMethod = spline
minF = 20
maxF = -1
nPointsTarget = 0
specSmooth = 1
specEnhance = 1
auditoryWeighting = 1 

[shs:cPitchShs]
reader.dmLevel=hpsG60
writer.dmLevel=pitchShsG60
copyInputName = 1
processArrayFields = 0
maxPitch = 620
minPitch = 42
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

[pitchSmooth:cPitchSmootherViterbi]
reader.dmLevel=pitchShsG60
reader2.dmLevel=pitchShsG60
writer.dmLevel=pitchG60a
copyInputName = 1
bufferLength=90
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

// SHS/Viterbi pitch algo does strange things at very low amplitudes, so we filter these out 
[volmerge:cValbasedSelector]
reader.dmLevel = e60;pitchG60a
writer.dmLevel = pitchG60
idx=0
threshold=0.0008
removeIdx=1
zeroVec=1
outputVal=0.0


;;;;;;;;;;;;;;;;;;; VQ

[componentInstances:cComponentManager]
instance[pitchJitter].type=cPitchJitter


[pitchJitter:cPitchJitter]
reader.dmLevel = wave
writer.dmLevel = jitterShimmer
// nameAppend =
copyInputName = 1
; is pitchF really necessary, or can we use pitchG60 ?
F0reader.dmLevel = pitchG60
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
instance[melspec1].type=cMelspec
instance[audspec].type=cPlp
instance[audspecSum].type=cVectorOperation


[melspec1:cMelspec]
reader.dmLevel=fftmagH25
writer.dmLevel=melspec1
; htk compatible sample value scaling
htkcompatible = 0
nBands = 26
; use power spectrum instead of magnitude spectrum
usePower = 1
lofreq = 20
hifreq = 8000
specScale = bark

; perform auditory weighting of spectrum
[audspec:cPlp]
reader.dmLevel=melspec1
writer.dmLevel=audspec
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

[audspecSum:cVectorOperation]
reader.dmLevel = audspec
writer.dmLevel = audspecSum
// nameAppend = 
copyInputName = 1
processArrayFields = 0
operation = ll1
nameBase = audspec

;;;;;;;;;;;;;;; spectral

[componentInstances:cComponentManager]
instance[spectral].type=cSpectral


[spectral:cSpectral]
reader.dmLevel=fftmagH25
writer.dmLevel=spectral
bands[0]=250-650
bands[1]=1000-4000
rollOff[0] = 0.25
rollOff[1] = 0.50
rollOff[2] = 0.75
rollOff[3] = 0.90
flux=1
centroid=0
maxPos=0
minPos=0
entropy=1
variance=1
skewness=1
kurtosis=1
slope=0
sharpness = 1
tonality = 0
harmonicity = 1
flatness = 1


;;;;;;;;;;;;;;; mfcc

[componentInstances:cComponentManager]
instance[melspecMfcc].type=cMelspec
instance[mfcc].type=cMfcc

[melspecMfcc:cMelspec]
reader.dmLevel=fftmagH25
writer.dmLevel=melspecMfcc
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

[mfcc:cMfcc]
reader.dmLevel=melspecMfcc
writer.dmLevel=mfcc1_12
copyInputName = 1
processArrayFields = 1
firstMfcc = 1
lastMfcc  = 16
cepLifter = 22.0
htkcompatible = 1


;;;;;;;;;;;;;;;;  zcr

[componentInstances:cComponentManager]
instance[mzcr].type=cMZcr

[mzcr:cMZcr]
reader.dmLevel = frame25
writer.dmLevel = zcr
copyInputName = 1
processArrayFields = 1
zcr = 1
mcr = 0
amax = 0
maxmin = 0
dc = 0


;;;;;;;;;;;;;;;;;;;; smoothing and selection

[componentInstances:cComponentManager]
instance[smoNz].type=cContourSmoother
instance[smoA].type=cContourSmoother
instance[smoB].type=cContourSmoother
instance[f0sel].type=cDataSelector
instance[f0psel].type=cDataSelector

[smoNz:cContourSmoother]
reader.dmLevel = pitchG60;jitterShimmer
writer.dmLevel = lld_nzsmo
writer.levelconf.isRb=0
writer.levelconf.growDyn=1
nameAppend = sma
copyInputName = 1
noPostEOIprocessing = 0
smaWin = 3
noZeroSma = 1

[f0sel:cDataSelector]
reader.dmLevel = lld_nzsmo
writer.dmLevel = lld_f0v_nzsmo
writer.levelconf.isRb=0
writer.levelconf.growDyn=1
nameAppend = f0v
selected = F0final_sma

[f0psel:cDataSelector]
reader.dmLevel = lld_nzsmo
writer.dmLevel = lld_f0p_nzsmo
writer.levelconf.isRb=0
writer.levelconf.growDyn=1
nameAppend = f0p
selected = F0final_sma


[smoA:cContourSmoother]
reader.dmLevel = audspecSum;zcr
writer.dmLevel = lldA_smo
writer.levelconf.isRb=0
writer.levelconf.growDyn=1
nameAppend = sma
copyInputName = 1
noPostEOIprocessing = 0
smaWin = 3

[smoB:cContourSmoother]
reader.dmLevel = spectral;mfcc1_12
writer.dmLevel = lldB_smo
writer.levelconf.isRb=0
writer.levelconf.growDyn=1
nameAppend = sma
copyInputName = 1
noPostEOIprocessing = 0
smaWin = 3

;;;;;;;;; deltas
[componentInstances:cComponentManager]
instance[deNz].type=cDeltaRegression
instance[deA].type=cDeltaRegression
instance[deB].type=cDeltaRegression

[deNz:cDeltaRegression]
reader.dmLevel = lld_nzsmo
writer.dmLevel = lld_nzsmo_de
writer.levelconf.isRb=0
writer.levelconf.growDyn=1
onlyInSegments = 1
zeroSegBound = 1
; TODO?  delta with zero segments removed...

[deA:cDeltaRegression]
reader.dmLevel = lldA_smo
writer.dmLevel = lldA_smo_de
writer.levelconf.isRb=0
writer.levelconf.growDyn=1

[deB:cDeltaRegression]
reader.dmLevel = lldB_smo
writer.dmLevel = lldB_smo_de
writer.levelconf.isRb=0
writer.levelconf.growDyn=1



; OUTPUTS: lldA_smo;lldA_smo_de;lldB_smo;lldB_smo_de;lld_nzsmo;lld_nzsmo_de
;  optional for pitch functionals: lld_f0v_smo;lld_f0p_smo

