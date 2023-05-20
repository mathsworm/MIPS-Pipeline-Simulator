/**
 * @file MIPS_Processor.hpp
 * @author Mallika Prabhakar and Sayam Sethi
 * 
 */

#ifndef __MIPS_PROCESSOR_HPP__
#define __MIPS_PROCESSOR_HPP__

#include <unordered_map>
#include <string>
#include <functional>
#include <vector>
#include <fstream>
#include <exception>
#include <iostream>
#include <queue>
#include <boost/tokenizer.hpp>

struct MIPS_Architecture
{
	int registers[32] = {0}, PCcurr = 0, PCnext = 0;
    int latch_reg[32] = {0};
	std::unordered_map<std::string, std::function<int(MIPS_Architecture &, std::string, std::string, std::string)>> instructions;
	std::unordered_map<std::string, int> registerMap, address;
	static const int MAX = (1 << 20);
	int data[MAX >> 2] = {0};
	std::vector<std::vector<std::string>> commands;
	std::vector<int> commandCount;

	std::unordered_map<int, int> memoryDelta;

	bool proceed = true;

    std::vector <int> removeLock;

	bool firstHalf = true;

	bool branchStall = false;
    int noOfStalls = 0;

	int lock[32] = {0};
	bool endPipeline = false;


    struct ID{
        std::vector<std::string> command;
		ID(){
			command = {"noOp", "$s1", "$s1", "$s1"};
		}
    };

    struct EX{
        std::vector<std::string> command;
		int s1_val;
		int s2_val;
		std::string label;
		int offset;

		EX(){
			command = {"noOp", "$s1", "$s1", "$s1"};
			s1_val = 0;
			s2_val = 0;
			label = "";
			offset = 0;
		}
    };

    struct MEM{
        std::vector<std::string> command;
        int computed_value;
		int s1_val;
		int s2_val;
		std::string label;
		int offset;
		MEM(){
			command = {"noOp", "$s1", "$s1", "$s1"};
			computed_value = 0;
			s1_val = 0;
			s2_val = 0;
			label = "";
			offset = 0;
		}
    };

    struct WB{
        std::vector<std::string> command;
        int memory_value;
		int computed_value;
		int s1_val;
		int s2_val;
		std::string label;
		int offset;
		WB(){
			command = {"noOp", "$s1", "$s1", "$s1"};
			memory_value = 0;
			computed_value = 0;
			s1_val = 0;
			s2_val = 0;
			label = "";
			offset = 0;
		}
    };

	// void removeFirst(std::vector<int> &v){
	// 	for (int i=0;i<v.size()-1;i++) v[i] = v[i+1];
	// 	if (!v.empty()) v.pop_back();
	// 	return;
	// }

	// bool isUnlocked(int i){
	// 	return lock[i].empty();
	// }

	// void reduceLock(){

	// 	for(int i=0;i<32;i++){
	// 		for(int j=0;j<lock[i].size();j++){
	// 			lock[i][j] -= 1;
	// 		}

	// 		while (!lock[i].empty()){
	// 			if(lock[i][0] != 0) break;
	// 			else removeFirst(lock[i]);
	// 		}
	// 	}
	// }

	// initialise all structs

	ID id;
	EX ex;
	MEM mem;
	WB wb;



	enum exit_code
	{
		SUCCESS = 0,
		INVALID_REGISTER,
		INVALID_LABEL,
		INVALID_ADDRESS,
		SYNTAX_ERROR,
		MEMORY_ERROR
	};

