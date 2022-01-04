###########################################################
#  perl functions for simplified text file access         #
#  these functions read/write text files into/from arrays #
#  by Florian Eyben, TUM, MMK  (2008)                     #
###########################################################

# usage: @lines=readFile("filename");
sub readTextFile {
  my @my_lines=();
  if (-f "$_[0]") {
    open(FILE,"<$_[0]");
    @my_lines = <FILE>;
    chop(@my_lines); # remove newline char.
    close(FILE);
    return @my_lines;
  } 
}

sub readTextFile_asString {
  my @my_lines2=&readTextFile($_[0]);
  my $my_tmp = join("\n",@my_lines2);
  return "$my_tmp\n";
}

sub readTextFile_asRawString {  # does NOT remove any newlines during reading...
  my @my_lines=();
  if (-f "$_[0]") {
    open(FILE,"<$_[0]");
    @my_lines = <FILE>;
    close(FILE);
    my $my_tmp = join("",@my_lines);
    return "$my_tmp";
  } else { return ""; }
}

# usage: &writeToTextFile(filename,$text) - save $text without appending \n at the end
# usage: &writeToTextFile(filename,@text) - save array joined by \n and with a \n after the last element (=line)
#  caution: file will be overwritten if it exists!
sub writeToTextFile {
  my $my_tmp = shift;
  open(FILE,">$my_tmp");
  if ($#_ > 0) { # array mode
    print FILE join("\n",@_);
    print FILE "\n";
  } else { # string mode
    print FILE "$_[0]";
  }
  close(FILE);
}

sub appendToTextFile {
  my $my_tmp = shift;
  if (-f $my_tmp) {
    open(FILE,">>$my_tmp");
  } else {
    open(FILE,">$my_tmp");
  }
  if ($#_ > 0) { # array mode
    print FILE join("\n",@_);
    print FILE "\n";
  } else { # string mode
    print FILE "$_[0]";
  }
  close(FILE);
}

1;