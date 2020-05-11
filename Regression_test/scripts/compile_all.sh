#!/bin/sh
#Authors: Andrea Guerrieri andrea.guerrieri@epfl.ch

report=$2
report_file=$3

workingdir=`pwd`

echo "" >> $report_file
echo "" >> $report_file
echo "**************************************************" >> $report_file
date >> $report_file

testcase=$1

echo "****Executing test case $testcase ****" >> $report_file
#Wo optimization
sh scripts/execute.sh $testcase $workingdir/$report >> $report_file

echo "****Executing test case $testcase optimized ****" >> $report_file
#With optimization
sh scripts/execute.sh $testcase $workingdir/$report optimized >> $report_file

