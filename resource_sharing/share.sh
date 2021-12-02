#!/bin/bash
exampleLocation=/home/dynamatic/Dynamatic/etc/dynamatic/Regression_test/examples
bufferExec=/home/dynamatic/Dynamatic/etc/dynamatic/Buffers/bin/buffers
ressourceMinExec=/home/dynamatic/Dynamatic/etc/dynamatic/resource_sharing/resource_minimization
dot2vhdlExec=/home/dynamatic/Dynamatic/etc/dynamatic/dot2vhdl/bin/dot2vhdl
verifierExec=/home/dynamatic/Dynamatic/etc/dynamatic/Regression_test/hls_verifier/HlsVerifier/build/hlsverifier
compFolder=/home/dynamatic/Dynamatic/etc/dynamatic/components

fileext=""
currentDir=$(pwd)
targetFolder=$currentDir/output/$1

rm -r $targetFolder
mkdir $targetFolder


cp $exampleLocation/$1/reports/$1_optimized.dot $targetFolder/$1.dot 
cp $exampleLocation/$1/reports/$1_bbgraph_buf.dot $targetFolder/$1_bbgraph.dot

$bufferExec buffers -filename=$targetFolder/$1 -period=5 -timeout=30 

mv $targetFolder/$1_graph_buf.dot $targetFolder/$1_graph.dot 
mv $targetFolder/$1_bbgraph_buf.dot $targetFolder/$1_bbgraph.dot


echo "Running resource minimization"
#run resource minimization
(
  cd $targetFolder
  mkdir _input
  mkdir _output
  mkdir _tmp
  cp $1_graph.dot _input
  cp $1_bbgraph.dot _input
  cp $1_bbgraph.dot _tmp/out_bbgraph.dot  
  $ressourceMinExec min $1 
  cp _output/$1_graph.dot .
  dot -Tpng ./_tmp/out.dot > ./_tmp/out.png
)


#run dot2vhdl
( 
  cd $targetFolder
  #need to rename $1_graph to $1 for the verfier afterwards
  cp $1_graph.dot $1.dot
  $dot2vhdlExec $targetFolder/$1
  rm $1.dot
)

cp -r $exampleLocation/$1/sim $targetFolder/sim_sharing 
cp $targetFolder/$1.vhd $targetFolder/sim_sharing/VHDL_SRC/$1_shared.vhd
#cp $compFolder/sharing_components.vhd $targetFolder/sim_sharing/VHDL_SRC/sharing_components.vhd
rm $targetFolder/sim_sharing/VHDL_SRC/$1_optimized.vhd
cp -r $targetFolder/_output $targetFolder/out_sharing
cd $targetFolder/sim_sharing/HLS_VERIFY/
rm -r work
$verifierExec cover -aw32 $targetFolder/sim_sharing/C_SRC/$1.cpp $targetFolder/sim_sharing/C_SRC/$1.cpp $1 

if [ $? = 0 ]; then

	echo "PASS" 

else
	echo "FAIL" 
fi

cd ../VHDL_OUT

ls *.dat

if [ $? = 0 ]; then

	echo "PASS"

else
	echo "FAIL" 
fi

