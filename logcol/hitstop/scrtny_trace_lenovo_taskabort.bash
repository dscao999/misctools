#!/bin/bash
#
#FWBUFSZ=4095
#Script will finish log collection when hit below event
ENDMARK="mpt3sas_cm0: _config_request: timeout"
#
TARGS=$(getopt -l crash,test -o cx -- "$@")
[ $? -eq 0 ] || exit 1
#
MARK="attempting task abort"
kdump=
eval set -- $TARGS
while true
do
	case "$1" in
	-c)
		kdump=trigger;
		;;
	--crash)
		kdump=trigger;
		;;
	--test)
		MARK="TASKTYPE_ABORT_TASK"
		;;
	-x)
		MARK="TASKTYPE_ABORT_TASK"
		;;
	--)
		shift
		break
		;;
	esac
	shift
done
[ -n "$1" ] && ENDMARK="$1"
#
# Set scsi and mpt3sas debug flags
#
MPT3_DEBUG=/sys/module/mpt3sas/parameters/logging_level
echo 0x03f8 >  ${MPT3_DEBUG}
#
echo ${MPT3_DEBUG}=$(printf "%#x\n" $(cat ${MPT3_DEBUG}))
#
PATH=./bin
STAMP="/tmp/scrtny_stamp.txt"
#
#collect current drive status by scrtnycli tool before test
scrtnycli.x86_64 -i 1 cli pl status > pl_status_pretest.txt
scrtnycli.x86_64 -i 1 scan > pre_scan.txt
dmesg --nopager -k > dmesg_pretest.txt

# Register the Ring Buffer
#scrtnycli.x86_64 -i 1 db -register -trace -size ${FWBUFSZ}

#function restore, restore scsi and mpt3sas debug flags when script is stopped
function restore()
{
#  release and unregister trace
	#scrtnycli.x86_64 -i 1 db -release -trace
	#scrtnycli.x86_64 -i 1 db -unregister -trace
# restore debugging flags
	echo 0 >  ${MPT3_DEBUG}
	sync -f pl_status_pretest.txt
	if [ -n "$kdump" ]
	then
		echo c > /proc/sysrq-trigger
	fi
}

function scrtny_exit()
{
	echo "Killed by operator! at $(date)"
	restore
	exit 0
}

trap scrtny_exit 1 2 3 15

numabort=0
#
# Function fwdump, dump HBA FW logs and OS log
#
function fwdump()
{
	#scrtnycli.x86_64 -i 1 db -release -trace
	#scrtnycli.x86_64 -i 1 db -read -trace -file rb_logs${numabort}.txt
	#scrtnycli.x86_64 -i 1 db -unregister -trace
	scrtnycli.x86_64 -i 1 cli iop show diag > scrutiny_abort_diag${numabort}.txt
	scrtnycli.x86_64 -i 1 cli pl status > pl_status${numabort}.txt
	dmesg --nopager -k > kernel${numabort}.txt
	sync -f pl_status_pretest.txt
	# reRegister the Ring Buffer for next read
	#scrtnycli.x86_64 -i 1 db -register -trace -size ${FWBUFSZ}
}

#Look for task abort event, when meet "attempting task abort", begin to capture logs
#SEDSTP=sed -e 's/^\[ *//' -e 's/\.//' -e 's/]/ /'
#
dmesg --nopager -k | sed -e '$!d' | sed -e 's/^\[ *//' -e 's/\.//' -e 's/]/ /' | sed -e 's/ .*$//' > $STAMP
count=0
status=0
#
while [ $status -ne 11 ]
do
	read stplast < $STAMP
	dmesg --nopager -k | while read line
	do
		stpcur=$(echo $line | sed -e 's/^\[ *//' -e 's/\.//' -e 's/]/ /')
		stpcur=${stpcur%% *}
		[ $stpcur -le $stplast ] && continue
		stplast=$stpcur
		echo $stplast > $STAMP
#
		if echo $line | grep -F "${ENDMARK}" > /dev/null 2>&1
		then
			exit 11
		fi
#
		if echo $line |grep -F "$MARK" > /dev/null 2>&1
		then
			fwdump
			numabort=$(((numabort+1)%10))
		fi
	done
	status=$?
#
	[ ${count} -eq 0 ] && echo "I'm still alive at $(date)"
	sleep
	count=$(((count+1)%10))
done
#
# Dump 5 more times after timeout
#
count=0
while [ $count -lt 5 ]
do
	scrtnycli.x86_64 -i 1 cli iop show diag > scrutiny_abort_diag${numabort}.txt
	dmesg --nopager -k > kernel${numabort}.txt
	numabort=$(((numabort+1)%10))
	sleep 30
	count=$((count+1))
done
#
restore
exit 0
