/*
 * DisjointSet.h
 *
 *  Created on: Apr 17, 2020
 *      Author: dynamatic
 */

#ifndef DISJOINTSET_H_
#define DISJOINTSET_H_


#include <map>
#include "Dataflow.h"
#include "MarkedGraph.h"

using namespace std;
using namespace Dataflow;

class DisjointSet {
public:
	map<int, MarkedGraph> marked_graphs;

	DisjointSet(int disjoint_set_id);
	virtual ~DisjointSet();

	bool contains(int marked_graph_id);
	void insert(int id, MarkedGraph marked_graph);

	friend bool operator==(const DisjointSet & d1, const DisjointSet & d2) {return d1.id == d2.id;}
	friend bool operator!=(const DisjointSet & d1, const DisjointSet & d2) {return d1.id != d2.id;}

private:
	int id = -1;
};


#endif /* DISJOINTSET_H_ */
