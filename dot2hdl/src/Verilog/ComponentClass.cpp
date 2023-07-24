/*
 * ComponentClass.cpp
 *
 *  Created on: 31-May-2021
 *      Author: madhur
 */

#include "ComponentClass.h"

//Default constructor to set every data item a default value
Component::Component(){
	index = 0;
	moduleName = DEFAULT_MODULE_NAME;
	instanceName = DEFAULT_INSTANCE_NAME;
	name = DEFAULT_NAME;
	type = DEFAULT_TYPE;
	bbID = DEFAULT_BBID;
	delay = DEFAULT_DELAY; latency = DEFAULT_LATENCY;
	II = DEFAULT_II;
	slots = DEFAULT_SLOTS;
	transparent = DEFAULT_TRANSPARENT;
	op = DEFAULT_OP;
	value = DEFAULT_VALUE;
}

//Prints data about a component
void Component::printComponent(){
	std::cout << "name: " << name << "\t"
			<< "type: " << type << "\t"
			<< "bbID: " << bbID << "\t"
			<< "op: " << op << "\t"
			<< "delay: " << delay << "\t"
			<< "latency" << latency << "\t"
			<< "II: " << II << "\t"
			<< "slots: " << slots << "\t"
			<< "transparent: " << transparent << "\t"
			<< "value: " << value << "\t"
			<< "IO Size: " << io.size() << std::endl;
}


//Returns a verilog vector: "[<from> : <to>]"
std::string Component::generateVector(int from, int to){
	from = from < 0 ? 0 : from;
	return ("[" + std::to_string(from) + " : " + std::to_string(to) + "]");
}

