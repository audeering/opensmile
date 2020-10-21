#!/usr/bin/perl


# load train.arff in weka and so CFS
# convert resulting arff to openEAR feature selection file

$input_arff = $ARGV[0];

##### CHANGE THIS TO YOUR WEKA PATH
$wekapath = "\$CLASSPATH:/home/don/eyb/inst/weka-3-5-6/weka.jar";
$wekacmd = "java -Xmx2048m -classpath $wekapath ";  
##############################################################

# run weka...
sub run_weka {
  my $cmdline = $_[0];
  
  my $retval = system($wekacmd." ".$cmdline);
  
  if ($retval) {
    print "ERROR running Weka\n";
    exit 1;
  }
  
  return $retval;
}

require "arff-functions.pl";

print "removing filename and timestamp fields from $input_arff ...\n";
$stripped_arff = $input_arff.".stripped.arff";
$fsel_tmp_arff = $input_arff.".fsel.arff";
 
my $hr = &load_arff($input_arff);
my $numattr = $#{$hr->{"attributes"}};
    
$hr->{"attributes"}[0]{"selected"} = 0;  # filename
$hr->{"attributes"}[1]{"selected"} = 0;  # timestamp
    
&save_arff_AttrSelected($stripped_arff,$hr);

print "Running WEKA CFS feature selection ...\n";   
&run_weka("weka.filters.supervised.attribute.AttributeSelection -E \"weka.attributeSelection.CfsSubsetEval\" -S \"weka.attributeSelection.BestFirst -D 1 -N 5\" -i \"$stripped_arff\" -o \"$fsel_tmp_arff\" ");



system("./arffToLsvm.pl $fsel_tmp_arff");

# convert arff:

$outpArff=$fsel_tmp_arff;
$feaTUMfselfile=$input_arff;
$feaTUMfselfile=~ s/\.arff/.fselection/;

# count number of features first
$nStr = 0;
open(FILE,"<$outpArff");
while (<FILE>) {
  my $line = $_; chop($line);
  if ($line =~ /^\@attribute (.+?) numeric/) {
    $nStr++;  
  }
  if ($line =~ /^\@data/) { last; }
}
close(FILE);

open(OUT,">$feaTUMfselfile");
print OUT "str\n";
print OUT "$nStr\n";
open(FILE,"<$outpArff");
while (<FILE>) {
  my $line = $_; chop($line);
  if ($line =~ /^\@attribute (.+?) numeric/) {
    $attr = $1;
    print "selecting: $attr \n";
    print OUT "$attr\n";
  }
  if ($line =~ /^\@data/) { last; }
}
close(FILE);
close(OUT);

