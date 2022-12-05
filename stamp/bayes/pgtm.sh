#!/bin/bash

make -f Makefile.stm.pgtm clean
make -f Makefile.stm.pgtm
./bayes -t8
