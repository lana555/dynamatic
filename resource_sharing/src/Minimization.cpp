//
// Created by pejic on 08/07/19.
//

#include <iostream>
#include <vector>
#include <string>
#include "Dataflow.h"
#include <set>
#include <algorithm>
#include <list>
#include <map>
#include <regex>
#include <math.h>
#include "resource_sharing.h"
#include "MyChannel.h"
#include "MyPort.h"
#include "MyBlock.h"
#include "DFnetlist.h"
#include "ControlPathAnalysis.h"


using namespace std;
using namespace Dataflow;

int unique_counter = 0;

void readChannelsIntoDict(DFnetlist& df, map<string, list<MyChannel>>& dict, vector<MyPort> ports)
{
	for(MyPort port: ports)
	{
		MyChannel channel(df, port.channelId);
		map<string, list<MyChannel>>::iterator it = dict.find(port.name);
		if(it == dict.end())
		{
			dict.insert({port.name, list<MyChannel>()});
		}
		dict.find(port.name)->second.push_back(channel);
	}
}

void copyPorts(DFnetlist& df,MyBlock& new_block, MyBlock& block, bool areInputPorts)
{
	vector<MyPort> ports = areInputPorts? block.inPorts : block.outPorts;

	for(MyPort port: ports)
	{
		MyPort new_port;
		new_port.name = port.name;
		new_port.blockId = new_block.id;
		new_port.isInput = areInputPorts;
		new_port.width = port.width;
		new_port.delay = port.delay;
		new_port.type = port.type;
		new_port.portId = df.createPort(new_block.id, areInputPorts, new_port.name, new_port.width, new_port.type);

		new_port.writeToDF(df);

		if(areInputPorts)
		{
			new_block.inPorts.push_back(new_port);
		}
		else
		{
			new_block.outPorts.push_back(new_port);
		};
	}
}

bool blockIsForkControl(DFnetlist& df, blockID id){
	//a control path fork has all ports of width 0
	for(auto port : df.DFI->getPorts(id, INPUT_PORTS)){
		if(!df.DFI->isControlPort(port)){
			return false;
		}
	}
	for(auto port : df.DFI->getPorts(id, OUTPUT_PORTS)){
		if(!df.DFI->isControlPort(port)){
			return false;
		}
	}
	return true;
}

blockID getCpFork(DFnetlist& df, bbID id) {
	set<blockID> blocks_in_bb = getNodesPerBBs(df)[id];
	blockID forkBlock = getSingletonCpBlock(df, blocks_in_bb, FORK);
	if (forkBlock == -1) {
		cout << "did not find a forkC, creating one " << endl;
		forkBlock = addForkAfterPhi(df, blocks_in_bb);
		if (forkBlock == -1) {
			cerr << "could not create fork after phi" << endl;
			assert(forkBlock != -1);
		}
	}
	return forkBlock;
}


MyBlock makeNewBlockFromNExisting(DFnetlist& df, vector<MyBlock> blocks, bbID bbId)
{
	//TODO reimplement checking for N instead of 2
	// if (!MyBlock::checkIfBlockAreCompatible(block1, block2))
	// {
	// 	throw "Error - blocks are not compatible and cannot be merged!";
	// }

	MyBlock new_block;
	MyBlock block1 = *blocks.begin();
	//TODO checks
	// TODO get the best values for some of the pars
	new_block.bbParent = bbId;
	new_block.delay = block1.delay;
	new_block.latency = block1.latency;
	new_block.II = block1.II;
	new_block.slot = block1.slot;
	new_block.transparent = block1.transparent;
	new_block.type = block1.type;
	new_block.operation = block1.operation;
	new_block.functionName = block1.functionName;
	//NOT SURE
	new_block.retimingDiff = 0;
	for(auto block : blocks){
		new_block.retimingDiff += block.retimingDiff;
	}

	//create name
	string newName = block1.name;
	for(auto i = blocks.begin() + 1; i < blocks.end(); ++i){
		string tmp_string = (*i).name;
		std::replace(tmp_string.begin(), tmp_string.end(), '_', ' ');
		stringstream ss(tmp_string);
		string basename;
		int num;
		ss >> basename >> num;
		newName = newName + "_" + to_string(num);
	}

	new_block.name = newName;


	// TODO condition
	//new_block.conditionPort
	new_block.freq = block1.freq;
	new_block.id = df.createBlock(new_block.type, new_block.name);
	copyPorts(df, new_block, block1, true);
	copyPorts(df,new_block,  block1, false);

	new_block.writeToDF(df);

	return new_block;
}

