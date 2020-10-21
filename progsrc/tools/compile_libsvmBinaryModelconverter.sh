#!/bin/sh

# This file is part of openSMILE.
#
# Copyright (c) audEERING. All rights reserved.
# See the file COPYING for details on license terms.

echo g++ -g -DNOSMILE -I../../src/include -c ../../src/classifiers/libsvm/svm.cpp -o svm.o
g++ -g -DNOSMILE -c -I../../src/include ../../src/classifiers/libsvm/svm.cpp -o svm.o
echo g++ -g -DNOSMILE -I../../src/include libsvmBinaryModelconverter.cpp svm.o -o modelconverter
g++ -g -DNOSMILE -I../../src/include libsvmBinaryModelconverter.cpp svm.o -o modelconverter

