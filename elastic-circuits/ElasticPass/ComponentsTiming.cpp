#include "ElasticPass/ComponentsTiming.h"
#include "ElasticPass/Pragmas.h"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <iterator>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

using namespace std;

int get_component_index(std::string component) {

    int component_indx = CMP_MAX;

    if (!(component.empty())) {
        for (int indx = 0; indx < CMP_MAX; ++indx) {
            if (!component.compare(components_name[indx])) {
                component_indx = indx;
                break;
            }
        }
    }
    return component_indx;
}

int get_bitsize_index(int datasize) {

    int bitsize_indx;

    switch (64 / datasize) {
        case 1:
            bitsize_indx = BITSIZE_64_INDX;
            break;
        case 2:
            bitsize_indx = BITSIZE_32_INDX;
            break;
        case 4:
            bitsize_indx = BITSIZE_16_INDX;
            break;
        case 8:
            bitsize_indx = BITSIZE_8_INDX;
            break;
        case 16:
            bitsize_indx = BITSIZE_4_INDX;
            break;
        case 32:
            bitsize_indx = BITSIZE_2_INDX;
            break;
        case 64:
            bitsize_indx = BITSIZE_1_INDX;
            break;

        default:
            bitsize_indx = BITSIZE_32_INDX;
    }

    return bitsize_indx;
}

float get_component_delay(std::string component, int datasize) {
    float route_delay = ROUTING_DELAY_0;

    if (get_pragma_generic("USE_ROUTE_DELAY_10")) {
        route_delay = ROUTING_DELAY_10;
    }
    if (get_pragma_generic("USE_ROUTE_DELAY_20")) {
        route_delay = ROUTING_DELAY_20;
    }
    if (get_pragma_generic("USE_ROUTE_DELAY_30")) {
        route_delay = ROUTING_DELAY_30;
    }
    if (get_pragma_generic("USE_ROUTE_DELAY_40")) {
        route_delay = ROUTING_DELAY_40;
    }

    return (datapath_delay[get_component_index(component)][get_bitsize_index(datasize)] *
            route_delay);
}

int get_component_latency(std::string component, int datasize) {
    return datapath_latency[get_component_index(component)][get_bitsize_index(datasize)];
}