MyBlock createDistributor(DFnetlist& df, int width, list<MyChannel> outputChannels, MyPort srcPort, MyPort selPort, string name, bbID bbId)
{
	MyBlock branch;
	branch.delay = 0;
	branch.bbParent = bbId;
	branch.freq = 0;
	branch.id = -1;
	branch.II = 0;
	branch.latency = 0;
	branch.name = name;
	branch.operation = "";
	branch.slot = 0;
	branch.transparent = false;
	branch.type = BlockType::DISTRIBUTOR;
	branch.id = df.createBlock(branch.type, name);

	branch.writeToDF(df);


	for(int i = 0; i< outputChannels.size(); i++)
	{
		auto it_front = outputChannels.begin();
		advance(it_front, i);
		MyChannel channel = *it_front;
		MyPort port;
		// TODO possible problem, further investigation needed
		port.type = GENERIC_PORT;
		port.delay = 0;
		port.width = width;
		port.portId = -1;
		port.blockId = branch.id;
		port.isInput = false;
		port.group = to_string(i);
		port.name = "out"+to_string(i+1);
		port.portId = df.createPort(branch.id, false, port.name, port.width, port.type);

		port.writeToDF(df);

		branch.outPorts.push_back(port);

		MyChannel new_channel(df, channel.id);
		new_channel.srcPort = port.portId;
		df.removeChannel(channel.id);
		new_channel.id = df.DFI->createChannel(new_channel.srcPort, new_channel.dstPort, new_channel.slots, new_channel.transparent);

		new_channel.writeToDF(df);
	}

	//input port
	MyPort input_port;
	input_port.type = PortType ::GENERIC_PORT;
	input_port.delay = 0;
	input_port.width = width;
	input_port.portId = -1;
	input_port.blockId = branch.id;
	input_port.isInput = true;
	input_port.name = "in1";
	input_port.portId = df.createPort(branch.id, input_port.isInput, input_port.name,input_port.width, input_port.type);

	input_port.writeToDF(df);

	branch.inPorts.push_back(input_port);


	//why no remove ?
	//create channel between src and input port
	MyChannel new_channel;
	new_channel.srcPort = srcPort.portId;
	new_channel.dstPort = input_port.portId;
	new_channel.id = df.createChannel(new_channel.srcPort, new_channel.dstPort);

	new_channel.writeToDF(df);


	//create the condition port
	MyPort condition_port;
	condition_port.type = PortType ::SELECTION_PORT;
	condition_port.delay = 0;
	condition_port.width = ceil(log2(outputChannels.size()));
	condition_port.portId = -1;
	condition_port.blockId = branch.id;
	condition_port.isInput = true;
	condition_port.name = "in2";
	condition_port.portId = df.createPort(branch.id, condition_port.isInput, condition_port.name, condition_port.width, condition_port.type);

	condition_port.writeToDF(df);

	branch.inPorts.push_back(condition_port);

	//create channel between sel condition and branch condition
	MyChannel channel;
	channel.srcPort = selPort.portId;
	channel.dstPort = condition_port.portId;
	channel.id = df.createChannel(channel.srcPort, channel.dstPort);

	channel.writeToDF(df);

	return branch;
}


