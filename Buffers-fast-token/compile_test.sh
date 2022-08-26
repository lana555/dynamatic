#!/bin/sh


file=filelist.lst

while IFS= read -r line
do
   echo "\n\n\n\n==========================================="
   echo "================== "$line" ====================\n"
   #make name=$line graph
   bin/buffers buffers 6 0.0 cbc "examples/"$line"_graph.dot" _build/t.dot
    #echo "Testing" $line
    #scripts/regression_test.sh $line
done < "$file"
