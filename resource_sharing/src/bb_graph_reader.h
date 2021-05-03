//
// Created by pejic on 21/08/19.
//

#ifndef RESOURCE_MINIMIZATION_BB_GRAPH_READER_H
#define RESOURCE_MINIMIZATION_BB_GRAPH_READER_H

#include <string>
#include <iostream>
#include <vector>
#include <fstream>


using namespace std;


class BB_graph
{

public:
	struct Link{
		int src;
		int dst;
		int set_id;
		vector<int> markedGraphs;

		Link(string line);
		Link(int src, int dst, int set_id, vector<int> mgs);

		friend bool operator== (Link l1, Link l2);
		friend bool operator!= (Link l1, Link l2);

		static bool isLink(string line);
	};

	vector<int> bbs;
	vector<Link> links;

	static bool isBlock(string line);
	static int blockId(string line);

	BB_graph(string filename);
};

#endif //RESOURCE_MINIMIZATION_BB_GRAPH_READER_H
