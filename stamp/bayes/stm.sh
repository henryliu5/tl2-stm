#!/bin/bash

make -f Makefile.stm clean
make -f Makefile.stm
./bayes -t8
