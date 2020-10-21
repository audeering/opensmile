#!/usr/bin/perl

# This file is part of openSMILE.
#
# Copyright (c) audEERING. All rights reserved.
# See the file COPYING for details on license terms.

# script to clone an openSMILE component
#   .cpp and .hpp files are copied and the classname is updated

if ($#ARGV <3 ) { 
  print "USAGE: clonecomp.pl <inputCompBase> <outputCompBase> <inputCompName> <outputCompName>\n";
  exit;
}

$src="../src";

$in=$ARGV[2];
$on=$ARGV[3];
$inT=$in;
$inT=~tr/a-z/A-Z/;
$onT=$on;
$onT=~tr/a-z/A-Z/;
$if=$ARGV[0];
$of=$ARGV[1];

print "$in (in)\n";
print "$on (out)\n";

@ex=("cpp","hpp");
foreach $ext (@ex) {

my $pref = "";
if ($ext =~ /h/) {
  $pref = "include/";
}

open(IN,"<$src/$pref".$ARGV[0].".$ext");
open(OUT,">$src/$pref".$ARGV[1].".$ext");
while(<IN>) {
  my $line=$_;
  $line =~ s/$in/$on/g;
  $line =~ s/_$inT/_$onT/;
  if ($ext eq "cpp") {
    $line =~ s/<$if.hpp>/<$of.hpp>/;
  } else {
    $onTh = $onT."_HPP";
    $line =~ s/^#ifndef __.+?_HPP/#ifndef __$onTh/;
    $line =~ s/^#define __.+?_HPP/#define __$onTh/;
    $line =~ s/^#endif *\/\/ *__.+?_HPP/#endif \/\/ __$onTh/;
  }
  print OUT $line;
}
close(OUT);
close(IN);

}
