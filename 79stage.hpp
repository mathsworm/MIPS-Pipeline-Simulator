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
	std::unordered_map<std::string, std::function<int(MIPS_Architecture &, std::string, std::string, std::string)>> instructions;
	std::unordered_map<std::string, int> registerMap, address;
	static const int MAX = (1 << 20);
	int data[MAX >> 2] = {0};
	std::vector<std::vector<std::string>> commands;
	std::vector<int> commandCount;

	std::unordered_map<int, int> memoryDelta;


    int insNo = 1;
    int lastWrite = 0;

	// std::vector <int> removeLock;

	bool proceed = false;

	bool proceedRtype = false;
	bool proceedItype = false;

	bool firstHalf = false;

	bool branchStall = false;

	std::vector<int> removeLock;

	int noOfStalls = 0;

	int lock[32] = {0};
	bool endRtype = false;
	bool endItype = false;

    struct IF{
        std::vector<std::string> command;
        int insNo;
		IF(){
			command = {"noOp", "$s1", "$s1", "$s1"};
            insNo = 0;
		}
    };


    struct ID{
        std::vector<std::string> command;
        int insNo;
		ID(){
			command = {"noOp", "$s1", "$s1", "$s1"};
            insNo = 0;
		}
    };

    struct RR{
        std::vector<std::string> command;
        int insNo;
		RR(){
        	command = {"noOp", "$s1", "$s1", "$s1"};
            insNo = 0;
		}
    };

    struct EX{
        std::vector<std::string> command;
		int s1_val;
		int s2_val;
		std::string label;
		int offset;
        int insNo;

		EX(){
			command = {"noOp", "$s1", "$s1", "$s1"};
			s1_val = 0;
			s2_val = 0;
			label = "";
			offset = 0;
            insNo = 0;
		}
    };

    struct MEM{
        std::vector<std::string> command;
        int computed_value;
		int s1_val;
		int s2_val;
		std::string label;
		int offset;
        int insNo;
		MEM(){
			command = {"noOp", "$s1", "$s1", "$s1"};
			computed_value = 0;
			s1_val = 0;
			s2_val = 0;
			label = "";
			offset = 0;
            insNo = 0;
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
        int insNo;
		WB(){
			command = {"noOp", "$s1", "$s1", "$s1"};
			memory_value = 0;
			computed_value = 0;
			s1_val = 0;
			s2_val = 0;
			label = "";
			offset = 0;
            insNo = 0;
		}
    };


	// initialise all structs

    IF if1;
    IF if2;
	ID id1;
    ID id2;
    RR rr;
	EX exItype;
    EX exRtype;
	MEM mem1;
    MEM mem2;
	WB wbItype;
	WB wbRtype;


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



    void instructionFetch1(){
		if (!firstHalf){
			PCcurr = PCnext; // last me ya shuru me pcnext ke related changes
			// std::cout<<PCcurr;
			if (branchStall){
				if2.command = {"noOp", "$s1", "$s1", "$s1"};
				if2.insNo = -10;
				noOfStalls -= 1;

				if (noOfStalls == 0) branchStall = false;
			}
			else if (PCcurr < commands.size()){
				// std::cout<<"fetching instruction"<<" "<<PCcurr;
				if2.command = commands[PCcurr];
				if2.insNo = insNo;
				if (if2.command[0] == "beq" || if2.command[0] == "bne" || if2.command[0] == "j" || if2.command[0] == "sw"){if2.insNo = -10;}
				else{
					insNo += 1;
				}
				
				// if (id.command[0] == "beq" || id.command[0] == "bne" || id.command[0] == "j") 
				// branchStall = true;
			}
			else{
				if2.command = {"end", "$s1", "$s1", "$s1"};
				if2.insNo = -10;
			}

			// std::cout<<"the next instruction is \n";

			// for (auto x: if2.command) std::cout<<x<<" "; std::cout<<"\n";
		}
    }


    void instructionFetch2(){
		if (!firstHalf){
			id1.command = if2.command;
			id1.insNo = if2.insNo;

			if (if2.command[0] == "beq" || if2.command[0] == "bne"){
				branchStall = true;
				noOfStalls = 5;
			}
			else if (if2.command[0] == "j"){
				branchStall = true;
				noOfStalls = 3;
			}	
			else if (if2.command[0] == "addi" || if2.command[0] == "add" || if2.command[0] == "sub" || if2.command[0] == "mul" || if2.command[0] == "slt" || if2.command[0] == "lw" || if2.command[0] == "sw"){
				// std::cout<<"pc increments at point 1 \n";
				PCnext = PCcurr + 1;
			}
		}
    }


    void instructionDecode1(){
		if (!firstHalf){
			id2.command = id1.command;
			id2.insNo = id1.insNo;
		}
    }


	void instructionDecode2(){
		if (!firstHalf){
			rr.command = id2.command;
			rr.insNo = id2.insNo;


			if (id2.command[0] == "j"){
				// exRtype.command = rr.command;
				// exRtype.s1_val = 0;
				// exRtype.s2_val = 0;
				// exRtype.label = rr.command[1];
				// exRtype.offset = 0;
				// exRtype.insNo = rr.insNo;
				PCnext = address[id2.command[1]];

				// proceed = true;
				// branchStall = true;
				// noOfStalls = 3;
			}

		}
	}


	void registerReadRtype(){
		// std::cout<<"rr rtype is working"<<"\n";
		// for (auto x:rr.command) std::cout<<x<<" "; std::cout<<"\n";
		// std::cout<<"rr.insNo "<<rr.insNo<<"\n";
		if (!firstHalf){

			// if (rr.command[0] == "noOp" || rr.command[0] == "end"){
			// 	// if(noOfStalls != 1) 
			// 	ex.command = rr.command;
			// 	// PCnext = PCcurr + 1; // pcnext ke related changes
			// }
			 
			// write lock condition properly
			if ((rr.command[0] == "add" || rr.command[0] == "sub" || rr.command[0] == "mul" || rr.command[0] == "slt") && (lock[registerMap[rr.command[2]]] != 0 || lock[registerMap[rr.command[3]]] != 0)){
				exRtype.command = {"noOp", "$s1", "$s1", "$s1"};
				exRtype.insNo = -10;
				proceed = false;
			}

			else if ((rr.command[0] == "beq" || rr.command[0] == "bne") && (lock[registerMap[rr.command[1]]] != 0 || lock[registerMap[rr.command[2]]] != 0)){
				exRtype.command = {"noOp", "$s1", "$s1", "$s1"};
				exRtype.insNo = -10;
				proceed = false;
			}

			else if ((rr.command[0] == "addi") && (lock[registerMap[rr.command[2]]] != 0)){
				exRtype.command = {"noOp", "$s1", "$s1", "$s1"};
				exRtype.insNo = -10;
				proceed = false;
				// std::cout<<"addi statement but the register is locked \n";
			}

			else if (rr.command[0] == "add" || rr.command[0] == "sub" || rr.command[0] == "mul" || rr.command[0] == "slt"){
				exRtype.command = rr.command;
				lock[registerMap[rr.command[1]]] ++;
				exRtype.s1_val = registers[registerMap[rr.command[2]]];
				exRtype.s2_val = registers[registerMap[rr.command[3]]];
				exRtype.label = "";
				exRtype.offset = 0;
				exRtype.insNo = rr.insNo;
				// PCnext = PCcurr + 1;
				proceed = true;
			}

			else if (rr.command[0] == "addi"){
				exRtype.command = rr.command;
				lock[registerMap[rr.command[1]]] ++;
				exRtype.s1_val = registers[registerMap[rr.command[2]]];
				exRtype.s2_val = stoi(rr.command[3]);
				exRtype.label = "";
				exRtype.offset = 0;
				exRtype.insNo = rr.insNo;
				// PCnext = PCcurr + 1;
				proceed = true;
				// std::cout<<ex.s1_val<<" "<<ex.s2_val;
			}

			else if (rr.command[0] == "noOp"){
				exRtype.command = rr.command;
				exRtype.insNo = rr.insNo;
				proceed = true;
				// lock[registerMap[rr.command[1]]] = true;
				// exRtype.s1_val = registers[registerMap[rr.command[2]]];
				// exRtype.s2_val = stoi(rr.command[3]);
				// exRtype.label = "";
				// exRtype.offset = 0;
				// exRtype.insNo = rr.insNo;
				// PCnext = PCcurr + 1;
				// std::cout<<ex.s1_val<<" "<<ex.s2_val;
			}

			else if (rr.command[0] == "end"){
				exRtype.command = rr.command;
				exRtype.insNo = rr.insNo;
				proceed = true;
				// lock[registerMap[rr.command[1]]] = true;
				// exRtype.s1_val = registers[registerMap[rr.command[2]]];
				// exRtype.s2_val = stoi(rr.command[3]);
				// exRtype.label = "";
				// exRtype.offset = 0;
				// exRtype.insNo = rr.insNo;
				// PCnext = PCcurr + 1;
				// std::cout<<ex.s1_val<<" "<<ex.s2_val;
			}


			else if (rr.command[0] == "beq"){
				exRtype.command = rr.command;
				exRtype.s1_val = registers[registerMap[rr.command[1]]];
				exRtype.s2_val = registers[registerMap[rr.command[2]]];
				exRtype.label = rr.command[3];
				exRtype.offset = 0;
				exRtype.insNo = rr.insNo;
				// PCnext = (exRtype.s1_val == exRtype.s2_val) ? address[exRtype.label] : PCcurr + 1;
				proceed = true;
				// branchStall = true;
				// noOfStalls = 5;
			}

			else if (rr.command[0] == "bne"){
				exRtype.command = rr.command;
				exRtype.s1_val = registers[registerMap[rr.command[1]]];
				exRtype.s2_val = registers[registerMap[rr.command[2]]];
				exRtype.label = rr.command[3];
				exRtype.offset = 0;
				exRtype.insNo = rr.insNo;
				// PCnext = (exRtype.s1_val != exRtype.s2_val) ? address[exRtype.label] : PCcurr + 1;
				proceed = true;
				// branchStall = true;
				// noOfStalls = 5;
			}

				else if (rr.command[0] == "j"){
					exRtype.command = rr.command;
					exRtype.s1_val = 0;
					exRtype.s2_val = 0;
					exRtype.label = rr.command[1];
					exRtype.offset = 0;
					exRtype.insNo = rr.insNo;
					// PCnext = address[rr.command[1]];
					proceed = true;
					// branchStall = true;
					// noOfStalls = 3;
				}

		}
	}

	void registerReadItype(){
		// std::cout<<"rritype is working"<<"\n";
		// for (auto x:rr.command) std::cout<<x<<" "; std::cout<<"\n";
		if (!firstHalf){


			if (rr.command[0] == "noOp" || rr.command[0] == "end"){
				exItype.command = rr.command;
				exItype.insNo = rr.insNo;
				proceed = true;
			}



			if ((rr.command[0] == "lw" || rr.command[0] == "sw")){
				std::string location = rr.command[2];

				int lparen = location.find('('), offset = stoi(lparen == 0 ? "0" : location.substr(0, lparen));
				std::string reg = location.substr(lparen + 1);
				reg.pop_back();
				int address = registers[registerMap[reg]] + offset;
				address = address / 4;


				if (lock[registerMap[reg]] != 0){
					exItype.command = {"noOp", "$s1", "$s1", "$s1"};
					exItype.insNo = -10;
					proceed = false;
					return;
				}

				else if (rr.command[0] == "sw" && lock[registerMap[rr.command[1]]] != 0){
					exItype.command = {"noOp", "$s1", "$s1", "$s1"};
					exItype.insNo = -10;
					proceed = false;
					return;
				}


				exItype.command = rr.command;
				if (rr.command[0] == "lw"){
					lock[registerMap[rr.command[1]]] ++;
				}
				exItype.s1_val = registers[registerMap[rr.command[1]]];
				exItype.s2_val = 0;
				exItype.label = rr.command[2];
				exItype.offset = 0;
				exItype.insNo = rr.insNo;
				// std::cout<<"pc increments at point 2 \n";
				// PCnext = PCcurr + 1;
				proceed = true;
			}

		}
	}





	void executeRtype(){
		// std::cout<<"ex rtype is working"<<"\n";
		// for (auto x:exRtype.command) std::cout<<x<<" "; std::cout<<"\n";
		// std::cout<<"ex rtype.insNo "<<exRtype.insNo<<"\n";

		if (!firstHalf){
			wbRtype.command = exRtype.command;
			wbRtype.s1_val = exRtype.s1_val;
			wbRtype.s2_val = exRtype.s2_val;
			wbRtype.label = exRtype.label;
			wbRtype.offset = exRtype.offset;
			wbRtype.insNo = exRtype.insNo;

			// std::cout<<"this command is "<<ex.command[0]<<"\n";
			// std::cout<<(ex.command[0] == "addi")<<"\n";

			if (exRtype.command[0] == "noOp" || exRtype.command[0] == "end"){}

			else if (exRtype.command[0] == "add" || exRtype.command[0] == "addi"){
				wbRtype.computed_value = exRtype.s1_val + exRtype.s2_val;
				
				// std::cout<<"hellooo";
				// std::cout<<wbItype.computed_value;
			}
			else if (exRtype.command[0] == "sub"){
				wbRtype.computed_value = exRtype.s1_val - exRtype.s2_val;
			}
			else if (exRtype.command[0] == "mul"){
				wbRtype.computed_value = exRtype.s1_val * exRtype.s2_val;
			}
			else if (exRtype.command[0] == "slt"){
				if(exRtype.s1_val < exRtype.s2_val) wbRtype.computed_value = 1;
				else wbRtype.computed_value = 0;
			}


			else if (exRtype.command[0] == "beq"){
				// exRtype.command = rr.command;
				// exRtype.s1_val = registers[registerMap[rr.command[1]]];
				// exRtype.s2_val = registers[registerMap[rr.command[2]]];
				// exRtype.label = rr.command[3];
				// exRtype.offset = 0;
				// exRtype.insNo = rr.insNo;
				PCnext = (exRtype.s1_val == exRtype.s2_val) ? address[exRtype.label] : PCcurr + 1;
				// proceed = true;
				// branchStall = true;
				// noOfStalls = 5;
			}

			else if (exRtype.command[0] == "bne"){
				// exRtype.command = rr.command;
				// exRtype.s1_val = registers[registerMap[rr.command[1]]];
				// exRtype.s2_val = registers[registerMap[rr.command[2]]];
				// exRtype.label = rr.command[3];
				// exRtype.offset = 0;
				// exRtype.insNo = rr.insNo;
				PCnext = (exRtype.s1_val != exRtype.s2_val) ? address[exRtype.label] : PCcurr + 1;
				// std::cout<<"inside bne "<<exRtype.s1_val<<" "<<exRtype.s2_val<<" "<<"\n";
				// std::cout<<"the next address is "<<PCnext<<" "<<PCcurr;
				// proceed = true;
				// branchStall = true;
				// noOfStalls = 5;
			}


			
			exRtype.command = {"noOp", "$s1", "$s1", "$s1"};
			exRtype.insNo = -10;
			

			
			// else if (exRtype.command[0] == "lw" || exRtype.command[0] == "sw"){
			// 	wbRtype.computed_value = locateAddress(exRtype.label);
			// }
		}
	}


	void executeItype(){

		// std::cout<<"ex itype is working"<<"\n";
		// for (auto x:exItype.command) std::cout<<x<<" "; std::cout<<"\n";
		// std::cout<<"ex itype.insNo "<<exItype.insNo<<"\n";
		if (!firstHalf){
			mem1.command = exItype.command;
			mem1.s1_val = exItype.s1_val;
			mem1.s2_val = exItype.s2_val;
			mem1.label = exItype.label;
			mem1.offset = exItype.offset;
			mem1.insNo = exItype.insNo;


			if (exItype.command[0] == "lw" || exItype.command[0] == "sw"){
				mem1.computed_value = locateAddress(exItype.label);
			}

			exItype.command = {"noOp", "$s1", "$s1", "$s1"};
			exItype.insNo = -10;
		}
	}


	// void memory(){
	// 	std::cout<<"mem is working"<<"\n";
	// 	for (auto x:mem.command) std::cout<<x<<" "; std::cout<<"\n";

	// 	if (firstHalf){
	// 		if (mem.command[0] == "sw"){
	// 			std::cout<<mem.computed_value<<"\n"<<mem.s1_val<<"\n";
	// 			data[mem.computed_value] = mem.s1_val;
	// 		}
	// 	}

	// 	if (!firstHalf){
	// 		wb.command = mem.command;
	// 		wb.s1_val = mem.s1_val;
	// 		wb.s2_val = mem.s2_val;
	// 		wb.label = mem.label;
	// 		wb.offset = mem.offset;
	// 		wb.computed_value = mem.computed_value;
	// 		wb.memory_value = 0;
	// 		if (mem.command[0] == "lw"){
	// 			wb.memory_value = data[mem.computed_value];
	// 		}
	// 	}
	// }

	void memory1(){

			// std::cout<<"mem1 is working"<<"\n";
			// for (auto x:mem1.command) std::cout<<x<<" "; std::cout<<"\n";
			// std::cout<<"mem1.insNo "<<mem1.insNo<<"\n";

			mem2.command = mem1.command;
			mem2.s1_val = mem1.s1_val;
			mem2.s2_val = mem1.s2_val;
			mem2.label = mem1.label;
			mem2.offset = mem1.offset;		
			mem2.computed_value = mem1.computed_value;
			mem2.insNo = mem1.insNo;
	}

	void memory2(){

			// std::cout<<"mem2 is working"<<"\n";
			// for (auto x:mem2.command) std::cout<<x<<" "; std::cout<<"\n";
			// std::cout<<"mem2.insNo "<<mem2.insNo<<"\n";

		// std::cout<<"mem is working"<<"\n";
		// 	for (auto x:mem.command) std::cout<<x<<" "; std::cout<<"\n";

			if (!firstHalf){
				if (mem2.command[0] == "sw"){
					// std::cout<<mem2.computed_value<<"\n"<<mem.s1_val<<"\n";
					data[mem2.computed_value] = mem2.s1_val;
					memoryDelta[mem2.computed_value] = mem2.s1_val;
				}
			}

			if (!firstHalf){
				wbItype.command = mem2.command;
				wbItype.s1_val = mem2.s1_val;
				wbItype.s2_val = mem2.s2_val;
				wbItype.label = mem2.label;
				wbItype.offset = mem2.offset;
				wbItype.computed_value = mem2.computed_value;
				wbItype.memory_value = 0;
				wbItype.insNo = mem2.insNo;
				if (mem2.command[0] == "lw"){
					wbItype.memory_value = data[mem2.computed_value];
				}
			}
	}


	void writeBack(){
		// std::cout<<"wb itype is working"<<"\n";
		// for (auto x:wbItype.command) std::cout<<x<<" "; std::cout<<"\n";
		// std::cout<<"wb itype.insNo "<<wbItype.insNo<<"\n";

		// std::cout<<"wb rtype is working"<<"\n";
		// for (auto x:wbRtype.command) std::cout<<x<<" "; std::cout<<"\n";
		// std::cout<<"wb rtype.insNo "<<wbRtype.insNo<<"\n";
		if (!firstHalf){
			// if (wb.command[0] == "add" || wb.command[0] == "sub" || wb.command[0] == "mul" || wb.command[0] == "slt" || wb.command[0] == "addi"){
			// 	std::cout<<"bye "<<wb.computed_value<<"\n";

			// 	registers[registerMap[wb.command[1]]] = wb.computed_value;
			// 	lock[registerMap[wb.command[1]]] = false;
			// }

			// else if (wb.command[0] == "lw"){
			// 	std::cout<<"wb is working fine "<<wb.computed_value<<" "<<wb.memory_value<<"\n";
			// 	registers[registerMap[wb.command[1]]] = wb.memory_value;
			// 	lock[registerMap[wb.command[1]]] = false;
			// }

			// else if (wb.command[0] == "end"){
			// 	endPipeline = true;
			// }

			if (wbItype.command[0] == "end"){
				proceedItype = true;
				endItype = true;
			}

			else if (wbItype.command[0] == "noOp"){
				proceedItype = true;
			}

			else if (wbItype.command[0] == "sw"){
				proceedItype = true;
			}


			if (wbRtype.command[0] == "end"){
				proceedRtype = true;
				endRtype = true;
			}

			else if (wbRtype.command[0] == "noOp"){
				proceedRtype = true;
			}

			


			// std::cout<<"r type insNo "<< wbRtype.insNo<<"\n";
			// std::cout<<"lastWrite : "<< lastWrite<<"\n";
			// std::cout<<"i type insNo "<< wbItype.insNo<<"\n";
			// std::cout<<"r type insNo "<< wbRtype.insNo<<"\n";

			if (wbRtype.command[0] == "j" || wbRtype.command[0] == "beq" || wbRtype.command[0] == "bne"){
				proceedRtype = true;
				// std::cout<<"writeback me if else 1\n";
			}

			else if (wbRtype.command[0] != "noOp" && wbRtype.command[0] != "end" && wbRtype.insNo == lastWrite + 1){
				proceedRtype = true;
				registers[registerMap[wbRtype.command[1]]] = wbRtype.computed_value;
				removeLock.push_back(registerMap[wbRtype.command[1]]);
				lastWrite ++;

				// std::cout<<"writeback me if else 2\n";
				return;  // return is important here
			}


			else if (wbItype.command[0] != "noOp" && wbItype.command[0] != "end" && wbItype.insNo == lastWrite + 1){
				proceedItype = true;
				if (wbItype.command[0] == "lw"){
					removeLock.push_back(registerMap[wbItype.command[1]]);
				}
				registers[registerMap[wbItype.command[1]]] = wbItype.memory_value;
				lastWrite ++;
				// std::cout<<"writeback me if else 3\n";
				return;
			}
			

			// std::cout<<"proceedItype "<<proceedItype<<"\n";
			// std::cout<<"proceedRtype "<<proceedRtype<<"\n";
			// std::cout<<"writeback me if else ke bahar \n";

			// if {wbItype.insNo == lastWrite + 1 || wbRtype.insNo == lastwrite + 1} lastwrite++;

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

		while(!endRtype || !endItype){
			clockCycles++;

			// std::cout<<"           the current pc is"<<PCcurr<<"\n";

			proceedItype = false;
			proceedRtype = false;
			proceed = false;

			writeBack();

			if (proceedRtype){
				executeRtype();
				registerReadRtype();
			}

			if (proceedItype){
				memory2();
				memory1();
				executeItype();
				registerReadItype();
			}

			// if (proceedItype || proceedRtype){
			// 	registerReadItype();
			// 	registerReadRtype();
			// }

			if (proceed){
				instructionDecode2();
				instructionDecode1();
				instructionFetch2();
				instructionFetch1();
				// std::cout<<"hello";
			}

			for (auto x : removeLock) lock[x] --;
			while (!removeLock.empty()) removeLock.pop_back();

			printRegisters(clockCycles);

			if ((if2.command[0] == "end" || if2.command[0] == "noOp") && (id1.command[0] == "end" || id1.command[0] == "noOp") && (id2.command[0] == "end" || id2.command[0] == "noOp") && (rr.command[0] == "end" || rr.command[0] == "noOp") && (exItype.command[0] == "end" || exItype.command[0] == "noOp") && (wbItype.command[0] == "end" || wbItype.command[0] == "noOp") && (exRtype.command[0] == "end" || exRtype.command[0] == "noOp") && (mem1.command[0] == "end" || mem1.command[0] == "noOp") && (mem2.command[0] == "end" || mem2.command[0] == "noOp") && (wbRtype.command[0] == "end" || wbRtype.command[0] == "noOp")) break;

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