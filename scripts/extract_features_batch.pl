#############################################################
# Generic batch feature extraction script.
# Requires perl to run.
# Calls openSMILE on multiple files in a directory.
# 
# This file is part of openSMILE.
# (c) 2016 by audEERING.
# All rights reserved. See file COPYING for details.
#############################################################


use strict;
use FileHandle;
use File::Basename;

### change these variables to match your setting
my $openSMILE_root = "..";
my $SMILExtract = "$openSMILE_root/SMILExtract";
my $config = "$openSMILE_root/config/IS13_ComParE.conf";
my $corpus = "corpus_name";
## glob expression to list all wave files
my $wavs = "/path/to/*.wav";

## NOTE: comment out these two lines once the above settings have been changed...
print("\nThis script has no parameters.\nPlease edit the variables in the code to fit your configuration.\n\n");
exit 1;
###

# see load_labels documentation for details (loading labels from a list file)
# my $labels = "path to text based label file....";  

####
my $arff_all = "$corpus-all.arff";
## for data-set partitioning based on filename (see below)
#my $arff_train = "$corpus-train.arff";
#my $arff_devel = "$corpus-devel.arff";
#my $arff_test = "$corpus-test.arff";

###


## example function to load labels from
## a ';' separated table file
##   first line is a header with field names
##   first column is the filename (wav)
##   second column is the label
sub load_labels {
  my $labels = shift;
  my $F = FileHandle->new();
  open($F, "<$labels");
  my $head = <$F>;
  $head =~ s/\r?\n$//;
  my @headEl = split(/;/, $head);
  my %labs = ();
  while(<$F>) {
    my $line = $_;
    $line =~ s/\r?\n$//;
    my @x = split(/;/, $line);
    $labs{$x[0]}{"label"} = $x[1];
    # .. can add more labels here ...
  }
  $F->close();
  return \%labs;
}


## deletes existing arff files, so that openSMILE does not append to them.
unlink($arff_all);
#unlink($arff_train);
#unlink($arff_devel);
#unlink($arff_test);

## enable this to load labels...
#my $lab = load_labels($labels);
#my @labs = keys %{$lab};

## use this to iterate through labels....
#foreach my $l (@labs) {
#  my $wav = $wavs."/".$l;

## alternatively we iterate through the list of wave files
my @Wavs = glob($wavs);
foreach my $wav (@Wavs) {
  my $l = basename($wav);
####
  my $arff = $arff_all;
  ### example for splitting in 3 partitions based on filename expression
  #if ($l =~ /^training_/) {
  #  $arff = $arff_train;
  #}
  #if ($l =~ /^validation_/) {
  #  $arff = $arff_devel;  
  #}
  #if ($l =~ /^test_/) {
  #  $arff = $arff_test;
  #}
  if ($arff) {
    my $targets = "";
    ## this is an example for adding labels loaded from a text file directly into the arff file:
    ## the options -class and -classtype must be defined in the given arff_targets.conf.inc file,
    ## this file allows to define multiple such options for a multi-target arff file.
    # my $targets = "-arfftargetsfile ./arff_targets.conf.inc";
    # $targets .= " -class ".$lab->{$l}{"label"};
    # $targets .= " -classtype {emotional,neutral}";
    #################
    my $inst = $l;
    ## if you get errors that the wave file format cannot be read by openSMILE,
    ## you first need to convert your audio files with the open-source tool 'sox'.
    ##   e.g. sox -R $wav -c 1 -e signed-integer -b 16 tmp-output.wav
    ## then set $wav = "tmp-output.wav"
    ## and don't forget to delete tmp-output.wav at the end...
    my $cmd = "$SMILExtract -nologfile -l 2 -C \"$config\" -I \"$wav\" -O \"$arff\" -instname \"$inst\" $targets";
    print "running: $cmd\n";
    my $ret = system($cmd);
    if ($ret) {
      print "ERROR: $cmd\n";
      exit -1;
    }
  }
}

