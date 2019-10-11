
## Simulation using ModelSim

**Always use VHDL 2008** to compile the project files. 

Use the following project setting: in the project tab, choose *add to project -> simulation configuration -> select top module -> set resolution to 'ps'*


## Using Xilinx libraries with ModelSim

Make sure you have **compatible versions** of Vivado and ModelSim. The following link contains a list of compatible versions:
https://www.xilinx.com/support/answers/68324.html

### Compile the libraries from Vivado
In Vivado, select Tools -> Compile simulation libraries -> ModelSim simulator, and set the path to where your ModelSim is. 

### Include libraries into ModelSim project
Once the export is done, a **modelsim.ini** file in the folder with your ModelSim libraries should contain the names of the floating point libraries as well, for instance:

```floating_point_v7_1_4 = ../modelsim_lib/floating_point_v7_1_4```

Include the modelsim.ini in your ModelSim project. When creating the ModelSim project, there is a field 'Copy settings from' where you can select this file.


