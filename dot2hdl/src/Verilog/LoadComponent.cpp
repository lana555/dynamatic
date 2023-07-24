/*
 * LoadComponent.cpp
 *
 *  Created on: 19-Jun-2021
 *      Author: madhur
 */

#include "ComponentClass.h"

//Subclass for Entry type component
LoadComponent::LoadComponent(Component& c){
	index = c.index;
	moduleName = "mc_load_op";
	name = c.name;
	instanceName = moduleName + "_" + name;
	type = c.type;
	bbID = c.bbID;
	op = c.op;
	in = c.in;
	out = c.out;
	delay = c.delay;
	latency = c.latency;
	II = c.II;
	slots = c.slots;
	transparent = c.transparent;
	value = c.value;
	io = c.io;
	inputConnections = c.inputConnections;
	outputConnections = c.outputConnections;

	clk = c.clk;
	rst = c.rst;
}

std::string LoadComponent::getModuleInstantiation(std::string tabs){
	setInputPortBus();
	setOutputPortBus();

	std::string ret;
	//Module name followed by verilog parameters followed by the Instance name
	ret += tabs;
	ret += moduleName + " " + getVerilogParameters() + instanceName + "\n";
	ret += tabs + "\t";
	ret += "(.clk(" + clk + "), .rst(" + rst + "),\n";
	ret += tabs + "\t";
	ret += inputPortBus + ", \n";
	ret += tabs + "\t";
	ret += outputPortBus + ");";

	return ret;
}

std::string LoadComponent::getVerilogParameters(){
	std::string ret;
	//This method of generating module parameters will work because Start node has
	//only 1 input and 1 output
	ret += "#(.INPUTS(2), .OUTPUTS(2), ";
	ret += ".ADDRESS_SIZE(" + std::to_string(in.input[1].bit_size == 0 ? 1 : in.input[1].bit_size) + "), ";
	ret += ".DATA_SIZE(" + std::to_string(in.input[0].bit_size == 0 ? 1 : in.input[0].bit_size) + ")) ";
	//0 data size will lead to negative port length in verilog code. So 0 data size has to be made 1.
	return ret;
}


//1 will be address, 0 will be data
void LoadComponent::setInputPortBus(){
	InputConnection inConn;

	//First create data_in bus
	inputPortBus = ".data_in_bus({";

	inConn = inputConnections[0];
	inputPortBus += inConn.data + ", ";

	inputPortBus = inputPortBus.erase(inputPortBus.size() - 2, 2);//This is needed to remove extra comma and space after bus is populated
	inputPortBus += "}), ";

	inputPortBus += ".address_in_bus({";

	inConn = inputConnections[1];
	inputPortBus += inConn.data + ", ";

	inputPortBus = inputPortBus.erase(inputPortBus.size() - 2, 2);//This is needed to remove extra comma and space after bus is populated
	inputPortBus += "}), ";

	//Now create valid_in bus
	inputPortBus += ".valid_in_bus({";
	//The bus will be assigned from highest input to lowest input. eg {in3, in2, in1}
	for(int i = inputConnections.size() - 1; i >= 0; i--){
		inConn = inputConnections[i];
		inputPortBus += inConn.valid + ", ";
	}
	inputPortBus = inputPortBus.erase(inputPortBus.size() - 2, 2);//This is needed to remove extra comma and space after bus is populated
	inputPortBus += "}), ";

	//Now create ready_in bus
	inputPortBus += ".ready_in_bus({";
	//The bus will be assigned from highest input to lowest input. eg {in3, in2, in1}
	for(int i = inputConnections.size() - 1; i >= 0; i--){
		inConn = inputConnections[i];
		inputPortBus += inConn.ready + ", ";
	}
	inputPortBus = inputPortBus.erase(inputPortBus.size() - 2, 2);//This is needed to remove extra comma and space after bus is populated
	inputPortBus += "})";
}

//1 will be address, 0 will be data
void LoadComponent::setOutputPortBus(){
	OutputConnection outConn;

	//First create data_in bus
	outputPortBus = ".data_out_bus({";

	outConn = outputConnections[0];
	outputPortBus += outConn.data + ", ";

	outputPortBus = outputPortBus.erase(outputPortBus.size() - 2, 2);//This is needed to remove extra comma and space after bus is populated
	outputPortBus += "}), ";

	//Then create address_in bus
	outputPortBus += ".address_out_bus({";

	outConn = outputConnections[1];
	outputPortBus += outConn.data + ", ";

	outputPortBus = outputPortBus.erase(outputPortBus.size() - 2, 2);//This is needed to remove extra comma and space after bus is populated
	outputPortBus += "}), ";

	//Now create valid_in bus
	outputPortBus += ".valid_out_bus({";
	//The bus will be assigned from highest output to lowest output. eg {out3, out2, out1}
	for(int i = outputConnections.size() - 1; i >= 0; i--){
		outConn = outputConnections[i];
		outputPortBus += outConn.valid + ", ";
	}
	outputPortBus = outputPortBus.erase(outputPortBus.size() - 2, 2);//This is needed to remove extra comma and space after bus is populated
	outputPortBus += "}), ";

	//Now create ready_in bus
	outputPortBus += ".ready_out_bus({";
	//The bus will be assigned from highest output to lowest output. eg {out3, out2, out1}
	for(int i = outputConnections.size() - 1; i >= 0; i--){
		outConn = outputConnections[i];
		outputPortBus += outConn.ready + ", ";
	}
	outputPortBus = outputPortBus.erase(outputPortBus.size() - 2, 2);//This is needed to remove extra comma and space after bus is populated
	outputPortBus += "})";
}


std::string LoadComponent::getInputOutputConnections(){
	std::string ret;

	ret += "\tassign " + clk + " = clk;\n";
	ret += "\tassign " + rst + " = rst;\n";

	InputConnection inConn;
	OutputConnection outConn;
	Component* connectedToComponent;
	int connectedFromPort, connectedToPort;
	for(auto it = io.begin(); it != io.end(); it++){
		connectedToComponent = (*it).first;
		connectedFromPort = (*it).second.first;
		connectedToPort = (*it).second.second;
		inConn = connectedToComponent->inputConnections[connectedToPort];
		outConn = outputConnections[connectedFromPort];
		ret += connectInputOutput(inConn, outConn);
	}

	return ret;
}