	// constructor to initialise the instruction set
	MIPS_Architecture(std::ifstream &file)
	{
		instructions = {{"add", &MIPS_Architecture::add}, {"sub", &MIPS_Architecture::sub}, {"mul", &MIPS_Architecture::mul}, {"beq", &MIPS_Architecture::beq}, {"bne", &MIPS_Architecture::bne}, {"slt", &MIPS_Architecture::slt}, {"j", &MIPS_Architecture::j}, {"lw", &MIPS_Architecture::lw}, {"sw", &MIPS_Architecture::sw}, {"addi", &MIPS_Architecture::addi}};


		for (int i = 0; i < 32; ++i)
			registerMap["$" + std::to_string(i)] = i;
		registerMap["$zero"] = 0;
		registerMap["$at"] = 1;
		registerMap["$v0"] = 2;
		registerMap["$v1"] = 3;
		for (int i = 0; i < 4; ++i)
			registerMap["$a" + std::to_string(i)] = i + 4;
		for (int i = 0; i < 8; ++i)
			registerMap["$t" + std::to_string(i)] = i + 8, registerMap["$s" + std::to_string(i)] = i + 16;
		registerMap["$t8"] = 24;
		registerMap["$t9"] = 25;
		registerMap["$k0"] = 26;
		registerMap["$k1"] = 27;
		registerMap["$gp"] = 28;
		registerMap["$sp"] = 29;
		registerMap["$s8"] = 30;
		registerMap["$ra"] = 31;

		constructCommands(file);
		commandCount.assign(commands.size(), 0);

	}







	// perform add operation
	int add(std::string r1, std::string r2, std::string r3)
	{
		return op(r1, r2, r3, [&](int a, int b)
				  { return a + b; });
	}

	// perform subtraction operation
	int sub(std::string r1, std::string r2, std::string r3)
	{
		return op(r1, r2, r3, [&](int a, int b)
				  { return a - b; });
	}

	// perform multiplication operation
	int mul(std::string r1, std::string r2, std::string r3)
	{
		return op(r1, r2, r3, [&](int a, int b)
				  { return a * b; });
	}

	// perform the binary operation
	int op(std::string r1, std::string r2, std::string r3, std::function<int(int, int)> operation)
	{
		if (!checkRegisters({r1, r2, r3}) || registerMap[r1] == 0)
			return 1;
		registers[registerMap[r1]] = operation(registers[registerMap[r2]], registers[registerMap[r3]]);
		PCnext = PCcurr + 1;
		return 0;
	}

	// perform the beq operation
	int beq(std::string r1, std::string r2, std::string label)
	{
		return bOP(r1, r2, label, [](int a, int b)
				   { return a == b; });
	}

	// perform the bne operation
	int bne(std::string r1, std::string r2, std::string label)
	{
		return bOP(r1, r2, label, [](int a, int b)
				   { return a != b; });
	}

	// implements beq and bne by taking the comparator
	int bOP(std::string r1, std::string r2, std::string label, std::function<bool(int, int)> comp)
	{
		if (!checkLabel(label))
			return 4;
		if (address.find(label) == address.end() || address[label] == -1)
			return 2;
		if (!checkRegisters({r1, r2}))
			return 1;
		PCnext = comp(registers[registerMap[r1]], registers[registerMap[r2]]) ? address[label] : PCcurr + 1;
		return 0;
	}

	// implements slt operation
	int slt(std::string r1, std::string r2, std::string r3)
	{
		if (!checkRegisters({r1, r2, r3}) || registerMap[r1] == 0)
			return 1;
		registers[registerMap[r1]] = registers[registerMap[r2]] < registers[registerMap[r3]];
		PCnext = PCcurr + 1;
		return 0;
	}

	// perform the jump operation
	int j(std::string label, std::string unused1 = "", std::string unused2 = "")
	{
		if (!checkLabel(label))
			return 4;
		if (address.find(label) == address.end() || address[label] == -1)
			return 2;
		PCnext = address[label];
		return 0;
	}

	// perform load word operation
	int lw(std::string r, std::string location, std::string unused1 = "")
	{
		if (!checkRegister(r) || registerMap[r] == 0)
			return 1;
		int address = locateAddress(location);
		if (address < 0)
			return abs(address);
		registers[registerMap[r]] = data[address];
		PCnext = PCcurr + 1;
		return 0;
	}

	// perform store word operation
	int sw(std::string r, std::string location, std::string unused1 = "")
	{
		if (!checkRegister(r))
			return 1;
		int address = locateAddress(location);
		if (address < 0)
			return abs(address);
		data[address] = registers[registerMap[r]];
		PCnext = PCcurr + 1;
		return 0;
	}

