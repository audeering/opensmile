///////////////////////////////////////////////////////////////////////////////////////
///////// > openSMILE configuration file for wave input <            //////////////////
/////////                                                            //////////////////
///////// (c) audEERING GmbH,                                        //////////////////
/////////     All rights reserved.                                  //////////////////
///////////////////////////////////////////////////////////////////////////////////////



[componentInstances:cComponentManager]
instance[waveIn].type=cPortaudioSource

[waveIn:cPortaudioSource]
writer.dmLevel=wave
buffersize_sec = 5.0
device = 5
listDevices = 0
audioBuffersize_sec = 0.2
sampleRate = 16000

