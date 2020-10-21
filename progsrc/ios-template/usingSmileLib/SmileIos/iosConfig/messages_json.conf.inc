// This file is part of openSMILE.
// (c) 2016 by audEERING GmbH. All rights reserved.
// See file COPYING for details on licensing terms.
//
// Written by Florian Eyben. fe@audeering.com

[componentInstances:cComponentManager]
instance[bandEnergy00].type=cSimpleMessageSender
instance[bandEnergy01].type=cSimpleMessageSender
instance[bandEnergy02].type=cSimpleMessageSender

[bandEnergy00:cSimpleMessageSender]
reader.dmLevel = melspec;energyB
sendPeriodically = 2
messageName = energy_act
messageType = energy
messageRecp = externalMessageInterface00
enableDebugSender = 0

[bandEnergy01:cSimpleMessageSender]
reader.dmLevel = melspec;energyB
sendPeriodically = 2
messageName = energy_act
messageType = energy
messageRecp = externalMessageInterface01
enableDebugSender = 0

[bandEnergy02:cSimpleMessageSender]
reader.dmLevel = melspec;energyB
sendPeriodically = 2
messageName = energy_act
messageType = energy
messageRecp = externalMessageInterface02
enableDebugSender = 0