pair<MyBlock, MyPort> createSelector(DFnetlist& df, map<string, list<MyChannel>>& inputs, vector<MyPort> dstPorts, string name, map<bbID, vector<blockID>> orderings, bbID bbId)
{
	MyBlock selGen;
	selGen.type = BlockType ::SELECTOR;
	selGen.id = df.createBlock(selGen.type, name);
	selGen.bbParent = bbId;
	selGen.delay = 0;
	selGen.freq = 0;
	selGen.II = 0;
	selGen.latency = 0;
	selGen.name = name;
	selGen.operation = "";
	selGen.slot = 0;
	selGen.transparent = false;

	map<blockID, int> blockId_to_index{};
	//TODO POPULATE THAT MAP AND USE IT TO HAVE THE CORRECT ORDERING
	int mainIndex = 1;
	int total_amount_of_inputs_channels = 0;
	for(auto it: inputs)
	{
		int group = 0;
		int subIndex = 0;
		for(auto channel: it.second)
		{
			++total_amount_of_inputs_channels;

			MyPort port;
			port.type = PortType::GENERIC_PORT;
			port.delay = 0;
			port.width = df.getPortWidth(channel.dstPort);
			port.portId = -1;
			port.blockId = selGen.id;
			port.isInput = true;
			port.group = to_string(group);
			port.name = "in"+to_string(mainIndex + subIndex);
			port.portId = df.createPort(selGen.id, port.isInput, port.name,port.width, port.type);
			port.writeToDF(df);

			selGen.inPorts.push_back(port);

			//keep track of the index of each input
			blockID id = df.DFI->getBlockFromPort(channel.dstPort);
			if(blockId_to_index.find(id) == blockId_to_index.end()){
				blockId_to_index.insert({id, subIndex / 2});
			}

			MyChannel new_channel(df, channel.id);
			new_channel.dstPort = port.portId;
			df.removeChannel(channel.id);
			new_channel.id = df.DFI->createChannel(new_channel.srcPort, new_channel.dstPort, new_channel.slots, new_channel.transparent);
			//channelID DFnetlist_Impl::createChannel(portID src, portID dst, int slots, bool transparent)
			std::cout << "Created channel from " + it.first + " to " + port.name << endl;
			new_channel.writeToDF(df);
			subIndex += 2;
			++group;

		}
		mainIndex += 1;
	}

	for(auto mapping : orderings){
		selGen.orderings[mapping.first] = vector<int>{};
		for(auto pair : mapping.second){
			selGen.orderings[mapping.first].push_back(blockId_to_index[pair]);
		}
	}

	//need to connect control path of all BBs connected to the sel
	vector<blockID> cpForks = vector<blockID>();
	int max_shared_amount = 0;

	for(auto mapping : selGen.orderings){
		int size = mapping.second.size();
		if(max_shared_amount < size){
			max_shared_amount = size;
		}

		blockID cpFork = getCpFork(df, mapping.first);
		auto it = find(cpForks.begin(), cpForks.end(), cpFork);
		if(it == cpForks.end()){
			//if fork control path not yet in the vector we must add it
			cpForks.push_back(cpFork);
		}
	}

	// create channel between forkC and sel
	int bbIndexForOrdering = 0;
	int maxInfoWidth = max_shared_amount == 1 ? 1 : ceil(log2(max_shared_amount));
	int idInfoWidth = cpForks.size() == 1 ? 1 : ceil(log2(cpForks.size()));
	int constantWidth = maxInfoWidth + idInfoWidth;
	for(auto myFork : cpForks){
			//we must add a port to the nodes to connect them to the sel
			MyPort forkPort;
			forkPort.type = PortType::GENERIC_PORT;
			forkPort.delay = 0;
			forkPort.width = 0;
			forkPort.blockId = myFork;
			forkPort.isInput = false;
			forkPort.name = "";
			forkPort.portId = df.createPort(myFork, forkPort.isInput, forkPort.name ,forkPort.width, forkPort.type);
			forkPort.writeToDF(df);

			int bbIndexOfTheConstantBlock = df.DFI->getBasicBlock(myFork);
			string constant_name = "constant_sel_" + to_string(bbIndexForOrdering) + "_" + to_string(unique_counter++);
			int max_index_in_bb = (orderings[df.DFI->getBasicBlock(myFork)].size() - 1);
			int value = bbIndexForOrdering | (max_index_in_bb << idInfoWidth);
			MyBlock constant = MyBlock::createConstant(df, bbIndexOfTheConstantBlock, constant_name, value, constantWidth);
			bbIndexForOrdering += 1;

			MyPort selPort;
			selPort.type = PortType::GENERIC_PORT;
			selPort.delay = 0;
			selPort.width = constantWidth;
			selPort.blockId = selGen.id;
			selPort.isInput = true;
			selPort.name = "in"+to_string(++total_amount_of_inputs_channels);
			selPort.portId = df.createPort(selGen.id, selPort.isInput, selPort.name, selPort.width, selPort.type);
			selPort.writeToDF(df);
			selGen.inPorts.push_back(selPort);

			MyChannel fork_to_constant_channel;
			fork_to_constant_channel.srcPort = forkPort.portId;
			fork_to_constant_channel.dstPort = constant.inPorts[0].portId;
			fork_to_constant_channel.id = df.createChannel(fork_to_constant_channel.srcPort, fork_to_constant_channel.dstPort);

			MyChannel constant_to_sel_channel;
			constant_to_sel_channel.srcPort = constant.outPorts[0].portId;
			constant_to_sel_channel.dstPort = selPort.portId;
			constant_to_sel_channel.id = df.createChannel(constant_to_sel_channel.srcPort, constant_to_sel_channel.dstPort);
	}
	
//	//rewrite the inputsBBIds to minimize space, for example {12, 12, 32, 4} => {0, 0, 1, 2}
//	map<int, int> mappings = map<int, int>();
//	vector<int> newIds = vector<int>();
//	int current_index = 0;
//	for(auto bbId : selGen.inputsBBids){
//		auto it = mappings.find(bbId);
//		if(it == mappings.end()){
//			mappings[bbId] = current_index++;
//		}
//		newIds.push_back(mappings[bbId]);
//	}
//	selGen.inputsBBids = newIds;

	int i = 1;

	for(auto dstPort: dstPorts)
	{
		MyPort output_port;
		output_port.type = PortType ::GENERIC_PORT;
		output_port.delay = 0;
		output_port.width = df.getPortWidth(dstPort.portId);;
		output_port.portId = -1;
		output_port.blockId = selGen.id;
		output_port.isInput = false;
		//output_port.group = group++;
		output_port.name = "out"+to_string(i++);
		output_port.portId = df.createPort(selGen.id, output_port.isInput, output_port.name,output_port.width, output_port.type);

		output_port.writeToDF(df);

		selGen.outPorts.push_back(output_port);

		MyChannel new_channel;
		new_channel.srcPort = output_port.portId;
		new_channel.dstPort = dstPort.portId;
		//df.removeChannel(df.getConnectedChannel(dstPort.portId));
		new_channel.id = df.createChannel(new_channel.srcPort, new_channel.dstPort);

		new_channel.writeToDF(df);
	}

	int total_number_of_inputs = 0;
	for(auto input : inputs) {
		total_number_of_inputs += input.second.size();
	}
	MyPort cond_port;
	cond_port.type = PortType ::GENERIC_PORT;
	cond_port.delay = 0;
	//we subtract 1 because the size of the condition transmitted must be adequate for the branch which has half the outputs that sel has inputs
	cond_port.width = ceil(log2(total_number_of_inputs)) - 1;
	cond_port.portId = -1;
	cond_port.blockId = selGen.id;
	cond_port.isInput = false;
	cond_port.name = "out"+to_string(i++);
	cond_port.group = "s";
	cond_port.portId = df.createPort(selGen.id, cond_port.isInput, cond_port.name,cond_port.width, cond_port.type);

	cond_port.writeToDF(df);

	selGen.outPorts.push_back(cond_port);

	selGen.writeToDF(df);

	return make_pair(selGen, cond_port);
}

