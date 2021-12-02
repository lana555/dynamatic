//
// Created by pejic on 09/07/19.
//

#ifndef RESOURCE_MINIMIZATION_MYBLOCK_H
#define RESOURCE_MINIMIZATION_MYBLOCK_H

#include <iostream>
#include <vector>
#include <string>
#include <DFnetlist/Dataflow.h>
#include <DFnetlist/DFnetlist.h>

#include <set>
#include <algorithm>
#include <list>
#include <map>
#include <initializer_list>
#include "MyPort.h"



using namespace std;
using namespace Dataflow;


class MyBlock
{
public:
	int id = -1;
	double delay = 0;
	int latency = 0;
	int II = 0;
	int slot = 0;
	bool transparent = true;
	double freq = 0;
	//bool bool_value;
	long long value = 0;
	BlockType type;
	string operation = "";
	string functionName = "";
	string name = "";

	int bbParent;           // Disjoint set parent for BB calculation
	int bbRank;                 // Disjoint set rank for BB calculation
	int memPortID;              // Lana: used to connect load/store to MC/LSQ
	int memOffset;              // Lana: used to connect load/store to MC/LSQ
	int memBBCount;             // Lana: number of BBs connected to MC/LSQ
	int memLdCount;             // Lana: number of loads connected to MC/LSQ
	int memStCount;             // Lana: number of stores connected to MC/LSQ
	std::string memName;        // LSQ/MC name

	//Axel : for sel_gen, used in "scheduling" algorithm
	map<bbID, vector<int>> orderings;

	double retimingDiff = -1;
	vector<MyPort> inPorts;
	vector<MyPort> outPorts;
	MyPort conditionPort;

	MyBlock(DFnetlist& df, int block_id);

	MyBlock();

	void readFromDF(DFnetlist& df, int block_id);

	void writeToDF(DFnetlist& df);

	static bool checkIfBlockAreCompatible(MyBlock block1, MyBlock block2);

	static pair<MyBlock, MyBlock> createSource(int index, int width);

	static MyBlock createSink(string name, int width);

	static MyBlock createBuffer(DFnetlist & df, bbID bbId, string name, int slots, int width);

	static MyBlock createBufferForCP(DFnetlist & df, bbID bbId, string name);

	static MyBlock createFork(DFnetlist & df, bbID bbId, string name, int width);

	static MyBlock createConstant(DFnetlist & df, int bbId, string name, int value, int width);

};

string block_type_to_string(BlockType type);


#endif //RESOURCE_MINIMIZATION_MYBLOCK_H
