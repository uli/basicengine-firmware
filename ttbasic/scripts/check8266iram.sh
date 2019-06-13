#!/bin/bash
bufs=`grep define.*TSF_BUFFS[^I] tsf.h|head -n 1|sed s,^.*BUFFS,,`
num=`grep define.*TSF_BUFFSIZE tsf.h|head -n 1|sed s,^.*BUFFSIZE,,`
eoi=$(( 16#`objdump -x "$1"|grep _lit4_end|cut -c1-8` ))
#echo $bufs $num $eoi
if test $(( 16#40108000 - eoi )) -lt $(( bufs * num * 1 )); then
	echo "error: IRAM code collides with TSF buffers"
	exit 1
fi
exit 0
