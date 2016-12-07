#!/bin/bash

BENCH=(ms-queue linuxrwlocks mcs-lock \
    chase-lev-deque-bugfix spsc-bugfix mpmc-queue ticket-lock \
    concurrent-hashmap seqlock read-copy-update)

ClassPath=$(dirname ${BASH_SOURCE[0]})/classes

Class=edu/uci/eecs/codeGenerator/CodeGenerator

# Use your own directory.
# We recommend the original benchmarks and generated instrumented benchmarks to
# be within the model checker's directory.
BenchDir=../benchmarks
GenerateDir=../test-cdsspec

mkdir -p $GenerateDir

java -cp $ClassPath $Class $BenchDir $GenerateDir ${BENCH[*]}
