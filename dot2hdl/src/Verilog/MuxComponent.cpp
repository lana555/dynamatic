/*
 * MuxComponent.cpp
 *
 *  Created on: 13-Jun-2021
 *      Author: madhur
 */


#include "ComponentClass.h"

//Subclass for Entry type component
MuxComponent::MuxComponent(Component& c){
	index = c.index;
	moduleName = "mux_node";
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


std::string MuxComponent::getModuleInstantiation(std::string tabs){
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

//In Mux, the first input(input[0]) is always the condition. It can be 1 or more bits wide
//The following inputs are the data to be selected
std::string MuxComponent::getVerilogParameters(){
	std::string ret;
	//This method of generating module parameters will work because Start node has
	//only 1 input and 1 output
	ret += "#(.INPUTS(" + std::to_string(in.size) + "), .OUTPUTS(" + std::to_string(out.size) + "), ";
	ret += ".DATA_IN_SIZE(" + std::to_string(in.input[1].bit_size == 0 ? 1 : in.input[1].bit_size) + "), ";
	ret += ".DATA_OUT_SIZE(" + std::to_string(out.output[0].bit_size == 0 ? 1 : out.output[0].bit_size) + "), ";
	ret += ".COND_SIZE(" + std::to_string(in.input[0].bit_size) + ")) ";
	//0 data size will lead to negative port length in verilog code. So 0 data size has to be made 1.
	return ret;
}


void MuxComponent::setInputPortBus(){
	InputConnection inConn;

	//First create data_in bus
	inputPortBus = ".data_in_bus({";

	//	//Since mux has {cond, data[N-1], data[N-2].....data[0]}
	//	inputPortBus += "{" + std::to_string(in.input[1].bit_size - in.input[0].bit_size) + "'b0, ";//Since Condition is N bit wide but Mux takes a 32 bit wide data for condition, so
	//
	//we need to add extra 0s to make it 32 bit wide
	inputPortBus += "{";
	//Condition bit

	if(in.input[0].bit_size < in.input[1].bit_size)
		inputPortBus += std::to_string(in.input[1].bit_size - in.input[0].bit_size) + "'b0, ";

	inConn = inputConnections[0];
	inputPortBus += inConn.data + "}, ";

	//The bus will be assigned from highest input to lowest input. eg {in3, in2, in1}
	for(int i = inputConnections.size() - 1; i > 0; i--){
		inConn = inputConnections[i];
		inputPortBus += inConn.data + ", ";
	}

	inputPortBus = inputPortBus.erase(inputPortBus.size() - 2, 2);//This is needed to remove extra comma and space after bus is populated
	inputPortBus += "}), ";

	//Now create valid_in bus
	inputPortBus += ".valid_in_bus({";
	inConn = inputConnections[0];
	inputPortBus += inConn.valid + ", ";
	//The bus will be assigned from highest input to lowest input. eg {in3, in2, in1}
	for(int i = inputConnections.size() - 1; i > 0; i--){
		inConn = inputConnections[i];
		inputPortBus += inConn.valid + ", ";
	}
	inputPortBus = inputPortBus.erase(inputPortBus.size() - 2, 2);//This is needed to remove extra comma and space after bus is populated
	inputPortBus += "}), ";

	//Now create ready_in bus
	inputPortBus += ".ready_in_bus({";
	inConn = inputConnections[0];
	inputPortBus += inConn.ready + ", ";
	//The bus will be assigned from highest input to lowest input. eg {in3, in2, in1}
	for(int i = inputConnections.size() - 1; i > 0; i--){
		inConn = inputConnections[i];
		inputPortBus += inConn.ready + ", ";
	}
	inputPortBus = inputPortBus.erase(inputPortBus.size() - 2, 2);//This is needed to remove extra comma and space after bus is populated
	inputPortBus += "})";
}
