///////////////////////////////////////////////////////////////////////////////////////
///////// > openSMILE configuration file for IS09 emotion challenge< //////////////////
///////// > core features <                                          //////////////////
///////// (c) 2014 by audEERING                                      //////////////////
/////////     All rights reserved. See file COPYING for details.     //////////////////
///////////////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////////////
;
; This section is always required in openSMILE configuration files
;   it configures the componentManager and gives a list of all components which are to be loaded
; The order in which the components are listed should match 
;   the order of the data flow for most efficient processing
;
///////////////////////////////////////////////////////////////////////////////////////
[componentInstances:cComponentManager]
instance[is09_fr1].type=cFramer
instance[is09_pe2].type=cVectorPreemphasis
instance[is09_w1].type=cWindower
instance[is09_fft1].type=cTransformFFT
instance[is09_fftmp1].type=cFFTmagphase
instance[is09_mspec].type=cMelspec
instance[is09_mfcc].type=cMfcc
instance[is09_mzcr].type=cMZcr
instance[is09_acf].type=cAcf
instance[is09_cepstrum].type=cAcf
instance[is09_pitchACF].type=cPitchACF
instance[is09_energy].type=cEnergy
instance[is09_lld].type=cContourSmoother
instance[is09_delta1].type=cDeltaRegression

/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////   component configuration  ////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
; the following sections configure the components listed above
; a help on configuration parameters can be obtained with 
;  SMILExtract -H
; or
;  SMILExtract -H configTypeName (= componentTypeName)
/////////////////////////////////////////////////////////////////////////////////////////////

[is09_fr1:cFramer]
reader.dmLevel=wave
writer.dmLevel=is09_frames
\{\cm[bufferModeRbConf{../shared/BufferModeRb.conf.inc}:path to included config to set the buffer mode for the standard ringbuffer levels]}
copyInputName = 1
noPostEOIprocessing = 1
frameSize = 0.0250
frameStep = 0.010
frameMode = fixed
frameCenterSpecial = left

[is09_pe2:cVectorPreemphasis]
reader.dmLevel=is09_frames
writer.dmLevel=is09_framespe
copyInputName = 1
processArrayFields = 1
k=0.97
de = 0

[is09_w1:cWindower]
reader.dmLevel=is09_framespe
writer.dmLevel=is09_winframe
copyInputName = 1
processArrayFields = 1
winFunc = ham
gain = 1.0
offset = 0

  // ---- LLD -----

[is09_fft1:cTransformFFT]
reader.dmLevel=is09_winframe
writer.dmLevel=is09_fftc
copyInputName = 1
processArrayFields = 1
inverse = 0
 ; for compatibility with 2.2.0 and older versions
zeroPadSymmetric = 0

[is09_fftmp1:cFFTmagphase]
reader.dmLevel=is09_fftc
writer.dmLevel=is09_fftmag
copyInputName = 1
processArrayFields = 1
inverse = 0
magnitude = 1
phase = 0

[is09_mspec:cMelspec]
reader.dmLevel=is09_fftmag
writer.dmLevel=is09_mspec1
copyInputName = 1
processArrayFields = 1
htkcompatible = 1
nBands = 26
usePower = 0
lofreq = 0
hifreq = 8000
inverse = 0
specScale = mel

[is09_mfcc:cMfcc]
reader.dmLevel=is09_mspec1
writer.dmLevel=is09_mfcc1
copyInputName = 1
processArrayFields = 1
firstMfcc = 1
lastMfcc =  12
cepLifter = 22.0
htkcompatible = 1


[is09_acf:cAcf]
reader.dmLevel=is09_fftmag
writer.dmLevel=is09_acf
nameAppend = acf
copyInputName = 1
processArrayFields = 1
usePower = 1
cepstrum = 0

[is09_cepstrum:cAcf]
reader.dmLevel=is09_fftmag
writer.dmLevel=is09_cepstrum
nameAppend = acf
copyInputName = 1
processArrayFields = 1
usePower = 1
cepstrum = 1

[is09_pitchACF:cPitchACF]
  ; the pitchACF component must ALWAYS read from acf AND cepstrum in the given order!
reader.dmLevel=is09_acf;is09_cepstrum
writer.dmLevel=is09_pitch
copyInputName = 1
processArrayFields=0
maxPitch = 500
voiceProb = 1
voiceQual = 0
HNR = 0
F0 = 1
F0raw = 0
F0env = 0
voicingCutoff = 0.550000

[is09_energy:cEnergy]
reader.dmLevel=is09_winframe
writer.dmLevel=is09_energy
nameAppend=energy
rms=1
log=0

[is09_mzcr:cMZcr]
reader.dmLevel=is09_frames
writer.dmLevel=is09_mzcr
copyInputName = 1
processArrayFields = 1
zcr = 1
amax = 0
mcr = 0
maxmin = 0
dc = 0

[is09_lld:cContourSmoother]
reader.dmLevel=is09_energy;is09_mfcc1;is09_mzcr;is09_pitch
writer.dmLevel=is09_lld
\{\cm[bufferModeConf{../shared/BufferMode.conf.inc}:path to included config to set buffer mode for all LLD that feed into functionals]}
nameAppend = sma
copyInputName = 1
noPostEOIprocessing = 0
smaWin = 3

// ---- delta regression of LLD ----
[is09_delta1:cDeltaRegression]
reader.dmLevel=is09_lld
writer.dmLevel=is09_lld_de
\{\cm[bufferModeConf]}
nameAppend = de
copyInputName = 1
noPostEOIprocessing = 0
deltawin=2
blocksize=1

