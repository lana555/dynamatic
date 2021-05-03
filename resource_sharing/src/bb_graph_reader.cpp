//
// Created by pejic on 21/08/19.
//

#include "bb_graph_reader.h"
#include <string>
#include <regex>
#include <stdio.h>
#include <stdlib.h>
#include <fstream>
#include <iostream>
#include <unistd.h>

using namespace std;

regex line_regex("^\\s*(\"\\w+\") \\-> (\"\\w+\") \\[.*DSU = ([0-9]+).*MG = \"([^\"]*)\"\\];\\s*$");
regex mg_regex("([0-9]+)");


regex block_regex("^\\s*\"?block([0-9]+)\"?\\s*;?\\s*$");

BB_graph::Link::Link(string line)
{
	smatch res;
	if(!regex_match(line, res, line_regex))
	{
		cerr << "*** Exception - Invalid line format." << endl;
		throw std::exception();
	}

	src = blockId(res[1]);
	dst = blockId(res[2]);
	set_id = stoi(res[3]);

	string mgs = res[4];
	while (regex_search(mgs, res, mg_regex)) {
		this->markedGraphs.push_back(stoi(res[1]));
		mgs = res.suffix().str(); // Proceed to the next match
	}
}

BB_graph::Link::Link(int src, int dst, int set_id, vector<int> mgs)
{
	this->src = src;
	this->dst = dst;
	this->set_id = set_id;
	this->markedGraphs = mgs;
}

bool operator== (BB_graph::Link l1, BB_graph::Link l2)
{
	if(l1.src != l2.src || l1.dst != l2.dst || l1.set_id != l2.set_id || l1.markedGraphs.size() != l2.markedGraphs.size())
	{
		return false;
	}

	for(auto item1: l1.markedGraphs)
	{
		if(find(l2.markedGraphs.begin(), l2.markedGraphs.end(), item1) == l2.markedGraphs.end())
		{
			return false;
		}
	}

	return true;
}

bool operator!= (BB_graph::Link l1, BB_graph::Link l2)
{
	return !(l1 == l2);
}




bool BB_graph::Link::isLink(string line)
{
	try
	{
		Link link(line);
		link.markedGraphs.push_back(1);
		return true;
	}
	catch(...)
	{
		return false;
	}
}

bool BB_graph::isBlock(string line)
{
	try
	{
		blockId(line);
		return true;
	}
	catch (...)
	{
		return false;
	}
}

int BB_graph::blockId(string line)
{
	smatch res;
	if(!regex_match(line, res, block_regex))
	{
		throw "*** Exception - line is not a block";
	}

	return stoi(res[1]);
}

BB_graph::BB_graph(string filename)
{
	ifstream file;
	file.open(filename);

	auto res = get_current_dir_name();

	if(!file.is_open()){
		cerr << "could not open file :" + filename << endl;
		throw std::exception();
	}

	string line;

	while (std::getline(file, line))
	{
		if (line.rfind("Digraph", 0) == 0) {
			continue;
		}

		if(line.rfind("spline") != string::npos)
		{
			continue;
		}

		if(line.rfind("{") != string::npos || line.rfind("}") != string::npos)
		{
			continue;
		}

		if(isBlock(line))
		{
			bbs.push_back(blockId(line));
		}
		else
		{
			links.push_back(Link(line));
		}
	}

	file.close();
}

