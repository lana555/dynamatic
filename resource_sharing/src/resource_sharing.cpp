//
// Created by pejic on 29/07/19.
//

#include "resource_sharing.h"

#include <algorithm>
#include <iomanip>
#include <list>
#include <map>

#include "BuffersUtil.h"
#include "ControlPathAnalysis.h"
#include "DFnetlist.h"
#include "Dataflow.h"
#include "DisjointSet.h"
#include "MILP_Model.h"
#include "MarkedGraph.h"
#include "Minimization.h"
#include "bb_graph_reader.h"

void update_buffer_bbIDs(DFnetlist &df) {
	for (auto id : df.DFI->allBlocks) {
		if (df.DFI->getBasicBlock(id) == -1) {
			int maxId = -1;
			for (auto port : df.getPorts(id, PortDirection::ALL_PORTS)) {
				int channel = df.getConnectedChannel(port);
				int block;

				if (df.getSrcPort(channel) == port) {
					block = df.getBlockFromPort(df.getDstPort(channel));
				} else {
					block = df.getBlockFromPort(df.getSrcPort(channel));
				}

				maxId = max(maxId, df.DFI->getBasicBlock(block));
			}
			df.DFI->setBasicBlock(id, maxId);
		}
	}
}

map<bbID, set<blockID>> getNodesPerBBs(DFnetlist &df) {
	map<int, set<int>> ret;

	for (auto node : df.DFI->allBlocks) {
		int bbId = df.DFI->getBasicBlock(node);
		// cout << "name " + df.DFI->getBlockName(node) << " BBid " +
		// to_string(bbId) << endl;

		if (ret.find(bbId) == ret.end()) {
			ret.insert( { bbId, { node } });
		} else {
			ret.find(bbId)->second.insert(node);
		}
	}

	return ret;
}

vector<DisjointSet> extractSets(DFnetlist &df, BB_graph &bb_graph) {
	map<bbID, set<blockID>> bbs = getNodesPerBBs(df);

	// setid to
	map<int, DisjointSet> disjoint_sets;

	for (auto link : bb_graph.links) {
		if (link.set_id <= 0) {
			continue;
		}

		// if new disjoint set discovered add it to the set of disjoint sets
		if (disjoint_sets.find(link.set_id) == disjoint_sets.end()) {
			disjoint_sets.insert( { link.set_id, DisjointSet(link.set_id) });
		}

		DisjointSet &disjoint_set = disjoint_sets.find(link.set_id)->second;

		for (auto mg_id : link.markedGraphs) {
			// if marked graph id not present in disjoint set
			if (!disjoint_set.contains(mg_id)) {
				disjoint_set.insert(mg_id, MarkedGraph());
			}
			MarkedGraph &mg = disjoint_set.marked_graphs.find(mg_id)->second;

			// insert all nodes in the BBs linked together
			mg.blocks.insert(bbs.find(link.src)->second.begin(),
					bbs.find(link.src)->second.end());
			mg.blocks.insert(bbs.find(link.dst)->second.begin(),
					bbs.find(link.dst)->second.end());

			// iterates over all channels
			for (auto channel : df.DFI->allChannels) {
				int srcBlock = df.getBlockFromPort(df.getSrcPort(channel));
				int dstBlock = df.getBlockFromPort(df.getDstPort(channel));

				bool internalChannelForSrcBB = inSet(bbs.find(link.src)->second,
						srcBlock)
						&& inSet(bbs.find(link.src)->second, dstBlock);
				bool internalChannelForDstBB = inSet(bbs.find(link.dst)->second,
						srcBlock)
						&& inSet(bbs.find(link.dst)->second, dstBlock);
				bool connectingChannel = inSet(bbs.find(link.src)->second,
						srcBlock)
						&& inSet(bbs.find(link.dst)->second, dstBlock);

				if (internalChannelForSrcBB || internalChannelForDstBB
						|| connectingChannel) {
					mg.channels.insert(channel);
				}
			}
		}
	}

	vector<DisjointSet> result { };
	for (auto set : disjoint_sets) {
		result.push_back(set.second);
	}
	return result;
}

initializer_list<string> mergeable_operation = { "mul_op", "fmul_op", "fsub_op",
		"fadd_op", "fdiv_op", "sdiv_op" };

bool test_node_type(DFnetlist df, blockID id) {
	if (df.DFI->getBlockType(id) != OPERATOR)
		return false;
	string operation = df.DFI->getOperation(id);
	for (auto op : mergeable_operation) {
		if (op == operation) {
			return true;
		}
	}

	return false;
}

bool test_node_type(DFnetlist df, blockID id, string mergeable_operation) {
	return df.DFI->getBlockType(id) == OPERATOR
			&& df.DFI->getOperation(id) == mergeable_operation;
}
bool inSet(set<int> group, int item) {
	return group.find(item) != group.end();
}

