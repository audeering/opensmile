#!/usr/bin/perl

# convert an arff file to lSVM feature file format

$input_arff = $ARGV[0];
$output = $input_arff;
$output =~ s/\.arff$/.lsvm/;
if ($ARGV[1]) { $output = $ARGV[1]; }
$clsfile = $output;
$clsfile =~ s/\.lsvm$/.classes/;
$discfile = $output;
$discfile =~ s/\.lsvm$/.disc/;

$indata = 0;
my %cla=();
my $discrete = 0;

open(OUT,">$output");
open(FILE,"<$input_arff");
while (<FILE>) {
  my $line = $_; $line=~s/\r?\n$//;
  if ($line =~ /^\@data/) { 
    $indata = 1; 
#print "LL: '$lastline'\n";
    if ($lastline =~ /^\@attribute\s+([^\s]+)\s+(.+)$/) {
      $at = $2;
      if ($at =~ /numeric/i) {
        # no conversion necessary
        $discrete = 0;
      } else {
	$discrete = 1;
        $at =~ s/^\s*[\{\"]\s*//;
        $at =~ s/\s*[\}\"]\s*$//;
	@cls = split(/\s*,\s*/,$at);
	for ($i=0; $i<=$#cls; $i++) { $cla{$cls[$i]}  = "$i"; }
#print "here $at\n";
      }

    }
  } else {
 
  if (($indata)&&($line =~ /,/)) {
    @els = split(/,/,$line);
    $class = $els[$#els];
#print "class: $class\n";
    if ($discrete == 1) {
      print OUT $cla{$class}." ";
#    print "  $cla{$class}\n";
    } else {
      print OUT "$class ";
    }
    for ($i=0; $i<$#els; $i++) {
      $i1 = $i + 1;
    #for ($i=2; $i<$#els; $i++) {
    #  $i1 = $i - 1;
      print OUT $i1.":".$els[$i]." ";
    }
    print OUT "\n";
  }
  unless ($indata) {
    if ($line!~/^\s*$/) {
      if ($line=~/^\@attribute /) {
        $lastline = $line;
      }
    } 
  }

  }
}
close(FILE);
close(OUT);
if ($discrete) {
  open(FILE,">$clsfile");
  for ($i=0; $i<=$#cls; $i++) { print FILE $i.":".$cls[$i]."\n"; }
  close(FILE);
  open(FILE,">$discfile");
  print FILE "1\n";
  close(FILE);
} else {
  open(FILE,">$discfile");
  print FILE "0\n";
  close(FILE);
}
