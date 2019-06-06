#!/bin/bash
#
declare -a pids
numcpu=$(cat /proc/cpuinfo | fgrep processor | wc -l)
num=$1
#
function abrt ()
{
	count=0
	while [ $count -lt $num ]; do
		kill ${pids[$count]}
		count=$((count+1))
	done
}
#
trap abrt INT
#
[ -z "$num" ] && num=8
echo "Total number of cpus: $numcpu, allocating $num cpus"
count=0
cpu=$((numcpu-1))
while [ $count -lt $num ]
do
	taskset -c $cpu ./sp &
	pids[$count]="$!"
	count=$((count+1))
	cpu=$((cpu-1))
done
echo "Jobs: ${pids[@]}"
wait