	int locateAddress(std::string location)
	{
		if (location.back() == ')')
		{
			try
			{
				int lparen = location.find('('), offset = stoi(lparen == 0 ? "0" : location.substr(0, lparen));
				std::string reg = location.substr(lparen + 1);
				reg.pop_back();
				if (!checkRegister(reg))
					return -3;
				int address = registers[registerMap[reg]] + offset;
				if (address % 4 || address < int(4 * commands.size()) || address >= MAX)
					return -3;
				return address / 4;
			}
			catch (std::exception &e)
			{
				return -4;
			}
		}
		try
		{
			int address = stoi(location);
			if (address % 4 || address < int(4 * commands.size()) || address >= MAX)
				return -3;
			return address / 4;
		}
		catch (std::exception &e)
		{
			return -4;
		}
	}

	// perform add immediate operation
	int addi(std::string r1, std::string r2, std::string num)
	{
		if (!checkRegisters({r1, r2}) || registerMap[r1] == 0)
			return 1;
		try
		{
			registers[registerMap[r1]] = registers[registerMap[r2]] + stoi(num);
			PCnext = PCcurr + 1;
			return 0;
		}
		catch (std::exception &e)
		{
			return 4;
		}
	}

	// checks if label is valid
	inline bool checkLabel(std::string str)
	{
		return str.size() > 0 && isalpha(str[0]) && all_of(++str.begin(), str.end(), [](char c)
														   { return (bool)isalnum(c); }) &&
			   instructions.find(str) == instructions.end();
	}

	// checks if the register is a valid one
	inline bool checkRegister(std::string r)
	{
		return registerMap.find(r) != registerMap.end();
	}

	// checks if all of the registers are valid or not
	bool checkRegisters(std::vector<std::string> regs)
	{
		return std::all_of(regs.begin(), regs.end(), [&](std::string r)
						   { return checkRegister(r); });
	}

	/*
		handle all exit codes:
		0: correct execution
		1: register provided is incorrect
		2: invalid label
		3: unaligned or invalid address
		4: syntax error
		5: commands exceed memory limit
	*/
	void handleExit(exit_code code, int cycleCount)
	{
		std::cout << '\n';
		switch (code)
		{
		case 1:
			std::cerr << "Invalid register provided or syntax error in providing register\n";
			break;
		case 2:
			std::cerr << "Label used not defined or defined too many times\n";
			break;
		case 3:
			std::cerr << "Unaligned or invalid memory address specified\n";
			break;
		case 4:
			std::cerr << "Syntax error encountered\n";
			break;
		case 5:
			std::cerr << "Memory limit exceeded\n";
			break;
		default:
			break;
		}
		if (code != 0)
		{
			std::cerr << "Error encountered at:\n";
			for (auto &s : commands[PCcurr])
				std::cerr << s << ' ';
			std::cerr << '\n';
		}
		std::cout << "\nFollowing are the non-zero data values:\n";
		for (int i = 0; i < MAX / 4; ++i)
			if (data[i] != 0)
				std::cout << 4 * i << '-' << 4 * i + 3 << std::hex << ": " << data[i] << '\n'
						  << std::dec;
		std::cout << "\nTotal number of cycles: " << cycleCount << '\n';
		std::cout << "Count of instructions executed:\n";
		for (int i = 0; i < (int)commands.size(); ++i)
		{
			std::cout << commandCount[i] << " times:\t";
			for (auto &s : commands[i])
				std::cout << s << ' ';
			std::cout << '\n';
		}
	}

	// parse the command assuming correctly formatted MIPS instruction (or label)
	void parseCommand(std::string line)
	{
		// strip until before the comment begins
		line = line.substr(0, line.find('#'));
		std::vector<std::string> command;
		boost::tokenizer<boost::char_separator<char>> tokens(line, boost::char_separator<char>(", \t"));
		for (auto &s : tokens)
			command.push_back(s);
		// empty line or a comment only line
		if (command.empty())
			return;
		else if (command.size() == 1)
		{
			std::string label = command[0].back() == ':' ? command[0].substr(0, command[0].size() - 1) : "?";
			if (address.find(label) == address.end())
				address[label] = commands.size();
			else
				address[label] = -1;
			command.clear();
		}
		else if (command[0].back() == ':')
		{
			std::string label = command[0].substr(0, command[0].size() - 1);
			if (address.find(label) == address.end())
				address[label] = commands.size();
			else
				address[label] = -1;
			command = std::vector<std::string>(command.begin() + 1, command.end());
		}
		else if (command[0].find(':') != std::string::npos)
		{
			int idx = command[0].find(':');
			std::string label = command[0].substr(0, idx);
			if (address.find(label) == address.end())
				address[label] = commands.size();
			else
				address[label] = -1;
			command[0] = command[0].substr(idx + 1);
		}
		else if (command[1][0] == ':')
		{
			if (address.find(command[0]) == address.end())
				address[command[0]] = commands.size();
			else
				address[command[0]] = -1;
			command[1] = command[1].substr(1);
			if (command[1] == "")
				command.erase(command.begin(), command.begin() + 2);
			else
				command.erase(command.begin(), command.begin() + 1);
		}
		if (command.empty())
			return;
		if (command.size() > 4)
			for (int i = 4; i < (int)command.size(); ++i)
				command[3] += " " + command[i];
		command.resize(4);
		commands.push_back(command);
	}