vector<string> getThroughputs(vector<MergeGroup> merge_groups, string filename,
		bool verbose = false) {
	DFnetlist tmp("./_input/" + filename + "_graph.dot",
			"./_input/" + filename + "_bbgraph.dot");
	map<bbID, set<blockID>> nodesPerBB = getNodesPerBBs(tmp);
	map<bbID, vector<blockID>> controlPathsPerBB { };

	for (auto merge_group : merge_groups) {
		for (auto bb_to_blocks_ordering : merge_group.blocks) {
			int bbId = bb_to_blocks_ordering.first;
			vector<pair<int, blockID>> ordering = bb_to_blocks_ordering.second;

			if (controlPathsPerBB.find(bbId) == controlPathsPerBB.end()) {
				controlPathsPerBB.insert(
						{ bbId, findControlPath(tmp, nodesPerBB[bbId]) });
			}

			vector<blockID> &controlPath = controlPathsPerBB[bbId];

			int index = 0;
			for (auto order_id_pair : ordering) {
				if (index >= controlPath.size() - 1) {
					addStageToControlPath(tmp, controlPath);
				}
				connectForkToBlock(tmp, controlPath[index],
						order_id_pair.second);
				index += 2;
			}
		}
	}

	tmp.writeDot("./_tmp/out.dot");
	return getThroughputFromFile("./_tmp/out", verbose);
}

bool checkThroughput(vector<MergeGroup> merge_groups, vector<string> &expected,
		string filename, bool verbose = false) {
	cout << "checking throughput" << endl;
	vector<string> results = getThroughputs(merge_groups, filename, verbose);
	if (verbose) {
		for (auto result : results)
			cout << result << endl;
		return true;
	}
	assert(results.size() == expected.size());
	for (int i = 0; i < results.size(); ++i) {
		if (results[i] != expected[i]) {
			cout << "Mismatch : " << endl;
			cout << "- expected : " << expected[i] << endl;
			cout << "- but got  : " << results[i] << endl;
			return false;
		}
	}
	cout << "Throughput unharmed" << endl;
	return true;
}

MergeGroup combine_groups(MergeGroup mg_1, MergeGroup mg_2) {
	MergeGroup new_merge_group { };
	new_merge_group.blocks = mg_1.blocks;
	for (auto mapping : mg_2.blocks) {
		for (auto unit : mg_2.blocks[mapping.first]) {
			new_merge_group.insert(mapping.first, unit.second);
		}
	}
	return new_merge_group;
}

bool try_combine_groups(vector<MergeGroup> &merge_groups,
		vector<string> &initial_throughputs, string filename) {
	for (auto it_1 = merge_groups.begin(); it_1 != merge_groups.end(); ++it_1) {
		for (auto it_2 = it_1 + 1; it_2 != merge_groups.end(); ++it_2) {
			vector<MergeGroup> copy { combine_groups(*it_1, *it_2) };
			copy.insert(copy.end(), merge_groups.begin(), it_1);
			copy.insert(copy.end(), it_1 + 1, it_2);
			copy.insert(copy.end(), it_2 + 1, merge_groups.end());

			if (checkThroughput(copy, initial_throughputs, filename)) {
				merge_groups = copy;
				return true;
			}
		}
	}
	return false;
}

vector<MergeGroup> intra_set_sharing(DFnetlist &df, DisjointSet disjoint_set,
		map<int, MyBlock> &nodes, vector<string> &initial_throughputs,
		string merged_operation, string filename) {
	vector<MergeGroup> merge_groups { };

	set<blockID> flattened_set = { };
	for (auto marked_graph_mapping : disjoint_set.marked_graphs) {
		for (auto blockId : marked_graph_mapping.second.blocks) {
			flattened_set.insert(blockId);
		}
	}
	// create a merge group for each unit
	for (auto blockId : flattened_set) {
		if (test_node_type(df, blockId, merged_operation)) {
			MergeGroup singleton { };
			bbID key = df.DFI->getBasicBlock(blockId);
			vector<pair<int, blockID>> value = { { 0, blockId } };
			singleton.blocks[key] = value;
			merge_groups.push_back(singleton);
		}

	}
	while (try_combine_groups(merge_groups, initial_throughputs, filename))
		;
	return merge_groups;
}

