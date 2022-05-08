/*
 * ControlPathAnalysis.cpp
 *
 *  Created on: May 16, 2020
 *      Author: dynamatic
 */

#include "MyBlock.h"
#include "MyChannel.h"
#include "MarkedGraph.h"
#include "bb_graph_reader.h"
#include "DisjointSet.h"
#include "ControlPathAnalysis.h"
#include "resource_sharing.h"

int my_buffer_cnt = 0;
int my_fork_cnt = 0;

bool isControlBlock(DFnetlist &df, blockID id) {
	//a control path fork has all  output ports of width 0
	bool all_output_control = true;
	for (auto port : df.DFI->getPorts(id, OUTPUT_PORTS)) {
		if (!df.DFI->isControlPort(port)) {
			all_output_control = false;
		}
	}
	bool all_input_control = true;
	for (auto port : df.DFI->getPorts(id, INPUT_PORTS)) {
		if (!df.DFI->isControlPort(port)) {
			all_input_control = false;
		}
	}
	return all_output_control || all_input_control;
}

set<blockID> getCpBlock(DFnetlist &df, set<blockID> &blocks, BlockType type) {
	set<blockID> matching_blocks { };
	for (auto block : blocks) {
		if (df.DFI->getBlockType(block) == type && isControlBlock(df, block)) {
			matching_blocks.insert(block);
		}
	}
	return matching_blocks;
}

blockID getSingletonCpBlock(DFnetlist &df, set<blockID> &blocks,
		BlockType type) {
	set<blockID> singleton = getCpBlock(df, blocks, type);
	if (singleton.size() != 1) {
		cerr
				<< "Expected to find 1 block of type "
						+ block_type_to_string(type);
		cerr << " but found " + to_string(singleton.size());
		cerr << " in BB " + to_string(df.DFI->getBasicBlock(*(blocks.begin())))
				<< endl;
		return -1;
	}
	return *singleton.begin();
}

blockID addForkAfterPhi(DFnetlist &df, set<blockID> blocks_in_bb) {
	blockID phiID = getSingletonCpBlock(df, blocks_in_bb, CNTRL_MG);
	bbID bbId = df.DFI->getBasicBlock(phiID);
	assert(phiID != -1);
	portID phiPort = -1;
	for (auto port : df.DFI->getPorts(phiID, OUTPUT_PORTS)) {
		if (df.DFI->getPortWidth(port) == 0) {
			phiPort = port;
		}
	}

	channelID channel_to_remove = df.DFI->getConnectedChannel(phiPort);

	portID dstPort = df.DFI->getDstPort(channel_to_remove);

	MyBlock newFork = MyBlock::createFork(df, bbId,
			"additional_fork_cp" + to_string(my_fork_cnt++), 0);

	MyChannel phi_to_fork(df, channel_to_remove);
	phi_to_fork.dstPort = newFork.inPorts[0].portId;
	df.DFI->removeChannel(phi_to_fork.id);
	phi_to_fork.id = df.createChannel(phi_to_fork.srcPort, phi_to_fork.dstPort);
	phi_to_fork.writeToDF(df);

	MyChannel fork_to_dst(df, newFork.outPorts[0].portId, dstPort);

	return newFork.id;
}

blockID predecessor(DFnetlist &df, blockID current) {
	set<portID> allPorts = df.DFI->getPorts(current, INPUT_PORTS);
	set<portID> ctrlPorts { };
	for (auto port : allPorts) {
		if (df.DFI->getPortWidth(port) == 0) {
			ctrlPorts.insert(port);
		}
	}
	assert(ctrlPorts.size() == 1);
	channelID channel = df.DFI->getConnectedChannel(*ctrlPorts.begin());
	return df.DFI->getSrcBlock(channel);
}