int merge_cnt = 0;
int fork_cnt = 0;
int sel_cnt = 0;
int buffer_cnt = 0;

int minimizeNBlocks(DFnetlist& df, MergeGroup merge_group){

	vector<MyBlock> blocks{};

	for(auto mapping : merge_group.blocks){
		for(auto block : mapping.second){
			blocks.push_back(MyBlock(df, block));
		}
	}

	int bbId = df.DFI->BBG.createBasicBlock();

	map<string, list<MyChannel>> inputs;
	map<string, list<MyChannel>> outputs;

	for(auto block : blocks){
		readChannelsIntoDict(df, inputs, block.inPorts);
		readChannelsIntoDict(df, outputs, block.outPorts);
	}

	MyBlock new_block = makeNewBlockFromNExisting(df, blocks, bbId);

	auto selection = createSelector(df, inputs, new_block.inPorts, "sel_gen_am"+to_string(sel_cnt++), merge_group.blocks, bbId);
	MyBlock sel = selection.first;
	MyPort sel_port = selection.second;

	for(auto const& item: outputs)
	{
		string port_name = item.first;
		list<MyChannel> channels = item.second;

		MyPort port;
		for(MyPort prt: new_block.outPorts)
		{
			if(prt.name == port_name)
			{
				port  = prt;
			}
		}

		if (port.portId == -1)
		{
			throw "Error - port_id is still -1 - no port found.";
		}

		MyBlock conditionBuffer = MyBlock::createBuffer(df, bbId, "buffer_np_"+to_string(buffer_cnt++), new_block.latency, sel_port.width);
		MyPort& in1Condition = conditionBuffer.inPorts[0];
		MyPort& out1Condition = conditionBuffer.outPorts[0];

		df.createChannel(sel_port.portId, in1Condition.portId);

		// df.createChannel(port.portId, in1Result.portId);

		MyBlock new_branch = createDistributor(df, port.width, channels, port, out1Condition, "branch_gen_am_"+to_string(fork_cnt++), bbId);//+"_"+df.getPortName(port.portId));

	} 

	for(auto bbMapping : merge_group.blocks){
		for(auto blockIds : bbMapping.second){
			df.removeBlock(blockIds);
		}
	}

	return new_block.id;
}