	// construct the commands vector from the input file
	void constructCommands(std::ifstream &file)
	{
		std::string line;
		while (getline(file, line))
			parseCommand(line);
		file.close();
	}



    // // pipeline stages
	// struct IF{
    // };
	
	// std::vector <std::string> instructionFetch(std::vector<std::vector<std::string>> commands){
	// 	return commands[PCcurr++];
	// }

    // struct ID{
    //     std::vector<std::string> command;
    // };

    // struct EX{
    //     std::vector<std::string> command;
	// 	std::unordered_map <std::string, std::string> m;
    // };

    // struct MEM{
    //     std::vector<std::string> command;
    //     int computed_value; // reg2 aur reg3 use ho rhe hai
    //     int reg_1 ;
    // };

    // struct WB{
    //     std::vector<std::string> command;
    //     int computed_value; 
    //     int memory_output;
    // };

	// int instructionDecode(std::vector<std::string> command){
	// 	std::unordered_map <std::string, std::string> m;

	// 	if (command[0] == "add"){
	// 		std::string r1 = command[1];
	// 		std::string r2 = command[2];
	// 		std::string r3 = command[3];
	// 		if (!checkRegisters({r1, r2, r3}) || registerMap[r1] == 0)
	// 			return 1;
	// 		m["s1_val"] = std::to_string(registers[registerMap[r2]]);
	// 		m["s2_val"] = std::to_string(registers[registerMap[r3]]);
	// 		m["offset"] = "unused";
	// 		m["label"] = "unused";


	// 	return 0;
	// 	}
	// }




	// std::unordered_map<std::string, std::string>

	void instructionFetch(){
		// std::cout<<"if is working,"<<" branch stall is "<<branchStall<<"\n";
		if (!firstHalf){
			PCcurr = PCnext;

			if (branchStall){
				id.command = {"noOp", "$s1", "$s1", "$s1"};

                noOfStalls -= 1;
				if (noOfStalls == 0) branchStall = false;
			}
			else if (PCcurr < commands.size()){
				id.command = commands[PCcurr];
				// if (id.command[0] == "beq" || id.command[0] == "bne" || id.command[0] == "j") 
				// branchStall = true;
			}
			else{
				id.command = {"end", "$s1", "$s1", "$s1"};
			}
		}
		// std::cout<<"next instruction is ";
		// for (auto x: id.command) std::cout<<x<<" "; std::cout<<"\n";
	}



