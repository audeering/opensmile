### Functions to handle .arff files (WEKA)


# returns reference to arff hash
sub load_arff {
  my $file = $_[0];
  
  my %hash = ();
  my $data = 0;
  my $featnr = 0;
  my $instnr = 0;
  
  open(FILE,"<$file");
  while (<FILE>) {
    my $line = $_;
    $line =~ s/\n$//;
    $line =~ s/\r$//;
    if ($line !~ /^\s*$/) {
      if ($data) { 
        #my @els = split(/,/,$line);  # TODO: quoting "" '' 
        $line =~ s/,,/, ,/g;
        my @els = ($line =~ /(\'.*?\'|".*?"|[^,]+)/g);
        my $i;
        for ($i=0; $i <= $#els; $i++) {
          $hash{"instances"}[$instnr][$i] = "$els[$i]";
        }
        #$hash{"instances"}[$instnr][$i] = \@els;
        $instnr++;
      } else {
        if ($line =~ /^\@data/i)  {
          $data = 1; $instnr = 0;
        } else {
          if ($line =~ /^\@relation\s+(.+)$/i) {
            $hash{"relation"} = $1;
          }
          if ($line =~ /^\@attribute\s+(.+?)\s+(.+?)$/i) {
            $hash{"attributes"}[$featnr]{"name"} = $1;
            $hash{"attributes"}[$featnr]{"type"} = $2;
            $hash{"attribute_names"}{$1} = $featnr+1;  # reverse lookup hash  ## +1 !!
            $hash{"attributes"}[$featnr]{"selected"} = 1;
            #print $hash{"attributes"}[$featnr]{"name"}."\n";
            $featnr++;
          }
        
        }
        
      }
    }
  }
  close(FILE);
  
  return \%hash;
}


# USAGE: &save_arff( $file, \%hash )
sub save_arff {
  my $file = $_[0];
  my $hashref = $_[1];
  
  my $tmp;
  my $name; my $type;
  my $i = 0;
  
  open(FILE,">$file");
  $tmp = $hashref->{"relation"};
  print FILE "\@relation $tmp\n\n";
  
  for ($i=0; $i <= $#{$hashref->{"attributes"}}; $i++) {
    #if ($hashref->{"attributes"}[$i]{"selected"}) {
      $name = $hashref->{"attributes"}[$i]{"name"};
      $type = $hashref->{"attributes"}[$i]{"type"};
      print FILE "\@attribute $name $type\n";
    #}
  }
  
  print FILE "\n\@data\n\n";
  
  for ($i=0; $i <= $#{$hashref->{"instances"}}; $i++) {
    my $line = "";
    for ($j= 0; $j <= $#{$hashref->{"instances"}[$i]}; $j++) {
      $line .= $hashref->{"instances"}[$i][$j].",";
    }
    chop($line);
    print FILE $line."\n";
  }
  
  close(FILE);
  
}


# Perform attribute selection while saving. Saves memory and is faster
sub save_arff_AttrSelected {
  my $file = $_[0];
  my $hashref = $_[1];
  
  my $tmp;
  my $name; my $type;
  my $i = 0; my $j;
  
  open(FILE,">$file");
  $tmp = $hashref->{"relation"};
  print FILE "\@relation $tmp\n\n";
  
  for ($i=0; $i <= $#{$hashref->{"attributes"}}; $i++) {
    if ($hashref->{"attributes"}[$i]{"selected"}) {
      $name = $hashref->{"attributes"}[$i]{"name"};
      $type = $hashref->{"attributes"}[$i]{"type"};
      print FILE "\@attribute $name $type\n";
    }
  }
  
  print FILE "\n\@data\n\n";
  
  for ($i=0; $i <= $#{$hashref->{"instances"}}; $i++) {
    my $line = "";
    for ($j= 0; $j <= $#{$hashref->{"instances"}[$i]}; $j++) {
      #print $j."\n";
      if ($hashref->{"attributes"}[$j]{"selected"}) {
        $line .= $hashref->{"instances"}[$i][$j].",";
      }
    }
    chop($line); # remove last comma
    print FILE $line."\n";
  }
  
  close(FILE);
}

## convert arff to csv, first line contains header with feature names and types in []
sub arff_to_csv {
  my $arff = $_[0];
  my $csv = $_[1];
  my $csvDelim = ",";

  my $hr = &load_arff($arff);
  
  open(FILE,">$csv");
  #header line
  my $line = ""; my $i;
  for ($i = 0; $i <= $#{$hr->{"attributes"}}; $i++) {
    $line .= $hr->{"attributes"}[$i]{"name"}."[".$hr->{"attributes"}[$i]{"type"}."]$csvDelim";
  }
  chop($line);
  print FILE $line."\n";
  # print data
  for ($i = 0; $i <= $#{$hr->{"instances"}}; $i++) {
    $line = "";
    my $j;
    for ($j = 0; $j <= $#{$hr->{"instances"}[$i]}; $j++) {
           if ($hr->{"instances"}[$i][$j] eq "nan") { print $hr->{"attributes"}[$j]{"name"}."\n"; }
      $line .= $hr->{"instances"}[$i][$j].$csvDelim;
    }
    chop($line);
    print FILE $line."\n";
  }
  close(FILE);
}

## convert csv to arff, 
# first line in csv file contains header with feature names (and types in [] -> optional!)
sub csv_to_arff { # return value: arff hash reference
  my $csv = $_[0];  # filename
  my $arff = $_[1]; # arff filename
  
  my $csvdelim = ",";
  if ($_[2]) { $csvdelim = $_[2]; }
  
  #my $hr = &load_arff($arff);
  my %hr=();
  $hr{"relation"}="from_csv";

  open(FILE,"<$csv");
  #header line
  my $line = ""; my $i;
  $line=<FILE>; 
  chop($line);
  my @els=split(/$csvdelim/,$line);
  my $i=0;
  for ($i=0; $i <= $#els; $i++) {
    if ($els[$i] =~ /^(.+?)\[(.+?)\]$/) {
      $hr{"attributes"}[$i]{"name"} = $1;
      $hr{"attributes"}[$i]{"type"} = $2;
    } else {
      $hr{"attributes"}[$i]{"name"} = $els[$i];
      $hr{"attributes"}[$i]{"type"} = "numeric";
    }
  }
  print "attributes: $i \n";

  my $i=0;
  while (<FILE>)  {
    $line = $_; chop($line);
    my @els=split(/$csvdelim/,$line);
    my $j;
    for ($j=0; $j<=$#els; $j++) {
      $hr{"instances"}[$i][$j] = $els[$j];
    }
    print "attr[$i]: $j\n";
    $i++;
  }
  
  close(FILE);
  
  &save_arff($arff,\%hr);
  return \%hr;
}


## convert arff to csvs, first line contains header with feature names and types in []
## special function that creates multiple csvs by parsing first attribute (filename)
sub arff_to_csvs {
  my $arff = $_[0];
  my $csv = $_[1];

  my $hr = &load_arff($arff);
  
  #load session names:
  open(FILE,"all.txt");
  my @sessnames = ();
  while(<FILE>) {
    chop; push(@sessnames,$_);
  }
  close(FILE);
  
  my $sess;
  foreach $sess (@sessnames) {
  
  my $ccsv = $csv.".$sess.csv";
  $ccsv =~ s/csv\.$sess\.csv$/$sess.csv/;
  open(FILE,">$ccsv");
  #header line
  my $line = ""; my $i;
  for ($i = 0; $i <= $#{$hr->{"attributes"}}; $i++) {
    $line .= $hr->{"attributes"}[$i]{"name"}."[".$hr->{"attributes"}[$i]{"type"}."],";
  }
  chop($line);
  print FILE $line."\n";
  # print data
  for ($i = 0; $i <= $#{$hr->{"instances"}}; $i++) {
    if ($hr->{"instances"}[$i][0] =~ /$sess/) {
    
    $line = "";
    my $j;
    for ($j = 0; $j <= $#{$hr->{"instances"}[$i]}; $j++) {
      $line .= $hr->{"instances"}[$i][$j].",";
    }
    chop($line);
    print FILE $line."\n";
    
    }
  }
  close(FILE);
  
  }
}

## load both files, check if attributes are the same, and save, SLOW!!
## (this funtions concats INSTANCES)
sub concat_arffs_save {
  my $file1 = $_[0];
  my $file2 = $_[1];
  my $outfile = $_[2];

  my $hr1 = &load_arff($file1);
  my $hr2 = &load_arff($file2);
  
  my $error = 0;
  
  my $i;
  #check attributes:
  for($i=0; $i <= $#{$hr1->{"attributes"}}; $i++) {
    if ($hr1->{"attributes"}[$i]{"name"} ne $hr2->{"attributes"}[$i]{"name"}) {
      print "Attribute name mismatch: (1) ".$hr1->{"attributes"}[$i]{"name"}." <-> (2) ".$hr2->{"attributes"}[$i]{"name"}."\n";
      $error = 1;
      last;
    }
    if ($hr1->{"attributes"}[$i]{"type"} ne $hr2->{"attributes"}[$i]{"type"}) {
      print "Attribute type mismatch: ".$hr2->{"attributes"}[$i]{"type"}." :: (1) ".$hr1->{"attributes"}[$i]{"type"}." <-> (2) ".$hr2->{"attributes"}[$i]{"type"}."\n";
      $error = 2;
      last;
    }  
  }
  
  unless ($error) {
    # concat
    push(@{$hr1->{"instances"}},@{$hr2->{"instances"}}); 
    # save
    &save_arff($outfile,$hr1);
  }
  
  return $error;
}


## copy and append only, fast!
sub concat_arffs_simple {
  my $file1 = $_[0];
  my $file2 = $_[1];
  my $outfile = $_[2];

  open(FILE,"<$file1");
  open(OF,">$outfile");
  while(<FILE>) {
    print OF $_;
  }  
  close(FILE);
  
  open(FILE,"<$file2");
  while(<FILE>) {
    my $line = $_;
    if ($line =~ /^\@data/i) { last; }
  }
  while(<FILE>) {
    print OF $_;
  }
  close(FILE);
  close(OF);
}


# concat attributes! (only works when the number of instances is the same for both files!)
# USAGE: join_arff(input1, input2, $output, [# features to skip at end of first file])
# optional: return value is hash ref. to joined arff hash
sub join_arff() {
  my $in1 = $_[0];
  my $in2 = $_[1];
  my $out = $_[2];
  my $skip = 1; # attributes of in1 to skip/overwrite (from end)
  if ($_[3]) { $skip = $_[3]; }

  my $hr1=&load_arff($in2);
  my $hr2=&load_arff($in1);

  my @attr1 = @{$hr1->{"attributes"}};
  my @inst1 = @{$hr1->{"instances"}};
  my @attr2 = @{$hr2->{"attributes"}};
  my @inst2 = @{$hr2->{"instances"}};
  my $na2 = $#attr2;

  if ($#inst2 != $#inst1) {
    print "Number of instances does not match! cannot merge!\n";
    return -1;
  }
  
  my $i;
  # append hr1 attributes to hr2
  for ($i=0; $i<=$#attr1; $i++) {
    $hr2->{"attributes"}[$na2+$i+1-$skip] = $attr1[$i];
  }

  #append hr1 instances to hr2 instances
  for ($i=0; $i<=$#inst2; $i++) {
    for ($j=0; $j<=$#attr1; $j++) {
      $hr2->{"instances"}[$i][$na2+$j+1-$skip] = $inst1[$i][$j];
    }
  }


  # save result (hr2):
  &save_arff($out , $hr2);
  
  return \%hr2;
}

# synchronize two arff files, so that they have the same attributes
# this one only works if, the attributes are in the same order in both files
sub sync_arffs {
  my $file1 = $_[0];  # arff file to be modified
  my $file2 = $_[1]; # arff file to sync to...
  my $outfile = $_[2];  # file to save result to
  my $addnames = $_[3];  # reference to hash w. attribute names that will be added as keys 
                         # (attr. must exist in file1, and not in file2)
  
  my %addnames_local = ();
  if ($addnames) {  #  check if it is a reference ???!!!
    %addnames_local = %{$addnames};
  }
  
  print "Loading $file1...\n";
  my $hr1 = &load_arff($file1);
  print "Loading $file2...\n";
  my $hr2 = &load_arff($file2);

  print "Syncing attributes...\n";
  my $i;  
  # in hr1 select only those that are present in hr2:
  for($i=0; $i <= $#{$hr1->{"attributes"}}; $i++) {
    #print "$i\n";
    if ( ($hr2->{"attribute_names"}{$hr1->{"attributes"}[$i]{"name"}}) || 
          ($addnames_local{$hr1->{"attributes"}[$i]{"name"}}) ) {
      $hr1->{"attributes"}[$i]{"selected"} = 1;
    } else {
      $hr1->{"attributes"}[$i]{"selected"} = 0;
    }
  }
  
  print "Saving $outfile...\n";
  &save_arff_AttrSelected($outfile,$hr1);
}

# synchronize two arff files, so that they have the same attributes
# can handle attributes in arbitrary order, will produce output in the same attr. order as reference
sub sync_arffs_order {
  my $file1 = $_[0];  # arff file to be modified
  my $file2 = $_[1]; # arff file to sync to...
  my $outfile = $_[2];  # file to save result to
  my $addnames = $_[3];  # reference to hash w. attribute names that will be added as keys 
                         # (attr. must exist in file1, and not in file2)
  
  my %addnames_local = ();
  if ($addnames) {  #  check if it is a reference ???!!!
    %addnames_local = %{$addnames};
  }
  
  print "Loading $file1...\n";
  my $hr1 = &load_arff($file1);
  print "Loading $file2...\n";
  my $hr2 = &load_arff($file2);
  my %hrOut={};
  
  print "Syncing attributes...\n";
  $hrOut{"relation"} = $hr1->{"relation"};
  my $i;  
  # copy attributes from hr1 in hrOut, if present in hr2
  for($i=0; $i <= $#{$hr2->{"attributes"}}; $i++) {
  
    $hrOut{"attributes"}[$i]{"name"} = $hr2->{"attributes"}[$i]{"name"};
    $hrOut{"attributes"}[$i]{"type"} = $hr2->{"attributes"}[$i]{"type"};
    $hrOut{"attributes"}[$i]{"selected"} = 1;
    $hrOut{"attribute_names"}{$hr2->{"attributes"}[$i]{"name"}} = $i+1;
    my $attrNr1 = $hr1->{"attribute_names"}{$hr2->{"attributes"}[$i]{"name"}}-1;
    
    for ($j=0; $j <= $#{$hr1->{"instances"}}; $j++) {
      $hrOut{"instances"}[$j][$i] = $hr1->{"instances"}[$j][$attrNr1];
    }
  }
  #TODO: add addnames_local names at the end...
  #if ( ($hr2->{"attribute_names"}{$hr1->{"attributes"}[$i]{"name"}}) || 
  #      ($addnames_local{$hr1->{"attributes"}[$i]{"name"}}) ) {
  #  $hr1->{"attributes"}[$i]{"selected"} = 1;
  #} else {
  #  $hr1->{"attributes"}[$i]{"selected"} = 0;
  #}

  
  print "Saving $outfile...\n";
  &save_arff($outfile,\%hrOut);
}

# synchronize two arff files, so that they have the same attributes
# can handle attributes in arbitrary order, will produce output in the same attr. order as reference
sub sync_arff_to_attrlist_order {
  my $file1 = $_[0];  # arff file to be modified
  my $attrlist = $_[1]; # attr. list
  my $outfile = $_[2];  # file to save result to
  
  print "Loading input $file1...\n";
  my $hr1 = &load_arff($file1);
  print "Loading attribute list $file2...\n";
  open(FFFILE,"<$attrlist");
  my @ftlist;
  while (<FFFILE>) {
    my $line=$_;
    $line=~s/\n$//;
    $line=~s/\r$//;
    $line=~s/^\s*//;
    $line=~s/\s*$//;
    if ($line !~ /^$/) {
      push(@ftlist,$line);
print "ATTR:$line\n";
    }
  }
  close(FFFILE);

  my %hrOut={};
  
  print "Syncing attributes...\n";
  $hrOut{"relation"} = $hr1->{"relation"};
  my $i;  
  # copy attributes from hr1 in hrOut, if present in hr2
  for($i=0; $i <= $#ftlist; $i++) {
    my $anum = $hr1->{"attribute_names"}{$ftlist[$i]}-1;    

    $hrOut{"attributes"}[$i]{"name"} = $hr1->{"attributes"}[$anum]{"name"};
    $hrOut{"attributes"}[$i]{"type"} = $hr1->{"attributes"}[$anum]{"type"};
    $hrOut{"attributes"}[$i]{"selected"} = 1;
    $hrOut{"attribute_names"}{$hr1->{"attributes"}[$anum]{"name"}} = $i+1;
    #my $attrNr1 = $hr1->{"attribute_names"}{$hr2->{"attributes"}[$i]{"name"}}-1;
    
    for ($j=0; $j <= $#{$hr1->{"instances"}}; $j++) {
      $hrOut{"instances"}[$j][$i] = $hr1->{"instances"}[$j][$anum];
    }
  }
  #TODO: add addnames_local names at the end...
  #if ( ($hr2->{"attribute_names"}{$hr1->{"attributes"}[$i]{"name"}}) || 
  #      ($addnames_local{$hr1->{"attributes"}[$i]{"name"}}) ) {
  #  $hr1->{"attributes"}[$i]{"selected"} = 1;
  #} else {
  #  $hr1->{"attributes"}[$i]{"selected"} = 0;
  #}

  
  print "Saving $outfile...\n";
  &save_arff($outfile,\%hrOut);
}

# add a new attribute at the beginning with a unique instance id
# USAGE: &add_instance_id($inputarff, $outputarff);
sub add_instance_id {
  my $input = $_[0];
  my $output = $_[1];
  
  my $hr = &load_arff($input);
  my @attr = @{$hr->{"attributes"}};
  my $i;
  my %id;
  $id{"name"} = "id";
  $id{"type"} = "numeric";
  unshift(@{$hr->{"attributes"}},\%id);
  
  for ($i=0; $i<=$#{$hr->{"instances"}}; $i++) {
    unshift(@{$hr->{"instances"}[$i]},$i+1);
  }
  &save_arff($output,$hr);
}

# remove first attribute (instance id) and return array with its values
# USAGE: @instance_ids = &remove_instance_id($inputarff, [$outputarff]);
sub remove_instance_id {
  my $input = $_[0];
  my $output = $_[1];
  
  my $hr = &load_arff($input);
  my @attr = @{$hr->{"attributes"}};
  my $id = shift(@{$hr->{"attributes"}});
  
  my $i; my @ids=();
  for ($i=0; $i<=$#{$hr->{"instances"}}; $i++) {
    $ids[$i] = shift(@{$hr->{"instances"}[$i]});
  }
  
  if ($output) {
    &save_arff($output,$hr);
  }
  
  return @ids;
}


# USAGE : arff_concat_orderd( $output, $unique_hashref, @inputs ) 
# unique_hash:   hash{"filename"}[linenr] = uniqueid
# @inputs:  array of input filenames as in hash index
# $output : output filename
sub arff_concat_ordered {
  my $output = shift;
  my $unique_hashref = shift;
  my @inputs = @_;
  
  my %h=();
  # concat all input files with instances in correct order:
  foreach $f (@inputs) {
     my $in = &load_arff($f);
     if ($in->{"atrributes"}[0]{"name"} =~ /^id$/i) {
       print "ERROR: first attribute is not an id! in file $f\n";
       return;
     }
     unless ($h{"attributes"}) {
       $h{"attributes"} = $in->{"attributes"};
     }
     my $i;
     for ($i=0; $i<=$#{$in->{"instances"}}; $i++) {
       $h{"instances"}[$in->{"$instances"}[$i][0]] = $in->{"instances"}[$i];
     }
  }
  # now remove the id field:
  my $id = shift(@{$h{"attributes"}});
  my $i; my @ids=();
  for ($i=0; $i<=$#{$h{"instances"}}; $i++) {
    $ids[$i] = shift(@{$h{"instances"}[$i]});
  }
  # and save to output file:
  &save_arff($output,\%h);
}

# remove specific attributes from an arff file (by name)
sub arff_remove_attr {
  my $input = shift;  # input arff
  my $output = shift; # output arff
  my @names = @_;  # names of attributes to remove
  
  my $in=&load_arff($input);
  # get indicies of attributes to remove:
  # $hash{"attribute_names"}{$1}
  foreach $f (@names) {
    splice(@{$in->{"attributes"}}, $in->{"attribute_names"}{$f});
    
  }
  &save_arff($output,$in);
}

####################### weka calls: ######################
sub chop_newline {
  my $i;
  #for ($i=0; $i<=$#_; $i++) {
    $_ =~ s/\n$//;
    $_ =~ s/\n$//;
  #}
  return $_;
}

$wekacmd = "java -Xmx1024m -classpath /home/don/eyb/inst/weka-3-5-6/weka.jar ";  
# run a weka function:
sub run_weka {
  #my @cmdline = @_;
  #unshift(@cmdline,split(/ /,$wekacmd));
  #print "running command: ".join(" ",@cmdline)."\n";
  #my $cmd = "\"".join("\" \"",@cmdline)."\"";
  #system(@cmdline);

  my $cmdline = $_[0];
 print "running command: ".$wekacmd." ".$cmdline."\n";
  system($wekacmd." ".$cmdline);
  
  if ($?) {
    print "ERROR running Weka\n";
    return $retval;
  }
  
  return $retval;
}
#weka.filters.unsupervised.attribute.Remove -R 5 -i $input_arff -o $input_arff.arff


# &scv_split($input_arff, $nfolds, [$output base])
# split arff file into N folds , create train and test arff file for each fold
#  append _fNN_train to each filename base
# return value: hash reference to hash with arrays of train/test files
sub scv_split {
  my $input = $_[0];
  my $nfolds = $_[1];
  my $outbase = $_[2];
  my $class = "-c last";  # classification (nominal class)
  #$class = "";         # regression (numerical class)
  
  unless ($outbase) {
    $outbase = $input;
    $outbase =~ s/\.arff$//i;
  }
  
  my %files=();
  $files{"nfolds"} = $nfolds;
  my $i;
  for ($i=1; $i<=$nfolds; $i++) {
    my $fnr = sprintf("%3i",$i); $fnr =~ s/\s/0/g;
    my $train = $outbase."_f$fnr"."_train.arff";
    my $test  = $outbase."_f$fnr"."_test.arff";
    $files{"train"}[$i-1] = $train;
    $files{"test"}[$i-1]  = $test;
    #print "FOLD: $fnr -> $train\n";
    print "Generating train/test files for FOLD $fnr ... ";
    unless (-f $train) {
      if (&run_weka("weka.filters.supervised.instance.StratifiedRemoveFolds -i $input -o $train $class -V -S 0 -N $nfolds -F $i ")) { return 1; }
    } else { print "(trainfile exists) "; }
    unless (-f $test) {
      if (&run_weka("weka.filters.supervised.instance.StratifiedRemoveFolds -i $input -o $test $class -S 0 -N $nfolds -F $i ")) { return 1; }
    } else { print "(testfile exists) "; }
    print "done\n";
  } 
  
  return \%files;
}


sub weka_cfs {
  my $input = shift;
  my $output = shift;
  return &run_weka("weka.filters.supervised.attribute.AttributeSelection -E \"weka.attributeSelection.CfsSubsetEval \" -S \"weka.attributeSelection.BestFirst -D 1 -N 5\" -i $input -o $output ");
}

$default_classifier = "weka.classifiers.functions.SVMreg -C 1.0 -N 0 -I \"weka.classifiers.functions.supportVector.RegSMOImproved -L 0.0010 -W 1 -P 1.0E-12 -T 0.0010 -V\" -K \"weka.classifiers.functions.supportVector.PolyKernel -C 250007 -E 1.0\"";

# train on train_arff, test with test_arff, save predicitons in file
# this function does not save a model
sub classify_predict {
  my $trainarff = shift;
  my $testarff = shift;
  my $predictions = shift;
  my $classifier = shift;
  unless ($classifier) { $classifier = $default_classifier; }
  
  return &run_weka("$classifier -T $testarff -t $trainarff -no-cv -p 0 > $predictions");
}

# train on train_arff, test with test_arff, save results in file
# this function does not save a model
sub classify_eval {
  my $trainarff = shift;
  my $testarff = shift;
  my $results = shift;
  my $classifier = shift;
  unless ($classifier) { $classifier = $default_classifier; }
  
  #TODO: how to get confusion matrix???
  return &run_weka("$classifier -T $testarff -t $trainarff -no-cv > $results");
}

# parse weka results and return as perl hash ref
sub parse_weka_results {
  my $results = shift;  # results filename
  my %res;
  open(FFILE,"<$results");
  my $flag=0;
  while(<FFILE>) {
    chop; my $line=$_;
    if ($line=~/^=== .+ test .+ ===\s*$/) { $flag=1; }
    else {
      if ($flag) {
        if ($line=~/^===/) { $flag=0; }
        if ($line=~/Correctly Classified Instances\s+([0-9]+)\s/) {
          $res{"correct"}=$1;
        }
        if ($line=~/Incorrectly Classified Instances\s+([0-9]+)\s/) {
          $res{"incorrect"}=$1;
        }
        if ($line=~/Total Number of Instances\s+([0-9]+)\s/) {
          $res{"total"}=$1;
        }
      }
    }
  }
  close(FFILE);
  return \%res;
}

# get confusion matrix from weka results file
sub parse_weka_confmat {
  my $results = shift;  # results filename
  my $clsref = shift;
  open(FFILE,"<$results");
  my $flag=0;
  my %cfmat, %cfmat0;
  my @clses;
  while(<FFILE>) {
    chop; my $line=$_;
    if ($line=~/^=== Confusion Matrix ===\s*$/) { $flag=1; }
    else {
      if ($flag==1) { # read header
	if ($line=~/<-- classified as/) { $flag = 2; 
	  
	}
      } elsif ($flag==2) {
        # read elements
	if ($line =~ /^ +(.+?) +\| +[a-z]+ = (.+)$/) {
          my $els = $1;
          my $cls = $2;
          $cfmat0{$cls} = $els;
          my $fl=0;
          my $tmp;
	  foreach $tmp (@{$clsref}) {
            if ($cls eq $tmp) { $fl=1; }
	  }
          unless ($fl) { push(@{$clsref},$cls); }
        }
      }
    }
  }
  my $ttt=0;
  my $c;
  foreach $c (@{$clsref}) {
    my $l = $cfmat0{$c};
    my @el = split(/ +/,$l);
    if ($#el != $#{$clsref}) { print "CONF MAT PARSE ERROR $#el != ".$#{$clsref}."!\n"; }
    my $i;
    for ($i=0; $i<=$#el; $i++)  {
      $cfmat{$c}{$clsref->[$i]} = $el[$i];
      $ttt+=$el[$i];
    }
  }
#print "total:::: $ttt\n";
  close(FFILE);
  return \%cfmat;
}

# build a model from a given arff file
sub build_model {
  my $trainarff = $_[0];
  my $model = $_[1];
  my $classifier = $_[2];
  my $trainlog = $_[3];
  unless ($trainlog) { $trainlog = "/dev/null"; }
  unless ($classifier) { $classifier = $default_classifier; }

  &run_weka("$classifier -t $trainarff -d $model -no-cv > $trainlog");
}

# test a model with a given test data set (arff)
sub test_model {
  my $testarff = $_[0];
  my $model = $_[1];
  my $results = $_[2];
  my $classifier = $_[3];

  &run_weka("$classifier -T $testarff -l $model > $results");
}

# compute predictions for a given arff file using a given model
# USAGE: [ @pred = ] &compute_predictions( $testarff, $model, $predicitions, [$classifier] );
sub compute_predictions {
  my $testarff = shift;
  my $model = shift;
  my $predictions = shift;
#  my @classifier = @_;
#  unless (@classifier) { @classifier = @default_classifier; }
  my $classifier = shift;
  unless ($classifier) { $classifier = $default_classifier; }

#  &run_weka(@classifier,"-T", "$testarff", "-l", "$model", "-p",  "0", "-no-cv");
  &run_weka($classifier." -T $testarff -l $model -p 0 -no-cv");
  
  my @pred = &load_predictions($predicitons);
    
  return @pred;
}


# load weka predicitons:
# USAGE: @pred = &load_predictions($filename);
sub load_predictions {
  my $fname = $_[0];
  
  my $i;
  my @predictions=();
  
  open(FFILE,"<$fname");
  my $header=<FFILE>; $header=&chop_newline($header);
  $header =~ s/^\s+//; $header =~ s/\s+$//;
  my @attrs = split(/\s+/,$header);
  my $predNr = 2;
  for ($i=0;$i<=$#attrs;$i++) { if ($attrs[$i] =~ /^predicted$/i) {$predNr = $i; last;} }
  
  #$i=0;
  while(<FFILE>) {
    my $line=&chop_newline($_);
    $line =~ s/^\s+//; $line =~ s/\s+$//;
    print "l: $line\n";
    my @els = split(/\s+/,$line);
    if ($#els > 0) {
      $predictions[$els[0]] = $els[$predNr];
    }
    print $els[$predNr]."\n";
    print $predNr."\n";
  }
  close(FFILE);
  
  return @predictions;
}


1;
