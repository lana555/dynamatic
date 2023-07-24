#!/bin/sh
#Authors: Andrea Guerrieri andrea.guerrieri@epfl.ch

export TODAY_IS=`date +%F%H%M%S`

report_file=regression_test_detailed_$TODAY_IS.rpt
report=regression_test_$TODAY_IS.rpt

file=filelist.lst

clean="clean"

while IFS= read -r line
do
    if [ "$clean" = "$1" ]; then
    	echo "Clean-up" $line
    	rm $line/dynamatic_*
    	rm -r $line/hdl
    	rm -r $line/reports
    	rm -r $line/sim
    else
	if [ "$line" != "" ]; then
    	echo "Testing" $line
    	scripts/regression_test.sh $line $report $report_file
    fi
    fi
 
done < "$file"
echo "Done" $line