	void instructionDecode(){
		// std::cout<<"id is working"<<"\n";
		// for (auto x:id.command) std::cout<<x<<" "; std::cout<<"\n";
		if (!firstHalf){

			if (id.command[0] == "noOp" || id.command[0] == "end"){
				ex.command = id.command;
				// PCnext = PCcurr + 1;
			}
			 
			// write lock condition properly

			// else if ((id.command[0] == "add" || id.command[0] == "sub" || id.command[0] == "mul" || id.command[0] == "slt") && (lock[registerMap[id.command[2]]] || lock[registerMap[id.command[3]]])){
			// 	ex.command = {"noOp", "$s1", "$s1", "$s1"};
			// 	proceed = false;
			// }


			// else if ((id.command[0] == "beq" || id.command[0] == "bne") && (lock[registerMap[id.command[1]]] || lock[registerMap[id.command[2]]])){
			// 	ex.command = {"noOp", "$s1", "$s1", "$s1"};
			// 	proceed = false;
			// }


			// else if ((id.command[0] == "addi") && (lock[registerMap[id.command[1]]])){
			// 	ex.command = {"noOp", "$s1", "$s1", "$s1"};
			// 	proceed = false;
			// }


			else if ((id.command[0] == "lw" || id.command[0] == "sw")){
				ex.command = id.command;
				// if (id.command[0] == "lw"){
				// 	lock[registerMap[id.command[1]]] = true;
				// }
				ex.s1_val = registers[registerMap[id.command[1]]];
				ex.s2_val = 0;
				ex.label = id.command[2];
				ex.offset = 0;
				PCnext = PCcurr + 1;


				std::string location = id.command[2];

				int lparen = location.find('('), offset = stoi(lparen == 0 ? "0" : location.substr(0, lparen));
				std::string reg = location.substr(lparen + 1);
				reg.pop_back();
				int address = registers[registerMap[reg]] + offset;
				address = address / 4;


				// std::cout<<reg<<"\n";

				// if (lock[registerMap[reg]]){
				// 	ex.command = {"noOp", "$s1", "$s1", "$s1"};
				// 	proceed = false;
				// }

				// else if (id.command[0] == "sw" && lock[registerMap[id.command[1]]]){
				// 	ex.command = {"noOp", "$s1", "$s1", "$s1"};
				// 	proceed = false;
				// }
			}

			else if (id.command[0] == "add" || id.command[0] == "sub" || id.command[0] == "mul" || id.command[0] == "slt"){
				ex.command = id.command;
				// lock[registerMap[id.command[1]]] = true;
				ex.s1_val = registers[registerMap[id.command[2]]];
				ex.s2_val = registers[registerMap[id.command[3]]];
				ex.label = "";
				ex.offset = 0;
				PCnext = PCcurr + 1;
			}

			else if (id.command[0] == "addi"){
				ex.command = id.command;
				// lock[registerMap[id.command[1]]] = true;
				ex.s1_val = registers[registerMap[id.command[2]]];
				ex.s2_val = stoi(id.command[3]);
				ex.label = "";
				ex.offset = 0;
				PCnext = PCcurr + 1;
				// std::cout<<ex.s1_val<<" "<<ex.s2_val;
			}

			// else if (id.command[0] == "lw"){
			// 	ex.command = id.command;
			// 	lock[registerMap[id.command[1]]] = true;
			// 	ex.s1_val = registers[registerMap[id.command[2]]];
			// 	ex.s2_val = 0;
			// 	ex.label = id.command[3];
			// 	ex.offset = 0;
			// 	PCnext = PCcurr + 1;
			// }

			// else if (id.command[0] == "sw"){
			// 	ex.command = id.command;
			// 	ex.s1_val = registers[registerMap[id.command[2]]];
			// 	ex.s2_val = 0;
			// 	ex.label = id.command[3];
			// 	ex.offset = 0;
			// 	PCnext = PCcurr + 1;
			// }


			else if (id.command[0] == "beq"){
				ex.command = id.command;
				ex.s1_val = latch_reg[registerMap[id.command[1]]];
				ex.s2_val = latch_reg[registerMap[id.command[2]]];
				ex.label = id.command[3];
				ex.offset = 0;
				// PCnext = (ex.s1_val == ex.s2_val) ? address[ex.label] : PCcurr + 1;
				branchStall = true;
                noOfStalls = 2;
			}

			else if (id.command[0] == "bne"){
				ex.command = id.command;
				ex.s1_val = latch_reg[registerMap[id.command[1]]];
				ex.s2_val = latch_reg[registerMap[id.command[2]]];
				ex.label = id.command[3];
				ex.offset = 0;
				// PCnext = (ex.s1_val != ex.s2_val) ? address[ex.label] : PCcurr + 1;
				branchStall = true;
                noOfStalls = 2;
			}

			else if (id.command[0] == "j"){
				ex.command = id.command;
				ex.s1_val = 0;
				ex.s2_val = 0;
				ex.label = id.command[1];
				ex.offset = 0;
				PCnext = address[id.command[1]];
				branchStall = true;
                noOfStalls = 1;
			}
		}
	}