vector<blockID> findControlPath(DFnetlist &df, set<blockID> blocks_in_bb) {
	blockID branchBlock = getSingletonCpBlock(df, blocks_in_bb, BRANCH);
	assert(branchBlock != -1);
	blockID forkBlock = getSingletonCpBlock(df, blocks_in_bb, FORK);
	if (forkBlock == -1) {
		cout << "did not find a forkC, creating one " << endl;
		forkBlock = addForkAfterPhi(df, blocks_in_bb);
		if (forkBlock == -1) {
			cerr << "could not create fork after phi" << endl;
			assert(forkBlock != -1);
		}
	}
	vector<blockID> reverse_order_path { };
	blockID currentBlock = branchBlock;

	while (currentBlock != forkBlock) {
		reverse_order_path.push_back(currentBlock);
		currentBlock = predecessor(df, currentBlock);
	}
	reverse_order_path.push_back(currentBlock);

	vector<blockID> correct_order(reverse_order_path.rbegin(),
			reverse_order_path.rend());
	return correct_order;
}

void addStageToControlPath(DFnetlist &df, vector<blockID> &control_path) {
	blockID blockBeforeBranch = *(control_path.end() - 2);
	cout << "block before branch is of type " + block_type_to_string(df.DFI->getBlockType(blockBeforeBranch)) << endl;

	//assert(df.DFI->getBlockType(blockBeforeBranch) == ELASTIC_BUFFER);

	blockID branch = *(control_path.end() - 1);
	assert(df.DFI->getBlockType(branch) == BRANCH);

	portID portOfBlockBeforeBranch = -1;
	for (portID port : df.DFI->getPorts(blockBeforeBranch, OUTPUT_PORTS)) {
		if(df.DFI->getPortWidth(port) == 0){
			assert(portOfBlockBeforeBranch == -1); //only one port should have a width of 0
			portOfBlockBeforeBranch = port;
		}
	}

	bbID bbId = df.DFI->getBasicBlock(blockBeforeBranch);

	portID inputPortOfBranch = -1;
	for (auto port : df.DFI->getPorts(branch, INPUT_PORTS)) {
		if (df.DFI->getPortWidth(port) == 0) {
			inputPortOfBranch = port;
		}
	}

	//create the two new blocks
	MyBlock newBuffer = MyBlock::createBufferForCP(df, bbId,
			"additional_buffer_cp_" + to_string(my_buffer_cnt++));
	MyBlock newFork = MyBlock::createFork(df, bbId,
			"additional_fork_cp" + to_string(my_fork_cnt++), 0);

	//link the newly created blocks together
	MyChannel new_fork_to_new_buf(df, newFork.outPorts[0].portId,
			newBuffer.inPorts[0].portId);

	//undo the channel last buffer -> branch to create last buffer -> new fork
	MyChannel old_buf_to_new_fork(df,
			df.DFI->getConnectedChannel(portOfBlockBeforeBranch));
	old_buf_to_new_fork.dstPort = newFork.inPorts[0].portId;
	df.DFI->removeChannel(old_buf_to_new_fork.id);
	old_buf_to_new_fork.id = df.createChannel(old_buf_to_new_fork.srcPort,
			old_buf_to_new_fork.dstPort);
	old_buf_to_new_fork.writeToDF(df);

	MyChannel new_buf_to_branch(df, newBuffer.outPorts[0].portId,
			inputPortOfBranch);

	control_path.insert(control_path.end() - 1, newFork.id);
	control_path.insert(control_path.end() - 1, newBuffer.id);

}

void connectForkToBlock(DFnetlist &df, blockID fork, blockID to_be_connected) {

	//manipulation easy enough to not have to use MyBlock
	MyPort out_port;
	out_port.width = 0;
	out_port.type = GENERIC_PORT;
	out_port.name = "out"
			+ to_string(df.DFI->getPorts(fork, OUTPUT_PORTS).size() + 1);
	out_port.isInput = false;
	out_port.portId = df.createPort(fork, out_port.isInput, out_port.name,
			out_port.width, out_port.type);

	MyPort in_port;
	in_port.width = 0;
	in_port.type = GENERIC_PORT;
	in_port.name = "in"
			+ to_string(
					df.DFI->getPorts(to_be_connected, INPUT_PORTS).size() + 1);
	in_port.isInput = true;
	in_port.portId = df.createPort(to_be_connected, in_port.isInput,
			in_port.name, in_port.width, in_port.type);

	MyChannel newChannel(df, out_port.portId, in_port.portId);
}

