#!/usr/bin/perl

# USAGE: buildmodel.pl <training lsvm file> [svm config file]

$input_lsvm = $ARGV[0];
$conf = $input_lsvm;
$conf =~ s/\.lsvm$/.config/;

$scale = $input_lsvm; $scale=~s/\.lsvm$/.scale/; 
$scaled_lsvm = $input_lsvm; $scaled_lsvm =~ s/\.lsvm/.scaled.lsvm/;
$model = $input_lsvm; $model=~s/\.lsvm$/.model/;

if ($^O =~ /win/i) {
  $ext = ".exe";
} else {
  $ext = "";
}
 
# scale features, build model

unless (-e $scaled_lsvm) {
  system("libsvm-small/svm-scale$ext -s $scale $input_lsvm > $scaled_lsvm");
}

$discfile = $input_lsvm; $discfile=~s/\.lsvm$/.disc/;
open(FILE,"<$discfile");
$disc=<FILE>; $disc=~s/\r?\n$//;
close(FILE);

if ($disc) { # classification if disc==1

print "  building an SVM CLASSFICATION model...\n";
#classification:
## change LibSVM parameters here, if you want to use a different configuration:
## See the LibSVM documentation for more information:

if (-e $ARGV[1]) {
  $svm_config = `cat $ARGV[1]`;
  $svm_config =~ s/\n//g;
  $svm_config =~ s/\r//g;
} else {
  $svm_config = "-b 1 -s 0 -t 1 -d 1 -c 0.3";
}

#system("libsvm-small/svm-train$ext -n 0.5 -b 1 -s 1 -t 1 -d 2 -c 0.7 $scaled_lsvm $model");

# Original:
system("libsvm-small/svm-train$ext $svm_config $scaled_lsvm $model");

} else { # regression otherwise:

print "  building an SVR REGRESSION model...\n";
if (-e $ARGV[1]) {
  $svm_config = `cat $ARGV[1]`;
  $svm_config =~ s/\n//g;
  $svm_config =~ s/\r//g;
} else {
  $svm_config = "-s 3 -t 1 -d 1 -c 0.4";
}

##### regression:
## change LibSVM parameters here, if you want to use a different configuration:
## See the LibSVM documentation for more information:
system("svm-train$ext $svm_config $scaled_lsvm $model");


}

open(FILE,">$conf");
print FILE "$svm_config\n";
close(FILE);

