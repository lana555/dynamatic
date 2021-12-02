# Resource sharing

## From resource_sharing, create symbolic link to Buffers:

```bash
ln -s /home/dynamatic/Dynamatic/etc/dynamatic/Buffers/src src/DFnetlist
```

## From resource_sharing, compile the code:

```bash
cmake /home/dynamatic/Dynamatic/etc/dynamatic/resource_sharing
make
```

## To run the code (temporary):

Use the regression test to create the optimized version of an example in its regular folder. 
Add the example to filelist.lst in the resource_sharing folder.

In resource_sharing, create folder for output files:

```bash
mkdir output
```
From resource_sharing, run: 

```bash
./run_all.sh
```

This script will run resource sharing on all examples in filelist.lst.