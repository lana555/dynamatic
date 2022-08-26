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
#include <regex>

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

#define TARGET_PATH	"/etc/dynamatic/data/"


float read_data_from_csv(int component_index, int bitsize_index, std::string data_type, std::string serial_number, int mode) {
int offset = 0;

    switch ( mode )
{
    	case DATA:
	offset = 0;
    break;
   	 case VALID:
	offset = 7;
    break;
    	case READY:
	offset = 14;
    break; 
    	case VR:
	offset = 21;
	bitsize_index = 0; //All bitwidth point to the same location
    break; 
    	case CV:
	offset = 22;
	bitsize_index = 0; //All bitwidth point to the same location
    break; 
    	case CR:
	offset = 23;
	bitsize_index = 0; //All bitwidth point to the same location
    break; 
    	case VC:
	offset = 24;
	bitsize_index = 0; //All bitwidth point to the same location
    break; 
    	case VD:
	offset = 25;
	bitsize_index = 0; //All bitwidth point to the same location
    break; 
 	default:
        offset = 0;
    break;
}
    std::string dhls_path(std::getenv("DHLS_INSTALL_DIR"));
    //std::string filename = dhls_path + "/etc/dynamatic/data/" + data_type + "/" + serial_number + "_" + data_type + ".csv";
    std::string filename = dhls_path + "/etc/dynamatic/data/targets/" + serial_number + "_" + data_type + ".dat";
    std::ifstream file(filename, std::ifstream::in);

    float read_data = 10.0;
    

    if (!file) {
        cerr << "Error opening " << filename << " use default values instead" << endl;
        //filename = dhls_path + "/etc/dynamatic/data/" + data_type + "/" + "default_" + data_type + ".csv";
        filename = dhls_path + "/etc/dynamatic/data/targets/" + "default_" + data_type + ".dat";
        file = std::ifstream(filename, std::ifstream::in);
    } 

    std::string line;
    int max_index = component_index+1 > CMP_MAX ? CMP_MAX : component_index+1;
    

    for (int i = 0; i<max_index; ++i) {
        std::getline(file, line);
    }
    
    std::regex regex("\\,");

    std::vector<std::string> out(
            std::sregex_token_iterator(line.begin(), line.end(), regex, -1),
            std::sregex_token_iterator()
    );

//cout << " component_index " << component_index << " bitsize_index " << bitsize_index << " offset " << offset;
//cout << " out[bitsize_index+offset] " << out[bitsize_index+offset] << endl;

   // if (  out[bitsize_index+offset] != NULL) 
    	read_data = std::stof(out[bitsize_index+offset]);

    file.close();
    

    return read_data;
}

float get_component_delay(std::string component, int datasize, std::string serial_number, int mode) {
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

	//cout << " component " << component << " datasize " << datasize << endl;

    return (read_data_from_csv(get_component_index(component), get_bitsize_index(datasize), "delay", serial_number, mode) *
            route_delay);
}

int get_component_latency(std::string component, int datasize, std::string serial_number) {
    return (int) read_data_from_csv(get_component_index(component), get_bitsize_index(datasize), "latency", serial_number, 0);
}
