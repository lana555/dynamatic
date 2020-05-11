#!/bin/sh
#Authors: Andrea Guerrieri andrea.guerrieri@epfl.ch

FILE1=$1

optimized="optimized"

echo "filename $FILE1" $3
cd $1
pwd

filename=`echo "$1" | sed 's/\<examples\>//g'`
filename=`echo "$filename" | tr -d /`



rm -r dynamatic* 2>/dev/null
rm -r hdl 2>/dev/null
rm -r reports 2>/dev/null

if [ "$optimized" = "$3" ]; then
    dynamatic synthesis_optimized.tcl || retval=1
    filename=$filename"_"$optimized
else
    dynamatic synthesis.tcl || retval=1
fi

echo "************************************" >> $2
echo "Synthesis $filename" >> $2

echo -n "Checking dot... " >> $2

cd reports
if [ -f $filename.dot ]; then

    echo "PASS" >> $2
    result="PASS"
  else
    echo "FAIL"  >> $2
    result="FAIL"
fi
cd ..
echo -n "Checking vhdl... " >> $2

cd hdl
if [ -f $filename.vhd ]; then

    echo "PASS" >> $2
    result="PASS"
  else
    echo "FAIL"  >> $2
    result="FAIL"
fi

cd ..
	
exit $ret_val
