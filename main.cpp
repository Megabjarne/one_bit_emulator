#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <iterator>
#include <algorithm>

#include "state.hpp"
#include "interpreter.hpp"

using std::cout;
using std::cerr;
using std::endl;
using std::vector;
using std::ifstream;
using std::string;

programstate state;

int main(int argn, char **args){
	
	if (argn != 2){
		cerr<<"usage: [filename]"<<endl;
		return -1;
	}
	//open file
	ifstream file(args[1]);
	
	//read file line by line
	string line;
	while (std::getline(file, line)){
		state.program.push_back(line);
	}
	
	for (auto i = state.program.begin(); i != state.program.end();){
		if ((*i).front() == '#')
			i = state.program.erase(i);
		else
			i++;
	}
	
	//check code for labels
	handlelabels(state);
	
	/*for (int i=0; i<state.program.size(); i++){
		cout<<state.program[i]<<"\n";
	}*/
	
	while (!state.halted){
		tick(state);
	}
	
}


