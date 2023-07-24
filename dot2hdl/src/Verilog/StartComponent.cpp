/*
 * StartComponent.cpp
 *
 *  Created on: 04-Jun-2021
 *      Author: madhur
 */

#include "ComponentClass.h"

//Subclass for Entry type component
StartComponent::StartComponent(Component& c){
	index = c.index;
	moduleName = "start_node";
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

	std::string start_name = name;
	if(name == "start_0"){
		start_name = "start";

		port_din = start_name + "_in";
		port_valid = start_name + "_valid";
		port_ready = start_name + "_ready";
	}else {
		port_din = start_name + "_din";
		port_valid = start_name + "_valid_in";
		port_ready = start_name + "_ready_in";
	}
}

//Returns the input/output declarations for top-module
std::string StartComponent::getModuleIODeclaration(std::string tabs){
	std::string ret = "";
	//Start has only one input
	ret += tabs + "input " + generateVector(in.input[0].bit_size - 1, 0) + port_din + ",\n";
	ret += tabs + "input " + port_valid + ",\n";
	ret += tabs + "output " + port_ready + ",\n";
	ret += "\n";

	return ret;
}


std::string StartComponent::getInputOutputConnections(){
	std::string ret;

	ret += "\tassign " + clk + " = clk;\n";
	ret += "\tassign " + rst + " = rst;\n";

	//First input of start component is connected to top module IO port
	ret += "\tassign " + inputConnections.at(0).data + " = " + port_din + ";\n";
	ret += "\tassign " + inputConnections.at(0).valid + " = " + port_valid + ";\n";
	ret += "\tassign " + port_ready + " = " + inputConnections.at(0).ready + ";\n";


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
