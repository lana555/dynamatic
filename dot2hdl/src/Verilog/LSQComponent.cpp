/*
 * LSQComponent.cpp
 *
 *  Created on: 24-Jun-2021
 *      Author: madhur
 */


#include "ComponentClass.h"

LSQComponent::LSQComponent(Component& c){
	index = c.index;
	name = c.name;
	moduleName = name;
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

	isNextMC = isNextMCF();

	std::string memory_name;

	if(!isNextMC){
		memory_name = nodes[index].memory;
	}else {
		memory_name = name;
	}

	port_address_0 = memory_name + "_" + "address0";
	port_ce_0 = memory_name + "_" + "ce0";
	port_we_0 = memory_name + "_" + "we0";
	port_din_0 = memory_name + "_" + "din0";
	port_dout_0 = memory_name + "_" + "dout0";

	port_address_1 = memory_name + "_" + "address1";
	port_ce_1 = memory_name + "_" + "ce1";
	port_we_1 = memory_name + "_" + "we1";
	port_din_1 = memory_name + "_" + "din1";
	port_dout_1 = memory_name + "_" + "dout1";

}


bool LSQComponent::isNextMCF(){
	bool ret = false;

	for(int i = 0; i < out.size; i++){
		if(nodes[out.output[i].next_nodes_id].memory == nodes[index].memory)
			ret = true;
	}

	return ret;
}


//Returns the input/output declarations for top-module
std::string LSQComponent::getModuleIODeclaration(std::string tabs){
	std::string ret = "";

	if(!isNextMC){
		//in1 is always the address and out1 is always the data in/out
		ret += tabs + "output " + generateVector(nodes[index].address_size - 1, 0) + port_address_0 + ",\n";
		ret += tabs + "output " + port_ce_0 + ",\n";
		ret += tabs + "output " + port_we_0 + ",\n";
		ret += tabs + "output " + generateVector(nodes[index].data_size - 1, 0) + port_dout_0 + ",\n";
		ret += tabs + "input " + generateVector(nodes[index].data_size- 1, 0) + port_din_0 + ",\n";


		//in1 is always the address and out1 is always the data in/out
		ret += tabs + "output " + generateVector(nodes[index].address_size - 1, 0) + port_address_1 + ",\n";
		ret += tabs + "output " + port_ce_1 + ",\n";
		ret += tabs + "output " + port_we_1 + ",\n";
		ret += tabs + "output " + generateVector(nodes[index].data_size - 1, 0) + port_dout_1 + ",\n";
		ret += tabs + "input " + generateVector(nodes[index].data_size - 1, 0) + port_din_1 + ",\n";
		ret += "\n";
	}

	return ret;
}


