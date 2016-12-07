#!/bin/bash
#

BENCHS=(ms-queue linuxrwlocks mcs-lock chase-lev-deque-bugfix \ 
    ticket-lock seqlock read-copy-update concurrent-hashmap spsc-bugfix \
    mpmc-queue)

for i in ${BENCHS[*]}; do
    echo "Cleansing $i..."
    echo "rm -rf $i/*.c $i/*.cc $i/*.h $i/*.o $i/main $i/testcase* $i/.*.d"
    rm -rf $i/*.c $i/*.cc $i/*.h $i/*.o $i/main $i/testcase* $i/.*.d
done
