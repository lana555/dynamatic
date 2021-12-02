#!/bin/sh
#Authors: Andrea Guerrieri andrea.guerrieri@epfl.ch

#export TODAY_IS=`date +%F%H%M%S`

#report_file=regression_test_detailed_$TODAY_IS.rpt
#report=regression_test_$TODAY_IS.rpt

report=$2
report_file=$3

workingdir=`pwd`

#Exits wrapper
#report () 
#{
#  printf "$(date)\t$1\t$2\n" >> log.csv
#}
#
#printf "Date\tOperation\tResult\n" >> log.csv

echo "" >> $report_file
echo "" >> $report_file
echo "**************************************************" >> $report_file
date >> $report_file

testcase=$1

echo "****Executing test case $testcase ****" >> $report_file
#Wo optimization
sh scripts/execute.sh $testcase $workingdir/$report >> $report_file
sh scripts/check.sh $testcase $workingdir/$report >> $report_file

echo "****Executing test case $testcase optimized ****" >> $report_file
#With optimization
sh scripts/execute.sh $testcase $workingdir/$report optimized >> $report_file
sh scripts/check.sh $testcase $workingdir/$report optimized >> $report_file

#report "Execution" $result