//This function is supposed to cast the Component Class object to their corresponding
//Entity classes and return those casted objects.
Component* Component::castToSubClass(Component* component){
	if(type == COMPONENT_START){
		StartComponent* obj = new StartComponent(*this);
		component = (Component *)obj;
		return component;
	} else if(type == COMPONENT_END){
		EndComponent* obj = new EndComponent(*this);
		component = (Component *)obj;
		return component;
	}  else if(type == COMPONENT_FORK){
		ForkComponent* obj = new ForkComponent(*this);
		component = (Component *)obj;
		return component;
	} else if(type == COMPONENT_OPERATOR){
		if(op == OPERATOR_ADD){
			AddComponent* obj = new AddComponent(*this);
			component = (Component *)obj;
			return component;
		} else if(op == OPERATOR_SUB){
			SubComponent* obj = new SubComponent(*this);
			component = (Component *)obj;
			return component;
		} else if(op == OPERATOR_MUL){
			MulComponent* obj = new MulComponent(*this);
			component = (Component *)obj;
			return component;
		} else if(op == OPERATOR_UREM){
			RemComponent* obj = new RemComponent(*this);
			component = (Component *)obj;
			return component;
		} else if(op == OPERATOR_AND){
			AndComponent* obj = new AndComponent(*this);
			component = (Component *)obj;
			return component;
		} else if(op == OPERATOR_OR){
			OrComponent* obj = new OrComponent(*this);
			component = (Component *)obj;
			return component;
		} else if(op == OPERATOR_XOR){
			XorComponent* obj = new XorComponent(*this);
			component = (Component *)obj;
			return component;
		} else if(op == OPERATOR_SHL){
			ShlComponent* obj = new ShlComponent(*this);
			component = (Component *)obj;
			return component;
		} else if(op == OPERATOR_LSHR){
			LshrComponent* obj = new LshrComponent(*this);
			component = (Component *)obj;
			return component;
		} else if(op == OPERATOR_ASHR){
			AshrComponent* obj = new AshrComponent(*this);
			component = (Component *)obj;
			return component;
		} else if(op == OPERATOR_EQ){
			EqComponent* obj = new EqComponent(*this);
			component = (Component *)obj;
			return component;
		} else if(op == OPERATOR_NE){
			NeComponent* obj = new NeComponent(*this);
			component = (Component *)obj;
			return component;
		} else if(op == OPERATOR_UGT){
			UgtComponent* obj = new UgtComponent(*this);
			component = (Component *)obj;
			return component;
		} else if(op == OPERATOR_UGE){
			UgeComponent* obj = new UgeComponent(*this);
			component = (Component *)obj;
			return component;
		} else if(op == OPERATOR_SGT){
			SgtComponent* obj = new SgtComponent(*this);
			component = (Component *)obj;
			return component;
		} else if(op == OPERATOR_SGE){
			SgeComponent* obj = new SgeComponent(*this);
			component = (Component *)obj;
			return component;
		} else if(op == OPERATOR_ULT){
			UltComponent* obj = new UltComponent(*this);
			component = (Component *)obj;
			return component;
		} else if(op == OPERATOR_ULE){
			UleComponent* obj = new UleComponent(*this);
			component = (Component *)obj;
			return component;
		} else if(op == OPERATOR_SLT){
			SltComponent* obj = new SltComponent(*this);
			component = (Component *)obj;
			return component;
		} else if(op == OPERATOR_SLE){
			SleComponent* obj = new SleComponent(*this);
			component = (Component *)obj;
			return component;
		} else if(op == OPERATOR_SEXT || op == OPERATOR_ZEXT){
			SZextComponent* obj = new SZextComponent(*this);
			component = (Component *)obj;
			return component;
		} else if(op == OPERATOR_GETPTR){
			GetPtrComponent* obj = new GetPtrComponent(*this);
			component = (Component *)obj;
			return component;
		} else if(op == OPERATOR_RET){
			RetComponent* obj = new RetComponent(*this);
			component = (Component *)obj;
			return component;
		} else if(op == OPERATOR_SELECT){
			SelectComponent* obj = new SelectComponent(*this);
			component = (Component *)obj;
			return component;
		} else if(op == OPERATOR_READ_MEMORY){
			LoadComponent* obj = new LoadComponent(*this);
			component = (Component *)obj;
			return component;
		} else if(op == OPERATOR_WRITE_MEMORY){
			StoreComponent* obj = new StoreComponent(*this);
			component = (Component *)obj;
			return component;
		} else if(op == OPERATOR_WRITE_LSQ || op == OPERATOR_READ_LSQ){
			LSQControllerComponent* obj = new LSQControllerComponent(*this);
			component = (Component *)obj;
			return component;
		}
	} else if(type == COMPONENT_MC){
		MemoryContentComponent* obj = new MemoryContentComponent(*this);
		component = (Component *)obj;
		return component;
	} else if(type == COMPONENT_LSQ){
		LSQComponent* obj = new LSQComponent(*this);
		component = (Component *)obj;
		return component;
	} else if(type == COMPONENT_SOURCE){
		SourceComponent* obj = new SourceComponent(*this);
		component = (Component *)obj;
		return component;
	} else if(type == COMPONENT_SINK){
		SinkComponent* obj = new SinkComponent(*this);
		component = (Component *)obj;
		return component;
	} else if(type == COMPONENT_CONSTANT_){
		ConstantComponent* obj = new ConstantComponent(*this);
		component = (Component *)obj;
		return component;
	} else if(type == COMPONENT_CTRLMERGE){
		ControlMergeComponent* obj = new ControlMergeComponent(*this);
		component = (Component *)obj;
		return component;
	} else if(type == COMPONENT_MERGE){
		MergeComponent* obj = new MergeComponent(*this);
		component = (Component *)obj;
		return component;
	} else if(type == COMPONENT_BUF || type == COMPONENT_TEHB || type == COMPONENT_OEHB){
		BufferComponent* obj = new BufferComponent(*this);
		component = (Component *)obj;
		return component;
	} else if(type == COMPONENT_MUX){
		MuxComponent* obj = new MuxComponent(*this);
		component = (Component *)obj;
		return component;
	} else if(type == COMPONENT_BRANCH){
		BranchComponent* obj = new BranchComponent(*this);
		component = (Component *)obj;
		return component;
	} else if(type == COMPONENT_MC){
		MemoryContentComponent* obj = new MemoryContentComponent(*this);
		component = (Component *)obj;
		return component;
	} else if(type == COMPONENT_TFIFO || type == COMPONENT_NFIFO){
		FIFOComponent* obj = new FIFOComponent(*this);
		component = (Component *)obj;
		return component;
	}

	return this;
}

//Populates the inputConnection map and sets the string(name) for each port of each input
//Eg. it gives unique names to data, ready and valid ports of a particular input
//The name will follow the convention:
//<Component name>_<input name>_data/ready/valid
void Component::setInputConnections(){
	for(int i = 0; i < in.size; i++)
	{
		int inputIndex = i;
		InputConnection inConn;
		inConn.vectorLength = in.input[inputIndex].bit_size;
		inConn.data = name + "_in" + std::to_string(inputIndex + 1) + "_" + "data";
		inConn.ready = name + "_in" + std::to_string(inputIndex + 1) + "_" + "ready";
		inConn.valid = name + "_in" + std::to_string(inputIndex + 1) + "_" + "valid";
		inputConnections[inputIndex] = inConn;
	}
}