void resource_sharing2(DFnetlist &df, vector<DisjointSet> disjoint_sets,
		map<int, MyBlock> &nodes, string filename) {
	vector<string> initial_throughputs = getThroughputs(vector<MergeGroup> { },
			filename);
	// intra set
	map<string, vector<vector<MergeGroup>>> merge_groups_per_set { };
	for (auto merge_op : mergeable_operation) {
		cout << "intra set sharing for " + merge_op << endl;
		vector<vector<MergeGroup>> merge_group_for_op { };
		for (auto set : disjoint_sets) {
			merge_group_for_op.push_back(
					intra_set_sharing(df, set, nodes, initial_throughputs,
							merge_op, filename));
		}
		merge_groups_per_set[merge_op] = merge_group_for_op;
	}

	// inter set
	map<string, vector<MergeGroup>> merge_groups { };
	for (auto merge_op : mergeable_operation) {
		cout << "inter set sharing for " + merge_op << endl;
		for (auto set_merge_groups : merge_groups_per_set[merge_op]) {
			for (int i = 0; i < set_merge_groups.size(); ++i) {
				if (i < merge_groups[merge_op].size()) {
					merge_groups[merge_op][i] = combine_groups(
							merge_groups[merge_op][i], set_merge_groups[i]);
				} else {
					merge_groups[merge_op].push_back(set_merge_groups[i]);
				}
			}
		}
	}
	vector<MergeGroup> flattened_merge_groups { };
	for (auto it : merge_groups) {
		vector<MergeGroup> merge_group_per_op = it.second;
		flattened_merge_groups.insert(flattened_merge_groups.end(),
				merge_group_per_op.begin(), merge_group_per_op.end());
	}
	// once we have shared all components in the same sets we run buffers one
	// last time to obtain ideal buffer placement when considering the sharing
	cout << "checking final throughputs == initial throughputs" << endl;
	assert(
			checkThroughput(flattened_merge_groups, initial_throughputs, filename));
	DFnetlist newDf("./_tmp/out_graph_buf.dot",
			"./_input/" + filename + "_bbgraph.dot");
	removeAdditionToCp(newDf);
	map<string, blockID> names_to_ids { };
	for (auto id : newDf.DFI->allBlocks) {
		if (newDf.DFI->getBlockType(id) == OPERATOR) {
			names_to_ids.insert( { newDf.DFI->getBlockName(id), id });
		}
	}

	vector<MergeGroup> new_merge_groups { };
	for (auto merge_group : flattened_merge_groups) {
		MergeGroup new_merge_group { };
		for (auto bb_to_set : merge_group.blocks) {
			vector<pair<int, blockID>> new_ordering { };
			for (auto it = merge_group.blocks[bb_to_set.first].begin();
					it != merge_group.blocks[bb_to_set.first].end(); ++it) {
				new_ordering.push_back(
						{ it->first, names_to_ids[df.DFI->getBlockName(
								it->second)] });
			}
			new_merge_group.blocks.insert( { bb_to_set.first, new_ordering });
		}
		new_merge_groups.push_back(new_merge_group);
	}

	// actual merging
	for (auto merge_group : new_merge_groups) {
		if (merge_group.size() > 1) {
			merge_group.print(newDf);
			minimizeNBlocks(newDf, merge_group);
		}
	}

	newDf.writeDot("./_output/" + filename + "_graph.dot");
}

blockID getBlockWithName(map<int, MyBlock> &nodes, string name) {
	for (auto mapping : nodes) {
		MyBlock &block = nodes[mapping.first];
		if (block.name == name) {
			return mapping.first;
		}
	}
	return -1;
}

void try_suggestion(DFnetlist &df, vector<vector<string>> suggestion,
		vector<DisjointSet> disjoint_sets, map<int, MyBlock> &nodes,
		string filename) {

	cout << "getting initial throughputs" << endl;
	vector<string> initial_throughputs = getThroughputs(vector<MergeGroup> { },
			filename);

	vector<MergeGroup> merge_groups = { };
	for (auto suggested_merge_group : suggestion) {
		MergeGroup new_merge_group { };
		for (auto unit_name : suggested_merge_group) {
			blockID id = getBlockWithName(nodes, unit_name);
			if (id == -1) {
				cout << "could not find block to be shared called " + unit_name
						<< endl;
				throw 1; //TODO: better
			} else {
				new_merge_group.insert(df.DFI->getBasicBlock(id), id);
			}
		}
		merge_groups.push_back(new_merge_group);
	}

	cout << "checking final throughputs == initial throughputs" << endl;
	assert(checkThroughput(merge_groups, initial_throughputs, filename));

	DFnetlist newDf("./_tmp/out_graph_buf.dot",
			"./_input/" + filename + "_bbgraph.dot");
	removeAdditionToCp(newDf);
	map<string, blockID> names_to_ids { };
	for (auto id : newDf.DFI->allBlocks) {
		if (newDf.DFI->getBlockType(id) == OPERATOR) {
			names_to_ids.insert( { newDf.DFI->getBlockName(id), id });
		}
	}

	vector<MergeGroup> new_merge_groups { };
	for (auto merge_group : merge_groups) {
		MergeGroup new_merge_group { };
		for (auto bb_to_set : merge_group.blocks) {
			vector<pair<int, blockID>> new_ordering { };
			for (auto it = merge_group.blocks[bb_to_set.first].begin();
					it != merge_group.blocks[bb_to_set.first].end(); ++it) {
				new_ordering.push_back(
						{ it->first, names_to_ids[df.DFI->getBlockName(
								it->second)] });
			}
			new_merge_group.blocks.insert( { bb_to_set.first, new_ordering });
		}
		new_merge_groups.push_back(new_merge_group);
	}

	// actual merging
	for (auto merge_group : new_merge_groups) {
		if (merge_group.size() > 1) {
			merge_group.print(newDf);
			minimizeNBlocks(newDf, merge_group);
		}
	}

	newDf.writeDot("./_output/" + filename + "_graph.dot");
}
