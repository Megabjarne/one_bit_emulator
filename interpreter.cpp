#include <cstdio>
#include <iostream>	//DEBUG
#include <string>
#include <regex>
using std::string;
using std::make_pair;
using std::basic_regex;
using std::regex_match;
using std::regex_replace;

#include "state.hpp"
#include "interpreter.hpp"

void handlelabels(programstate &state){
	state.labels.clear();
	string s;
	//We find lines ending in :, meaning labels
	for (int i=0; i<state.program.size();){
		s = state.program[i];
		if (s.back() == ':'){
			state.labels.push_back(make_pair( s.substr(0, s.size()-1), i ));
			//std::cout<<"added label '"<<state.labels.back().first<<"' - "<<state.labels.back().second<<"\n";
			state.program.erase(state.program.begin() + i);
		}else
		if (s.size()==0){
			state.program.erase(state.program.begin()+i);
		}else
			i++;
	}
	
	//Sort labels by label length, to avoid labels containing other labels and stuff
	bool done = false;
	pair<string, int> temp;
	while (!done){
		done = true;
		for (int i=0; i<state.labels.size()-1; i++){
			if (state.labels[i].first.size() < state.labels[i+1].first.size()){
				temp = state.labels[i];
				state.labels[i] = state.labels[i+1];
				state.labels[i+1] = temp;
				done = false;
			}
		}
	}
	
	//We find all occurences of labels in the code, and replace them with the addresses
	for (int i=0; i<state.labels.size(); i++){
		pair<string, int> label = state.labels[i];
		
		for (int j=0; j<state.program.size(); j++){
			string val = std::to_string(label.second);
			int u;
			while ((u = state.program[j].find(label.first)) != string::npos){
				//std::cout<<state.program[j]<<" "<<u<<" "<<label.first<<" "<<label.second<<"\n";
				state.program[j].replace( u, label.first.size(), val );
				//std::cout<<state.program[j]<<" "<<u<<" "<<label.first<<" "<<label.second<<"\n";
			}
		}
	}
}

