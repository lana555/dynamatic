/*
 * ControlPathAnalysis.h
 *
 *  Created on: May 16, 2020
 *      Author: dynamatic
 */

#ifndef CONTROLPATHANALYSIS_H_
#define CONTROLPATHANALYSIS_H_


#include "MyBlock.h"
#include "MyChannel.h"
#include "MarkedGraph.h"
#include "bb_graph_reader.h"
#include "DisjointSet.h"

vector<blockID> findControlPath(DFnetlist & df, set<blockID> blocks_in_bb);

void addStageToControlPath(DFnetlist & df, vector<blockID> & control_path);

void connectForkToBlock(DFnetlist & df, blockID fork, blockID to_be_connected);

void removeAdditionToCp(DFnetlist & df);

blockID getSingletonCpBlock(DFnetlist &df, set<blockID> &blocks,
		BlockType type);

set<blockID> getCpBlock(DFnetlist &df, set<blockID> &blocks, BlockType type);

blockID addForkAfterPhi(DFnetlist &df, set<blockID> blocks_in_bb);

#endif /* CONTROLPATHANALYSIS_H_ */
