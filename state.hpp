#ifndef _state
#define _state

#include <vector>
#include <string>

using std::vector;
using std::pair;
using std::string;

struct programstate{
	bool memory[513];
	bool reg_a = 0;
	bool reg_b = 0;
	bool reg_c = 0;
	vector<string> program;
	vector<pair<string, int> > labels;
	vector<int> stack;
	int reg_ip = 0;
	bool output[513];
	int output_count = 0;
	bool halted = false;
};

#endif