void tick(programstate &state){
	if (state.program.size() <= state.reg_ip){
		std::cerr<<"exceeded program boundaries\n";
		for (int i=0;i<92;i++)
			std::cout<<state.memory[i]<<" ";
		std::cout<<std::endl;
		std::cout<<state.reg_a<<" "<<state.reg_b<<" "<<state.reg_c<<"\n";
		std::cout<<state.reg_ip<<"/"<<state.program.size()<<"\n";
		exit(-1);
	}
	string ins = state.program[state.reg_ip];
	//std::cout<<"instruction: '"<<ins<<"'\n";
	//constant -> register
	if (regex_match( ins.c_str(), basic_regex<char>("mov [01], [ABCabc]") )){
		char val;
		char reg;
		sscanf(ins.c_str(), "mov %c, %c", &val, &reg);
		//std::cout<<"moving "<<val<<" to "<<reg<<"\n";
		switch (reg){
			case 'a':
			case 'A':
				state.reg_a = (val=='0')?0:1;
			break;
			case 'b':
			case 'B':
				state.reg_b = (val=='0')?0:1;
			break;
			case 'c':
			case 'C':
				state.reg_c = (val=='0')?0:1;
			break;
		}
		state.reg_ip++;
	}else
	//constant -> memory
	if (regex_match( ins.c_str(), basic_regex<char>("mov [01], \\([0-9]+\\)") )){
		char val;
		int ind;
		sscanf(ins.c_str(), "mov %c, (%i)", &val, &ind);
		//std::cout<<"moving "<<val<<" to "<<ind<<"\n";
		if (ind >= 512){
			std::cerr<<"segmentation fault\n"; exit(-1);}
		state.memory[ind] = (val == '0')?0:1;
		state.reg_ip++;
	}else
	//constant -> memory range
	if (regex_match( ins.c_str(), basic_regex<char>("mov [01], \\([0-9]+\\-[0-9]+\\)") )){
		char val;
		int ind1, ind2;
		sscanf(ins.c_str(), "mov %c, (%i-%i)", &val, &ind1, &ind2);
		//std::cout<<"moving "<<val<<" to "<<ind<<"\n";
		if (ind1 >= 512 || ind2 >= 512){
			std::cerr<<"segmentation fault\n"; exit(-1);}
		for (int i=ind1; i<=ind2; i++){
			state.memory[i] = (val == '0')?0:1;
		}
		state.reg_ip++;
	}else
	//memory -> register
	if (regex_match( ins.c_str(), basic_regex<char>("mov \\([0-9]+\\), [ABCabc]") )){
		int val;
		char reg;
		sscanf(ins.c_str(), "mov (%i), %c", &val, &reg);
		//std::cout<<"moving "<<val<<" to "<<reg<<"\n";
		if (val >= 512){
			std::cerr<<"segmentation fault\n"; exit(-1);}
		switch (reg){
			case 'a':
			case 'A':
				state.reg_a = state.memory[val];
			break;
			case 'b':
			case 'B':
				state.reg_b = state.memory[val];
			break;
			case 'c':
			case 'C':
				state.reg_c = state.memory[val];
			break;
		}
		state.reg_ip++;
	}else
	//register -> memory
	if (regex_match( ins.c_str(), basic_regex<char>("mov [ABCabc], \\([0-9]+\\)") )){
		int val;
		char reg;
		sscanf(ins.c_str(), "mov %c, (%i)", &reg, &val);
		//std::cout<<"moving "<<reg<<" to "<<val<<"\n";
		if (val >= 512){
			std::cerr<<"segmentation fault\n"; exit(-1);}
		switch (reg){
			case 'a':
			case 'A':
				state.memory[val] = state.reg_a;
			break;
			case 'b':
			case 'B':
				state.memory[val] = state.reg_b;
			break;
			case 'c':
			case 'C':
				state.memory[val] = state.reg_c;
			break;
		}
		state.reg_ip++;
	}else
	//memory -> memory
	if (regex_match( ins.c_str(), basic_regex<char>("mov \\([0-9]+\\), \\([0-9]+\\)") )){
		int ind1;
		int ind2;
		sscanf(ins.c_str(), "mov (%i), (%i)", &ind1, &ind2);
		//std::cout<<"moving "<<ind1<<" to "<<ind2<<"\n";
		if (ind1 >= 512 || ind2 >= 512){
			std::cerr<<"segmentation fault\n"; exit(-1);}
		state.memory[ind2] = state.memory[ind1];
		state.reg_ip++;
	}else
	//memory range -> memory range
	if (regex_match( ins.c_str(), basic_regex<char>("mov \\([0-9]+\\-[0-9]+\\), \\([0-9]+\\-[0-9]+\\)") )){
		int ind11, ind12;
		int ind21, ind22;
		sscanf(ins.c_str(), "mov (%i-%i), (%i-%i)", &ind11, &ind12, &ind21, &ind22);
		//std::cout<<"moving "<<ind1<<" to "<<ind2<<"\n";
		if (ind11 >= 512 || ind12 >= 512 || ind21 >= 512 || ind22 >= 512){
			std::cerr<<"segmentation fault\n"; exit(-1);}
		if (ind11>ind12 || ind21>ind22){
			std::cerr<<"upper memory range lower than lower memory range (memory range copy)\n";
			exit(-1);
		}
		if (ind12-ind11 != ind22-ind21){
			std::cerr<<"range mismatch (memory range copy)\n";
			exit(-1);
		}
		for (int i=0;i<=ind12-ind11; i++){
			state.memory[ind21 + i] = state.memory[ind11 + i];
		}
		state.reg_ip++;
	}else
	//register -> register
	if (regex_match( ins.c_str(), basic_regex<char>("mov [ABCabc], [ABCabc]") )){
		char reg1, reg2;
		bool *r1, *r2;
		sscanf(ins.c_str(), "mov %c, %c", &reg1, &reg2);
		//std::cout<<"moving "<<reg1<<" to "<<reg2<<"\n";
		
		if (reg1 == 'a' || reg1 == 'A'){ r1 = &state.reg_a;}
		if (reg1 == 'b' || reg1 == 'B'){ r1 = &state.reg_b;}
		if (reg1 == 'c' || reg1 == 'C'){ r1 = &state.reg_c;}
		
		if (reg2 == 'a' || reg2 == 'A'){ r2 = &state.reg_a;}
		if (reg2 == 'b' || reg2 == 'B'){ r2 = &state.reg_b;}
		if (reg2 == 'c' || reg2 == 'C'){ r2 = &state.reg_c;}
		
		*r2 = *r1;
		
		state.reg_ip++;
	}else
	//jmp unconditional
	if (regex_match( ins.c_str(), basic_regex<char>("jmp [0-9]+") )){
		int dest;
		sscanf(ins.c_str(), "jmp %i", &dest);
		//std::cout<<"jmp to "<<dest<<"\n";
		state.reg_ip = dest;
	}else
	//jmp conditional zero register
	if (regex_match( ins.c_str(), basic_regex<char>("jmpz [0-9]+, [abcABC]") )){
		int dest;
		char reg;
		sscanf(ins.c_str(), "jmpz %i, %c", &dest, &reg);
		//std::cout<<"jmpz to "<<dest<<" dependent on "<<reg<<"\n";
		if (((reg=='a'||reg=='A') && state.reg_a == 0) ||
			((reg=='b'||reg=='B') && state.reg_b == 0) ||
			((reg=='c'||reg=='C') && state.reg_c == 0)){
			state.reg_ip = dest;
		}else
			state.reg_ip++;
	}else
	//jmp conditional not zero register
	if (regex_match( ins.c_str(), basic_regex<char>("jmpnz [0-9]+, [abcABC]") )){
		int dest;
		char reg;
		sscanf(ins.c_str(), "jmpnz %i, %c", &dest, &reg);
		//std::cout<<"jmpnz to "<<dest<<" dependent on "<<reg<<"\n";
		if (((reg=='a'||reg=='A') && state.reg_a != 0) ||
			((reg=='b'||reg=='B') && state.reg_b != 0) ||
			((reg=='c'||reg=='C') && state.reg_c != 0)){
			state.reg_ip = dest;
		}else
			state.reg_ip++;
	}else
	//jmp relative unconditional
	if (regex_match( ins.c_str(), basic_regex<char>("jmpr \\-{0,1}[0-9]+") )){
		int dest;
		sscanf(ins.c_str(), "jmpr %i", &dest);
		state.reg_ip += dest;
	}else
	//jmp relative conditional zero register
	if (regex_match( ins.c_str(), basic_regex<char>("jmprz \\-{0,1}[0-9]+, [abcABC]") )){
		int dest;
		char reg;
		sscanf(ins.c_str(), "jmprz %i, %c", &dest, &reg);
		//std::cout<<"jmpz to "<<dest<<" dependent on "<<reg<<"\n";
		if (((reg=='a'||reg=='A') && state.reg_a == 0) ||
			((reg=='b'||reg=='B') && state.reg_b == 0) ||
			((reg=='c'||reg=='C') && state.reg_c == 0)){
			state.reg_ip += dest;
		}else
			state.reg_ip++;
	}else
	//jmp relative conditional not zero register
	if (regex_match( ins.c_str(), basic_regex<char>("jmprnz \\-{0,1}[0-9]+, [abcABC]") )){
		int dest;
		char reg;
		sscanf(ins.c_str(), "jmprnz %i, %c", &dest, &reg);
		//std::cout<<"jmpnz to "<<dest<<" dependent on "<<reg<<"\n";
		if (((reg=='a'||reg=='A') && state.reg_a != 0) ||
			((reg=='b'||reg=='B') && state.reg_b != 0) ||
			((reg=='c'||reg=='C') && state.reg_c != 0)){
			state.reg_ip += dest;
		}else
			state.reg_ip++;
	}else
	//and constant
	if (regex_match( ins.c_str(), basic_regex<char>("and [01]") )){
		char val;
		sscanf(ins.c_str(), "and %c", &val);
		//std::cout<<"and "<<val<<"\n";
		
		state.reg_a = (val == '1')?state.reg_a:0;
		state.reg_ip++;
	}else
	//and register
	if (regex_match( ins.c_str(), basic_regex<char>("and [abcABC]") )){
		char reg;
		sscanf(ins.c_str(), "and %c", &reg);
		//std::cout<<"and "<<reg<<"\n";
		if (reg=='a'||reg=='A') state.reg_a &= state.reg_a;
		if (reg=='b'||reg=='B') state.reg_a &= state.reg_b;
		if (reg=='c'||reg=='C') state.reg_a &= state.reg_c;
		
		state.reg_ip++;
	}else
	//or constant
	if (regex_match( ins.c_str(), basic_regex<char>("or [01]") )){
		char val;
		sscanf(ins.c_str(), "or %c", &val);
		//std::cout<<"and "<<val<<"\n";
		
		state.reg_a |= (val == '1')?1:0;
		state.reg_ip++;
	}else
	//or register
	if (regex_match( ins.c_str(), basic_regex<char>("or [abcABC]") )){
		char reg;
		sscanf(ins.c_str(), "or %c", &reg);
		//std::cout<<"and "<<reg<<"\n";
		if (reg=='a'||reg=='A') state.reg_a |= state.reg_a;
		if (reg=='b'||reg=='B') state.reg_a |= state.reg_b;
		if (reg=='c'||reg=='C') state.reg_a |= state.reg_c;
		
		state.reg_ip++;
	}else
	//not, inverts a
	if (regex_match( ins.c_str(), basic_regex<char>("not") )){
		state.reg_a = !state.reg_a;
		state.reg_ip++;
	}else
	//swp, swaps two registers
	if (regex_match( ins.c_str(), basic_regex<char>("swp [abcABC], [abcABC]") )){
		char reg1, reg2;
		bool *r1, *r2, tmp;
		sscanf(ins.c_str(), "swp %c, %c", &reg1, &reg2);
		
		if (reg1 == 'a' || reg1 == 'A'){ r1 = &state.reg_a;}
		if (reg1 == 'b' || reg1 == 'B'){ r1 = &state.reg_b;}
		if (reg1 == 'c' || reg1 == 'C'){ r1 = &state.reg_c;}
		
		if (reg2 == 'a' || reg2 == 'A'){ r2 = &state.reg_a;}
		if (reg2 == 'b' || reg2 == 'B'){ r2 = &state.reg_b;}
		if (reg2 == 'c' || reg2 == 'C'){ r2 = &state.reg_c;}
		
		tmp = *r1;
		*r1 = *r2;
		*r2 = tmp;
		
		state.reg_ip++;
	}else
	//out, outs a
	if (regex_match( ins.c_str(), basic_regex<char>("out") )){
		if (state.output_count >= 512){
			std::cerr<<"output overflow\n";
			exit(-1);
		}
		state.output[state.output_count] = state.reg_a;
		state.output_count++;
		state.reg_ip++;
	}else
	//out memory, outs memory
	if (regex_match( ins.c_str(), basic_regex<char>("out \\([0-9]+\\)") )){
		int ind;
		sscanf(ins.c_str(), "out (%i)", &ind);
		if (state.output_count >= 512){
			std::cerr<<"output overflow\n";
			exit(-1);
		}
		if (ind >= 512){
			std::cerr<<"segmentation fault\n";
			exit(-1);
		}
		state.output[state.output_count] = state.memory[ind];
		state.output_count++;
		state.reg_ip++;
	}else
	//out memory range, outs memory range
	if (regex_match( ins.c_str(), basic_regex<char>("out \\([0-9]+\\-[0-9]+\\)") )){
		int ind1, ind2;
		sscanf(ins.c_str(), "out (%i-%i)", &ind1, &ind2);
		if (ind1 >= 512 || ind2 >= 512){
			std::cerr<<"segmentation fault\n";
			exit(-1);
		}
		if (ind1>ind2){
			std::cerr<<"upper index cannot be lower than the lower index (out memory range)\n";
			exit(-1);
		}
		for (int i=ind1; i<=ind2; i++){
			if (state.output_count >= 512){
				std::cerr<<"output overflow\n";
				exit(-1);
			}
			state.output[state.output_count] = state.memory[i];
			state.output_count++;
		}
		state.reg_ip++;
	}else
	//print, prints stored output
	if (regex_match( ins.c_str(), basic_regex<char>("print") )){
		unsigned long long n = 0;
		for (int i=0; i<state.output_count; i++){
			std::cout<<state.output[i];
			n <<= 1;
			n += state.output[i];
		}
		std::cout<<" = "<<n<<"\n";
		state.output_count = 0;
		state.reg_ip++;
	}else
	//halt
	if (regex_match( ins.c_str(), basic_regex<char>("halt") )){
		state.halted = true;
		state.reg_ip++;
	}else
	//in, reads input into reg a
	if (regex_match( ins.c_str(), basic_regex<char>("in") )){
		char c = 'p';
		do{
			std::cin>>c;
		}while (c!='0' && c!='1');
		state.reg_a = c=='1';
		std::cout<<"\n";
		state.reg_ip++;
	}else
	//in, reads input into memory
	if (regex_match( ins.c_str(), basic_regex<char>("in \\([0-9]+\\)") )){
		char c = 'p';
		int ind;
		sscanf(ins.c_str(), "in (%i)", &ind);
		
		if (ind >= 512){
			std::cerr<<"segmentation fault\n";
			exit(-1);
		}
		
		do{
			std::cin>>c;
		}while (c!='0' && c!='1');
		
		state.memory[ind] = c=='1';
		std::cout<<"\n";
		state.reg_ip++;
	}else
	//in, reads input into memory range
	if (regex_match( ins.c_str(), basic_regex<char>("in \\([0-9]+\\-[0-9]+\\)") )){
		char c = 'p';
		int ind1, ind2;
		sscanf(ins.c_str(), "in (%i-%i)", &ind1, &ind2);
		if (ind1>=512 || ind2>=512){
			std::cerr<<"segmentation fault\n";
			exit(-1);
		}
		if (ind1>ind2){
			std::cerr<<"upper memory range lower than lower memory range (out memory range)\n";
			exit(-1);
		}
		
		for (int i=ind1; i<=ind2; i++){
			do{
				std::cin>>c;
			}while (c!='0' && c!='1');
			state.memory[i] = c=='1';
			c='p';
		}
		std::cout<<"\n";
		state.reg_ip++;
	}else
	//call, pushes stack and jumps
	if (regex_match( ins.c_str(), basic_regex<char>("call [0-9]+") )){
		int dest;
		sscanf( ins.c_str(), "call %i", &dest);
		state.stack.push_back(state.reg_ip);
		state.reg_ip = dest;
	}else
	//ret, pops stack and jumps
	if (regex_match( ins.c_str(), basic_regex<char>("ret") )){
		int dest = state.stack.back();
		state.stack.pop_back();
		state.reg_ip = dest;
		state.reg_ip++;
	}else{
		std::cout<<"unknown - "<<ins<<"\n";
		state.reg_ip++;
	}
	
}

