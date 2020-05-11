#!/bin/sh
#Authors: Andrea Guerrieri andrea.guerrieri@epfl.ch

export TODAY_IS=`date +%F%H%M%S`

report_file=report_detailed_$TODAY_IS.rpt
report=report_$TODAY_IS.rpt

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
    	echo "Synthesizing" $line
    	scripts/compile_all.sh $line $report $report_file
    fi
 
done < "$file"
