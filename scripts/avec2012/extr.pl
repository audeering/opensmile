use File::Basename;

##
## openSMILE audio feature extraction script
## for AVEC 2012 challenge
## written by Florian Eyben, TUM, MMK. All rights reserved.
##  (c) 2012-2013, TUM, MMK
##  (c) 2014 audEERING UG (limited)                           
##    All rights reserved, see file COPYING for license terms.
##
## This script was written for Linux, but should be portable
## to windows with a few modifications.
## You need "sox" to convert wave files to a format that openSMILE
## can read (the wavext header is currently not yet supported)
##

# IMPORTANT: when running multiple instances of this script
#  at the same time, please run from a different directory
#  as the scripts will use the same filenames in the current
#  directory, and thus interefere with each other!!

# path to openSMILE binary
$OS = "../opensmile/inst/bin/SMILExtract";

# if set to 1, don't do extraction, just print details
$debugprint = 0;

# incremental functionals window parameters (.segments.arff) in seconds
$samplingStep = 0.5;
$samplingSize = 2.0;

# the affective dimensions used in the challenge
@dimensions = ("arousal", "expectancy", "power", "valence");

# on which sets to run: use one of  devel, train, tests
@sets = ("devel", "train", "tests");

$label_samplingrate = 0.02;  # in seconds

# temporary files created in the working directory
$framelistfile = ".tmp.frame.list";
$labelincludefile = "labels.inc";

