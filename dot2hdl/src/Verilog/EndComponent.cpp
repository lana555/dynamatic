/*
 * EndComponent.cpp
 *
 *  Created on: 04-Jun-2021
 *      Author: madhur
 */


#include "ComponentClass.h"

EndComponent::EndComponent(Component& c){
	index = c.index;
	moduleName = "end_node";
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

	//here pvalid and pready are to distinguish these TopModule IO ports from(p stands for port)
	//local entity valid and ready signals
	//	port_dout = substring(name, 0, name.find('_')) + "_" + "dout";
	//	port_valid = substring(name, 0, name.find('_')) + "_" + "pvalid";
	//	port_ready = substring(name, 0, name.find('_')) + "_" + "pready";

	std::string end_name = substring(name, 0, name.rfind("_"));

	port_dout = end_name + "_out";
	port_valid = end_name + "_valid";
	port_ready = end_name + "_ready";

	if(out.size == 0){
		out.size = 1;
		out.output[0].bit_size = 1;
	}

}


//As long as there are no memory components, this will suffice.
//This will need to integrate outputs of memory components in the future
//Returns the input/output declarations for top-module
std::string EndComponent::getModuleIODeclaration(std::string tabs){
	std::string ret = "";
	ret += tabs + "output " + generateVector(out.output[0].bit_size - 1, 0) + port_dout + ",\n";
	ret += tabs + "output " + port_valid + ",\n";
	ret += tabs + "input " + port_ready + ",\n";
	ret += "\n";

	return ret;
}


void EndComponent::setInputPortBus(){
	InputConnection inConn;

	//First create data_in bus
	inputPortBus = ".data_in_bus({";
	//The bus will be assigned from highest input to lowest input. eg {in3, in2, in1}
	for(int i = inputConnections.size() - 1; i >= 0; i--){
		if(in.input[i].type == "u"){
			inConn = inputConnections[i];
			inputPortBus += inConn.data + ", ";
		}
	}
	inputPortBus = inputPortBus.erase(inputPortBus.size() - 2, 2);//This is needed to remove extra comma and space after bus is populated
	inputPortBus += "}), ";

	//Now create valid_in bus
	inputPortBus += ".valid_in_bus({";
	//The bus will be assigned from highest input to lowest input. eg {in3, in2, in1}
	for(int i = inputConnections.size() - 1; i >= 0; i--){
		if(in.input[i].type == "u"){
			inConn = inputConnections[i];
			inputPortBus += inConn.valid + ", ";
		}
	}
	inputPortBus = inputPortBus.erase(inputPortBus.size() - 2, 2);//This is needed to remove extra comma and space after bus is populated
	inputPortBus += "}), ";

	//Now create ready_in bus
	inputPortBus += ".ready_in_bus({";
	//The bus will be assigned from highest input to lowest input. eg {in3, in2, in1}
	for(int i = inputConnections.size() - 1; i >= 0; i--){
		if(in.input[i].type == "u"){
			inConn = inputConnections[i];
			inputPortBus += inConn.ready + ", ";
		}
	}
	inputPortBus = inputPortBus.erase(inputPortBus.size() - 2, 2);//This is needed to remove extra comma and space after bus is populated

	if(in.size == 1){
		inputPortBus += "})";//If no memory Inputs
	} else{
		inputPortBus += "}), ";//If there are memory inputs
	}

	if(in.size > 1){
		inputPortBus += ".e_valid_bus({";
		//The bus will be assigned from highest input to lowest input. eg {in3, in2, in1}
		for(int i = inputConnections.size() - 1; i >= 0; i--){
			if(in.input[i].type == "e"){
				inConn = inputConnections[i];
				inputPortBus += inConn.valid + ", ";
			}
		}
		inputPortBus = inputPortBus.erase(inputPortBus.size() - 2, 2);//This is needed to remove extra comma and space after bus is populated
		inputPortBus += "}), ";

		inputPortBus += ".e_ready_bus({";
		//The bus will be assigned from highest input to lowest input. eg {in3, in2, in1}
		for(int i = inputConnections.size() - 1; i >= 0; i--){
			if(in.input[i].type == "e"){
				inConn = inputConnections[i];
				inputPortBus += inConn.ready + ", ";
			}
		}
		inputPortBus = inputPortBus.erase(inputPortBus.size() - 2, 2);//This is needed to remove extra comma and space after bus is populated
		inputPortBus += "})";
	}
}


std::string EndComponent::getModuleInstantiation(std::string tabs){
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

std::string EndComponent::getVerilogParameters(){
	std::string ret;

	ret += "#(.INPUTS(1), .OUTPUTS(1), .MEMORY_INPUTS(" + std::to_string(in.size - 1) + "), ";
	ret += ".DATA_IN_SIZE(" + std::to_string(in.input[in.size - 1].bit_size == 0 ? 1 : in.input[in.size - 1].bit_size) + "), ";
	ret += ".DATA_OUT_SIZE(" + std::to_string(out.output[0].bit_size == 0 ? 1 : out.output[0].bit_size) + ")) ";

	return ret;
}


std::string EndComponent::getInputOutputConnections(){
	std::string ret;

	ret += "\tassign " + clk + " = clk;\n";
	ret += "\tassign " + rst + " = rst;\n";

	//**Not Complete Implementation. Need to integrate  emory Ports when memory is included**//
	//First one output of end component is connected to top module IO port
	//Which one is connected to top module is decided by the output list. The one which does not have
	//e as a suffix in output name is the one connected to top module
	if(outputConnections.size() != 0){
		ret += "\tassign " + port_dout + " = " + outputConnections.at(0).data + ";\n";
		ret += "\tassign " + outputConnections.at(0).ready + " = " + port_ready + ";\n";
		ret += "\tassign " + port_valid + " = " + outputConnections.at(0).valid + ";\n";
	}

	return ret;
}










