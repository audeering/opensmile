#!/usr/bin/perl
$do_standardise = 0; # TODO: implement speaker standardisation...
$do_select = 0;
$regression = 0;  # 0: classification model , 1: regression model

#--------------
use File::Basename;
require "arff-functions.pl";

#print "Operating system: $^O\n\n";
if ($#ARGV < 0) {
  print "\nUsage: perl makemodel.pl <corpus_path ¦ arff file (must end in .arff)>  [SMILExtract config for corpus mode] [SVM Config]\n\n";
  exit -1;
}

# check if we run on windows...
if ($^O =~ /win/i) {
  $ext = ".exe";
  unless (-f "libsvm-small/svm-train$ext") {
    print "ERROR: The libsvm binaries in scripts/modeltrain/libsvm-small \n";
    print "       cannot be found (svm-train.exe, svm-scale.exe, ...)\n";
    print "       Please compile them first, or get a new openEAR realease\n";
    print "       which contains the binaries (or contact the authors!)\n";
    exit;
  }
} else {
  $ext = "";
  # check for libsvm binaries...
  unless (-f "libsvm-small/svm-train") {
    # assume binaries have not yet been compiled
    print "WARNING: you have not yet compiled the LibSVM binaries!\n";
    print "  I will try to compile, this should work, if you have \n";
    print "  called this script from the 'scripts/modeltrain'\n";
    print "  directory. \n";
    print "  If you get errors in the following output, please do \n";
    print "  the following manually (relative to openEAR root path): \n";
    print "    cd scripts/modeltrain/libsvm-small \n";
    print "    make \n";
    $ret = system("cd libsvm-small ; make");
    if ($ret == 0) {
      print "LibSVM built successfully!\n";
    } else {
      print "FAILED building LibSVM!!\n";
      exit -1;
    }
  }
}
$xtract = "../../SMILExtract$ext";

mkdir ("built_models");
mkdir ("work");
$corp = $ARGV[0];  # full path to corpus OR direct arff file...
if ($corp =~ /\.arff$/i) {
  $mode="arff";
  unless (-f "$corp") {
    print "ERROR '$corp' is not an ARFF file or does not exist!\n";
    exit;
  }
  $cname=basename($corp);
  $cname =~s/\.arff$//i;
  $arff = $corp;
  $modelpath = "built_models/$cname";
  $workpath = "work/$cname";
  $svmConfig = $ARGV[1];
} else {
  $mode = "corp";
  unless (-d "$corp") {
    print "ERROR '$corp' is not a corpus directory or does not exist!\n";
    exit;
  }
  if ($corp =~ /\/([^\/]+)_FloEmoStdCls/) {
    $cname = $1;
  } else {
    $cname = basename($corp);
  }
  $conf = $ARGV[1];
  $svmConfig = $ARGV[2];
  unless ($conf) { $conf = "is09s.conf"; }
  $cb=$conf; $cb=~s/\.conf$//;
  $modelpath = "built_models/$cname";
  $workpath = "work/$cname";
  $arff = "$workpath/$cb.arff";
}

  mkdir("$modelpath");
  mkdir("$workpath");


#extract features
if ($mode eq "corp") {
  print "-- Corpus mode --\n  Running feature extraction on corpus '$corp' ...\n";
  system("perl stddirectory_smileextract.pl \"$corp\" \"$conf\" \"$arff\"");
} else {
  print "-- Arff mode --\n  Copying '$arff' to work directory ...\n";
  $arffb = basename($arff);
#  print("cp $arff $workpath/$arffb");
  system("cp $arff $workpath/$arffb");
  $arff = "$workpath/$arffb";
}

# ? standardise features
if ($do_standardise) {
 print "NOTE: standardsise not implemented yet, svm-scale will do the job during building of model\n";
}