//Same as setInputConnections(), but for outputs
void Component::setOutputConnections(){
	for(int i = 0; i < out.size; i++)
	{
		int outputIndex = i;
		OutputConnection outConn;
		outConn.vectorLength = out.output[outputIndex].bit_size;
		outConn.data = name + "_out" + std::to_string(outputIndex + 1) + "_" + "data";
		outConn.ready = name + "_out" + std::to_string(outputIndex + 1) + "_" + "ready";
		outConn.valid = name + "_out" + std::to_string(outputIndex + 1) + "_" + "valid";
		outputConnections[outputIndex] = outConn;
	}
}


//This returns the wires of an entity which connect it to other modules
//**This function also calls setInputConnections and setOutputConnections
//So, input Connections and outputConnections vectors of a component
//can be accessed only after this function has been called!
std::string Component::getModulePortDeclarations(std::string tabs){
	std::string ret = "";

	//**Clock and Reset signals are finally initialized here.
	//This should be moved to a function that is definitely called before this function.
	//Just in case this is required before this function.
	//A potential location for this snippet is the castToSubClass function as it is called as soon as
	//the component is created in dot_reader.
	//This could be moved to the respective entity subclass constructors
	//Just like port_din, port_valid and port_ready are moved in start, end and MC
	clk = name + "_" + "clk";
	rst = name + "_" + "rst";

	ret += tabs + "wire " + clk + ";\n";
	ret += tabs + "wire " + rst + ";\n";

	//If this Entity has any inputs:
	if(in.size != 0){
		//set inputConnections and fill it with all the available inputs
		setInputConnections();
		//Iterate over the inputConnections and add a wire for each connection
		for(auto it = inputConnections.begin(); it != inputConnections.end(); it++){
			ret += tabs + "wire " + generateVector((*it).second.vectorLength - 1, 0) + (*it).second.data + ";\n";
			ret += tabs + "wire " + (*it).second.ready + ";\n";
			ret += tabs + "wire " + (*it).second.valid + ";\n";
		}
	}
	//If this entity has any outputs:
	if(out.size != 0){
		//set outputConnections and fill it with all the available outputs
		setOutputConnections();
		//Iterate over the inputConnections and add a wire for each connection
		for(auto it = outputConnections.begin(); it != outputConnections.end(); it++){
			ret += tabs + "wire " + generateVector((*it).second.vectorLength - 1, 0) + (*it).second.data + ";\n";
			ret += tabs + "wire " + (*it).second.ready + ";\n";
			ret += tabs + "wire " + (*it).second.valid + ";\n";
		}
	}

	return ret;
}


//Overridden by Const and Select and LoadComponent and StoreComponent and EndComponent
void Component::setInputPortBus(){
	InputConnection inConn;

	//First create data_in bus
	inputPortBus = ".data_in_bus({";
	//The bus will be assigned from highest input to lowest input. eg {in3, in2, in1}
	for(int i = inputConnections.size() - 1; i >= 0; i--){
		inConn = inputConnections[i];
		inputPortBus += inConn.data + ", ";
	}
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

//Overridden by LoadComponent and StoreComponent
void Component::setOutputPortBus(){
	OutputConnection outConn;

	//First create data_in bus
	outputPortBus = ".data_out_bus({";
	//The bus will be assigned from highest output to lowest output. eg {out3, out2, out1}
	for(int i = outputConnections.size() - 1; i >= 0; i--){
		outConn = outputConnections[i];
		outputPortBus += outConn.data + ", ";
	}
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



//These two functions will be defined individually for each component subclass
std::string Component::getModuleInstantiation(std::string tabs){
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

std::string Component::getVerilogParameters(){
	std::string ret;

	ret += "#(.INPUTS(" + std::to_string(in.size) + "), .OUTPUTS(" + std::to_string(out.size) + "), ";
	ret += ".DATA_IN_SIZE(" + std::to_string(in.input[0].bit_size == 0 ? 1 : in.input[0].bit_size) + "), ";
	ret += ".DATA_OUT_SIZE(" + std::to_string(out.output[0].bit_size == 0 ? 1 : out.output[0].bit_size) + ")) ";
	//0 data size will lead to negative port length in verilog code. So 0 data size has to be made 1.
	return ret;
}


std::string Component::connectInputOutput(InputConnection inConn, OutputConnection outConn){
	std::string ret;

	ret += "\tassign " + inConn.data + " = " + outConn.data + ";\n";
	ret += "\tassign " + inConn.valid + " = " + outConn.valid + ";\n";
	ret += "\tassign " + outConn.ready + " = " + inConn.ready + ";\n";

	return ret;
}

std::string Component::getInputOutputConnections(){
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












