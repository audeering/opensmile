///////////////////////////////////////////////////////////////////////////////////////
///////// > openSMILE LSTM-RNN voice activity detector<              //////////////////
/////////                                                            //////////////////
///////// (c) audEERING UG (haftungsbeschränkt),                     //////////////////
/////////     All rights reserved.                                  //////////////////
///////////////////////////////////////////////////////////////////////////////////////




;; turn detector configuration module

[componentInstances:cComponentManager]
instance[turn].type=cTurnDetector

[turn:cTurnDetector]
reader.dmLevel=vad_VAD_voice
writer.dmLevel=isTurn
readVad=1
threshold = -0.1
threshold2 = -0.1
writer.levelconf.noHang=1
msgInterval = 0
messageRecp = waveSinkCut
eventRecp = waveSinkCut
statusRecp = waveSinkCut
debug=\cm[turndebug{4}:set this to 1 to see turn/speaking debug messages]
;; examples for constraining the turn length
;; minTurnLengthTurnFrameTimeMessage=0.9
;; maxTurnLength=10.0
; default (0) is infinite maximum length
maxTurnLength=0
maxTurnLengthGrace=1
nPre = 8
nPost = 35


