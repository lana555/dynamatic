/*
 * GraphToVerilog.cpp
 *
 *  Created on: 31-May-2021
 *      Author: madhur
 */

#include "GraphToVerilog.h"

//Constructor
//GraphToVerilog::GraphToVerilog(DotReader dotReader){
GraphToVerilog::GraphToVerilog(std::string filen){
	//	this->dotReader = dotReader;

	DotReader dotReader(filen);
	dotReader.lineReader();
	this->dotReader = dotReader;
	std::cout << "Dot Read for Verilog" << std::endl;

	verilogCode = "";
	tabs = "";
}


void GraphToVerilog::writeToFile(){
	std::string file_n = dotReader.getFileName() + ".v";
	std::ofstream outStream(file_n);
	writeVerilogCode();
	outStream << getVerilogCode() << std::endl;

	outStream.close();
}

//Function that calls other functions to append their generated verilog code snippet
//The sequence in which the sub-functions are called determines in which sequence the snippet
//will appear in the final code.
//Eg. Module Name is the first thing of a verilog code
//Followed by Top Module Ports Input output declarations
//Followed by declaring wires that connect various sub components in the top module
void GraphToVerilog::writeVerilogCode(){
	verilogCode += writeTopModuleName();

	insertTab();
	verilogCode += writeTopModulePorts();
	removeTab();

	insertTab();
	verilogCode += writeModulePortWires();
	removeTab();


	insertTab();
	verilogCode += writeInputOutputConnections();
	removeTab();


	insertTab();
	verilogCode += writeModuleInstantiation();
	removeTab();

	verilogCode += writeEndModule();
}


//Declares the top module.
std::string GraphToVerilog::writeTopModuleName(){
	std::string moduleName;

	std::string filennn = dotReader.getFileName();

	unsigned long pos = filennn.rfind("_optimized");
	if(pos != std::string::npos){
		std::cout << "Found" << std::endl;
		filennn = substring(filennn, 0, pos);
	}

	moduleName = "module " + filennn + "(\n";

	return moduleName;
}

//This function populates the topModulePortComponents which will contain
//All the components that need to have input output declarations in the top module
//port list
void GraphToVerilog::generateTopModulePortComponents(){
	for(auto it = dotReader.getComponentList().begin(); it != dotReader.getComponentList().end(); it++){
		if((*it)->type == COMPONENT_START ||
				(*it)->type == COMPONENT_END ||
				(*it)->type == COMPONENT_MC ||
				(*it)->type == COMPONENT_LSQ){
			topModulePortComponents.push_back((*it));
		}
	}
}

//IO Ports are declared for components which are directly interfacing
//with the outside world.
//The following component types are to be provided IO Ports:
//1. Entry
//2. Exit
//3. LSQ
//4. MC

std::string GraphToVerilog::writeTopModulePorts(){
	std::string topModulePortList;
	generateTopModulePortComponents();

	//Clock and Reset signal are always present in the design ports
	topModulePortList += tabs + "input clk,\n" + tabs + "input rst,\n\n";

	//Writing IO Ports:

	for(auto it = topModulePortComponents.begin(); it != topModulePortComponents.end(); it++){
		if((*it)->type == COMPONENT_START){
			StartComponent* startComponent = (StartComponent*)(*it);
			topModulePortList += startComponent->getModuleIODeclaration(tabs);
		} else if((*it)->type == COMPONENT_END){
			EndComponent* endComponent = (EndComponent*)(*it);
			topModulePortList += endComponent->getModuleIODeclaration(tabs);
		} else if((*it)->type == COMPONENT_MC){
			MemoryContentComponent* memoryContentComponent = (MemoryContentComponent*)(*it);
			topModulePortList += memoryContentComponent->getModuleIODeclaration(tabs);
		} else if((*it)->type == COMPONENT_LSQ){
			//If LSQ has isNextMC true, then this function will return empty string
			//Hence no io ports will be generated
			LSQComponent* LSQContentComponent = (LSQComponent*)(*it);
			topModulePortList += LSQContentComponent->getModuleIODeclaration(tabs);
		}
	}

	//Erasing this necessary to get rid of an extra "," just before closing top module port declarations
	topModulePortList = topModulePortList.erase(topModulePortList.size() - 3, 1);

	topModulePortList += ");\n\n";

	return topModulePortList;
}



