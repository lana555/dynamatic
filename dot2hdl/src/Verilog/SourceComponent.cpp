/*
 * SourceComponent.cpp
 *
 *  Created on: 09-Jun-2021
 *      Author: madhur
 */

#include "ComponentClass.h"

SourceComponent::SourceComponent(Component& c){
	index = c.index;
	moduleName = "source_node";
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

std::string SourceComponent::getModuleInstantiation(std::string tabs){
	setInputPortBus();
	setOutputPortBus();

	std::string ret;
	//Module name followed by verilog parameters followed by the Instance name
	ret += tabs;
	ret += moduleName + " " + getVerilogParameters() + instanceName + "\n";
	ret += tabs + "\t";
	ret += "(.clk(" + clk + "), .rst(" + rst + "),\n";
	ret += tabs + "\t";
	ret += outputPortBus + ");";
	//Source does not have any Input port bus

	return ret;
}

std::string SourceComponent::getVerilogParameters(){
	std::string ret;

	ret += "#(.INPUTS(0), .OUTPUTS(1), ";
	ret += ".DATA_OUT_SIZE(" + std::to_string(out.output[0].bit_size == 0 ? 1 : out.output[0].bit_size) + "))";
	//Since there is no Input, DATA_IN_SIZE can be default
	return ret;
}
