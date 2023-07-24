/*
 * MemoryComponent.cpp
 *
 *  Created on: 04-Jun-2021
 *      Author: madhur
 */

#include "ComponentClass.h"

MemoryContentComponent::MemoryContentComponent(Component& c){
	index = c.index;
	moduleName = "MemCont";
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

	//MC_ is removed from the name to specify that it is Top Module IO port connection
	std::string memory_name = nodes[index].memory;
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

//Returns the input/output declarations for top-module
std::string MemoryContentComponent::getModuleIODeclaration(std::string tabs){
	std::string ret = "";

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

	return ret;
}


std::string MemoryContentComponent::getVerilogParameters(){
	std::string ret;


	ret += "#(";
	ret += ".DATA_SIZE(" + std::to_string(nodes[index].data_size) + "), ";
	ret += ".ADDRESS_SIZE(" + std::to_string(nodes[index].address_size) + "), ";
	ret += ".BB_COUNT(" + std::to_string(nodes[index].bbcount) + "), ";
	ret += ".LOAD_COUNT(" + std::to_string(nodes[index].load_count) + "), ";
	ret += ".STORE_COUNT(" + std::to_string(nodes[index].store_count) + "))";
	//0 data size will lead to negative port length in verilog code. So 0 data size has to be made 1.
	return ret;
}


std::string MemoryContentComponent::getInputOutputConnections(){
	std::string ret;

	ret += "\tassign " + clk + " = clk;\n";
	ret += "\tassign " + rst + " = rst;\n";

	ret += "\tassign " + port_ce_0 + " = " + port_we_0 + ";\n";

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



std::string MemoryContentComponent::getModuleInstantiation(std::string tabs){

	//Module Instantiation will be hard-coded because MemCont has a fixed instantiation

	std::string ret;
	//Module name followed by verilog parameters followed by the Instance name
	ret += tabs;

	ret += moduleName + " " + getVerilogParameters() + instanceName + "\n";
	ret += tabs + "\t";

	ret += "(.clk(" + clk + "), .rst(" + rst + "),\n";
	ret += tabs + "\t";

	ret +=  ".io_storeDataOut(" + port_dout_0 + "), .io_storeAddrOut(" + port_address_0 + "), .io_storeEnable(" + port_we_0 + "),\n";
	ret += tabs + "\t";

	ret +=  ".io_loadDataIn(" + port_din_1 + "), .io_loadAddrOut(" + port_address_1 + "), .io_loadEnable(" + port_ce_1 + "),\n";
	ret += tabs + "\t";


	ret +=  ".io_bbpValids({";
	for(int i = in.size - 1; i >= 0; i--){
		if(in.input[i].type == "c")
			ret +=  inputConnections[i].valid + ", ";
	}
	ret = ret.erase(ret.size() - 2, 2);
	ret += "}), ";

	ret +=  ".io_bb_stCountArray({";
	for(int i = in.size - 1; i >= 0; i--){
		if(in.input[i].type == "c")
			ret +=  inputConnections[i].data + ", ";
	}
	ret = ret.erase(ret.size() - 2, 2);
	ret += "}), ";


	ret +=  ".io_bbReadyToPrevs({";
	for(int i = in.size - 1; i >= 0; i--){
		if(in.input[i].type == "c")
			ret +=  inputConnections[i].ready + ", ";
	}
	ret = ret.erase(ret.size() - 2, 2);
	ret += "}), ";


	ret +=  ".io_rdPortsPrev_ready({";
	for(int i = in.size - 1; i >= 0; i--){
		if(in.input[i].info_type == "a" && in.input[i].type == "l")
			ret +=  inputConnections[i].ready + ", ";
	}
	ret = ret.erase(ret.size() - 2, 2);
	ret += "}), ";

	ret +=  ".io_rdPortsPrev_valid({";
	for(int i = in.size - 1; i >= 0; i--){
		if(in.input[i].info_type == "a" && in.input[i].type == "l")
			ret +=  inputConnections[i].valid + ", ";
	}
	ret = ret.erase(ret.size() - 2, 2);
	ret += "}), ";

	ret +=  ".io_rdPortsPrev_bits({";
	for(int i = in.size - 1; i >= 0; i--){
		if(in.input[i].info_type == "a" && in.input[i].type == "l")
			ret +=  inputConnections[i].data + ", ";
	}
	ret = ret.erase(ret.size() - 2, 2);
	ret += "}), ";
	ret += tabs + "\n\t";


	ret +=  ".io_wrAddrPorts_ready({";
	for(int i = in.size - 1; i >= 0; i--){
		if(in.input[i].info_type == "a" && in.input[i].type == "s")
			ret +=  inputConnections[i].ready + ", ";
	}
	ret = ret.erase(ret.size() - 2, 2);
	ret += "}), ";

	ret +=  ".io_wrAddrPorts_valid({";
	for(int i = in.size - 1; i >= 0; i--){
		if(in.input[i].info_type == "a" && in.input[i].type == "s")
			ret +=  inputConnections[i].valid + ", ";
	}
	ret = ret.erase(ret.size() - 2, 2);
	ret += "}), ";

	ret +=  ".io_wrAddrPorts_bits({";
	for(int i = in.size - 1; i >= 0; i--){
		if(in.input[i].info_type == "a" && in.input[i].type == "s")
			ret +=  inputConnections[i].data + ", ";
	}
	ret = ret.erase(ret.size() - 2, 2);
	ret += "}), ";
	ret += tabs + "\n\t";


	ret +=  ".io_wrDataPorts_ready({";
	for(int i = in.size - 1; i >= 0; i--){
		if(in.input[i].info_type == "d" && in.input[i].type == "s")
			ret +=  inputConnections[i].ready + ", ";
	}
	ret = ret.erase(ret.size() - 2, 2);
	ret += "}), ";

	ret +=  ".io_wrDataPorts_valid({";
	for(int i = in.size - 1; i >= 0; i--){
		if(in.input[i].info_type == "d" && in.input[i].type == "s")
			ret +=  inputConnections[i].valid + ", ";
	}
	ret = ret.erase(ret.size() - 2, 2);
	ret += "}), ";

	ret +=  ".io_wrDataPorts_bits({";
	for(int i = in.size - 1; i >= 0; i--){
		if(in.input[i].info_type == "d" && in.input[i].type == "s")
			ret +=  inputConnections[i].data + ", ";
	}
	ret = ret.erase(ret.size() - 2, 2);
	ret += "}), ";
	ret += tabs + "\n\t";


	ret +=  ".io_rdPortsNext_ready({";
	for(int i = out.size - 1; i >= 0; i--){
		if(out.output[i].type == "l")
			ret +=  outputConnections[i].ready + ", ";
	}
	ret = ret.erase(ret.size() - 2, 2);
	ret += "}), ";

	ret +=  ".io_rdPortsNext_valid({";
	for(int i = out.size - 1; i >= 0; i--){
		if(out.output[i].type == "l")
			ret +=  outputConnections[i].valid + ", ";
	}
	ret = ret.erase(ret.size() - 2, 2);
	ret += "}), ";

	ret +=  ".io_rdPortsNext_bits({";
	for(int i = out.size - 1; i >= 0; i--){
		if(out.output[i].type == "l")
			ret +=  outputConnections[i].data + ", ";
	}
	ret = ret.erase(ret.size() - 2, 2);
	ret += "}), ";
	ret += tabs + "\n\t";

	//ret +=  ".io_Empty_Ready(" + outputConnections[out.size - 1].ready + "), .io_Empty_Valid(" + outputConnections[out.size - 1].valid + "));";

	ret +=  ".io_Empty_Valid({";
	for(int i = out.size - 1; i >= 0; i--){
		if(out.output[i].type == "e")
			ret +=  outputConnections[i].valid + ", ";
	}
	ret = ret.erase(ret.size() - 2, 2);
	ret += "}), ";

	ret +=  ".io_Empty_Ready({";
		for(int i = out.size - 1; i >= 0; i--){
			if(out.output[i].type == "e")
				ret +=  outputConnections[i].ready + ", ";
		}
		ret = ret.erase(ret.size() - 2, 2);
		ret += "}));";

	return ret;
}

