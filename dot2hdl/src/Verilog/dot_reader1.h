/*
 * dot_reader1.h
 *
 *  Created on: 11-Jun-2021
 *      Author: madhur
 */

#ifndef DOT_READER1_H_
#define DOT_READER1_H_
#pragma once
#include <fstream>
#include <iostream>
#include <cstdint>
#include <string>
#include <vector>
#include <map>
#include <utility>
#include "ComponentClass.h"

class DotReader{
private:
	std::string fileName;
	int numberOfBlocks;

	std::vector<Component*> componentList;//List of all the Components present in the design
	std::map<std::string, Component*> componentMap;//List of all the Components indexed by their names(string)


public:
	DotReader();//Default Constructor
	DotReader(std::string file_name);//Parameterized constructor
	int lineReader();


	std::vector<Component*>& getComponentList() {
		return componentList;
	}

	std::map<std::string, Component*>& getComponentMap(){
		return componentMap;
	}

	std::string& getFileName() {
		return fileName;
	}

private:
	void generateComponentList();
	void generateConnectionMap();
};



#endif /* DOT_READER1_H_ */