void removeNode(DFnetlist &df, blockID id) {
	//remove a node and connect it's unique input node to it's unique output node
	assert(df.DFI->getPorts(id, INPUT_PORTS).size() == 1);
	assert(df.DFI->getPorts(id, OUTPUT_PORTS).size() == 1);

	portID in_port_to_remove = *(df.DFI->getPorts(id, INPUT_PORTS).begin());
	channelID pred_channel = df.DFI->getConnectedChannel(in_port_to_remove);
	portID in_port_to_keep = df.DFI->getSrcPort(pred_channel);

	portID out_port_to_remove = *(df.DFI->getPorts(id, OUTPUT_PORTS).begin());
	channelID succ_channel = df.DFI->getConnectedChannel(out_port_to_remove);
	portID out_port_to_keep = df.DFI->getDstPort(succ_channel);

	cout << "Removing " + df.DFI->getBlockName(id) << endl;
	df.DFI->removeChannel(pred_channel);
	df.DFI->removeChannel(succ_channel);
	df.DFI->removePort(in_port_to_remove);
	df.DFI->removePort(out_port_to_remove);
	df.DFI->removeBlock(id);
	df.DFI->createChannel(in_port_to_keep, out_port_to_keep, 0, true);
}

void removeForkToBlockConnection(DFnetlist &df, blockID id) {
	for (auto portId : df.DFI->getPorts(id, INPUT_PORTS)) {
		if (df.DFI->getPortWidth(portId) != 0)
			continue;

		channelID channel = df.DFI->getConnectedChannel(portId);
		portID srcPort = df.DFI->getSrcPort(channel);
		blockID associated_block = df.DFI->getBlockFromPort(srcPort);
		if (df.DFI->getBlockType(associated_block) == FORK) {
			df.DFI->removeChannel(channel);
			df.DFI->removePort(portId);
			df.DFI->removePort(srcPort);
		} else {
			assert(df.DFI->getBlockType(associated_block) == ELASTIC_BUFFER);
			cout << "removing buffer " << df.DFI->getBlockName(associated_block)
					<< " from " + df.DFI->getBlockName(id)
					<< " with slots "
							+ to_string(df.DFI->getBufferSize(associated_block))
							+ " and transparency "
							+ to_string(
									df.DFI->isBufferTransparent(
											associated_block)) << endl;
			removeNode(df, associated_block);
			removeForkToBlockConnection(df, id);
		}

	}
}

void removeForkToBlockConnection(DFnetlist &df) {
	for (auto id : df.DFI->allBlocks) {
		if (test_node_type(df, id)) {
			removeForkToBlockConnection(df, id);
		}
	}
}

void removeFakeBuffer(DFnetlist &df) {
	for (auto id : df.DFI->allBlocks) {
		if (df.DFI->getBlockType(id) == OPERATOR
				&& df.DFI->getOperation(id) == "fakeBuffer") {
			removeNode(df, id);
		}
	}
}

void removeOneInOneOutFork(DFnetlist &df) {
	for (auto id : df.DFI->allBlocks) {
		if (df.DFI->getBlockType(id) == FORK
				&& df.DFI->getPorts(id, OUTPUT_PORTS).size() == 1) {
			cout << "removing " << df.DFI->getBlockName(id) << endl;
			removeNode(df, id);
		}
	}
}

void removeAdditionToCp(DFnetlist &df) {
	removeForkToBlockConnection(df);
	removeFakeBuffer(df);
	removeOneInOneOutFork(df);
}
