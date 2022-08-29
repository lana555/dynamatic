//
// Created by pejic on 09/07/19.
//

#include "MyBlock.h"
#include "DFnetlist.h"
#include <string>
#include <cstdarg>

MyBlock::MyBlock(): id(-1){}


MyBlock::MyBlock(DFnetlist& df, int block_id)//: conditionPort(MyPort(df, df.getConditionalPort(block_id), block_id))
{
	readFromDF(df, block_id);
}

void MyBlock::readFromDF(DFnetlist& df, int block_id)
{
	id = block_id;
	delay = df.getBlockDelay(block_id, 0);
	latency = df.getLatency(block_id);
	II = df.getInitiationInterval(block_id);
	slot = df.getBufferSize(block_id);
	transparent = df.isBufferTransparent(block_id);
	name = df.getBlockName(block_id);
	type = df.getBlockType(block_id);
	operation = df.DFI->getOperation(block_id);
	retimingDiff = df.DFI->getBlockRetimingDiff(block_id);
	value = df.DFI->getValue(id);

	bbParent = df.DFI->getBasicBlock(id);
	memPortID = df.DFI->getMemPortID(id);
	memOffset = df.DFI->getMemOffset(id);
	memBBCount = df.DFI->getMemBBCount(id);
	memLdCount = df.DFI->getMemLdCount(id);
	memStCount = df.DFI->getMemStCount(id);
	memName = df.DFI->getMemName(id);

	inPorts = vector<MyPort>();
	for(int portId :df.getPorts(block_id, PortDirection::INPUT_PORTS))
	{
		MyPort port(df, portId, block_id);
		inPorts.push_back(port);
	}

	outPorts = vector<MyPort>();
	for(int portId :df.getPorts(block_id, PortDirection::OUTPUT_PORTS))
	{
		MyPort port(df, portId, block_id);
		outPorts.push_back(port);
	}

	freq = df.getExecutionFrequency(block_id);
}

// TODO check width
bool MyBlock::checkIfBlockAreCompatible(MyBlock block1, MyBlock block2)
{
	if(block1.id == block2.id)
	{
		return false;
	}
	return 	block1.type == block2.type && block1.operation == block2.operation;
}

pair<MyBlock, MyBlock> MyBlock::createSource(int index, int width)
{
	MyBlock source;
	source.type = BlockType ::FUNC_ENTRY;
	source.delay = 0;
	source.name = "start_np_"+to_string(index);

	MyPort port;
	port.width = width;
	port.type = GENERIC_PORT;
	port.name = "out1";
	port.isInput = false;

	source.outPorts.push_back(port);

	MyPort port2;
	port.width = 0;
	port.type = PortType ::GENERIC_PORT;
	port.name = "in1";
	port.isInput = true;

	source.inPorts.push_back(port2);

	MyBlock cnst;
/*
	cnst.type = BlockType ::CONSTANT;
	cnst.delay = 0;
	cnst.name = "cnst_"+to_string(index);
	cnst.value = 0x12;

	MyPort port3;
	port.width = width;
	port.type = GENERIC_PORT;
	port.name = "out1";
	port.isInput = false;

	cnst.outPorts.push_back(port3);


	MyPort port4;
	port.width = width;
	port.type = PortType ::GENERIC_PORT;
	port.name = "in1";
	port.isInput = true;

	cnst.inPorts.push_back(port4);
*/

	return make_pair(source, cnst);
}

MyBlock MyBlock::createSink(string name, int width)
{
	MyBlock sink;
	sink.type = SINK;
	sink.delay = 0;
	sink.name = name;

	MyPort port;
	port.width = width;
	port.type = GENERIC_PORT;
	port.name = "in1";
	port.isInput = true;

	sink.inPorts.push_back(port);

	return sink;
}

MyBlock MyBlock::createBuffer(DFnetlist & df, bbID bbId, string name, int slots, int width)
{
	MyBlock buffer;
	buffer.type = ELASTIC_BUFFER;
	buffer.bbParent = bbId;
	buffer.delay = 0;
	buffer.name = name;
	buffer.slot = slots;
	buffer.transparent = false;
	buffer.id = df.DFI->createBlock(buffer.type, buffer.name);

	MyPort in_port;
	in_port.width = width;
	in_port.type = GENERIC_PORT;
	in_port.name = "in1";
	in_port.isInput = true;
	in_port.portId = df.createPort(buffer.id, in_port.isInput, in_port.name, in_port.width, in_port.type);
	buffer.inPorts.push_back(in_port);


	MyPort out_port;
	out_port.width = width;
	out_port.type = GENERIC_PORT;
	out_port.name = "out1";
	out_port.isInput = false;
	out_port.portId = df.createPort(buffer.id, out_port.isInput, out_port.name, out_port.width, out_port.type);
	buffer.outPorts.push_back(out_port);


	buffer.writeToDF(df);
	in_port.writeToDF(df);
	out_port.writeToDF(df);

	return buffer;
}

