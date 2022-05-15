//
// Created by pejic on 05/07/19.
//

#include <iostream>
#include <vector>
#include <string>
#include "Dataflow.h"
#include <stdio.h>
#include <stdlib.h>
#include <cstring>

#include <unistd.h>


#include "DFnetlist.h"
#include "MyBlock.h"
#include "resource_sharing.h"
#include "BuffersUtil.h"

using namespace std;
using namespace Dataflow;


using vecParams = vector<string>;

int main_help()
{
	cerr << "Available commands:" << endl;
	cerr << "  min filename [timeout duration for MILP]: Execute the minimization of the component graph." << endl;
	cerr << "  help:     Print tool help." << endl;

	return 0;
}

int min(vecParams& params)
{
	cout<<"Executing min"<<endl;

	if(params.size() == 0)
	{
		cerr<<"No input file provided."<<endl;
		cerr<<"Aborting."<<endl;

		main_help();

		return 0;
	}

	string filename = params[0];
	cout<<filename<<endl;

	int timeout = params.size() > 1 ? stoi(params[1]) : DEFAULT_MILP_TIMEOUT;

	DFnetlist DF("./_input/"+filename+"_graph.dot");
	try {
		BB_graph bbGraph("./_input/"+filename+"_bbgraph.dot");

		update_buffer_bbIDs(DF);


		map<int, MyBlock> nodes;
		for(auto block: DF.DFI->allBlocks)
		{
			MyBlock node(DF, block);
			nodes.emplace(block, node);
		}
		vector<DisjointSet> sets = extractSets(DF, bbGraph);

		 resource_sharing2(DF, sets, nodes, filename, timeout);

		//example for gsum
		//try_suggestion(DF, {{"fadd_11", "fadd_14"}, 
		//{"fmul_10", "fmul_12", "fmul_13", "fmul_15"}}, sets, nodes, filename);
		//try_suggestion(DF, {{"fadd_9", "fadd_11"}}, sets, nodes, filename);

	} catch (std::exception&) {
		return EXIT_FAILURE;
	}

	string cmd_str = "dot -Tpng ./_output/"+filename+"_graph.dot > ./_output/"+filename+"_graph.png";
	system(cmd_str.c_str());

	cout<<"Done"<<endl;

	return 0;
}


int main(int argc, char *argv[])
{
	string exec = argv[0];

	if (argc == 1)
	{
		return main_help();
	}

	string command = argv[1];

	if (command == "help")
	{
		return main_help();
	}

	vecParams params;

	for (int i = 2; i < argc; ++i)
	{
		params.push_back(argv[i]);
	}

	if(command == "min")
	{
		return min(params);
	}

	cerr << command << ": Unknown command." << endl;
	main_help();
	return 1;
}
