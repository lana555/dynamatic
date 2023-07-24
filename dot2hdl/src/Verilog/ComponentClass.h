/*
 * ComponentClass.h
 *
 *  Created on: 01-Jun-2021
 *      Author: madhur
 */

#ifndef COMPONENTCLASS_H_
#define COMPONENTCLASS_H_

#pragma once

#include "entity_names.h"

//Input/Output Connections have the name as follows:
//<name>_data
//<name>_valid
//<name>_ready
struct InputConnection{
	int vectorLength;
	std::string data;
	std::string valid;
	std::string ready;
};

struct OutputConnection{
	int vectorLength;
	std::string data;
	std::string valid;
	std::string ready;
};


//Stores all the information about a component
//If the component does not support any parameter,
//it is assigned the default value
class Component{
public:
	//Index is new and has to be added to all subclass constructors
	int index;
	std::string moduleName;
	std::string instanceName;
	std::string name;
	std::string type;
	int bbID;
	IN_T in;
	OUT_T out;
	float delay, latency;
	int II;
	int slots;
	bool transparent;
	std::string op;
	uint64_t value;

	/**Confirm this assumption*/ //=========>>> Assumption wrong for StoreComponent.
	//Assuming that two different blocks have only one unique connection between them
	//Then connection is vector containing all the connections that component has with other components
	//The vector contains pairs. The first element of that pair is the name of the component that is connected to this component
	//The second element of the pair is the input-output mapping of this component and the connected component
	std::vector<std::pair<Component*, std::pair<int, int>>> io;

	//Every entity of top module has clk and rst signal for synchronization
	std::string clk, rst;
	//Stores the module port input and output connections, using which
	//modules are connected to outside world
	std::string inputPortBus;
	std::map<int, InputConnection> inputConnections;
	std::string outputPortBus;
	std::map<int, OutputConnection> outputConnections;



	Component();
	void printComponent();
	int getVectorLength(std::string str);
	std::string generateVector(std::string str);
	std::string generateVector(int from, int to);
	Component* castToSubClass(Component* component);
	//Creates the port connections for individual modules like fork, join, merge...
	std::string getModulePortDeclarations(std::string tabs);
	//Instantiates the Component and generates a verilog code to instantiate this component
	std::string getModuleInstantiation(std::string tabs);

	std::string getInputOutputConnections();


protected:
	std::string getDataPortVector(std::string _str);
	std::string getAddressPortVector(std::string _str);


	//Fills the inputConnections and outputConnections vectors with input and output ports
	void setInputPortBus();//Connects Component Wires with component module inputs. This is called inside getModuleInstantiation
	void setInputConnections();
	void setOutputPortBus();//Connects Component Wires with component module outputs. This is called inside getModuleInstantiation
	void setOutputConnections();

	//This will create verilog code connecting outConn to inConn
	std::string connectInputOutput(InputConnection inConn, OutputConnection outConn);

	std::string getVerilogParameters();
private:
};



//Type Component Classes (To be moved to their respective class files)
class StartComponent : public Component{
public:
	std::string port_din;
	std::string port_valid;
	std::string port_ready;

	StartComponent(Component& c);
	std::string getModuleIODeclaration(std::string tabs);
	//Instantiates the Component and generates a verilog code to instantiate this component

	//Returns a string containing verilog code for connecting all the outputs of this components to the inputs
	//Of the components it is connected to
	std::string getInputOutputConnections();
};

class EndComponent : public Component{
public:
	std::string port_dout;
	std::string port_valid;
	std::string port_ready;

	EndComponent(Component& c);
	std::string getModuleIODeclaration(std::string tabs);
	//Instantiates the Component and generates a verilog code to instantiate this component
	std::string getModuleInstantiation(std::string tabs);

	//Returns a string containing verilog code for connecting all the outputs of this components to the inputs
	//Of the components it is connected to
	std::string getInputOutputConnections();
	void setInputPortBus();

private:
	std::string getVerilogParameters();
};



//Type Component Classes (To be moved to their respective class files)
class SourceComponent : public Component{
public:
	SourceComponent(Component& c);
	//Instantiates the Component and generates a verilog code to instantiate this component
	std::string getModuleInstantiation(std::string tabs);


private:
	//Overriding setInputPortBus functions as
	//Source only has Output port and no input ports
	std::string getVerilogParameters();
};

//Type Component Classes (To be moved to their respective class files)
class SinkComponent : public Component{
public:
	SinkComponent(Component& c);
	//Instantiates the Component and generates a verilog code to instantiate this component
	std::string getModuleInstantiation(std::string tabs);

	//Since this has no outputs hence no getInputOutputConnections() function for this Class
	//However, to maintain uniformity of code, we will connect the clk and rst in this sink
	//,although they are of no use.
	std::string getInputOutputConnections();

private:
	std::string getVerilogParameters();
};


