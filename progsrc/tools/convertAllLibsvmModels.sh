#!/bin/sh

BINDIR=`dirname "$0"`
MODELDIR="$BINDIR/../models"
MODELDIR=`(cd $MODELDIR ; pwd)`

for f in `find $MODELDIR -name allft.model -print` ; do
  $BINDIR/modelconverter $f ${f}.bin
done
