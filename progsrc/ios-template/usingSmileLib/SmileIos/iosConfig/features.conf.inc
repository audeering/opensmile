// This file is part of openSMILE.
// (c) 2016 by audEERING GmbH. All rights reserved.
// See file COPYING for details on licensing terms.
//
// Written by Florian Eyben. fe@audeering.com

[componentInstances:cComponentManager]
instance[framesB].type=cFramer
instance[energyMeter].type=cEnergy
instance[winB].type=cWindower
instance[fftB].type=cTransformFFT
instance[fftmagB].type=cFFTmagphase
instance[bandspec].type=cMelspec

[framesB:cFramer]
reader.dmLevel = wave
writer.dmLevel = framesB
\{\cm[bufferModeRbConf{BufferModeRb.conf.inc}:buffer mode for pre-functionals levels]}
frameSize = 0.025
frameStep = 0.010

[energyMeter:cEnergy]
reader.dmLevel=framesB
writer.dmLevel=energyB
log=1
rms=0
ebiasLog = 30
escaleLog = 12

[winB:cWindower]
reader.dmLevel=framesB
writer.dmLevel=windowsB
copyInputName = 1
processArrayFields = 1
 ; hamming window
winFunc = ham

[fftB:cTransformFFT]
reader.dmLevel=windowsB
writer.dmLevel=fftB
copyInputName = 1
processArrayFields = 1
inverse = 0

[fftmagB:cFFTmagphase]
reader.dmLevel=fftB
writer.dmLevel=fftmagB
copyInputName = 1
processArrayFields = 1
inverse = 0
magnitude = 1
phase = 0

[fftmagBphase:cFFTmagphase]
reader.dmLevel=fftB
writer.dmLevel=fftmagBphase
copyInputName = 1
processArrayFields = 1
inverse = 0
magnitude = 0
phase = 1

[bandspec:cMelspec]
reader.dmLevel=fftmagB
writer.dmLevel=melspec
copyInputName = 1
processArrayFields = 1
htkcompatible = 0
nBands = 16
usePower = 1
lofreq = 0
hifreq = 8000
specScale = bark
inverse = 0