MyBlock MyBlock::createBufferForCP(DFnetlist & df, bbID bbId, string name)
{
	MyBlock buffer;
	buffer.type = OPERATOR;
	buffer.operation = "fakeBuffer";
	buffer.bbParent = bbId;
	buffer.delay = 0;
	buffer.name = name;
	buffer.latency = 1;

	buffer.id = df.DFI->createBlock(buffer.type, buffer.name);

	MyPort in_port;
	in_port.width = 0;
	in_port.type = GENERIC_PORT;
	in_port.name = "in1";
	in_port.isInput = true;
	in_port.portId = df.createPort(buffer.id, in_port.isInput, in_port.name, in_port.width, in_port.type);
	buffer.inPorts.push_back(in_port);


	MyPort out_port;
	out_port.width = 0;
	out_port.type = GENERIC_PORT;
	out_port.name = "out1";
	out_port.isInput = false;
	out_port.portId = df.createPort(buffer.id, out_port.isInput, out_port.name, out_port.width, out_port.type);
	buffer.outPorts.push_back(out_port);


	buffer.writeToDF(df);
	in_port.writeToDF(df);
	out_port.writeToDF(df);

	return buffer;
}


MyBlock MyBlock::createFork(DFnetlist & df, bbID bbId, string name, int width)
{
	MyBlock fork;
	fork.type = FORK;
	fork.bbParent = bbId;
	fork.delay = 0;
	fork.name = name;
	fork.slot = 0;
	fork.transparent = false;
	fork.id = df.createBlock(fork.type, fork.name);

	MyPort in_port;
	in_port.width = width;
	in_port.type = GENERIC_PORT;
	in_port.name = "in1";
	in_port.isInput = true;
	in_port.portId = df.createPort(fork.id, in_port.isInput, in_port.name, in_port.width, in_port.type);
	fork.inPorts.push_back(in_port);

	MyPort out_port;
	out_port.width = width;
	out_port.type = GENERIC_PORT;
	out_port.name = "out1";
	out_port.isInput = false;
	out_port.portId = df.createPort(fork.id, out_port.isInput, out_port.name, out_port.width, out_port.type);
	fork.outPorts.push_back(out_port);

	fork.writeToDF(df);
	in_port.writeToDF(df);
	out_port.writeToDF(df);

	return fork;
}

MyBlock MyBlock::createConstant(DFnetlist & df, int bbId, string name, int value, int width)
{
	MyBlock constant;
	constant.type = CONSTANT;
	constant.delay = 0;
	constant.bbParent = bbId;
	constant.name = name;
	constant.value = value;
	constant.id = df.createBlock(constant.type, constant.name);

	MyPort in_port;
	in_port.width = width;
	in_port.type = GENERIC_PORT;
	in_port.name = "in1";
	in_port.isInput = true;
	in_port.portId = df.createPort(constant.id, in_port.isInput, in_port.name, in_port.width, in_port.type);
	constant.inPorts.push_back(in_port);

	MyPort out_port;
	out_port.width = width;
	out_port.type = GENERIC_PORT;
	out_port.name = "out1";
	out_port.isInput = false;
	out_port.portId = df.createPort(constant.id, out_port.isInput, out_port.name, out_port.width, out_port.type);
	constant.outPorts.push_back(out_port);

	constant.writeToDF(df);
	in_port.writeToDF(df);
	out_port.writeToDF(df);

	return constant;
}

void MyBlock::writeToDF(DFnetlist &df)
{
	df.setBlockDelay(id, delay, 0);
	df.setLatency(id, latency);
	df.setInitiationInterval(id, II);
	df.setBufferSize(id, slot);
	df.setBufferTransparency(id, transparent);
	df.DFI->setOperation(id, operation);
	df.DFI->setBlockRetimingDiff(id, retimingDiff);
	df.DFI->setValue(id, value);
	//df.DFI->bool

	df.DFI->setBasicBlock(id, bbParent);
	//bbRank =;
	df.DFI->setMemPortID(id, memPortID);
	df.DFI->setMemOffset(id, memOffset);
	df.DFI->setMemBBCount(id, memBBCount);
	df.DFI->setMemLdCount(id, memLdCount);
	df.DFI->setMemStCount(id, memStCount);
	df.DFI->setMemName(id, memName);

	df.DFI->setOrderings(id, orderings);

}

string block_type_to_string(BlockType type){
	switch(type){
		case OPERATOR : return "OPERATOR";
        case ELASTIC_BUFFER : return "ELASTIC_BUFFER";
        case FORK : return "FORK";
        case BRANCH : return "BRANCH";
        case MERGE : return "MERGE";
        case DEMUX : return "DEMUX";
        case SELECT : return "SELECT";
        case CONSTANT : return "CONSTANT";
        case FUNC_ENTRY : return "FUNC_ENTRY";
        case FUNC_EXIT : return "FUNC_EXIT";
        case SOURCE : return "SOURCE";
        case SINK : return "SINK";
    	case MUX : return "MUX";
    	case CNTRL_MG : return "CNTRL_MG";
    	case LSQ : return "LSQ";
    	case MC : return "MC";
        case DISTRIBUTOR : return "DISTRIBUTOR";
        case SELECTOR : return "SELECTOR";
        default : return "UNKNOWN";
	}
}

