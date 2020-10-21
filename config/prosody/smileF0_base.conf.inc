///////////////////////////////////////////////////////////////////////////////////////
///////// > openSMILE config for SHS viterbi smoothed pitch <        //////////////////
/////////                                                            //////////////////
///////// (c) 2013-2016 audEERING.                                   //////////////////
/////////     All rights reserved. See file COPYING for details.    //////////////////
///////////////////////////////////////////////////////////////////////////////////////

//  output frame rate: 10ms
//  analysis window length : 60ms, gaussian window

;;;;;;; component list ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

[componentInstances:cComponentManager]
instance[dataMemory].type=cDataMemory
printLevelStats=0


;;;;;;;;;;;;;;;;;;;;;;;;;;;; main section ;;;;;;;;;;;;;;;;;;;;;;;;;;;

[componentInstances:cComponentManager]
instance[waveIn].type=cWaveSource

[waveIn:cWaveSource]
writer.dmLevel=wave
filename=\cm[inputfile(I){test.wav}:name of input file]
monoMixdown=1

;;;;;;;;;;;;;;;;;;;;;;

[componentInstances:cComponentManager]
instance[frame60].type=cFramer
instance[win60].type=cWindower
instance[fft60].type=cTransformFFT
instance[fftmp60].type=cFFTmagphase
instance[energy60].type=cEnergy

[frame60:cFramer]
reader.dmLevel=wave
writer.dmLevel=frame60
frameSize = 0.050
frameStep = 0.010
frameCenterSpecial = center


[win60:cWindower]
reader.dmLevel=frame60
writer.dmLevel=winG60
winFunc=gauss
gain=1.0
sigma=0.4

[fft60:cTransformFFT]
reader.dmLevel=winG60
writer.dmLevel=fftcG60

[fftmp60:cFFTmagphase]
reader.dmLevel=fftcG60
writer.dmLevel=fftmagG60

[energy60:cEnergy]
reader.dmLevel=winG60
writer.dmLevel=e60
rms=1
log=0
writer.levelconf.nT=100

[componentInstances:cComponentManager]
instance[scale].type=cSpecScale
instance[shs].type=cPitchShs
instance[pitchSmooth].type=cPitchSmootherViterbi


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
writer.dmLevel=pitchG60
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
; old parameters for greedyPeakAlgo=0
;wTvv=10.0
;wTvuv=25.0
;wRange=2.0

[componentInstances:cComponentManager]
instance[smoF0].type=cContourSmoother
instance[f0Selector].type=cDataSelector
instance[volmerge].type = cValbasedSelector
instance[f0Selector2].type=cDataSelector

[smoF0:cContourSmoother]
reader.dmLevel = pitchG60
writer.dmLevel = pitchSmo
writer.levelconf.isRb=0
writer.levelconf.growDyn=1
nameAppend = sma
copyInputName = 1
noPostEOIprocessing = 0
smaWin = 2
noZeroSma = 1

[f0Selector:cDataSelector]
reader.dmLevel = pitchSmo
writer.dmLevel = F0a
// nameAppend = 
copyInputName = 1
// selFile = 
selected[0] = F0final_sma
// selectedRange[] = 
// newNames[] = 
elementMode = 1
//dummyMode =1

[volmerge:cValbasedSelector]
reader.dmLevel = e60;F0a
writer.dmLevel = F0cl
idx=0
threshold=0.0008
removeIdx=0
zeroVec=1
outputVal=0.0

[f0Selector2:cDataSelector]
reader.dmLevel = F0cl
writer.dmLevel = F0
// nameAppend = 
copyInputName = 1
// selFile = 
selected[0] = F0final_sma
// selectedRange[] = 
// newNames[] = 
elementMode = 1
//dummyMode =1


