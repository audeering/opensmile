#!/usr/bin/perl

# This file is part of openSMILE.
#
# Copyright (c) audEERING. All rights reserved.
# See the file COPYING for details on license terms.

# locate nan in arff files (and inf) and display affected attribute

$arff=$ARGV[0];
unless ($arff) {
  print " -- locate nan in arff files (and inf) and display affected attribute --\n";
  print " -- this is useful for debugging new feature extractor components --\n";
  print "USAGE: $0 <arff_file_to_check.arff>\n";
  exit;
}

open(FILE,"<$arff");
$data=0; my @attr;
@nonzero; $N=0;
while(<FILE>) {
  my $line=$_;
  $line=~ s/\n$//; $line=~s/\r$//;
  if ($data) {
    @el = split(/,/,$line);
    $N=$#el+1;
    for ($i=0; $i<=$#el; $i++) { 
     if ($el[$i] != 0.0) {
       $nonzero[$i] = 1;
     }
     #if ($el[$i] !~ /^0\.000000e\+00/) {
     #  $nonzero[$i] = 1;
     #}
     if (($el[$i] =~ /nan/i)||($el[$i] =~ /inf/i)) {
       print "nan/inf @ # $i = $attr[$i]\n";
     }
    }
  } else {
    if($line=~/^\@data/) { $data = 1; }
    else {
      if($line=~/^\@attribute ([^ ]+) /) {
        push(@attr,$1);
      }
    }
  }
}
close(FILE);

for ($i=0; $i<$N; $i++) {
  unless ($nonzero[$i]) {
    print "all-zero: ".$attr[$i]."\n";
  }
}