	void execute(){
		// std::cout<<"ex is working"<<"\n";
		// for (auto x:ex.command) std::cout<<x<<" "; std::cout<<"\n";

		if (!firstHalf){
			mem.command = ex.command;
			mem.s1_val = ex.s1_val;
			mem.s2_val = ex.s2_val;
			mem.label = ex.label;
			mem.offset = ex.offset;

			// std::cout<<"this command is "<<ex.command[0]<<"\n";
			// std::cout<<(ex.command[0] == "addi")<<"\n";

			if (ex.command[0] == "noOp" || ex.command[0] == "end"){}

            // check for locks for the commands addi, add, sub, mul, slt

            else if ((ex.command[0] == "addi") && (lock[registerMap[ex.command[2]]] != 0)){
				mem.command = {"noOp", "$s1", "$s1", "$s1"};
				proceed = false;
			}

            else if ((ex.command[0] == "add" || ex.command[0] == "sub" || ex.command[0] == "mul" || ex.command[0] == "slt") && (lock[registerMap[ex.command[2]]] != 0 || lock[registerMap[ex.command[3]]] != 0)){
				mem.command = {"noOp", "$s1", "$s1", "$s1"};
				proceed = false;
			}

			else if ((ex.command[0] == "beq" || ex.command[0] == "bne") && (lock[registerMap[ex.command[1]]] != 0 || lock[registerMap[ex.command[2]]] != 0)){
				mem.command = {"noOp", "$s1", "$s1", "$s1"};
				proceed = false;
			}


            else if (ex.command[0] == "beq"){
				mem.command = ex.command;
				mem.s1_val = latch_reg[registerMap[ex.command[1]]];
				mem.s2_val = latch_reg[registerMap[ex.command[2]]];
				mem.label = ex.command[3];
				mem.offset = 0;
				PCnext = (mem.s1_val == mem.s2_val) ? address[mem.label] : PCcurr + 1;
				// branchStall = true;
                // noOfStalls = 2;
			}

			else if (ex.command[0] == "bne"){
				mem.command = ex.command;
				mem.s1_val = latch_reg[registerMap[ex.command[1]]];
				mem.s2_val = latch_reg[registerMap[ex.command[2]]];
				mem.label = ex.command[3];
				mem.offset = 0;
				PCnext = (mem.s1_val != mem.s2_val) ? address[mem.label] : PCcurr + 1;
				// branchStall = true;
                // noOfStalls = 2;
			}


            // std::string location = id.command[2];

			// 	int lparen = location.find('('), offset = stoi(lparen == 0 ? "0" : location.substr(0, lparen));
			// 	std::string reg = location.substr(lparen + 1);
			// 	reg.pop_back();
			// 	int address = registers[registerMap[reg]] + offset;
			// 	address = address / 4;


			// 	std::cout<<reg<<"\n";

				// if (lock[registerMap[reg]]){
				// 	ex.command = {"noOp", "$s1", "$s1", "$s1"};
				// 	proceed = false;
				// }

				// else if (id.command[0] == "sw" && lock[registerMap[id.command[1]]]){
				// 	ex.command = {"noOp", "$s1", "$s1", "$s1"};
				// 	proceed = false;
				// }

            else if (ex.command[0] == "lw" || ex.command[0] == "sw"){
                std::string location = ex.command[2];

                int lparen = location.find('('), offset = stoi(lparen == 0 ? "0" : location.substr(0, lparen));
                std::string reg = location.substr(lparen + 1);
                reg.pop_back();
                int address = latch_reg[registerMap[reg]] + offset;
                address = address / 4;

                // std::cout<<reg<<"\n";

                if (lock[registerMap[reg]] != 0){
					mem.command = {"noOp", "$s1", "$s1", "$s1"};
					proceed = false;
				}

                else{
                    mem.computed_value = address;
					// std::cout<<ex.label<<" "<<mem.computed_value<<"\n";
                    if (ex.command[0] == "lw"){
                        lock[registerMap[ex.command[1]]] ++;  // naya lock hai ye

                    }
                }

				// else if (id.command[0] == "sw" && lock[registerMap[id.command[1]]]){
				// 	ex.command = {"noOp", "$s1", "$s1", "$s1"};
				// 	proceed = false;     
            }


            // else if (ex.command[0] == "add" || ex.command[0] == "addi"){

			// 	mem.computed_value = ex.s1_val + ex.s2_val;
            //     lock[registerMap[ex.command[1]]] = true;  // naya lock hai ye
            //     removeLock.push_back(registerMap[ex.command[1]]);

			// 	std::cout<<"hellooo";
			// 	std::cout<<mem.computed_value;
			// }

            else if (ex.command[0] == "add"){

				mem.computed_value = latch_reg[registerMap[ex.command[3]]] + latch_reg[registerMap[ex.command[2]]];
                latch_reg[registerMap[ex.command[1]]] = mem.computed_value;

                lock[registerMap[ex.command[1]]] ++;  // naya lock hai ye

                // std::cout<<"the register is now locked"<<" "<<registerMap[ex.command[1]]<<"\n";

                removeLock.push_back(registerMap[ex.command[1]]);

				// std::cout<<"hellooo";
				// std::cout<<mem.computed_value;
			}

            else if (ex.command[0] == "addi"){

				mem.computed_value = ex.s2_val + latch_reg[registerMap[ex.command[2]]];
                latch_reg[registerMap[ex.command[1]]] = mem.computed_value;

                lock[registerMap[ex.command[1]]] ++;  // naya lock hai ye
                removeLock.push_back(registerMap[ex.command[1]]);

				// std::cout<<"hellooo";
				// std::cout<<mem.computed_value;
			}


			else if (ex.command[0] == "sub"){
                lock[registerMap[ex.command[1]]] ++;  // naya lock hai ye
                removeLock.push_back(registerMap[ex.command[1]]);
                mem.computed_value = latch_reg[registerMap[ex.command[2]]] - latch_reg[registerMap[ex.command[3]]];
                latch_reg[registerMap[ex.command[1]]] = mem.computed_value;
			}

			else if (ex.command[0] == "mul"){
                lock[registerMap[ex.command[1]]] ++;  // naya lock hai ye
                removeLock.push_back(registerMap[ex.command[1]]);

                // std::cout<<latch_reg[registerMap[ex.command[2]]]<<" "<<latch_reg[registerMap[ex.command[3]]]<<" "<<registerMap[ex.command[2]]<<" "<<ex.command[2]<<"\n";

				mem.computed_value = latch_reg[registerMap[ex.command[2]]] * latch_reg[registerMap[ex.command[3]]];
                latch_reg[registerMap[ex.command[1]]] = mem.computed_value;
			}

			else if (ex.command[0] == "slt"){
                lock[registerMap[ex.command[1]]] ++;  // naya lock hai ye
                removeLock.push_back(registerMap[ex.command[1]]);

				if(latch_reg[registerMap[ex.command[2]]] < latch_reg[registerMap[ex.command[3]]]) mem.computed_value = 1;
				else mem.computed_value = 0;

                latch_reg[registerMap[ex.command[1]]] = mem.computed_value;
			}
			// else if (ex.command[0] == "lw" || ex.command[0] == "sw"){
			// 	mem.computed_value = locateAddress(ex.label);
			// }
		}
	}

