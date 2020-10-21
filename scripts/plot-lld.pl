use strict;

my $wav = $ARGV[0];
my $lld = $ARGV[1];
my $feature = $ARGV[2];
my $feature2 = $ARGV[3];

my $tmp = "tmp.csv";

open(F, "<$lld");
open(O, ">$tmp");
my $head = <F>;
$head =~ s/\r?\n$//;
my @heads = split(/;/, $head);
my $n = 0;
my $n2 = -1;
for (my $i = 1; $i <= $#heads; $i++) {
  if ($heads[$i] eq $feature) {
    $n = $i;
  }
  if ($feature2 && $heads[$i] eq $feature2) {
    $n2 = $i;
  }
}
while(<F>) {
  $_=~s/\r?\n$//;
  my @x = split(/;/, $_);
  if ($n2 < 0) {
    print O $x[1]." ".$x[$n]."\n";
  } else {
    print O $x[1]." ".$x[$n]." ".$x[$n2]."\n";
  }
}
close(O);

if ($n2 < 0) {
  system("sonic-visualiser \"$wav\" \"$tmp\"");
} else {
  system("sonic-visualiser \"$wav\" \"$tmp\" \"$tmp\"");
}

