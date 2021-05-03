/*
 * DisjointSet.cpp
 *
 *  Created on: Apr 17, 2020
 *      Author: dynamatic
 */

#include "DisjointSet.h"
#include <map>

DisjointSet::DisjointSet(int disjoint_set_id) : id{disjoint_set_id}, marked_graphs{} {}

DisjointSet::~DisjointSet() {}

bool DisjointSet::contains(int id){
	return marked_graphs.find(id) != marked_graphs.end();
}

void DisjointSet::insert(int id, MarkedGraph mg){
	marked_graphs[id] = mg;
}


