///////////////////////////////////////////////////////////////////////////////////////
///////// > openSMILE configuration file for wave input <            //////////////////
/////////                                                            //////////////////
///////// (c) audEERING GmbH,                                        //////////////////
/////////     All rights reserved.                                  //////////////////
///////////////////////////////////////////////////////////////////////////////////////



[componentInstances:cComponentManager]
instance[waveIn].type=cWaveSource

[waveIn:cWaveSource]
writer.dmLevel=wave
buffersize_sec = 5.0
filename=\cm[inputfile(I){test.wav}:name of input file]
start=\cm[start{0}:audio start position in seconds]
end=\cm[end{-1}:audio end position in seconds, -1 for end of file]
monoMixdown=1
outFieldName = pcm

