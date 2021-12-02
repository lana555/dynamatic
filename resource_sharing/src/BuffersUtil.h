/*
 * BuffersUtil.h
 *
 *  Created on: May 17, 2020
 *      Author: dynamatic
 */

#ifndef BUFFERSUTIL_H_
#define BUFFERSUTIL_H_

#include <map>

#include "Dataflow.h"

using namespace std;
using namespace Dataflow;

const int DEFAULT_MILP_TIMEOUT = 100;

vector<string> getThroughputFromFile(std::string filename, bool verbose, int timeout=100);


#endif /* BUFFERSUTIL_H_ */