	void memory(){
		// std::cout<<"mem is working"<<"\n";
		// for (auto x:mem.command) std::cout<<x<<" "; std::cout<<"\n";


		if (firstHalf){
			if (mem.command[0] == "sw"){
                if (lock[registerMap[mem.command[1]]] != 0){
					wb.command = {"noOp", "$s1", "$s1", "$s1"};
					proceed = false;    
                }

                else{
                    // std::cout<<mem.computed_value<<"\n"<<mem.s1_val<<"\n";
                    data[mem.computed_value] = latch_reg[registerMap[mem.command[1]]];
					memoryDelta[mem.computed_value] = latch_reg[registerMap[mem.command[1]]];
                }
			}
                
		}


		if (!firstHalf){
			wb.command = mem.command;
			wb.s1_val = mem.s1_val;
			wb.s2_val = mem.s2_val;
			wb.label = mem.label;
			wb.offset = mem.offset;
			wb.computed_value = mem.computed_value;
			wb.memory_value = 0;
			if (mem.command[0] == "lw"){
				wb.memory_value = data[mem.computed_value];
                latch_reg[registerMap[mem.command[1]]] = wb.memory_value;

                // std::cout<<wb.memory_value<<" "<<mem.command[1]<<" "<<registerMap[mem.command[1]]<<" "<<latch_reg[registerMap[mem.command[1]]]<<"\n";
                removeLock.push_back(registerMap[mem.command[1]]);
			}
		}
	}