//Type Component Classes (To be moved to their respective class files)
class ConstantComponent : public Component{
public:
	ConstantComponent(Component& c);
	//Instantiates the Component and generates a verilog code to instantiate this component
	std::string getModuleInstantiation(std::string tabs);

	//Returns a string containing verilog code for connecting all the outputs of this components to the inputs
	//Of the components it is connected to

private:
	void setInputPortBus();
};


//Type Component Classes (To be moved to their respective class files)
class ForkComponent : public Component{
public:
	ForkComponent(Component& c);

private:
};


//Type Component Classes (To be moved to their respective class files)
class MuxComponent : public Component{
public:
	MuxComponent(Component& c);
	//Instantiates the Component and generates a verilog code to instantiate this component
	std::string getModuleInstantiation(std::string tabs);

	//Returns a string containing verilog code for connecting all the outputs of this components to the inputs
	//Of the components it is connected to
	//	std::string getInputOutputConnections();
	void setInputPortBus();

private:
	std::string getVerilogParameters();
};


//Type Component Classes (To be moved to their respective class files)
class BranchComponent : public Component{
public:
	BranchComponent(Component& c);
	//Instantiates the Component and generates a verilog code to instantiate this component
	std::string getModuleInstantiation(std::string tabs);

	//Returns a string containing verilog code for connecting all the outputs of this components to the inputs
	//Of the components it is connected to
	void setInputPortBus();

private:
	std::string getVerilogParameters();
};

//Type Component Classes (To be moved to their respective class files)
class MergeComponent : public Component{
public:
	MergeComponent(Component& c);
};


//Type Component Classes (To be moved to their respective class files)
class ControlMergeComponent : public Component{
public:
	ControlMergeComponent(Component& c);
};


//Type Component Classes (To be moved to their respective class files)
class BufferComponent : public Component{
public:
	BufferComponent(Component& c);
};

//Type Component Classes (To be moved to their respective class files)
class AddComponent : public Component{
public:
	AddComponent(Component& c);
};


//Type Component Classes (To be moved to their respective class files)
class SubComponent : public Component{
public:
	SubComponent(Component& c);
};


//Type Component Classes (To be moved to their respective class files)
class MulComponent : public Component{
public:
	MulComponent(Component& c);
};



//Type Component Classes (To be moved to their respective class files)
class RemComponent : public Component{
public:
	RemComponent(Component& c);
};



//Type Component Classes (To be moved to their respective class files)
class AndComponent : public Component{
public:
	AndComponent(Component& c);
};

//Type Component Classes (To be moved to their respective class files)
class OrComponent : public Component{
public:
	OrComponent(Component& c);
};


//Type Component Classes (To be moved to their respective class files)
class XorComponent : public Component{
public:
	XorComponent(Component& c);
};



//Type Component Classes (To be moved to their respective class files)
class ShlComponent : public Component{
public:
	ShlComponent(Component& c);
};



//Type Component Classes (To be moved to their respective class files)
class LshrComponent : public Component{
public:
	LshrComponent(Component& c);
};


//Type Component Classes (To be moved to their respective class files)
class AshrComponent : public Component{
public:
	AshrComponent(Component& c);
};



//Type Component Classes (To be moved to their respective class files)
class EqComponent : public Component{
public:
	EqComponent(Component& c);
};


//Type Component Classes (To be moved to their respective class files)
class NeComponent : public Component{
public:
	NeComponent(Component& c);
};


//Type Component Classes (To be moved to their respective class files)
class UgtComponent : public Component{
public:
	UgtComponent(Component& c);
};


//Type Component Classes (To be moved to their respective class files)
class UgeComponent : public Component{
public:
	UgeComponent(Component& c);
};


//Type Component Classes (To be moved to their respective class files)
class SgtComponent : public Component{
public:
	SgtComponent(Component& c);
};


//Type Component Classes (To be moved to their respective class files)
class SgeComponent : public Component{
public:
	SgeComponent(Component& c);
};



//Type Component Classes (To be moved to their respective class files)
class UltComponent : public Component{
public:
	UltComponent(Component& c);
};


//Type Component Classes (To be moved to their respective class files)
class UleComponent : public Component{
public:
	UleComponent(Component& c);
};


//Type Component Classes (To be moved to their respective class files)
class SltComponent : public Component{
public:
	SltComponent(Component& c);
};


//Type Component Classes (To be moved to their respective class files)
class SleComponent : public Component{
public:
	SleComponent(Component& c);
};


//Type Component Classes (To be moved to their respective class files)
class SelectComponent : public Component{
public:
	SelectComponent(Component& c);
	//Instantiates the Component and generates a verilog code to instantiate this component
	std::string getModuleInstantiation(std::string tabs);

private:
	void setInputPortBus();
	std::string getVerilogParameters();
};