# ? select features
$lsvm=$arff; $lsvm=~s/\.arff$/.lsvm/i;
if ($do_select) {
 print "Selecting features (CFS)... (this requires weka)\n";
 $fself = $arff;
 $fself=~s/\.arff/.fselection/i;
 system("perl fsel.pl $arff");
 $arff = "$arff.fsel.arff";
 # TOOD: remove index fields, convert to LSVM, re-use code from below..
 system("perl arffToLsvm.pl $arff $lsvm");
} else {
  $fself="";
  print "Converting arff to libsvm feature file (lsvm) ...\n";
  # convert to lsvm
  my $hr = &load_arff($arff);
  my $numattr = $#{$hr->{"attributes"}};
  if ($hr->{"attributes"}[0]{"name"} =~ /^name$/) {
    $hr->{"attributes"}[0]{"selected"} = 0;  # remove filename
  }
  if ($hr->{"attributes"}[0]{"name"} =~ /^filename$/) {
    $hr->{"attributes"}[0]{"selected"} = 0;  # remove filename
  }
  if ($hr->{"attributes"}[1]{"name"} =~ /^timestamp$/) {
    $hr->{"attributes"}[1]{"selected"} = 0;  # remove filename
  }
  if ($hr->{"attributes"}[1]{"name"} =~ /^frameIndex$/) {
    $hr->{"attributes"}[1]{"selected"} = 0;  # remove filename
    if ($hr->{"attributes"}[2]{"name"} =~ /^frameTime$/) {
      $hr->{"attributes"}[2]{"selected"} = 0;  # remove filename
    }
  }
  if ($hr->{"attributes"}[1]{"name"} =~ /^frameTime$/) {
    $hr->{"attributes"}[1]{"selected"} = 0;  # remove filename
  }
   #$hr->{"attributes"}[$numattr-1]{"selected"} = 0; # remove continuous label
  &save_arff_AttrSelected($arff,$hr);
  system("perl arffToLsvm.pl $arff $lsvm");

  # generate fsel file with list of features:
  $outpArff=$arff;
  $feaTUMfselfile=basename($arff);
  $feaTUMfselfile=~ s/\.arff/.fselection/;
  $feaTUMfselfile="work/$feaTUMfselfile";

# count number of features first
$nStr = 0; $haveNom = 0;
open(FILE,"<$outpArff");
while (<FILE>) {
  my $line = $_; chop($line);
  if ($line =~ /^\@attribute (.+?) numeric/) {
    $nStr++;
  }
  if ($line =~ /^\@attribute (.+?) \{?.+,.+\}?/) {
    $haveNom=1;
  }
  if ($line =~ /^\@data/) { last; }
}
close(FILE);

if ($haveNom==0) { $nStr--; }
else { print "haveNom\n"; }

#default : skip the last numeric attribute, if no nominal attribute is present
$nNumCur=0;
open(OUT,">$feaTUMfselfile");
print OUT "str\n";
print OUT "$nStr\n";
open(FILE,"<$outpArff");
while (<FILE>) {
  my $line = $_; chop($line);
  if ($line =~ /^\@attribute (.+?) numeric/) {
    $attr = $1;
    $nNumCur++;
#    print "selecting: $attr \n";
    if ($haveNom==1) {
      print OUT "$attr\n";
    } elsif (($nNumCur <= $nStr)) {
      print OUT "$attr\n";
    }
  }
  if ($line =~ /^\@data/) { last; }
}
close(FILE);
close(OUT);

$fself = $feaTUMfselfile;

}

# train model
print "Training libsvm model ...\n";
system("perl buildmodel.pl $lsvm $svmConfig");

$discfile = $lsvm; $discfile=~s/\.lsvm$/.disc/;
open(FILE,"<$discfile");
$disc=<FILE>; $disc=~s/\r?\n$//;
close(FILE);

$conf = $lsvm;
$conf =~ s/\.lsvm$/.config/;

# copy final files to modelpath
$scale = $lsvm; $scale=~s/\.lsvm$/.scale/;
$model = $lsvm; $model=~s/\.lsvm$/.model/;
$cls = $lsvm; $cls=~s/\.lsvm$/.classes/;
if ($cb) { $cb = "$cb."; }

if ($do_select) { $sel="cfssel."; }
else { $sel="allft."; }

system("cp $scale $modelpath/$cb$sel"."scale");
system("cp $model $modelpath/$cb$sel"."model");
# convert to binary model
unless (-f "../../svm/modelconverter$ext") {
  print "WARNING: modelconverter not found in ../../svm ! If you are running this script on windows, you must compile the modelconverter.cpp file manually!\n";
  system("cd ../../svm ; sh compileConverter.sh");
}
system("../../svm/modelconverter $modelpath/$cb$sel"."model $modelpath/$cb$sel"."model.bin");

system("cp $conf $modelpath/$cb$sel"."config");
if ($disc) {
  system("cp $cls $modelpath/$cb$sel"."classes");
}
if ($fself) {
  system("cp $fself $modelpath/$cb$sel"."fselection");
}

