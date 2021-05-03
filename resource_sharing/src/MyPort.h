//
// Created by pejic on 09/07/19.
//

#ifndef RESOURCE_MINIMIZATION_MYPORT_H
#define RESOURCE_MINIMIZATION_MYPORT_H

#include <iostream>
#include <vector>
#include <string>
#include "DFnetlist/Dataflow.h"
#include <set>
#include <algorithm>
#include <list>
#include <map>


using namespace std;
using namespace Dataflow;


class MyPort
{
public:
	int portId = -1;
	string name = "";
	int blockId = -1;
	bool isInput = true;
	int width = -1;
	double delay = 0;
	PortType type = GENERIC_PORT;
	int channelId = -1;
	string group = "-1";


	MyPort(DFnetlist& df, int port_id, int block_id);

	MyPort();

	void readFromDF(DFnetlist& df, int port_id, int blockId);

	void writeToDF(DFnetlist& df);
};


#endif //RESOURCE_MINIMIZATION_MYPORT_H
