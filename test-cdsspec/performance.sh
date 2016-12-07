#!/bin/bash
#

if [[ $# -gt 0 ]]; then
	BENCHS=($1)
else
	BENCHS=(ms-queue linuxrwlocks mcs-lock chase-lev-deque-bugfix \ 
		ticket-lock seqlock read-copy-update concurrent-hashmap spsc-bugfix \
		mpmc-queue)
fi

echo ${BENCHS[*]}
for i in ${BENCHS[*]}; do
	if [ -e "$i/main" ]; then
		echo "----------    Start to run the main for $i    ----------"
		time ./run.sh $i/main -m2 -y -u3 -tSPEC
	else
		echo "No main for $i."
	fi
done
