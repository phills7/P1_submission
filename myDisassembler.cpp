#include <iostream>
#include <fstream>
#include <string>
#include <vector>

//define some masks
const int OPMASK = 63<<26;
const int FUNCMASK = 63;
const int RDMASK = 31 << 11;
const int RTMASK = 31 << 16;
const int RSMASK = 31 << 21;
const int SHIFTMASK = 31 << 6;
const int IMMMASK = 65535;

//vector to lookup register names
const std::string rNames[]= {"$zero", "$at", "$v0", "$v1", "$a0", "$a1", "$a2", "$a3",
						"$t0", "$t1", "$t2", "$t3", "$t4", "$t5", "$t6", "$t7",
						"$s0", "$s1", "$s2", "$s3", "$s4", "$s5", "$s6", "$s7",
						"$t8", "$t9", "$k0", "$k1", "$gp", "$sp", "$fp", "$ra"};				


//a helper function to add leading 0s to labels
std::string padd( int num )
{
	if(num<10)
		return "000" + std::to_string(num);
	else if(num<100)
		return "00" + std::to_string(num);
	else if(num<1000)
		return "0" + std::to_string(num);
	else
		return std::to_string(num);
	return "";
}

//takes 6 bit function returns instruction						
std::string funcLookup( int func )
{
	std::string str;
	const int funcCodes[] = 		{0x20 , 0x21 , 0x24, 0x08, 0x27, 0x25, 0x2a, 0x2b ,0x00 , 0x02, 0x22, 0x23};
	const std::string funcNames[] = {"add","addu","and","jr" ,"nor","or" ,"slt","sltu","sll","srl","sub","subu"}; 
	for(int i=0 ; i<12 ; i++)
	{
		if (funcCodes[i] == func)
			str = funcNames[i];
	}
	return str;
}

//takes 6 bit opcode returns instruction
std::string opLookup( int op )
{
	std::string str;
	const int opCodes[] = 			{0x8   ,0x9    ,0xc   ,0x4  ,0x5  ,0x2,0x3  ,0x24 ,0x25 ,0x30,0xf  ,0x23,0xd  ,0xa   ,0xb    ,0x28,0x38,0x29,0x2b};
	const std::string opNames[] = 	{"addi","addiu","andi","beq","bne","j","jal","lbu","lhu","ll","lui","lw","ori","slti","sltiu","sb","sc","sh","sw"}; 
	for(int i=0 ; i<19 ; i++)
	{
		if (opCodes[i] == op)
			str = opNames[i];
	}
	return str;
}
		
//takes 32 bit instruction adds it to instruction vector and adds labels to label vector		
bool addInstruction( uint32_t instruction, int address, std::vector<std::string> &instructions, std::vector<int> &labels)
{
	std::string newInstruction;
	
	//find rd rs rt names and shift amount and immediate values 
	//  ** could be more efficient but this was easier and not significantly slower to calculate all of them
	std::string rd = rNames[(instruction & RDMASK) >> 11];
	std::string rs = rNames[(instruction & RSMASK) >> 21];
	std::string rt = rNames[(instruction & RTMASK) >> 16]; 
	std::string shift = std::to_string((instruction & SHIFTMASK) >> 6);
	int16_t s_imm = IMMMASK & instruction;
	uint16_t u_imm = IMMMASK & instruction;
	
	if((instruction & OPMASK) >> 26 == 0)  //R-type
	{
		//std::cout<<"R" <<std::endl;		
		newInstruction = '\t' + funcLookup(instruction & FUNCMASK) + '\t';  //lookup the function
		if(newInstruction == "\t\t") //if not valid return false
			return false;
		else if(newInstruction == "\tjr\t") //if jr
		{
			newInstruction = newInstruction + rs;
		}	
		else if(newInstruction == "\tsll\t" || newInstruction == "\tsrl\t") //if shift
		{
			newInstruction = newInstruction + rd + ", " + rt + ", " + shift;
		}
		else //all other r types
		{
			newInstruction = newInstruction + rd + ", " + rs + ", " + rt;
		}
	}
	else if(((instruction & OPMASK)>>26 == 2) || ((instruction & OPMASK) == 3))  //J-type
	{
		//this is a stub **** not complete
		//std::cout<<"J" <<std::endl;
		if(newInstruction == "")
			return false;
		newInstruction = '\t' + opLookup((instruction & OPMASK)>>26) + '\t';
		
	}
	else  //I-type
	{
		//std::cout<<"I" <<std::endl;
		newInstruction = '\t' + opLookup((instruction & OPMASK)>>26) + '\t'; //lookup opcode to find instruction
		if(newInstruction == "\t\t") //if invalid return false
			return false;
		else if(newInstruction == "\tbne\t" || newInstruction == "\tbeq\t") //branch
		{
			int label = (address + 4) + (s_imm * 4); //calculate label
			
			newInstruction = newInstruction + rs + ", " + rt + ", " + "Addr_" + padd(label); //add label to instuction
			labels.push_back(label); //store address to labels vector
		}
		else if(newInstruction == "\tlbu\t" || newInstruction == "\tlhu\t" || newInstruction == "\tll\t" || newInstruction == "\tlw\t"   //loads
				|| newInstruction == "\tsb\t" || newInstruction == "\tsc\t" || newInstruction == "\tsh\t" || newInstruction == "\tsw\t") //stores 
		{
			newInstruction = newInstruction + rt + ", " + std::to_string(u_imm) + '(' + rs + ')';
		}
		else if(newInstruction == "\tlui\t") //if lui
		{
			newInstruction = newInstruction + rt + ", " + std::to_string(u_imm);
		}
		else //all other immediate types 
		{
			if(newInstruction[newInstruction.length()-2] == 'u') //unsigned imm
				newInstruction = newInstruction + rt + ", " + rs + ", " + std::to_string(u_imm);
			else //signed imm
				newInstruction = newInstruction + rt + ", " + rs + ", " + std::to_string(s_imm);
		}

	}
	//std::cout<< newInstruction<<std::endl;
	instructions.push_back(newInstruction);
}	