//This function accesses the getModulePortDeclarations function of each component
//(Because every component needs wires to connect) to generate
//a declaration of all wires of input/output connections of the sub modules
//They include the data, valid and ready signal for all input and outputs of that component
//Additional ports can be added if needed for a particular component by overriding getModulePortDeclarations
//function in a sub class
std::string GraphToVerilog::writeModulePortWires(){
	std::string modulePortWires;

	for(auto it = dotReader.getComponentList().begin(); it != dotReader.getComponentList().end(); it++){
		modulePortWires += (*it)->getModulePortDeclarations(tabs);
		modulePortWires += "\n";
	}

	return modulePortWires;
}


std::string GraphToVerilog::writeModuleInstantiation(){
	std::string moduleInstances = "";

	for(auto it = dotReader.getComponentList().begin(); it != dotReader.getComponentList().end(); it++){
		if((*it)->type == COMPONENT_START){
			moduleInstances += ((StartComponent*)(*it))->getModuleInstantiation(tabs);
			moduleInstances += "\n\n";
		} else if((*it)->type == COMPONENT_END){
			moduleInstances += ((EndComponent*)(*it))->getModuleInstantiation(tabs);
			moduleInstances += "\n\n";
		} else if((*it)->type == COMPONENT_FORK){
			moduleInstances += ((ForkComponent*)(*it))->getModuleInstantiation(tabs);
			moduleInstances += "\n\n";
		} else if((*it)->type == COMPONENT_OPERATOR){
			if((*it)->op == OPERATOR_ADD){
				moduleInstances += ((AddComponent*)(*it))->getModuleInstantiation(tabs);
				moduleInstances += "\n\n";
			} else if((*it)->op == OPERATOR_SUB){
				moduleInstances += ((SubComponent*)(*it))->getModuleInstantiation(tabs);
				moduleInstances += "\n\n";
			} else if((*it)->op == OPERATOR_MUL){
				moduleInstances += ((MulComponent*)(*it))->getModuleInstantiation(tabs);
				moduleInstances += "\n\n";
			} else if((*it)->op == OPERATOR_UREM){
				moduleInstances += ((RemComponent*)(*it))->getModuleInstantiation(tabs);
				moduleInstances += "\n\n";
			} else if((*it)->op == OPERATOR_AND){
				moduleInstances += ((AndComponent*)(*it))->getModuleInstantiation(tabs);
				moduleInstances += "\n\n";
			} else if((*it)->op == OPERATOR_OR){
				moduleInstances += ((OrComponent*)(*it))->getModuleInstantiation(tabs);
				moduleInstances += "\n\n";
			} else if((*it)->op == OPERATOR_XOR){
				moduleInstances += ((XorComponent*)(*it))->getModuleInstantiation(tabs);
				moduleInstances += "\n\n";
			} else if((*it)->op == OPERATOR_SHL){
				moduleInstances += ((ShlComponent*)(*it))->getModuleInstantiation(tabs);
				moduleInstances += "\n\n";
			} else if((*it)->op == OPERATOR_LSHR){
				moduleInstances += ((LshrComponent*)(*it))->getModuleInstantiation(tabs);
				moduleInstances += "\n\n";
			} else if((*it)->op == OPERATOR_ASHR){
				moduleInstances += ((AshrComponent*)(*it))->getModuleInstantiation(tabs);
				moduleInstances += "\n\n";
			} else if((*it)->op == OPERATOR_EQ){
				moduleInstances += ((EqComponent*)(*it))->getModuleInstantiation(tabs);
				moduleInstances += "\n\n";
			} else if((*it)->op == OPERATOR_NE){
				moduleInstances += ((NeComponent*)(*it))->getModuleInstantiation(tabs);
				moduleInstances += "\n\n";
			} else if((*it)->op == OPERATOR_UGT){
				moduleInstances += ((UgtComponent*)(*it))->getModuleInstantiation(tabs);
				moduleInstances += "\n\n";
			} else if((*it)->op == OPERATOR_UGE){
				moduleInstances += ((UgeComponent*)(*it))->getModuleInstantiation(tabs);
				moduleInstances += "\n\n";
			} else if((*it)->op == OPERATOR_SGT){
				moduleInstances += ((SgtComponent*)(*it))->getModuleInstantiation(tabs);
				moduleInstances += "\n\n";
			} else if((*it)->op == OPERATOR_SGE){
				moduleInstances += ((SgeComponent*)(*it))->getModuleInstantiation(tabs);
				moduleInstances += "\n\n";
			} else if((*it)->op == OPERATOR_ULT){
				moduleInstances += ((UltComponent*)(*it))->getModuleInstantiation(tabs);
				moduleInstances += "\n\n";
			} else if((*it)->op == OPERATOR_ULE){
				moduleInstances += ((UleComponent*)(*it))->getModuleInstantiation(tabs);
				moduleInstances += "\n\n";
			} else if((*it)->op == OPERATOR_SLT){
				moduleInstances += ((SltComponent*)(*it))->getModuleInstantiation(tabs);
				moduleInstances += "\n\n";
			} else if((*it)->op == OPERATOR_SLE){
				moduleInstances += ((SleComponent*)(*it))->getModuleInstantiation(tabs);
				moduleInstances += "\n\n";
			} else if((*it)->op == OPERATOR_SEXT || (*it)->op == OPERATOR_ZEXT){
				moduleInstances += ((SZextComponent*)(*it))->getModuleInstantiation(tabs);
				moduleInstances += "\n\n";
			} else if((*it)->op == OPERATOR_GETPTR){
				moduleInstances += ((GetPtrComponent*)(*it))->getModuleInstantiation(tabs);
				moduleInstances += "\n\n";
			} else if((*it)->op == OPERATOR_RET){
				moduleInstances += ((RetComponent*)(*it))->getModuleInstantiation(tabs);
				moduleInstances += "\n\n";
			} else if((*it)->op == OPERATOR_SELECT){
				moduleInstances += ((SelectComponent*)(*it))->getModuleInstantiation(tabs);
				moduleInstances += "\n\n";
			} else if((*it)->op == OPERATOR_READ_MEMORY){
				moduleInstances += ((LoadComponent*)(*it))->getModuleInstantiation(tabs);
				moduleInstances += "\n\n";
			} else if((*it)->op == OPERATOR_WRITE_MEMORY){
				moduleInstances += ((StoreComponent*)(*it))->getModuleInstantiation(tabs);
				moduleInstances += "\n\n";
			} else if((*it)->op == OPERATOR_WRITE_LSQ || (*it)->op == OPERATOR_READ_LSQ){
				moduleInstances += ((LSQControllerComponent*)(*it))->getModuleInstantiation(tabs);
				moduleInstances += "\n\n";
			}
		} else if((*it)->type == COMPONENT_SOURCE){
			moduleInstances += ((SourceComponent*)(*it))->getModuleInstantiation(tabs);
			moduleInstances += "\n\n";
		} else if((*it)->type == COMPONENT_SINK){
			moduleInstances += ((SinkComponent*)(*it))->getModuleInstantiation(tabs);
			moduleInstances += "\n\n";
		} else if((*it)->type == COMPONENT_CONSTANT_){
			moduleInstances += ((ConstantComponent*)(*it))->getModuleInstantiation(tabs);
			moduleInstances += "\n\n";
		} else if((*it)->type == COMPONENT_CTRLMERGE){
			moduleInstances += ((ControlMergeComponent*)(*it))->getModuleInstantiation(tabs);
			moduleInstances += "\n\n";
		} else if((*it)->type == COMPONENT_MERGE){
			moduleInstances += ((MergeComponent*)(*it))->getModuleInstantiation(tabs);
			moduleInstances += "\n\n";
		} else if((*it)->type == COMPONENT_BUF || (*it)->type == COMPONENT_TEHB || (*it)->type == COMPONENT_OEHB){
			moduleInstances += ((BufferComponent*)(*it))->getModuleInstantiation(tabs);
			moduleInstances += "\n\n";
		} else if((*it)->type == COMPONENT_MUX){
			moduleInstances += ((MuxComponent*)(*it))->getModuleInstantiation(tabs);
			moduleInstances += "\n\n";
		} else if((*it)->type == COMPONENT_BRANCH){
			moduleInstances += ((BranchComponent*)(*it))->getModuleInstantiation(tabs);
			moduleInstances += "\n\n";
		} else if((*it)->type == COMPONENT_MC){
			moduleInstances += ((MemoryContentComponent*)(*it))->getModuleInstantiation(tabs);
			moduleInstances += "\n\n";
		} else if((*it)->type == COMPONENT_TFIFO || (*it)->type == COMPONENT_NFIFO){
			moduleInstances += ((FIFOComponent*)(*it))->getModuleInstantiation(tabs);
			moduleInstances += "\n\n";
		} else if((*it)->type == COMPONENT_LSQ){
			moduleInstances += ((LSQComponent*)(*it))->getModuleInstantiation(tabs);
			moduleInstances += "\n\n";
		}
	}

	return moduleInstances;
}