//Type Component Classes (To be moved to their respective class files)
class SZextComponent : public Component{
public:
	SZextComponent(Component& c);
};

//Type Component Classes (To be moved to their respective class files)
class GetPtrComponent : public Component{
public:
	GetPtrComponent(Component& c);
	//Instantiates the Component and generates a verilog code to instantiate this component
	std::string getModuleInstantiation(std::string tabs);

	//Returns a string containing verilog code for connecting all the outputs of this components to the inputs
	//Of the components it is connected to

private:
	std::string getVerilogParameters();
};


//Type Component Classes (To be moved to their respective class files)
class RetComponent : public Component{
public:
	RetComponent(Component& c);
};



//This interfaces with a BRAM
//The Configuration is supposedly Dual Port BRAM.
//One port is used for Loading the data(Read):->Port 1
//Other port is used for solely storing the data(Write):->Port 0
//Every Port has: address, ce, we, dout, din
//in1 gives the address port length
//out1 gives the data port length
//out2 is connected to end
class MemoryContentComponent : public Component{
public:
	//For Store operation
	std::string port_address_0;
	std::string port_ce_0;
	std::string port_we_0;
	std::string port_dout_0;
	std::string port_din_0;

	//For Load Operation
	std::string port_address_1;
	std::string port_ce_1;
	std::string port_we_1;
	std::string port_dout_1;
	std::string port_din_1;

	MemoryContentComponent(Component& c);
	std::string getModuleIODeclaration(std::string tabs);

	//Instantiates the Component and generates a verilog code to instantiate this component
	std::string getModuleInstantiation(std::string tabs);

	//Returns a string containing verilog code for connecting all the outputs of this components to the inputs
	//Of the components it is connected to
	std::string getInputOutputConnections();

private:
	std::string getVerilogParameters();
};



class LSQComponent : public Component{
public:
	//For Store operation
	std::string port_address_0;
	std::string port_ce_0;
	std::string port_we_0;
	std::string port_dout_0;
	std::string port_din_0;

	//For Load Operation
	std::string port_address_1;
	std::string port_ce_1;
	std::string port_we_1;
	std::string port_dout_1;
	std::string port_din_1;

	bool isNextMC;//If true, this means that one of the next components
	//LSQ is connected to is a MemCont.

	LSQComponent(Component& c);
	std::string getModuleIODeclaration(std::string tabs);

	//Instantiates the Component and generates a verilog code to instantiate this component
	std::string getModuleInstantiation(std::string tabs);

	//Returns a string containing verilog code for connecting all the outputs of this components to the inputs
	//Of the components it is connected to
	std::string getInputOutputConnections();

private:
	bool isNextMCF();
};



//Type Component Classes (To be moved to their respective class files)
class LoadComponent : public Component{
public:
	LoadComponent(Component& c);
	//Instantiates the Component and generates a verilog code to instantiate this component
	std::string getModuleInstantiation(std::string tabs);

	//Returns a string containing verilog code for connecting all the outputs of this components to the inputs
	//Of the components it is connected to
	std::string getInputOutputConnections();

private:
	void setInputPortBus();
	void setOutputPortBus();
	std::string getVerilogParameters();
};


//Type Component Classes (To be moved to their respective class files)
class StoreComponent : public Component{
public:
	StoreComponent(Component& c);
	//Instantiates the Component and generates a verilog code to instantiate this component
	std::string getModuleInstantiation(std::string tabs);

	//Returns a string containing verilog code for connecting all the outputs of this components to the inputs
	//Of the components it is connected to
	std::string getInputOutputConnections();

private:
	void setInputPortBus();
	void setOutputPortBus();
	std::string getVerilogParameters();
};


//Type Component Classes (To be moved to their respective class files)
class LSQControllerComponent : public Component{
public:
	LSQControllerComponent(Component& c);
	//Instantiates the Component and generates a verilog code to instantiate this component
	std::string getModuleInstantiation(std::string tabs);

	//Returns a string containing verilog code for connecting all the outputs of this components to the inputs
	//Of the components it is connected to
	std::string getInputOutputConnections();

private:
	std::string getVerilogParameters();
};


//Type Component Classes (To be moved to their respective class files)
class FIFOComponent : public Component{
public:
	FIFOComponent(Component& c);
	//Instantiates the Component and generates a verilog code to instantiate this component
	std::string getModuleInstantiation(std::string tabs);

	//Returns a string containing verilog code for connecting all the outputs of this components to the inputs
	//Of the components it is connected to
	std::string getInputOutputConnections();

private:
	std::string getVerilogParameters();
};





#endif /* COMPONENTCLASS_H_ */