//convert hex to 32-bit int 
uint32_t asInt(std::string str, bool &valid)
{
	
	if(str.length() != 8)
	{
		valid = false;
	}
	uint32_t returnVal = 0;
	for( int i=0 ; i<8 ; i++)
	{
		if(str[i] >= '0' && str[i] <= '9') //if number
			returnVal = returnVal | ((str[i] - '0') << (4*(7-i)));
		else //if letter
			returnVal = returnVal | ((str[i] - 'a' + 10) << (4*(7-i)));
	}
	//std::cout << returnVal << std::endl;
	return returnVal;
}

//takes input stream and outputs parsed instructions to filename
void parse( std::ifstream &in , std::string fileName)
{
	uint32_t instruction;
	std::vector<std::string> instructions; // a vector to store instructions
	std::vector<int> labels; // a vector to store which instructions need labels
	bool fileValid = true; //if this varibale chnages to false exit without ouputing anything
	
	//keep track of address of instructions so we can store the label
	int address = 0;
	std::string curI;
	while( in >> curI )  //start reading in file and continue until EOF
	{
		bool valid = true;
		instruction = asInt(curI , valid); //get instruction from hex to int
		if(!valid) //if not a 32 bit instruction return and give error 
		{
			fileValid = false;
			std::cerr << "Cannot disassemble " << curI << " at line " << (address+4)/4 << std::endl;
		}
		//convert 32 bit instruction to string and add it to the vector of parsed instructions
		valid = addInstruction( instruction, address, instructions, labels); 
		if(!valid) //if not valid 32 bit instruction return and give error 
		{
			fileValid = false;
			std::cerr << "Cannot disassemble " << curI << " at line " << (address+4)/4 << std::endl;
		}
		address += 4;
	}
	
	if(!fileValid)
		return;
	
	//open output file
	std::ofstream out(fileName);
	
	address = 0; 
	//loop through vector of instructions and output to file
	for(int i=0 ; i<instructions.size() ; i++)
	{
		if(std::find(labels.begin(), labels.end(), address) != labels.end()) // if the current line needs a label, add it
			out << "Addr_" << padd(address) << ':' << std::endl;
		out << instructions[i] << std::endl;
		
		address+=4;
	}
	
}

int main(int argc, char *argv[])
{
	//check inputs
	if(argc != 2)
	{
		std::cerr << "Error: invalid number of input arguments";
		return 0;
	}
	
	//check if input is a valid file
	std::string fileName = argv[1];
	std::ifstream ifs(fileName);
	//if invalid exit and output error
	if(!ifs)
	{
		std::cerr << "Error: could not open file for reading.";
		return 0;
	}
	//change .obj to .s in filename
	fileName.pop_back();
	fileName.pop_back();
	fileName.pop_back();
	fileName += "s";
	
	//std::cout <<fileName;

	parse(ifs,fileName);
}