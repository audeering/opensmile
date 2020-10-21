///////////////////////////////////////////////////////////////////////////////////////
///////// > openSMILE LSTM-RNN voice activity detector<              //////////////////
/////////                                                            //////////////////
///////// (c) audEERING UG (haftungsbeschränkt),                     //////////////////
/////////     All rights reserved.                                  //////////////////
///////////////////////////////////////////////////////////////////////////////////////


[componentInstances:cComponentManager]
instance[framerVAD].type = cFramer
instance[vectorPreemphasisVAD].type = cVectorPreemphasis
instance[windowerVAD].type = cWindower
 ; magnitude FFT spectrum
instance[fftVAD].type=cTransformFFT
instance[fftmagVAD].type=cFFTmagphase
 ; compute critical-bands from power spectrum
instance[melspecVAD].type=cMelspec
 ; compute PLP 1-12 from critical-band spectrum
instance[plpVAD].type=cPlp
 ; compute log-energy from raw signal frames 
 ; (not windowed, not pre-emphasised: that's the way HTK does it)
;instance[energyVAD].type=cEnergy
 ; compute delta coefficients from PLP and energy
instance[deltaVAD].type=cDeltaRegression
 ; compute acceleration coefficients from delta coefficients of PLP and energy
;instance[accelVAD].type=cDeltaRegression
 ; MVN (2x)
instance[mvnVAD].type = cVectorMVN
;; experimental MVN adaption, uncomment to enable
;; and also change reader.dmLevel of lstm_vad component
;instance[mn2VAD].type = cVectorMVN
 ; LSTM for VAD
instance[lstmVAD].type=cRnnProcessor
instance[dataSelectorVAD].type = cDataSelector

[framerVAD:cFramer]
reader.dmLevel = wave
writer.dmLevel = framesVAD
// nameAppend = 
copyInputName = 1
frameMode = fixed
// frameListFile = 
// frameList = 
frameSize = 0.025000
frameStep = 0.01
frameCenterSpecial = left
buffersize = 100
noPostEOIprocessing = 1
 
[vectorPreemphasisVAD:cVectorPreemphasis]
reader.dmLevel = framesVAD
writer.dmLevel = prframesVAD
// nameAppend = 
copyInputName = 1
processArrayFields = 1
k = 0.970000
de = 0
   
[windowerVAD:cWindower]
reader.dmLevel = prframesVAD
writer.dmLevel = winframesVAD
// nameAppend = 
copyInputName = 1
processArrayFields = 1
gain = 1
offset = 0
winFunc = ham
;sigma = 0.400000
;alpha = 0.160000

[fftVAD:cTransformFFT]
reader.dmLevel=winframesVAD
writer.dmLevel=fftVAD

[fftmagVAD:cFFTmagphase]
reader.dmLevel=fftVAD
writer.dmLevel=fftmagVAD

[melspecVAD:cMelspec]
reader.dmLevel=fftmagVAD
writer.dmLevel=melspec_powerVAD
; no htk compatible sample value scaling
htkcompatible = 0
nBands = 26
; use power spectrum instead of magnitude spectrum
usePower = 1
lofreq = 0
hifreq = 8000
specScale = mel

[plpVAD:cPlp]
reader.dmLevel=melspec_powerVAD
writer.dmLevel=plp_VAD
buffersize=100
firstCC = 1
lpOrder = 18
cepLifter = 22
compression = 0.33
htkcompatible = 0
newRASTA = 1
RASTA = 0
rastaUpperCutoff = 29.0
rastaLowerCutoff = 0.9
doIDFT = 1
doLpToCeps = 1
doLP = 1
doInvLog = 1
doAud = 1
doLog = 1

[energyVAD:cEnergy]
reader.dmLevel=winframesVAD
writer.dmLevel=energy_VAD
htkcompatible=1
rms = 0
log = 1

[deltaVAD:cDeltaRegression]
reader.dmLevel=plp_VAD
writer.dmLevel=plpde_VAD
deltawin=2
blocksize=1

[accelVAD:cDeltaRegression]
reader.dmLevel=plpde_VAD
writer.dmLevel=plpdede_VAD
deltawin=2
blocksize=1

[mvnVAD:cVectorMVN]
reader.dmLevel = plp_VAD;plpde_VAD
writer.dmLevel = plpmvn_VAD
// nameAppend =
copyInputName = 1
processArrayFields = 0
mode = transform
initFile = rplp18d_norm.dat
htkcompatible = 0
meanEnable = 1
stdEnable = 1
normEnable = 0

; experimental incremental MVN adaptation
; disabled by default
[mn2VAD:cVectorMVN]
reader.dmLevel = plpmvn_VAD
writer.dmLevel = plpmvn2_VAD
processArrayFields = 0
mode = incremental
updateMethod = fix
fixedBuffer = 30.0
meanEnable = 1
stdEnable = 0
normEnable = 0

[lstmVAD:cRnnProcessor]
reader.dmLevel = plpmvn_VAD
;; enable this, if you enable the 
;; experimental incremental adaptaion
; reader.dmLevel = plpmvn2_VAD
writer.dmLevel = vad_VAD
netfile = lstmvad_rplp18d_12.net

[dataSelectorVAD:cDataSelector]
reader.dmLevel = vad_VAD
writer.dmLevel = vad_VAD_voice
nameAppend = vadBin
copyInputName = 1
selectedRange = 0
elementMode = 1


