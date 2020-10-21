///////////////////////////////////////////////////////////////////////////////////////
/////////     openSMILE arff targets generic config file             //////////////////
///////// (c) audEERING UG (haftungsbeschränkt),                     //////////////////
/////////     All rights reserved.                                  //////////////////
///////////////////////////////////////////////////////////////////////////////////////


//
// configuration of commandline options for target classes in an ARFF file
// (cArffSink)
//
// change this file to match your labels!
//

class[0].name = class
// use this for a nominal label (discrete classes)
// class[0].type = {classA,classB,classC}  
class[0].type = \cm[classtype{numeric}:type of labelA, either numeric or a list of comma separated classes in curly brackets]
target[0].all = \cm[class{?}:string value for labelA]

/*
class[1].name = labelB
class[0].type = \cm[labelBtype{numeric}:type of labelB, either numeric or a list of comma separated classes in curly brackets]
target[1].all = \cm[labelB{0.0}:numeric value for labelB]
// you can add as many labels here as you want
*/