std::string LSQComponent::getInputOutputConnections(){
	std::string ret;

	ret += "\tassign " + clk + " = clk;\n";
	ret += "\tassign " + rst + " = rst;\n";

	if(isNextMC){
		for(int i = in.size - 1; i >= 0; i--){
			if(in.input[i].type == "x" && in.input[i].info_type == "d"){
				ret += "\tassign " + inputConnections[i].ready + " = 1;\n";
			}
		}

		OutputConnection outConn_yd, outConn_ya;
		for(int i = out.size - 1; i >= 0; i--){
			if(out.output[i].type == "y" && out.output[i].info_type == "d"){
				outConn_yd = outputConnections[i];
			}
			if(out.output[i].type == "y" && out.output[i].info_type == "a"){
				outConn_ya = outputConnections[i];
			}
		}

		ret += "\tassign " + outConn_yd.valid + " = " + outConn_ya.valid + ";\n";

	} else{
		ret += "\tassign " + port_ce_0 + " = " + port_we_0 + ";\n";
	}

	for(int i = in.size - 1; i >= 0; i--){
		if(in.input[i].type == "c" && in.input[i].info_type == "fake"){
			ret += "\tassign " + inputConnections[i].valid + " = 0;\n";
			ret += "\tassign " + inputConnections[i].data + " = 0;\n";
		}
	}


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



std::string LSQComponent::getModuleInstantiation(std::string tabs){

	//Module Instantiation will be hard-coded because MemCont has a fixed instantiation

	std::string ret;
	//Module name followed by verilog parameters followed by the Instance name
	ret += tabs;

	ret += moduleName + " " + instanceName + "\n";
	ret += tabs + "\t";

	ret += "(.clock(" + clk + "), .reset(" + rst + "),\n";
	ret += tabs + "\t";

	if(!isNextMC){
		ret +=  ".io_storeDataOut(" + port_dout_0 + "), .io_storeAddrOut(" + port_address_0 + "), .io_storeEnable(" + port_we_0 + "),\n";
		ret += tabs + "\t";

		ret +=  ".io_loadDataIn(" + port_din_1 + "), .io_loadAddrOut(" + port_address_1 + "), .io_loadEnable(" + port_ce_1 + "),\n";
		ret += tabs + "\t";

		ret += ".io_memIsReadyForLoads(1'b1), .io_memIsReadyForStores(1'b1), ";
		ret += "\n\t" + tabs;
	} else{
		ret += ".io_memIsReadyForLoads(";
		//The bus will be assigned from highest input to lowest input. eg {in3, in2, in1}
		for(int i = out.size - 1; i >= 0; i--){
			if(out.output[i].type == "x" && out.output[i].info_type == "a")
				ret += outputConnections[i].ready;
		}
		ret += "), ";
		ret += "\n\t" + tabs;

		ret += ".io_memIsReadyForStores(";
		//The bus will be assigned from highest input to lowest input. eg {in3, in2, in1}
		for(int i = out.size - 1; i >= 0; i--){
			if(out.output[i].type == "y" && out.output[i].info_type == "a")
				ret += outputConnections[i].ready;
		}
		ret += "), ";
		ret += "\n\t" + tabs;


		ret += ".io_storeDataOut(";
		//The bus will be assigned from highest input to lowest input. eg {in3, in2, in1}
		for(int i = out.size - 1; i >= 0; i--){
			if(out.output[i].type == "y" && out.output[i].info_type == "d")
				ret += outputConnections[i].data;
		}
		ret += "), ";
		ret += "\n\t" + tabs;

		ret += ".io_storeAddrOut(";
		//The bus will be assigned from highest input to lowest input. eg {in3, in2, in1}
		for(int i = out.size - 1; i >= 0; i--){
			if(out.output[i].type == "y" && out.output[i].info_type == "a")
				ret += outputConnections[i].data;
		}
		ret += "), ";
		ret += "\n\t" + tabs;

		ret += ".io_storeEnable(";
		//The bus will be assigned from highest input to lowest input. eg {in3, in2, in1}
		for(int i = out.size - 1; i >= 0; i--){
			if(out.output[i].type == "y" && out.output[i].info_type == "a")
				ret += outputConnections[i].valid;
		}
		ret += "), ";
		ret += "\n\t" + tabs;


		ret += ".io_loadDataIn(";
		//The bus will be assigned from highest input to lowest input. eg {in3, in2, in1}
		for(int i = in.size - 1; i >= 0; i--){
			if(in.input[i].type == "x" && in.input[i].info_type == "d")
				ret += inputConnections[i].data;
		}
		ret += "), ";
		ret += "\n\t" + tabs;

		ret += ".io_loadAddrOut(";
		//The bus will be assigned from highest input to lowest input. eg {in3, in2, in1}
		for(int i = out.size - 1; i >= 0; i--){
			if(out.output[i].type == "x" && out.output[i].info_type == "a")
				ret += outputConnections[i].data;
		}
		ret += "), ";
		ret += "\n\t" + tabs;

		ret += ".io_loadEnable(";
		//The bus will be assigned from highest input to lowest input. eg {in3, in2, in1}
		for(int i = out.size - 1; i >= 0; i--){
			if(out.output[i].type == "x" && out.output[i].info_type == "a")
				ret += outputConnections[i].valid;
		}
		ret += "), ";
		ret += "\n\t" + tabs;
	}


	for(int i = 0, ind = 0; i < in.size; i++){
		if(in.input[i].type == "c"){
			ret +=  ".io_bbpValids_" + std::to_string(ind++) + "(";
			ret +=  inputConnections[i].valid;
			ret += "), ";
		}
	}
	ret += "\n\t" + tabs;


	for(int i = 0, ind = 0; i < in.size; i++){
		if(in.input[i].type == "c"){
			ret +=  ".io_bbReadyToPrevs_" + std::to_string(ind++) + "(";
			ret +=  inputConnections[i].ready;
			ret += "), ";
		}
	}
	ret += "\n\t" + tabs;



	for(int i = 0, ind = 0; i < in.size; i++){
		if(in.input[i].info_type == "a" && in.input[i].type == "l"){
			ret +=  ".io_rdPortsPrev_" + std::to_string(ind++) + "_ready" + "(";
			ret +=  inputConnections[i].ready;
			ret += "), ";
		}
	}
	ret += "\n\t" + tabs;

	for(int i = 0, ind = 0; i < in.size; i++){
		if(in.input[i].info_type == "a" && in.input[i].type == "l"){
			ret +=  ".io_rdPortsPrev_" + std::to_string(ind++) + "_valid" + "(";
			ret +=  inputConnections[i].valid;
			ret += "), ";
		}
	}
	ret += "\n\t" + tabs;

	for(int i = 0, ind = 0; i < in.size; i++){
		if(in.input[i].info_type == "a" && in.input[i].type == "l"){
			ret +=  ".io_rdPortsPrev_" + std::to_string(ind++) + "_bits" + "(";
			ret +=  inputConnections[i].data;
			ret += "), ";
		}
	}
	ret += "\n\t" + tabs;


	for(int i = 0, ind = 0; i < in.size; i++){
		if(in.input[i].info_type == "a" && in.input[i].type == "s"){
			ret +=  ".io_wrAddrPorts_" + std::to_string(ind++) + "_ready" + "(";
			ret +=  inputConnections[i].ready;
			ret += "), ";
		}
	}
	ret += "\n\t" + tabs;

	for(int i = 0, ind = 0; i < in.size; i++){
		if(in.input[i].info_type == "a" && in.input[i].type == "s"){
			ret +=  ".io_wrAddrPorts_" + std::to_string(ind++) + "_valid" + "(";
			ret +=  inputConnections[i].valid;
			ret += "), ";
		}
	}
	ret += "\n\t" + tabs;

	for(int i = 0, ind = 0; i < in.size; i++){
		if(in.input[i].info_type == "a" && in.input[i].type == "s"){
			ret +=  ".io_wrAddrPorts_" + std::to_string(ind++) + "_bits" + "(";
			ret +=  inputConnections[i].data;
			ret += "), ";
		}
	}
	ret += "\n\t" + tabs;


	for(int i = 0, ind = 0; i < in.size; i++){
		if(in.input[i].info_type == "d" && in.input[i].type == "s"){
			ret +=  ".io_wrDataPorts_" + std::to_string(ind++) + "_ready" + "(";
			ret +=  inputConnections[i].ready;
			ret += "), ";
		}
	}
	ret += "\n\t" + tabs;

	for(int i = 0, ind = 0; i < in.size; i++){
		if(in.input[i].info_type == "d" && in.input[i].type == "s"){
			ret +=  ".io_wrDataPorts_" + std::to_string(ind++) + "_valid" + "(";
			ret +=  inputConnections[i].valid;
			ret += "), ";
		}
	}
	ret += "\n\t" + tabs;

	for(int i = 0, ind = 0; i < in.size; i++){
		if(in.input[i].info_type == "d" && in.input[i].type == "s"){
			ret +=  ".io_wrDataPorts_" + std::to_string(ind++) + "_bits" + "(";
			ret +=  inputConnections[i].data;
			ret += "), ";
		}
	}
	ret += "\n\t" + tabs;


	for(int i = 0, ind = 0; i < out.size; i++){
		if(out.output[i].info_type == "d" && out.output[i].type == "l"){
			ret +=  ".io_rdPortsNext_" + std::to_string(ind++) + "_ready" + "(";
			ret +=  outputConnections[i].ready;
			ret += "), ";
		}
	}
	ret += "\n\t" + tabs;

	for(int i = 0, ind = 0; i < out.size; i++){
		if(out.output[i].info_type == "d" && out.output[i].type == "l"){
			ret +=  ".io_rdPortsNext_" + std::to_string(ind++) + "_valid" + "(";
			ret +=  outputConnections[i].valid;
			ret += "), ";
		}
	}
	ret += "\n\t" + tabs;

	for(int i = 0, ind = 0; i < out.size; i++){
		if(out.output[i].info_type == "d" && out.output[i].type == "l"){
			ret +=  ".io_rdPortsNext_" + std::to_string(ind++) + "_bits" + "(";
			ret +=  outputConnections[i].data;
			ret += "), ";
		}
	}
	ret += "\n\t" + tabs;


	ret +=  ".io_Empty_Valid({";
	for(int i = out.size - 1; i >= 0; i--){
		if(out.output[i].type == "e")
			ret +=  outputConnections[i].valid + ", ";
	}
	ret = ret.erase(ret.size() - 2, 2);
	ret += "}));";

	return ret;
}



