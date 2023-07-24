/*
 * Graph.cpp
 *
 *  Created on: 30-May-2021
 *      Author: madhur
 */

#include "Graph.h"

Graph::Graph(DotReader dotReader) {
	// TODO Auto-generated constructor stub
	this->dotReader = dotReader;
}


void Graph::startDFS(){

	initializeDFS();//populates the visited map, otherwise it throws error

	//Starts DFS from the "start_0" component
	Component* root = findRoot();
	DFS(root);

	//This loop will make sure that all those components that have not yet been visited
	//will also be traversed. Components which have no input will never be reached
	//Eg. "const" has no input, so it has to be traversed separately.
	//Additionally, the entry points are also one of the roots of the graph, so all the entry points
	//Other than "start" are also traversed here
	for(auto it = visited.begin(); it != visited.end(); it++){
		Component* comp = dotReader.getComponentMap().at(it->first);
		//		cout << comp->name << " " << comp->type << endl;
		//If a component has no input or component is an entry point, and not visited yet.
		if((comp->in.size == 0 || comp->type == COMPONENT_START) && !visited[comp->name]){
			root = comp;
			DFS(root);
		}
	}
}


void Graph::DFS(Component* v){
	//CurrentDFSDepth keeps track of current DFS Level. For debugging purposes only
	if(currentDFSDepth++ < maxDFSDepth){
		visited[v->name] = true;
		std::cout << v->name << " -> ";


		Component* nextComponent;
		//		cout << v->io.size() << endl;
		if(v->io.size() != 0){

			for(auto it = v->io.begin(); it != v->io.end(); it++){
				std::string name = ((*it).first)->name;
				nextComponent = (dotReader.getComponentMap().at(name));
				if(!visited[name]){
					DFS(nextComponent);
					currentDFSDepth--;
				}else{
					std::cout << std::endl;
				}
			}
		}else{
			std::cout << std::endl;
		}
	} else
		return;
}

//The graph starts with the component named "start_0"
Component* Graph::findRoot(){
	return dotReader.getComponentMap().at("start_0");
}

//Initialize visited map.
void Graph::initializeDFS(){
	for(auto it = dotReader.getComponentList().begin(); it != dotReader.getComponentList().end(); it++){
		visited.insert({(*it)->name, false});
	}
	std::cout << "DFS Starts: " << std::endl;
}



























