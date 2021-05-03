//
// Created by pejic on 30/07/19.
//

#include "MyBlock.h"
#include "MyChannel.h"
#include "MarkedGraph.h"
#include "bb_graph_reader.h"
#include "DisjointSet.h"
#include <list>
#include <set>
#include <map>
#include <vector>

#ifndef RESOURCE_MINIMIZATION_RESOURCE_SHARING_H
#define RESOURCE_MINIMIZATION_RESOURCE_SHARING_H

void update_buffer_bbIDs(DFnetlist &df);
double calc_th(DFnetlist &copy);

struct MergeGroup {
	//map BBid -> Set((order in bb, id))
	map<int, vector<pair<int, blockID> > > blocks;

	int size() {
		int res = 0;
		for (auto it : blocks) {
			res += blocks[it.first].size();
		}
		return res;
	}
	//double retiming_diff;
	void print(DFnetlist &df) {
		blockID first_block = blocks.begin()->second.begin()->second;
		cout
				<< "Merge groups of type "
						+ df.DFI->printBlockType(
								df.DFI->getBlockType(first_block)) << endl;
		for (auto mapping : blocks) {
			cout << "- Shared components in BB " << mapping.first << " : "
					<< endl;
			for (auto pair : blocks[mapping.first]) {
				cout << "\t- At index " << pair.first << ", "
						<< df.DFI->getBlockName(pair.second) << endl;
			}
		}
	}

	void insert(bbID bbId, blockID blockId) {
		if (blocks.find(bbId) == blocks.end()) {
			blocks[bbId] = { };
		}
		vector<pair<int, blockID>> &ordering = blocks[bbId];
		ordering.push_back( { ordering.size(), blockId });
	}
};

typedef bool (*test_type)(MyBlock);

bool test_node_type(DFnetlist df, blockID id);

bool compatible(MyBlock &node1, MyBlock &node2, bool check_retiming = true);

bool inSet(set<int> group, int item);

void merge(int node1, int node2, vector<MergeGroup> &mergeGroups,
		map<int, MyBlock> &nodes, bool is_intra_set = false);

DFnetlist copyNetwork(vector<MyBlock> &nodes, vector<MyChannel> &channels);

bool mergeAffectsThtoughput(DFnetlist &df, int node1_id, int node2_id,
		set<int> set_of_nodes, set<int> set_of_channels,
		map<int, MyBlock> &nodes, vector<MergeGroup> &mgs);

void resource_sharing(DFnetlist &df, vector<DisjointSet> S,
		map<int, MyBlock> &nodes);

void resource_sharing2(DFnetlist &df, vector<DisjointSet> disjoint_sets,
		map<int, MyBlock> &nodes, string filename);

map<int, set<int>> getNodesPerBBs(DFnetlist &df);

vector<DisjointSet> extractSets(DFnetlist &df, BB_graph &bb_graph);

void try_suggestion(DFnetlist &df, vector<vector<string>> suggestion,
		vector<DisjointSet> disjoint_sets, map<int, MyBlock> &nodes,
		string filename);

#endif //RESOURCE_MINIMIZATION_RESOURCE_SHARING_H
