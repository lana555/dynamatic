#!/bin/sh
#Authors: Andrea Guerrieri andrea.guerrieri@epfl.ch

export TODAY_IS=`date +%F%H%M%S`

report=report_$TODAY_IS.rpt

file=filelist.lst

clean="clean"

while IFS= read -r line
do

    echo "Testing" $line
    ./share.sh $line minimize cpp > $report

    
done < "$file"

