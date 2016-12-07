#!/bin/bash

AutoGenDir=$(pwd)/src/edu/uci/eecs/utilParser
echo "Deleting the old generated java files."
rm -rf $AutoGenDir

echo "Deleting the class files."
rm -rf ./classes
