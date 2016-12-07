#!/bin/bash
#

BENCHS=(ms-queue linuxrwlocks mcs-lock chase-lev-deque-bugfix treiber-stack
	ticket-lock seqlock read-copy-update concurrent-hashmap spsc-bugfix mpmc-queue)

echo ${BENCHS[*]}
for i in ${BENCHS[*]}; do
	echo "----------    Start to run $i    ----------"
	tests=$(ls $i/testcase[1-9])
	if [ -e "$i/main" ]; then
		tests=($tests "$i/main")
	fi
	if [ -e "$i/testcase" ]; then
		tests=($tests "$i/testcase")
	fi

	for j in ${tests[*]}; do
		./run.sh $j -m2 -y -u3 -tSPEC
	done
done
