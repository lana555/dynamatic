#!/bin/sh


file=filelist.lst

while IFS= read -r line
do
   echo "\n\n\n\n\n======================================\n"
   make name=$line graph
    #echo "Testing" $line
    #scripts/regression_test.sh $line
done < "$file"
