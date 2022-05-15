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
	using ordering = vector<blockID>;
	map<int, vector<blockID>> blocks;

	int size() {
		int res = 0;
		for (auto it : blocks) {
			res += blocks[it.first].size();
		}
		return res;
	}

	void print(DFnetlist &df) {
		blockID first_block = *(blocks.begin()->second.begin());
		cout
				<< "Merge groups of type "
						+ df.DFI->printBlockType(
								df.DFI->getBlockType(first_block)) << endl;
		for (auto mapping : blocks) {
			cout << "- Shared components in BB " << mapping.first << " : "
					<< endl;
			ordering ord = mapping.second;
			for (int i = 0; i < ord.size(); ++i) {
				cout << "\t- At index " << i << ", "
						<< df.DFI->getBlockName(ord[i]) << endl;
			}
		}
	}

	void insert(bbID bbId, blockID blockId) {
		if (blocks.find(bbId) == blocks.end()) {
			blocks[bbId] = { };
		}
		ordering &ord = blocks[bbId];
		ord.push_back(blockId);
	}

	vector<MergeGroup> get_all_orderings(){
		map<bbID, vector<ordering>> all_orderings_per_bbID = {};
		for(auto mapping : blocks){
			all_orderings_per_bbID[mapping.first] = get_ordering_permutations(mapping.second);
		}

		vector<map<bbID, ordering>> all_orderings = get_cartesian_product(all_orderings_per_bbID);

		vector<MergeGroup> all_merge_groups = {};
		for(auto possible_ordering : all_orderings){
			MergeGroup mg {};
			mg.blocks = possible_ordering;
			all_merge_groups.push_back(mg);
		}
		return all_merge_groups;
	}

	private:

		vector<map<bbID, ordering>> get_cartesian_product(map<bbID, vector<ordering>> possible_orderings_per_bbID){
			vector<map<bbID, ordering>> results {};
			//base case
			if(possible_orderings_per_bbID.size() == 1){
				pair<bbID, vector<ordering>> bb_ordering_pair = *(possible_orderings_per_bbID.begin());
				bbID id = bb_ordering_pair.first;
				vector<ordering> & orderings = bb_ordering_pair.second;
				for(auto ord : orderings){
					map<bbID, ordering> new_map{};
					new_map[id] = ord;
					results.push_back(new_map);
				}
			//recursive case
			}else{
				map<bbID, vector<ordering>> copy(possible_orderings_per_bbID);
				pair<bbID, vector<ordering>> head = *(copy.begin());
				copy.erase(head.first);
				vector<map<bbID, ordering>> inner_results = get_cartesian_product(copy);
				for(ordering ord : head.second){
					for(map<bbID, ordering> inner_result : inner_results){
						map<bbID, ordering> new_map{inner_result};
						new_map[head.first] = ord;
						results.push_back(new_map);
					}
				}
			}
			return results;
		}

		vector<ordering> get_ordering_permutations(ordering & initial_ordering){
			//base case
			if(initial_ordering.size() == 1){
				return {initial_ordering};
			//recursive case
			}else{
				vector<ordering> permutations {};
				for(int i = 0; i < initial_ordering.size(); ++i){
					ordering copy = initial_ordering;
					copy.erase(copy.begin() + i);
					vector<ordering> inner_permutations = get_ordering_permutations(copy);
					for(auto inner_permutation : inner_permutations){
						inner_permutation.push_back(initial_ordering.begin()[i]);
						permutations.push_back(inner_permutation);
					}
				}
				return permutations;
			}
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
		map<int, MyBlock> &nodes, vector<MergeGroup> &mgs, int timeout);


void resource_sharing2(DFnetlist &df, vector<DisjointSet> disjoint_sets,
		map<int, MyBlock> &nodes, string filename, int timeout);

map<int, set<int>> getNodesPerBBs(DFnetlist &df);

vector<DisjointSet> extractSets(DFnetlist &df, BB_graph &bb_graph);

void try_suggestion(DFnetlist &df, vector<vector<string>> suggestion,
		vector<DisjointSet> disjoint_sets, map<int, MyBlock> &nodes,
		string filename);

#endif //RESOURCE_MINIMIZATION_RESOURCE_SHARING_H
