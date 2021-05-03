/*
 * minimization.h
 *
 *  Created on: May 9, 2020
 *      Author: dynamatic
 */

#ifndef MINIMIZATION_H_
#define MINIMIZATION_H_

#include "DFnetlist/Dataflow.h"
#include "resource_sharing.h"
#include <vector>

using namespace Dataflow;
using namespace std;


int minimizeNBlocks(DFnetlist& df, MergeGroup merge_group);


#endif /* MINIMIZATION_H_ */