foreach $set (@sets) {

###################################################

$base = ".";
$setpath = "../audio/$set";
# output directory
$arffbase = "../arff";
$labelspath = "$setpath/labels";
mkdir($arffbase);

$conf_full = "$base/avec2011_full.conf";
$conf_list = "$base/avec2011_list.conf";
$arff_words = "$arffbase/$set\_words.arff";
$arff_phrases = "$arffbase/$set\_phrases.arff";
$arff_segments = "$arffbase/$set\_segments.arff";
$lldarff = "$arffbase/$set\_lld.arff";

sub deleteArffs {
  my $a = $_[0];
  $a =~ s/\.arff$/*.arff/;
  my @f = glob("$a");
  foreach $F (@f) {
    unlink($F);
  }
}

# delete existing files, as opensmile will append to them
&deleteArffs($arff_words);
&deleteArffs($arff_phrases);
&deleteArffs($arff_segments);
&deleteArffs($lldarff);

sub validate_word_duration {
  # old avec2011 algo for forcing minimum word length of 120ms by iteratively
  # increasing the length on both sides by 10ms steps
  my $start = ${$_[0]}[0];
  my $end = ${$_[0]}[1];
  while ($end - $start < 0.25) {
        $start -= 0.01;
        $end += 0.01;
  }
  ${$_[0]}[0] = $start;
  ${$_[0]}[1] = $end;
}

@wds = glob("$setpath/words/*.txt");
foreach $wordfile (@wds) {

print "==== wordfile: $wordfile ===\n";

open(F,"<$wordfile");
my @words;
my @segment;
my @phrase;
$n = 0; $ns=0; $np=0;
$t0 = 0;
$s0 = -1;
$p0 = -1;
$curpause = 1.0;
while(<F>) {
  chomp;
  $_=~s/^\s+//;
  $_=~s/\s+$//;
  @x = split(/\s+/,$_);
  $x[0] /= 1000;
  $x[1] /= 1000;
  $pause = $x[0] - $t0;
  $words[$n][0] = $x[0];
  $words[$n][1] = $x[1];
  &validate_word_duration($words[$n]);
  $n++;
  if ($pause < 0) { $pause = 0.0; }
  if ($pause > 1.0 && $s0 != -1) { # begin a new segment
    $segment[$ns][0] = $s0-0.2;
    $segment[$ns][1] = $t0+0.2;
    $s0 = -1; $ns++;
  }
  if ($pause > $curpause && $p0 != -1) { # begin a new phrase
    $phrase[$np][0] = $p0-0.2;
    $phrase[$np][1] = $t0+0.2;
    $p0 = -1; $np++; $curpause = 1.0;
  }
  if ($s0 == -1) { $s0 = $x[0]; }
  if ($p0 == -1) { $p0 = $x[0]; }
  $curpause -= ($x[1]-$t0)/5.0;
  if ($curpause < 0.0) { $curpause = 0.0; }
  $t0 = $x[1];
}
close(F);

# end the segment and phrase
$segment[$ns][0] = $s0-0.25;
$segment[$ns][1] = $t0+0.25;
$phrase[$np][0] = $p0-0.25;
$phrase[$np][1] = $t0+0.25;
$np++; $ns++;

sub recording_ID_from_basename {
  my $basename = shift;
  my $ID = "XXX";
  if ($basename =~ /(\d+)\.wav$/i) {
    $ID = $1;
  }
  return $ID;
}

# words, phrases: full turn conf, w. start/end
# segments: incremental conf, with start end of turn, and framelist for segs

sub runsmile {
  my $start = shift;
  my $end = shift;
  my $wave = shift;
  my $arff = shift;
  my $conf = shift;
  my $nn = shift;
  my $labelinc = shift;
  my $lldarff = shift;
  my $framelistfile = shift;
  my $extra = "";
  if ($start < 0) { $start = 0; }
  if ($end < $start) { return; }
  my $basename = basename($wave); 
  my $ID = &recording_ID_from_basename($basename);
  $basename =~ s/\.wav$//i;
  $arff =~ s/\.arff$/$ID.arff/i;
  if ($lldarff) {
    $extra .= "-lldArff \"$lldarff\" ";
    $extra .= "-instnameLld \"$basename.seg$nn.start$start\" ";
  }
  if ($framelistfile) {
    $extra .= "-frameListFile \"$framelistfile\"";
  }
  print "  File $wave - start=$start - end=$end\n";
  print("  $OS -l 1 -C \"$conf\" -start $start -end $end -I \"$wave\" -O \"$arff\" -instname \"$basename.seg$nn.start$start\" $extra\n");
  if ($debugprint) { return; }
  $ret = system("$OS -l 1 -C \"$conf\" -start $start -end $end -I \"$wave\" -O \"$arff\" -instname \"$basename.seg$nn.start$start\" $extra");
  if ($ret) { exit -1; }
}

$wave = $wordfile;
$wave =~ s/.txt$/.wav/;
$wave =~ s/words\//wav\//;
$wave =~ s/_wordTimings/_audio/;

$wavebase = basename($wave);
system("sox \"$wave\" -c 1 -2 -s ./$wavebase");
$wave = "$wavebase";

my %labels;
my $nolabels = 0;

sub load_labels_file {
  my $basename = shift;
  my $ID = &recording_ID_from_basename($basename);
  my $dim;
  foreach $dim (@dimensions) {
    my $labels_file = "$labelspath/labels_continuous_$set$ID\_$dim.dat";
    unless (-e $labels_file) {
      print "ERROR: label file $labels_file does not exist.\n  This is ok, if you're extracting features for the tests,\n  the target values in the arff files will all be set to 0.0 in this case.\n";
      return 0;
    }
    my $n = 0;
    open(LF, "<$labels_file");
    while(<LF>) {
      chomp;
      $_=~s/^\s+//; $_=~s/\s+$//;
      $labels{$dim}[$n] = $_;
      $n++;
    }
    close(LF);
  }
  return 1;
}

my $have_labels = &load_labels_file($wavebase);

sub compute_label_for_segment {
  my $start = shift;
  my $end = shift;
  my $dim = shift;
  my $i;
  my $labelsum = 0.0;
  my $start_i = int($start / $label_samplingrate);
  my $end_i = int($end / $label_samplingrate);
  for ($i = $start_i; $i <= $end_i; $i++) {
    $labelsum += $labels{$dim}[$i];
  }
  if ($end_i > $start_i) {
    $labelsum /= ($end_i - $start_i + 1);
  }
  return $labelsum;
}

sub compute_label_string_for_segment {
  my $start = shift;
  my $end = shift;
  my $have_labels = shift;
  my $label_string = "";
  if ($have_labels) {
    foreach $dim (@dimensions) {
      my $label = &compute_label_for_segment($start, $end, $dim);
      $label_string .= "-label_$dim \"$label\"";
      ## TODO: negative labels will break openSMILE option parsing!!
      # Solution: have this script generate an includable config file with the targets (or one file for each dimension)
    }
  }
  return $label_string;
}

sub generate_label_include_for_segment {
  my $start = shift;
  my $end = shift;
  my $have_labels = shift;
  my $segment_num = shift;
  my $label_string = "";
  if ($have_labels) {
    my $d = 0;
    foreach $dim (@dimensions) {
      my $label = &compute_label_for_segment($start, $end, $dim);
      if ($segment_num) {
        $label_string .= "target[$d].instance[".($segment_num - 1)."] = $label\n";
      } else {
        $label_string .= "target[$d].all = $label\n";
      }
      $d++;
    }
  } else {
    my $d = 0;
    foreach $dim (@dimensions) {
      if ($segment_num) {
        $label_string .= "target[$d].instance[".($segment_num - 1)."] = 0.0\n";
      } else {
        $label_string .= "target[$d].all = 0.0\n";
      }
      $d++;
    }
  }
  return $label_string;
}

# words
print "Num words: $n\n";
for ($i=0; $i<$n; $i++) {
  my $label_str = &generate_label_include_for_segment($words[$i][0], $words[$i][1], $have_labels);
  print "  word $i : $words[$i][0] - $words[$i][1]\n";
  open(F,">$labelincludefile");
  print F $label_str."\n";
  close(F);
  &runsmile($words[$i][0], $words[$i][1], $wave, $arff_words, $conf_full, $i, $labelincludefile);
}

# phrases
if(0) {
print "Num phrases: $np\n";
for ($i=0; $i<$np; $i++) {
  my $label_str = &generate_label_include_for_segment($phrase[$i][0], $phrase[$i][1], $have_labels);
  print "  phrase $i : $phrase[$i][0] - $phrase[$i][1]\n";
  open(F,">$labelincludefile");
  print F $label_str."\n";
  close(F);
  &runsmile($phrase[$i][0], $phrase[$i][1], $wave, $arff_phrases, $conf_full, $i, $labelincludeile);
}
}

# segments
print "Num segments: $ns\n";
for ($i=0; $i<$ns; $i++) {
  # generate frame list
  $len = $segment[$i][1] - $segment[$i][0];
  $framelist = "";
  my $label_str = "";
  my $n = 0;
  if ($len > ($samplingSize-$samplingStep+0.05)) {
  for ($j=0; $j<$len-($samplingSize-$samplingStep); $j+=$samplingStep) {
    $st = $j;
    $ed = $st+$samplingSize;
    if ($ed > $len-0.05) { $ed = $len-0.05; }
    $framelist .= $st."s-".$ed."s,";
    $label_str .= &generate_label_include_for_segment($segment[$i][0] + $st, $segment[$i][0] + $ed, $have_labels, $n + 1);
    $n++;
  }
  } else {
    $framelist = "0s-$len"."s,";
    $label_str .= &generate_label_include_for_segment($segment[$i][0] + $st, $segment[$i][0] + $ed, $have_labels, $n + 1);
    $n++;
  }
  chop($framelist);
  
  open(F,">$labelincludefile");
  print F $label_str."\n";
  close(F);
  open(F,">$framelistfile");
  print F $framelist;
  close(F);
  print "  segment $i : $segment[$i][0] - $segment[$i][1]\n";
  print "    FRAMELIST($ns, $len): $framelist\n";
  &runsmile($segment[$i][0], $segment[$i][1], $wave, $arff_segments, $conf_list, $i, $labelincludefile, $lldarff, $framelistfile);
}
unlink($framelistfile);
unlink("$wavebase");

}
}


