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

using namespace std;
using namespace Dataflow;


using vecParams = vector<string>;

int main_help()
{
	cerr << "Available commands:" << endl;
	cerr << "  min:      Execute the minimization of the component graph." << endl;
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

		resource_sharing2(DF, sets, nodes, filename);

		//example for bicg_float
		//try_suggestion(DF, {{"fadd_19", "fadd_26"}, {"fmul_18", "fmul_25"}}, sets, nodes, filename);

	} catch (std::exception&) {
		return EXIT_FAILURE;
	}

	// if (!DF.writeDot("./_output/"+filename+"_graph.dot"))
	// {
	// 	char* cwd = new char[2000];
	// 	cwd = getcwd(cwd, 2000);
	// 	cout<<cwd<<endl;
	// 	delete [] cwd;

	// 	cout<<"**********Failed to save file"<<endl;
	// }

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
