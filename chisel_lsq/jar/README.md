json file format
=======================
## json file format
All specifications of the LSQ should be defined in this file. 

1. name - This will be the module name and the file name.
2. speculation - false (speculation has not implemented yet).
3. dataWidth - width of load data and store data.
4. addrWidth - width of the address line. 
5. fifoDepth - depth of the store queue and load queue.
6. numLoadPorts - number of load ports associated with the lsq.
7. numStorePorts - number of store ports associated with the lsq.
8. numBBs - number of basic blocks. 
9. loadOffsets - left most value corresponds to 0-th load (LD0). 
10. storeOffsets - left most value corresponds to 0-th store (ST0).
11. loadPorts - loadPorts[i] is the list of load ports associated with the i-th basic block. Each element in the list should be an integer in [0, numLoadPorts)
12. storePorts -   storePorts[i] is the list of store ports associated with the i-th basic block. Each element in the list should be an integer in [0, numStorePorts)
13. numLoads - number of loads in each basic block. (leftmost value should correspond to BB0)
14. numStores - number of stores in each basic block. (leftmost value should correspond to BB0)
15. accessType - BRAM or AXI. Interfaces will be changed accordingly. 
16. bufferDepth - Buffer depth should be zero for BRAM. 

## How to generate the verilog

Invoke the following command.

java -jar [-Xmx{n}G]  lsq.jar [--target-dir  {target_dir}]  --spec-file  {spec_file.json}

The -Xmx{n}G (ex: -Xmx7G) parameter is optional and it instructs to allocate more heap memory, which is necessary for generating larger LSQs. n is the number of gigabytes allowed.

--target-dir is also an optional parameter. It can be used to specify a directory. 

