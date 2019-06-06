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
#
# Set scsi and mpt3sas debug flags
#
MPT3_DEBUG=/sys/module/mpt3sas/parameters/logging_level
echo 0x03f8 >  ${MPT3_DEBUG}
#
echo ${MPT3_DEBUG}=$(printf "%#x\n" $(cat ${MPT3_DEBUG}))
#
#collect current drive status by scrtnycli tool before test
./scrtnycli.x86_64 -i 1 cli pl status > pl_status_pretest.txt
./scrtnycli.x86_64 -i 1 scan > pre_scan.txt
./dmesg > dmesg_pretest.txt

# Register the Ring Buffer
#./scrtnycli.x86_64 -i 1 db -register -trace -size ${FWBUFSZ}

#function restore, restore scsi and mpt3sas debug flags when script is stopped
function restore()
{
#  release and unregister trace
	#./scrtnycli.x86_64 -i 1 db -release -trace
	#./scrtnycli.x86_64 -i 1 db -unregister -trace
# restore debugging flags
	echo 0 >  ${MPT3_DEBUG}
	./sync -f pl_status_pretest.txt
	if [ -n "$kdump" ]
	then
		echo c > /proc/sysrq-trigger
	fi
}

function scrtny_exit()
{
	echo "Killed by operator! at $(./date)"
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
	#./scrtnycli.x86_64 -i 1 db -release -trace
	#./scrtnycli.x86_64 -i 1 db -read -trace -file rb_logs${numabort}.txt
	#./scrtnycli.x86_64 -i 1 db -unregister -trace
	./scrtnycli.x86_64 -i 1 cli iop show diag > scrutiny_abort_diag${numabort}.txt
	./scrtnycli.x86_64 -i 1 cli pl status > pl_status${numabort}.txt
	./dmesg > kernel${numabort}.txt
	./sync -f pl_status_pretest.txt
	# reRegister the Ring Buffer for next read
	#./scrtnycli.x86_64 -i 1 db -register -trace -size ${FWBUFSZ}
}

#Look for task abort event, when meet "attempting task abort", begin to capture logs
#
stplast=0
count=0
while ! ./dmesg | ./grep -F "${ENDMARK}"
do
	keyline=$(./dmesg|./grep -F "$MARK"|./sed -e '$!d'| \
			./sed -e 's/^\[ *//' -e 's/\./ /')
	if [ -n "$keyline" ]
	then
		stpcur=${keyline%% *}
		if [ $stpcur -gt $stplast ]
		then
			stplast=$stpcur
			fwdump
			numabort=$(((numabort+1)%10))
		fi
	fi
	[ ${count} -eq 0 ] && echo "I'm still alive at $(./date)"
	./sleep
	count=$(((count+1)%10))
done
#
# Dump 5 more times after timeout
#
count=0
while [ $count -lt 121 ]
do
	if [ $((count%30)) -eq 0 ]
	then
		./scrtnycli.x86_64 -i 1 cli iop show diag > scrutiny_abort_diag${numabort}.txt
		./dmesg > kernel${numabort}.txt
		numabort=$((numabort+1))
	fi
	./sleep
	count=$((count+1))
done
#
restore
exit 0