std::string GraphToVerilog::writeInputOutputConnections(){
	std::string inputoutput = "\n\n";

	for(auto it = dotReader.getComponentList().begin(); it != dotReader.getComponentList().end(); it++){
		if((*it)->type == COMPONENT_START){
			inputoutput += ((StartComponent*)(*it))->getInputOutputConnections();
			inputoutput += "\n";
		} else if((*it)->type == COMPONENT_END){
			inputoutput += ((EndComponent*)(*it))->getInputOutputConnections();
			inputoutput += "\n";
		} else if((*it)->type == COMPONENT_FORK){
			inputoutput += ((ForkComponent*)(*it))->getInputOutputConnections();
			inputoutput += "\n";
		} else if((*it)->type == COMPONENT_OPERATOR){
			if((*it)->op == OPERATOR_ADD){
				inputoutput += ((AddComponent*)(*it))->getInputOutputConnections();
				inputoutput += "\n";
			} else if((*it)->op == OPERATOR_SUB){
				inputoutput += ((SubComponent*)(*it))->getInputOutputConnections();
				inputoutput += "\n";
			} else if((*it)->op == OPERATOR_MUL){
				inputoutput += ((MulComponent*)(*it))->getInputOutputConnections();
				inputoutput += "\n";
			} else if((*it)->op == OPERATOR_UREM){
				inputoutput += ((RemComponent*)(*it))->getInputOutputConnections();
				inputoutput += "\n";
			} else if((*it)->op == OPERATOR_AND){
				inputoutput += ((AndComponent*)(*it))->getInputOutputConnections();
				inputoutput += "\n";
			} else if((*it)->op == OPERATOR_OR){
				inputoutput += ((OrComponent*)(*it))->getInputOutputConnections();
				inputoutput += "\n";
			} else if((*it)->op == OPERATOR_XOR){
				inputoutput += ((XorComponent*)(*it))->getInputOutputConnections();
				inputoutput += "\n";
			} else if((*it)->op == OPERATOR_SHL){
				inputoutput += ((ShlComponent*)(*it))->getInputOutputConnections();
				inputoutput += "\n";
			} else if((*it)->op == OPERATOR_LSHR){
				inputoutput += ((LshrComponent*)(*it))->getInputOutputConnections();
				inputoutput += "\n";
			} else if((*it)->op == OPERATOR_ASHR){
				inputoutput += ((AshrComponent*)(*it))->getInputOutputConnections();
				inputoutput += "\n";
			} else if((*it)->op == OPERATOR_EQ){
				inputoutput += ((EqComponent*)(*it))->getInputOutputConnections();
				inputoutput += "\n";
			} else if((*it)->op == OPERATOR_NE){
				inputoutput += ((NeComponent*)(*it))->getInputOutputConnections();
				inputoutput += "\n";
			} else if((*it)->op == OPERATOR_UGT){
				inputoutput += ((UgtComponent*)(*it))->getInputOutputConnections();
				inputoutput += "\n";
			} else if((*it)->op == OPERATOR_UGE){
				inputoutput += ((UgeComponent*)(*it))->getInputOutputConnections();
				inputoutput += "\n";
			} else if((*it)->op == OPERATOR_SGT){
				inputoutput += ((SgtComponent*)(*it))->getInputOutputConnections();
				inputoutput += "\n";
			} else if((*it)->op == OPERATOR_SGE){
				inputoutput += ((SgeComponent*)(*it))->getInputOutputConnections();
				inputoutput += "\n";
			} else if((*it)->op == OPERATOR_ULT){
				inputoutput += ((UltComponent*)(*it))->getInputOutputConnections();
				inputoutput += "\n";
			} else if((*it)->op == OPERATOR_ULE){
				inputoutput += ((UleComponent*)(*it))->getInputOutputConnections();
				inputoutput += "\n";
			} else if((*it)->op == OPERATOR_SLT){
				inputoutput += ((SltComponent*)(*it))->getInputOutputConnections();
				inputoutput += "\n";
			} else if((*it)->op == OPERATOR_SLE){
				inputoutput += ((SleComponent*)(*it))->getInputOutputConnections();
				inputoutput += "\n";
			} else if((*it)->op == OPERATOR_SEXT || (*it)->op == OPERATOR_ZEXT){
				inputoutput += ((SZextComponent*)(*it))->getInputOutputConnections();
				inputoutput += "\n";
			} else if((*it)->op == OPERATOR_GETPTR){
				inputoutput += ((GetPtrComponent*)(*it))->getInputOutputConnections();
				inputoutput += "\n";
			} else if((*it)->op == OPERATOR_RET){
				inputoutput += ((RetComponent*)(*it))->getInputOutputConnections();
				inputoutput += "\n";
			} else if((*it)->op == OPERATOR_SELECT){
				inputoutput += ((SelectComponent*)(*it))->getInputOutputConnections();
				inputoutput += "\n";
			} else if((*it)->op == OPERATOR_READ_MEMORY){
				inputoutput += ((LoadComponent*)(*it))->getInputOutputConnections();
				inputoutput += "\n";
			} else if((*it)->op == OPERATOR_WRITE_MEMORY){
				inputoutput += ((StoreComponent*)(*it))->getInputOutputConnections();
				inputoutput += "\n";
			} else if((*it)->op == OPERATOR_WRITE_LSQ || (*it)->op == OPERATOR_READ_LSQ){
				inputoutput += ((LSQControllerComponent*)(*it))->getInputOutputConnections();
				inputoutput += "\n";
			}
		} else if((*it)->type == COMPONENT_SOURCE){
			inputoutput += ((SourceComponent*)(*it))->getInputOutputConnections();
			inputoutput += "\n";
		} else if((*it)->type == COMPONENT_SINK){
			inputoutput += ((SinkComponent*)(*it))->getInputOutputConnections();
			inputoutput += "\n";
		} else if((*it)->type == COMPONENT_CONSTANT_){
			inputoutput += ((ConstantComponent*)(*it))->getInputOutputConnections();
			inputoutput += "\n";
		} else if((*it)->type == COMPONENT_CTRLMERGE){
			inputoutput += ((ControlMergeComponent*)(*it))->getInputOutputConnections();
			inputoutput += "\n";
		} else if((*it)->type == COMPONENT_MERGE){
			inputoutput += ((MergeComponent*)(*it))->getInputOutputConnections();
			inputoutput += "\n";
		} else if((*it)->type == COMPONENT_BUF || (*it)->type == COMPONENT_TEHB || (*it)->type == COMPONENT_OEHB){
			inputoutput += ((BufferComponent*)(*it))->getInputOutputConnections();
			inputoutput += "\n";
		} else if((*it)->type == COMPONENT_MUX){
			inputoutput += ((MuxComponent*)(*it))->getInputOutputConnections();
			inputoutput += "\n";
		} else if((*it)->type == COMPONENT_BRANCH){
			inputoutput += ((BranchComponent*)(*it))->getInputOutputConnections();
			inputoutput += "\n";
		} else if((*it)->type == COMPONENT_MC){
			inputoutput += ((MemoryContentComponent*)(*it))->getInputOutputConnections();
			inputoutput += "\n";
		} else if((*it)->type == COMPONENT_TFIFO || (*it)->type == COMPONENT_NFIFO){
			inputoutput += ((FIFOComponent*)(*it))->getInputOutputConnections();
			inputoutput += "\n";
		} else if((*it)->type == COMPONENT_LSQ){
			inputoutput += ((LSQComponent*)(*it))->getInputOutputConnections();
			inputoutput += "\n";
		}
	}

	return inputoutput;
}


std::string GraphToVerilog::writeEndModule(){
	return "endmodule\n";
}

//Till now, a useless function
void GraphToVerilog::insertVerilogCode(std::string& str){
	str = tabs + str + '\n';
	verilogCode += str;
}


void GraphToVerilog::insertTab(){
	tabs += "\t";
}

void GraphToVerilog::removeTab(){
	if(tabs.size() != 0)
		tabs = tabs.erase(0, 1);
}

