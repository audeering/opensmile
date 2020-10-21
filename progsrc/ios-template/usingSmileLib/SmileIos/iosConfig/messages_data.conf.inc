// This file is part of openSMILE.
// (c) 2016 by audEERING GmbH. All rights reserved.
// See file COPYING for details on licensing terms.
//
// Written by Florian Eyben. fe@audeering.com

[componentInstances:cComponentManager]
instance[externalSink00].type=cExternalSink
instance[externalSink01].type=cExternalSink
instance[externalSink02].type=cExternalSink

[externalSink00:cExternalSink]
reader.dmLevel = melspec;energyB

[externalSink01:cExternalSink]
reader.dmLevel = melspec;energyB

[externalSink02:cExternalSink]
reader.dmLevel = melspec;energyB


