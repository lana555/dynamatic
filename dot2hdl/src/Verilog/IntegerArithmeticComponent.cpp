/*
 * IntegerArithmeticComponent.cpp
 *
 *  Created on: 05-Jun-2021
 *      Author: madhur
 */

#include "ComponentClass.h"


//Subclass for Entry type component
AddComponent::AddComponent(Component& c){
	index = c.index;
	moduleName = "add_op";
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



//Subclass for Entry type component
SubComponent::SubComponent(Component& c){
	index = c.index;
	moduleName = "sub_op";
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



//Subclass for Entry type component
MulComponent::MulComponent(Component& c){
	index = c.index;
	moduleName = "mul_op";
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



//Subclass for Entry type component
RemComponent::RemComponent(Component& c){
	index = c.index;
	moduleName = "urem_op";
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

//Subclass for Entry type component
AndComponent::AndComponent(Component& c){
	index = c.index;
	moduleName = "and_op";
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


//Subclass for Entry type component
OrComponent::OrComponent(Component& c){
	index = c.index;
	moduleName = "or_op";
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


//Subclass for Entry type component
XorComponent::XorComponent(Component& c){
	index = c.index;
	moduleName = "xor_op";
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


//Subclass for Entry type component
ShlComponent::ShlComponent(Component& c){
	index = c.index;
	moduleName = "shl_op";
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


//Subclass for Entry type component
LshrComponent::LshrComponent(Component& c){
	index = c.index;
	moduleName = "lshr_op";
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


//Subclass for Entry type component
AshrComponent::AshrComponent(Component& c){
	index = c.index;
	moduleName = "ashr_op";
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


//Subclass for Entry type component
EqComponent::EqComponent(Component& c){
	index = c.index;
	moduleName = "icmp_eq_op";
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


//Subclass for Entry type component
NeComponent::NeComponent(Component& c){
	index = c.index;
	moduleName = "icmp_ne_op";
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


//Subclass for Entry type component
UgtComponent::UgtComponent(Component& c){
	index = c.index;
	moduleName = "icmp_ugt_op";
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


//Subclass for Entry type component
UgeComponent::UgeComponent(Component& c){
	index = c.index;
	moduleName = "icmp_uge_op";
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


//Subclass for Entry type component
SgtComponent::SgtComponent(Component& c){
	index = c.index;
	moduleName = "icmp_sgt_op";
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




//Subclass for Entry type component
SgeComponent::SgeComponent(Component& c){
	index = c.index;
	moduleName = "icmp_sge_op";
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


//Subclass for Entry type component
UltComponent::UltComponent(Component& c){
	index = c.index;
	moduleName = "icmp_ult_op";
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


//Subclass for Entry type component
UleComponent::UleComponent(Component& c){
	index = c.index;
	moduleName = "icmp_ule_op";
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

//Subclass for Entry type component
SltComponent::SltComponent(Component& c){
	index = c.index;
	moduleName = "icmp_slt_op";
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


//Subclass for Entry type component
SleComponent::SleComponent(Component& c){
	index = c.index;
	moduleName = "icmp_sle_op";
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


//Subclass for Entry type component
SelectComponent::SelectComponent(Component& c){
	index = c.index;
	moduleName = "select_op";
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

void SelectComponent::setInputPortBus(){
	InputConnection inConn;

	//First create data_in bus
	inputPortBus = ".data_in_bus({";
	//The bus will be assigned from highest input to lowest input. eg {in3, in2, in1}
	for(int i = inputConnections.size() - 1; i > 0; i--){
		inConn = inputConnections[i];
		inputPortBus += inConn.data + ", ";
	}
	inputPortBus += "{" + std::to_string((in.input[1].bit_size == 0 ? 1 : in.input[1].bit_size) - 1) + "'b0, ";//Since Condition is one bit wide but Select takes a 32 bit wide data for condition, so
	//we need to add extra 31 0s to make it 32 bit wide
	//Condition bit
	inConn = inputConnections[0];
	inputPortBus += inConn.data + "}";

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

std::string SelectComponent::getModuleInstantiation(std::string tabs){
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

std::string SelectComponent::getVerilogParameters(){

	std::string ret;
	//This method of generating module parameters will work because Start node has
	//only 1 input and 1 output
	ret += "#(.INPUTS(3), .OUTPUTS(1), ";
	ret += ".DATA_IN_SIZE(" + std::to_string(in.input[1].bit_size == 0 ? 1 : in.input[1].bit_size) + "), ";
	ret += ".DATA_OUT_SIZE(" + std::to_string(out.output[0].bit_size == 0 ? 1 : out.output[0].bit_size) + ")) ";
	//0 data size will lead to negative port length in verilog code. So 0 data size has to be made 1.
	return ret;
}

//Subclass for Entry type component
RetComponent::RetComponent(Component& c){
	index = c.index;
	moduleName = "ret_op";
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

//Subclass for Entry type component
SZextComponent::SZextComponent(Component& c){
	index = c.index;
	if(c.op == OPERATOR_SEXT)
		moduleName = "sext_op";
	else if(c.op == OPERATOR_ZEXT)
		moduleName = "zext_op";
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


//Subclass for Entry type component
GetPtrComponent::GetPtrComponent(Component& c){
	index = c.index;
	moduleName = "getelementptr_op";
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

std::string GetPtrComponent::getModuleInstantiation(std::string tabs){
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

std::string GetPtrComponent::getVerilogParameters(){
	std::string ret;
	//This method of generating module parameters will work because Start node has
	//only 1 input and 1 output
	ret += "#(.INPUTS(" + std::to_string(in.size) +  "), .OUTPUTS(" + std::to_string(out.size) + "), ";
	ret += ".CONST_SIZE(" + std::to_string(nodes[index].constants) + "), ";
	ret += ".DATA_IN_SIZE(" + std::to_string(in.input[0].bit_size == 0 ? 1 : in.input[0].bit_size) + "), ";
	ret += ".DATA_OUT_SIZE(" + std::to_string(out.output[0].bit_size == 0 ? 1 : out.output[0].bit_size) + ")) ";
	//0 data size will lead to negative port length in verilog code. So 0 data size has to be made 1.
	return ret;
}