	void writeBack(){

		// std::cout<<"wb is working"<<"\n";
		// for (auto x:wb.command) std::cout<<x<<" "; std::cout<<"\n";
		if (firstHalf){
			if (wb.command[0] == "add" || wb.command[0] == "sub" || wb.command[0] == "mul" || wb.command[0] == "slt" || wb.command[0] == "addi"){
				// std::cout<<"bye "<<wb.computed_value<<"\n";

				registers[registerMap[wb.command[1]]] = wb.computed_value;
				// lock[registerMap[wb.command[1]]] = false;
			}

			else if (wb.command[0] == "lw"){
				// std::cout<<"wb is working fine "<<wb.computed_value<<" "<<wb.memory_value<<"\n";
				registers[registerMap[wb.command[1]]] = wb.memory_value;
				// lock[registerMap[wb.command[1]]] = false;
			}

			else if (wb.command[0] == "end"){
				endPipeline = true;
			}
		}
	}







	void executeCommandsPipelined()
	{
		if (commands.size() >= MAX / 4)
		{
			handleExit(MEMORY_ERROR, 0);
			return;
		}

		int clockCycles = 0;

		printRegisters(clockCycles);

		while(!endPipeline)
		{
			clockCycles++;


			// for (auto x:lock) std::cout<<x<<" "; std::cout<<"\n";


			firstHalf = true;
			writeBack();
			// if (!proceed){proceed = true; continue;}
			memory();
			// if (!proceed){proceed = true; continue;}
			execute();
			// if (!proceed){proceed = true; continue;}
			instructionDecode();
			// if (!proceed){proceed = true; continue;}
			instructionFetch();
			// if (!proceed){proceed = true; continue;}


			firstHalf = false;
			writeBack();
			if (!proceed){proceed = true; goto remove_locks;}
			memory();
			if (!proceed){proceed = true; goto remove_locks;}
			execute();
			if (!proceed){proceed = true; goto remove_locks;}
			instructionDecode();
			if (!proceed){proceed = true; goto remove_locks;}
			instructionFetch();
			if (!proceed){proceed = true; goto remove_locks;}


            remove_locks:
            for (auto x:removeLock){
                lock[x] --;
                // std::cout<<"unlocking register "<<x<<"\n";
            }
            while (!removeLock.empty()) removeLock.pop_back();

			// std::cout<<clockCycles<<"\n";
			// for (auto x:wb.command) std::cout<<x<<" "; std::cout<<"\n";
			printRegisters(clockCycles);

			if ((id.command[0] == "noOp" || id.command[0] == "end") && (ex.command[0] == "noOp" || ex.command[0] == "end") && (mem.command[0] == "noOp" || mem.command[0] == "end") && (wb.command[0] == "noOp" || wb.command[0] == "end")) break;

			

		}
		return;
	}




	// execute the commands sequentially (no pipelining)
	void executeCommandsUnpipelined()
	{
		if (commands.size() >= MAX / 4)
		{
			handleExit(MEMORY_ERROR, 0);
			return;
		}

		int clockCycles = 0;
		while (PCcurr < commands.size())
		{
			++clockCycles;
			std::vector<std::string> &command = commands[PCcurr];
			if (instructions.find(command[0]) == instructions.end())
			{
				handleExit(SYNTAX_ERROR, clockCycles);
				return;
			}




			exit_code ret = (exit_code) instructions[command[0]](*this, command[1], command[2], command[3]);




			if (ret != SUCCESS)
			{
				handleExit(ret, clockCycles);
				return;
			}
			++commandCount[PCcurr];
			PCcurr = PCnext;
			printRegisters(clockCycles);
		}
		handleExit(SUCCESS, clockCycles);
	}



	// print the register data in hexadecimal
	void printRegisters(int clockCycle)
	{
		for (int i = 0; i < 32; ++i)
			std::cout << registers[i] << ' ';
		std::cout << '\n';
		std::cout << memoryDelta.size() << ' ';
		if(memoryDelta.size()==0){
			std::cout<<'\n';
		}
		for (auto &p : memoryDelta)
		std::cout << p.first << ' ' << p.second << '\n';
		memoryDelta.clear();
	}
};

#endif