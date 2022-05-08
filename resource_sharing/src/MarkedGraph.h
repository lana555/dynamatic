/*
 * MarkedGraph.h
 *
 *  Created on: Apr 7, 2020
 *      Author: dynamatic
 */

#ifndef MARKEDGRAPH_H_
#define MARKEDGRAPH_H_

#include <set>
#include "DFnetlist/Dataflow.h"
//#include "Dataflow.h"

using namespace std;
using namespace Dataflow;

class MarkedGraph {
public:
	set<blockID> blocks;
	set<channelID> channels;
	MarkedGraph();
	virtual ~MarkedGraph();
};

#endif /* MARKEDGRAPH_H_ */
