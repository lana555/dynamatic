//
// Created by pejic on 09/07/19.
//

#ifndef RESOURCE_MINIMIZATION_MYCHANNEL_H
#define RESOURCE_MINIMIZATION_MYCHANNEL_H


#include <iostream>
#include <vector>
#include <string>
#include <DFnetlist/Dataflow.h>
#include <set>
#include <algorithm>
#include <list>
#include <map>


using namespace std;
using namespace Dataflow;


class MyChannel
{
public:
	int id = -1;
	int srcPort = -1;
	int dstPort = -1;
	int slots = -1;
	bool transparent;


	MyChannel();

	MyChannel(DFnetlist& df, int channel_id);

	MyChannel(DFnetlist & df, int srcPortId, int dstPortId);


	void readFromDF(DFnetlist& df, int channelId);

	void writeToDF(DFnetlist& df);

};

#endif //RESOURCE_MINIMIZATION_MYCHANNEL_H
