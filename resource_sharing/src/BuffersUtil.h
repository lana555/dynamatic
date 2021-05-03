/*
 * BuffersUtil.h
 *
 *  Created on: May 17, 2020
 *      Author: dynamatic
 */

#ifndef BUFFERSUTIL_H_
#define BUFFERSUTIL_H_

#include <map>

#include "DFnetlist/Dataflow.h"
//#include "Dataflow.h"


using namespace std;
using namespace Dataflow;

vector<string> getThroughputFromFile(std::string filename, bool verbose=false);

#endif /* BUFFERSUTIL_H_ */
