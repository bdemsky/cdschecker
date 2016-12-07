#!/bin/bash

SPEC_COMPILER_HOME=$(pwd)

echo "CDSSpec Compiler home: " $SPEC_COMPILER_HOME

JAVACC_PATH=$SPEC_COMPILER_HOME/lib

UTIL_FILE=$SPEC_COMPILER_HOME/grammer/util.jj

UTIL_OUTPUT_PATH=$SPEC_COMPILER_HOME/src/edu/uci/eecs/utilParser

echo "Deleting the old generated java files."
rm -rf $UTIL_OUTPUT_PATH/*

mkdir -p $UTIL_OUTPUT_PATH

java -cp $JAVACC_PATH/javacc.jar javacc -OUTPUT_DIRECTORY=$UTIL_OUTPUT_PATH $UTIL_FILE
