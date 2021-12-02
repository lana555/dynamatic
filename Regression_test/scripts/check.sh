#!/bin/sh
#Authors: Andrea Guerrieri andrea.guerrieri@epfl.ch

echo $1
cd $1
filename=$1
path="examples/"

filename=`echo "$1" | sed 's/\<examples\>//g'`
filename=`echo "$filename" | tr -d /`

echo $filename 

echo "Simulating $filename" >> $2

rm -r sim 2>/dev/null

mkdir -p sim
mkdir -p sim/C_OUT
mkdir -p sim/C_SRC
mkdir -p sim/HLS_VERIFY
mkdir -p sim/INPUT_VECTORS
mkdir -p sim/VHDL_OUT
mkdir -p sim/VHDL_SRC

cp src/* sim/C_SRC/
cp hdl/* sim/VHDL_SRC/

cd sim/HLS_VERIFY/

echo -n "Checking results..." >> $2

../../../../hls_verifier/HlsVerifier/build/hlsverifier cover -aw32 ../C_SRC/$filename.cpp ../C_SRC/$filename.cpp $filename

if [ $? = 0 ]; then

	echo "PASS" >> $2

else
	echo "FAIL" >> $2
fi

echo -n "Checking if simulations run..." >> $2

cd ../VHDL_OUT

ls *.dat

if [ $? = 0 ]; then

	echo "PASS" >> $2

else
	echo "FAIL" >> $2
fi

cd ../../

		
		
