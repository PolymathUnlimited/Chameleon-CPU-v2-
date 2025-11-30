#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <unordered_map>

using namespace std;

// helper function to help us determine which symbols we need to isolate during symbol generation
bool isOperator(char c)
{
	switch (c)
	{
	case '+':
	case '-':
	case '*':
	case '/':
	case '%':
	case '^':
	case '(':
	case ')':
	case '[':
	case ']':
	case '#':
	case '!':
	case '<':
	case '>':
	case ':':
	case '=':
		return true;
	default:
		break;
	}

	return false;
}

// returns true if the string is an integer (base 10, base 16, or base 2)
bool isInt(string str)
{
	for (int i = 0; i < str.length(); ++i)
	{
		char c = str[i];
		if (i == 1 && (c == 'x' || c == 'X' || c == 'b' || c == 'B')) continue;
		if ((c < '0' || c > '9') && (c < 'a' || c > 'f') && (c < 'A' || c > 'F')) return false;
	}

	return true;
}

// turns a string into an integer (more robust than stoi)
int toInt(string str)
{
	if (!isInt(str)) return 0;

	if (str.length() > 2 && (str[1] == 'x' || str[1] == 'X')) return stoi(str.substr(2), NULL, 16);
	if (str.length() > 2 && (str[1] == 'b' || str[1] == 'B')) return stoi(str.substr(2), NULL, 2);
	return stoi(str);
}

// reads assembly code from a file
string readAsm(string filename)
{
	// open a file
	ifstream fin(filename);
	if (fin.fail())
	{
		fin.close();
		cout << "ERROR: " << filename << " not found." << endl;
		return "";
	}

	// read the file
	string code;
	string line;
	while (getline(fin, line))
	{
		bool inString = false;
		for (int i = 0; i < line.length(); ++i)
		{
			// ignore comments
			if (line[i] == ';' && !inString) line.erase(i);

			// isolate operators by adding space on either side
			if (isOperator(line[i]) && !inString)
			{
				line.insert(i, " ");
				line.insert(i + 2, " ");
				++i;
			}

			// make sure we don't modify string literals
			if (line[i] == '"' && (i == 0 || line[i - 1] != '\\')) inString = !inString;
		}

		// add the line to the source code
		code += " " + line + " ";
	}

	// return file contents
	return code;
}

// takes formatted assembly code and parses it into symbols
vector<string> getSymbols(string code)
{
	// set up a container to hold the symbols
	vector<string> symbols;

	// generate symbols
	string symbol = "";
	for (int i = 0; i < code.length(); ++i)
	{
		// make sure that string literals don't get broken up into multiple symbols
		if (code[i] == '"' && (i == 0 || code[i - 1] != '\\'))
		{
			if (symbol != "")
			{
				symbols.push_back(symbol);
				symbol = "";
			}
			symbol = "\"";

			for (++i; !(code[i] == '"' && (i == 0 || code[i - 1] != '\\')); ++i)
				symbol.push_back(code[i]);

			symbol.push_back('"');
			symbols.push_back(symbol);
			symbol = "";
			++i;
		}

		// break up code into symbols
		if (code[i] > 32 && code[i] <= 126) symbol.push_back(code[i]);
		else
		{
			if (symbol != "") symbols.push_back(symbol);
			symbol = "";
		}
	}

	// additional formatting to enforce order or operations for mathematical expressions

	// put parentheses around exponents
	for (int i = 0; i < symbols.size(); ++i)
	{
		if (symbols[i] == "^")
		{
			int index = i;
			int parnum = 0;

			do
			{
				++index;
				if (symbols[index] == "(") ++parnum;
				if (symbols[index] == ")") --parnum;
			} while (parnum && index < symbols.size());

			symbols.insert(symbols.begin() + index + 1, ")");

			index = i;
			parnum = 0;

			do
			{
				--index;
				if (symbols[index] == ")") ++parnum;
				if (symbols[index] == "(") --parnum;
			} while (parnum && index >= 0);

			symbols.insert(symbols.begin() + index, "(");

			++i;
		}
	}

	// put parentheses around multiplication, division, and modulus
	for (int i = 0; i < symbols.size(); ++i)
	{
		if (symbols[i] == "*" || symbols[i] == "/" || symbols[i] == "%")
		{
			int index = i;
			int parnum = 0;

			do
			{
				++index;
				if (symbols[index] == "(") ++parnum;
				if (symbols[index] == ")") --parnum;
			} while (parnum && index < symbols.size());

			symbols.insert(symbols.begin() + index + 1, ")");

			index = i;
			parnum = 0;

			do
			{
				--index;
				if (symbols[index] == ")") ++parnum;
				if (symbols[index] == "(") --parnum;
			} while (parnum && index >= 0);

			symbols.insert(symbols.begin() + index, "(");

			++i;
		}
	}

	// put parentheses around addition and subtraction
	for (int i = 0; i < symbols.size(); ++i)
	{
		if (symbols[i] == "+" || symbols[i] == "-")
		{
			int index = i;
			int parnum = 0;

			do
			{
				++index;
				if (symbols[index] == "(") ++parnum;
				if (symbols[index] == ")") --parnum;
			} while (parnum && index < symbols.size());

			symbols.insert(symbols.begin() + index + 1, ")");

			index = i;
			parnum = 0;

			do
			{
				--index;
				if (symbols[index] == ")") ++parnum;
				if (symbols[index] == "(") --parnum;
			} while (parnum && index >= 0);

			symbols.insert(symbols.begin() + index, "(");

			++i;
		}
	}

	return symbols;
}

// function to handle macros
void resolveMacros(vector<string>& symbols)
{
	cout << "RESOLVING MACROS" << endl;
	// loop through all the symbols
	for (int i = 0; i < symbols.size(); ++i)
	{
		// if we find a macro, then apply it
		if (symbols[i] == ".macro" || symbols[i] == ".MACRO")
		{
			cout << "macro found:" << endl;
			// get all the symbols that are part of the macro
			vector<string> macroSymbols;
			for (int j = i + 1; j < symbols.size() && symbols[j] != ".end" && symbols[j] != ".END"; ++j)
				macroSymbols.push_back(symbols[j]);

			cout << "\tMacro Symbols Generated (" << macroSymbols.size() << " symbols)" << endl;
			symbols.erase(symbols.begin() + i, symbols.begin() + i + macroSymbols.size() + 2);
			cout << "\tMacro symbols removed from main code" << endl;

			if (macroSymbols.size() == 0) continue;

			// get macro parameters
			string macroName = macroSymbols[0];
			vector<string> macroArgs;
			vector<string> macroLabels;
			macroSymbols.erase(macroSymbols.begin(), macroSymbols.begin() + 1);

			while (macroSymbols.size() > 0 && macroSymbols[0] != ":")
			{
				macroArgs.push_back(macroSymbols[0]);
				macroSymbols.erase(macroSymbols.begin(), macroSymbols.begin() + 1);
			}
			macroSymbols.erase(macroSymbols.begin(), macroSymbols.begin() + 1);

			cout << "\tMacro name: " << macroName << endl;
			cout << "\tMacro args: ";
			for (int i = 0; i < macroArgs.size(); ++i) cout << macroArgs[i] + " ";
			cout << endl << "\tMacro labels: ";
			for (int i = 0; i < macroLabels.size(); ++i) cout << macroLabels[i] + " ";
			cout << endl << "\tMacro simbols: ";
			for (int i = 0; i < macroSymbols.size(); ++i) cout << macroSymbols[i] + " ";
			cout << endl;

			for (int j = 0; j < macroSymbols.size(); ++j)
			{
				if ((macroSymbols[j] == ":" || macroSymbols[j] == "=") && j > 0)
					macroLabels.push_back(macroSymbols[j - 1]);
			}

			// apply the macro wherever it is used in the program
			int n = 0;
			for (int j = 0; j < symbols.size(); ++j)
			{
				if (symbols[j] == macroName)
				{
					vector<string> localArgs;
					for (int k = 0; k < macroArgs.size(); ++k)
						localArgs.push_back(symbols[j + k + 1]);
					symbols.erase(symbols.begin() + j, symbols.begin() + j + macroArgs.size() + 1);

					for (int k = macroSymbols.size() - 1; k >= 0; --k)
					{
						// apply macro arguments
						bool arg = false;
						for (int l = 0; l < localArgs.size(); ++l)
							if (macroArgs[l] == macroSymbols[k])
							{
								arg = true;
								symbols.insert(symbols.begin() + j, { localArgs[l] });
								break;
							}
						if (arg) continue;

						// apply macro Labels
						bool label = false;
						for (int l = 0; l < macroLabels.size(); ++l)
						{
							if (macroSymbols[k] == macroLabels[l])
							{
								label = true;
								symbols.insert(symbols.begin() + j, { macroSymbols[k] + "_" + macroName + "_MACRO_LABEL_" + to_string(n) });
								break;
							}
						}
						if (label) continue;

						symbols.insert(symbols.begin() + j, { macroSymbols[k] });
					}

					++n;
				}
			}
			--i;
		}
	}
}

// function to transform all instruction arguments into pure numerical values
void processSymbols(vector<string>& symbols)
{
	// map to hold labels and definitions
	unordered_map<string, int> labels;

	// define lables
	int addr = 0;
	for (int i = 0; i < symbols.size(); ++i)
	{
		// increment the address by the correct ammount based on what the symbol is
		if (symbols[i] == "LOD" || symbols[i] == "lod" || symbols[i] == "STO" || symbols[i] == "sto" ||
			symbols[i] == "ADD" || symbols[i] == "add" || symbols[i] == "ADC" || symbols[i] == "adc" ||
			symbols[i] == "SUB" || symbols[i] == "sub" || symbols[i] == "SBB" || symbols[i] == "sbb" ||
			symbols[i] == "ONC" || symbols[i] == "onc" || symbols[i] == "TWC" || symbols[i] == "twc" ||
			symbols[i] == "AND" || symbols[i] == "and" || symbols[i] == "OR" || symbols[i] == "or" ||
			symbols[i] == "XOR" || symbols[i] == "xor" || symbols[i] == "LSL" || symbols[i] == "lsl" ||
			symbols[i] == "LSR" || symbols[i] == "lsr" || symbols[i] == "ASR" || symbols[i] == "asr" ||
			symbols[i] == "ROL" || symbols[i] == "rol" || symbols[i] == "ROR" || symbols[i] == "ror" ||
			symbols[i] == "RCL" || symbols[i] == "rcl" || symbols[i] == "RCR" || symbols[i] == "rcr")
		{
			string nextSymbol = "";
			if (i < symbols.size() - 1) nextSymbol = symbols[i + 1];

			if (nextSymbol == "#") addr += 2; // immediate addressing mode
			else if (nextSymbol == "!") addr += 1; // stack addressing mode
			else addr += 3; // direct and indirect modes
		}

		else if (symbols[i] == "JMP" || symbols[i] == "jmp" || symbols[i] == "NOP" || symbols[i] == "nop" ||
			symbols[i] == "BRC" || symbols[i] == "brc" || symbols[i] == "BNC" || symbols[i] == "bnc" ||
			symbols[i] == "BRZ" || symbols[i] == "brz" || symbols[i] == "BNZ" || symbols[i] == "bnz" ||
			symbols[i] == "BRN" || symbols[i] == "brn" || symbols[i] == "BNN" || symbols[i] == "bnn" ||
			symbols[i] == "BRV" || symbols[i] == "brv" || symbols[i] == "BNV" || symbols[i] == "bnv" ||
			symbols[i] == "HLT" || symbols[i] == "hlt" || symbols[i] == "JSR" || symbols[i] == "jsr")
			addr += 3; // jumps and branches (always directly addressed)

		else if (symbols[i] == "PSH" || symbols[i] == "psh" || symbols[i] == "POP" || symbols[i] == "pop" || symbols[i] == "RSR" || symbols[i] == "rsr")
			addr += 1; // stack loads and stores, and subroutine returns

		// bytes
		else if (symbols[i] == ".byte" || symbols[i] == ".BYTE") addr += 1;

		// data
		else if (symbols[i] == ".data" || symbols[i] == ".END")
			for (int j = i + 1; j < symbols.size() && symbols[j] != ".end" && symbols[i] != ".END"; ++j)
			{
				if (isInt(symbols[j]))
				{
					int value = toInt(symbols[j]);
					while (value)
					{
						value /= 256;
						++addr;
					}
				}
			}

		// strings
		else if (symbols[i][0] == '"' && symbols[i][symbols[i].length() - 1] == '"')
		{
			for (int j = 0; j < symbols[i].length(); ++j)
			{
				if (symbols[i][j] == '\\') ++j;
				++addr;
			}
			++addr; // extra byte for null-terminator
		}

		// define labels by adding them to the map
		else if (symbols[i] == ":")
		{
			string previousSymbol = "";
			if (i > 0) previousSymbol = symbols[i - 1];

			labels[previousSymbol] = addr;

			symbols.erase(symbols.begin() + i - 1, symbols.begin() + i + 1);
			i -= 2;
		}
	}

	// calculate assigned labels, evaluate expressions, and use label addresses
	bool progress = true;
	while (progress)
	{
		progress = false;

		for (int i = 0; i < symbols.size(); ++i)
		{
			string symbol = symbols[i];
			string previousSymbol = "";
			string nextSymbol = "";
			if (i > 0) previousSymbol = symbols[i - 1];
			if (i < symbols.size() - 1) nextSymbol = symbols[i + 1];

			// see if this symbol is a label, replace it if it is
			for (const auto& pair : labels)
			{
				if (pair.first == symbol)
				{
					symbols[i] = to_string(pair.second);
					progress = true;
					break;
				}
			}

			// evaluate mathematical expressions
			if (symbol == "+" && isInt(previousSymbol) && isInt(nextSymbol))
			{
				int result = toInt(previousSymbol) + toInt(nextSymbol);
				symbols[i - 1] = to_string(result);
				symbols.erase(symbols.begin() + i, symbols.begin() + i + 2);
				i -= 2;
				progress = true;
			}
			else if (symbol == "-" && isInt(previousSymbol) && isInt(nextSymbol))
			{
				int result = toInt(previousSymbol) - toInt(nextSymbol);
				symbols[i - 1] = to_string(result);
				symbols.erase(symbols.begin() + i, symbols.begin() + i + 2);
				i -= 2;
				progress = true;
			}
			else if (symbol == "*" && isInt(previousSymbol) && isInt(nextSymbol))
			{
				int result = toInt(previousSymbol) * toInt(nextSymbol);
				symbols[i - 1] = to_string(result);
				symbols.erase(symbols.begin() + i, symbols.begin() + i + 2);
				i -= 2;
				progress = true;
			}
			else if (symbol == "/" && isInt(previousSymbol) && isInt(nextSymbol))
			{
				int result = toInt(previousSymbol) / toInt(nextSymbol);
				symbols[i - 1] = to_string(result);
				symbols.erase(symbols.begin() + i, symbols.begin() + i + 2);
				i -= 2;
				progress = true;
			}
			else if (symbol == "%" && isInt(previousSymbol) && isInt(nextSymbol))
			{
				int result = toInt(previousSymbol) % toInt(nextSymbol);
				symbols[i - 1] = to_string(result);
				symbols.erase(symbols.begin() + i, symbols.begin() + i + 2);
				i -= 2;
				progress = true;
			}
			else if (symbol == "^" && isInt(previousSymbol) && isInt(nextSymbol))
			{
				int result = pow(toInt(previousSymbol), toInt(nextSymbol));
				symbols[i - 1] = to_string(result);
				symbols.erase(symbols.begin() + i, symbols.begin() + i + 2);
				i -= 2;
				progress = true;
			}
			else if (isInt(symbol) && previousSymbol == "(" && nextSymbol == ")")
			{
				symbols[i - 1] = symbols[i];
				symbols.erase(symbols.begin() + i, symbols.begin() + i + 2);
				i -= 2;
				progress = true;
			}

			// define assigned labels
			else if (symbols[i] == "=" && isInt(nextSymbol))
			{
				labels[previousSymbol] = toInt(nextSymbol);
				symbols.erase(symbols.begin() + i - 1, symbols.begin() + i + 2);
				i -= 2;
				progress = true;
			}
		}
	}

	cout << "labels: " << endl;
	for (auto i = labels.begin(); i != labels.end(); ++i)
		cout << "\t" << i->first << ": " << i->second << endl;

	cout << endl;
}

// function to assemble processed symbols into machine code
vector <unsigned char> assemble(vector<string> symbols)
{
	// container to hold machine code bytes
	vector <unsigned char> machineCode;

	// loop through symbols
	for (int i = 0; i < symbols.size(); ++i)
	{
		// loads
		if (symbols[i] == "LOD" || symbols[i] == "lod") // load instructions
		{
			if (i < symbols.size() - 2 && symbols[i + 1] == "#" && isInt(symbols[i + 2]))
			{
				int value = toInt(symbols[i + 2]);
				machineCode.push_back(0b00000000); // immediate load
				machineCode.push_back(value); // value to load
			}
			else if (i < symbols.size() - 1 && isInt(symbols[i + 1]))
			{
				int address = toInt(symbols[i + 1]);
				machineCode.push_back(0b00010000); // direct load
				machineCode.push_back(address / 256); // address high byte
				machineCode.push_back(address % 256); // address low byte
			}
			else if (i < symbols.size() - 2 && symbols[i + 1] == "[" && isInt(symbols[i + 2]))
			{
				int pointer = toInt(symbols[i + 2]);
				machineCode.push_back(0b00100000); // indirect load
				machineCode.push_back(pointer / 256); // pointer high byte
				machineCode.push_back(pointer % 256); // pointer low byte
			}
		}
		else if (symbols[i] == "POP" || symbols[i] == "pop") machineCode.push_back(0b00110000); // stack load

		// stores
		else if (symbols[i] == "STO" || symbols[i] == "sto")
		{
			if (i < symbols.size() - 2 && symbols[i + 1] == "#" && isInt(symbols[i + 2]))
			{
				int value = toInt(symbols[i + 2]);
				machineCode.push_back(0b01000000); // immediate store
				machineCode.push_back(value); // value to store
			}
			else if (i < symbols.size() - 1 && isInt(symbols[i + 1]))
			{
				int address = toInt(symbols[i + 1]);
				machineCode.push_back(0b01010000); // direct store
				machineCode.push_back(address / 256); // address high byte
				machineCode.push_back(address % 256); // address low byte
			}
			else if (i < symbols.size() - 2 && symbols[i + 1] == "[" && isInt(symbols[i + 2]))
			{
				int pointer = toInt(symbols[i + 2]);
				machineCode.push_back(0b01100000); // indirect store
				machineCode.push_back(pointer / 256); // pointer high byte
				machineCode.push_back(pointer % 256); // pointer low byte
			}
		}
		else if (symbols[i] == "PSH" || symbols[i] == "psh") machineCode.push_back(0b01110000); // stack store

		// ALU operations
		else if (symbols[i] == "ADD" || symbols[i] == "add" || symbols[i] == "ADC" || symbols[i] == "adc" ||
			symbols[i] == "SUB" || symbols[i] == "sub" || symbols[i] == "SBB" || symbols[i] == "sbb" ||
			symbols[i] == "ONC" || symbols[i] == "onc" || symbols[i] == "TWC" || symbols[i] == "twc" ||
			symbols[i] == "AND" || symbols[i] == "and" || symbols[i] == "OR" || symbols[i] == "or" ||
			symbols[i] == "XOR" || symbols[i] == "xor" || symbols[i] == "LSL" || symbols[i] == "lsl" ||
			symbols[i] == "LSR" || symbols[i] == "lsr" || symbols[i] == "ASR" || symbols[i] == "asr" ||
			symbols[i] == "ROL" || symbols[i] == "rol" || symbols[i] == "ROR" || symbols[i] == "ror" ||
			symbols[i] == "RCL" || symbols[i] == "rcl" || symbols[i] == "RCR" || symbols[i] == "rcr")
		{
			// ALU opcode
			unsigned char opcode;
			if (symbols[i] == "ADD" || symbols[i] == "add") opcode = 0;
			else if (symbols[i] == "ADC" || symbols[i] == "adc") opcode = 1;
			else if (symbols[i] == "SUB" || symbols[i] == "sub") opcode = 2;
			else if (symbols[i] == "SBB" || symbols[i] == "sbb") opcode = 3;
			else if (symbols[i] == "ONC" || symbols[i] == "onc") opcode = 4;
			else if (symbols[i] == "TWC" || symbols[i] == "twc") opcode = 5;
			else if (symbols[i] == "AND" || symbols[i] == "and") opcode = 6;
			else if (symbols[i] == "OR" || symbols[i] == "or") opcode = 7;
			else if (symbols[i] == "XOR" || symbols[i] == "xor") opcode = 8;
			else if (symbols[i] == "LSL" || symbols[i] == "lsl") opcode = 9;
			else if (symbols[i] == "LSR" || symbols[i] == "lsr") opcode = 10;
			else if (symbols[i] == "ASR" || symbols[i] == "asr") opcode = 11;
			else if (symbols[i] == "ROL" || symbols[i] == "rol") opcode = 12;
			else if (symbols[i] == "ROR" || symbols[i] == "ror") opcode = 13;
			else if (symbols[i] == "RCL" || symbols[i] == "rcl") opcode = 14;
			else if (symbols[i] == "RCR" || symbols[i] == "rcr") opcode = 15;

			// addressing modes
			if (i < symbols.size() - 2 && symbols[i + 1] == "#" && isInt(symbols[i + 2]))
			{
				int value = toInt(symbols[i + 2]);
				machineCode.push_back(0b10000000 | opcode); // immediate ALU
				machineCode.push_back(value); // value to store
			}
			else if (i < symbols.size() - 1 && isInt(symbols[i + 1]))
			{
				int address = toInt(symbols[i + 1]);
				machineCode.push_back(0b10010000 | opcode); // direct ALU
				machineCode.push_back(address / 256); // address high byte
				machineCode.push_back(address % 256); // address low byte
			}
			else if (i < symbols.size() - 2 && symbols[i + 1] == "[" && isInt(symbols[i + 2]))
			{
				int pointer = toInt(symbols[i + 2]);
				machineCode.push_back(0b10100000 | opcode); // indirect ALU
				machineCode.push_back(pointer / 256); // pointer high byte
				machineCode.push_back(pointer % 256); // pointer low byte
			}
			else if (i < symbols.size() - 1 && symbols[i + 1] == "!") machineCode.push_back(0b10110000 | opcode); // stack ALU
		}

		// jumps and branches
		else if (symbols[i] == "HLT" || symbols[i] == "hlt")
		{
			int jumpAddress = machineCode.size(); // set jump target to self
			machineCode.push_back(0b11010000); // unconditional jump
			machineCode.push_back(jumpAddress / 256); // target high byte
			machineCode.push_back(jumpAddress % 256); // target low byte
		}
		else if (symbols[i] == "NOP" || symbols[i] == "nop")
		{
			int jumpAddress = machineCode.size() + 3; // set jump target to next instruction
			machineCode.push_back(0b11010000); // unconditional jump
			machineCode.push_back(jumpAddress / 256); // target high byte
			machineCode.push_back(jumpAddress % 256); // target low byte
		}
		else if (symbols[i] == "JMP" || symbols[i] == "jmp")
		{
			int jumpAddress = 0;
			if (i < symbols.size() - 1 && isInt(symbols[i + 1])) jumpAddress = toInt(symbols[i + 1]);
			machineCode.push_back(0b11010000); // unconditional jump
			machineCode.push_back(jumpAddress / 256); // target high byte
			machineCode.push_back(jumpAddress % 256); // target low byte
		}
		else if (symbols[i] == "BRC" || symbols[i] == "brc")
		{
			int jumpAddress = 0;
			if (i < symbols.size() - 1 && isInt(symbols[i + 1])) jumpAddress = toInt(symbols[i + 1]);
			machineCode.push_back(0b11000001); // conditinal jump
			machineCode.push_back(jumpAddress / 256); // target high byte
			machineCode.push_back(jumpAddress % 256); // target low byte
		}
		else if (symbols[i] == "BNC" || symbols[i] == "bnc")
		{
			int jumpAddress = 0;
			if (i < symbols.size() - 1 && isInt(symbols[i + 1])) jumpAddress = toInt(symbols[i + 1]);
			machineCode.push_back(0b11010001); // conditinal jump
			machineCode.push_back(jumpAddress / 256); // target high byte
			machineCode.push_back(jumpAddress % 256); // target low byte
		}
		else if (symbols[i] == "BRZ" || symbols[i] == "brz")
		{
			int jumpAddress = 0;
			if (i < symbols.size() - 1 && isInt(symbols[i + 1])) jumpAddress = toInt(symbols[i + 1]);
			machineCode.push_back(0b11000010); // conditinal jump
			machineCode.push_back(jumpAddress / 256); // target high byte
			machineCode.push_back(jumpAddress % 256); // target low byte
		}
		else if (symbols[i] == "BNZ" || symbols[i] == "bnz")
		{
			int jumpAddress = 0;
			if (i < symbols.size() - 1 && isInt(symbols[i + 1])) jumpAddress = toInt(symbols[i + 1]);
			machineCode.push_back(0b11010010); // conditinal jump
			machineCode.push_back(jumpAddress / 256); // target high byte
			machineCode.push_back(jumpAddress % 256); // target low byte
		}
		else if (symbols[i] == "BRN" || symbols[i] == "brn")
		{
			int jumpAddress = 0;
			if (i < symbols.size() - 1 && isInt(symbols[i + 1])) jumpAddress = toInt(symbols[i + 1]);
			machineCode.push_back(0b11000100); // conditinal jump
			machineCode.push_back(jumpAddress / 256); // target high byte
			machineCode.push_back(jumpAddress % 256); // target low byte
		}
		else if (symbols[i] == "BNN" || symbols[i] == "bnn")
		{
			int jumpAddress = 0;
			if (i < symbols.size() - 1 && isInt(symbols[i + 1])) jumpAddress = toInt(symbols[i + 1]);
			machineCode.push_back(0b11010100); // conditinal jump
			machineCode.push_back(jumpAddress / 256); // target high byte
			machineCode.push_back(jumpAddress % 256); // target low byte
		}
		else if (symbols[i] == "BRV" || symbols[i] == "brv")
		{
			int jumpAddress = 0;
			if (i < symbols.size() - 1 && isInt(symbols[i + 1])) jumpAddress = toInt(symbols[i + 1]);
			machineCode.push_back(0b11001000); // conditinal jump
			machineCode.push_back(jumpAddress / 256); // target high byte
			machineCode.push_back(jumpAddress % 256); // target low byte
		}
		else if (symbols[i] == "BNV" || symbols[i] == "bnv")
		{
			int jumpAddress = 0;
			if (i < symbols.size() - 1 && isInt(symbols[i + 1])) jumpAddress = toInt(symbols[i + 1]);
			machineCode.push_back(0b11011000); // conditinal jump
			machineCode.push_back(jumpAddress / 256); // target high byte
			machineCode.push_back(jumpAddress % 256); // target low byte
		}
		else if (symbols[i] == "JSR" || symbols[i] == "jsr")
		{
			int jumpAddress = 0;
			if (i < symbols.size() - 1 && isInt(symbols[i + 1])) jumpAddress = toInt(symbols[i + 1]);
			machineCode.push_back(0b11100000); // jump to subroutine
			machineCode.push_back(jumpAddress / 256); // target high byte
			machineCode.push_back(jumpAddress % 256); // target low byte
		}
		else if (symbols[i] == "RSR" || symbols[i] == "rsr") machineCode.push_back(0b11110000); // return from subroutine

		// data directives
		else if (symbols[i] == ".byte" || symbols[i] == ".BYTE")
		{
			unsigned char byte = 0;
			if (i < symbols.size() - 1 && isInt(symbols[i + 1])) byte = toInt(symbols[i + 1]);
			machineCode.push_back(byte);
		}
		else if (symbols[i] == ".data" || symbols[i] == ".DATA")
		{
			for (int j = i + 1; j < symbols.size(); ++j)
			{
				if (symbols[j] == ".end" || symbols[j] == ".END") break;
				if (isInt(symbols[j]))
				{
					int value = toInt(symbols[j]);
					vector<unsigned char> data;
					while (value)
					{
						int byte = value % 256;
						value /= 256;
						data.push_back(byte);
					}

					for (int k = data.size() - 1; k >= 0; --k) machineCode.push_back(data[k]);
				}
			}
		}

		// strings
		else if (symbols[i][0] == '"' && symbols[i][symbols[i].length() - 1] == '"')
		{
			for (int j = 1; j < symbols[i].length() - 1; ++j)
			{
				if (symbols[i][j] == '\\')
				{
					switch (symbols[i][j + 1])
					{
					case 'n':
						machineCode.push_back('\n');
						break;
					case 't':
						machineCode.push_back('\t');
						break;
					case 'r':
						machineCode.push_back('\r');
						break;
					case 'b':
						machineCode.push_back('\b');
						break;
					case 'a':
						machineCode.push_back('\a');
						break;
					case 'f':
						machineCode.push_back('\f');
						break;
					case 'v':
						machineCode.push_back('\v');
						break;
					case '0':
						machineCode.push_back('\0');
						break;
					default:
						machineCode.push_back(symbols[i][j + 1]);
						break;
					}
					++j;
				}
				else machineCode.push_back(symbols[i][j]);
			}
			machineCode.push_back(0); // null terminator
		}
	}

	return machineCode;
}

// main function
int main()
{
	// read assembly code from a file
	string code = readAsm("code.asm");
	cout << "CODE: " << code << endl << endl;

	// parse the code into symbols
	vector<string> symbols = getSymbols(code);
	cout << "SYMBOLS: " << endl;
	for (int i = 0; i < symbols.size(); ++i) cout << "\t" << symbols[i] << endl;
	cout << endl;

	// process the symbols
	resolveMacros(symbols);
	cout << "SYMBOLS (after macros): " << endl;
	for (int i = 0; i < symbols.size(); ++i) cout << "\t" << symbols[i] << endl;
	cout << endl;

	processSymbols(symbols);
	cout << "SYMBOLS (after processing): " << endl;
	for (int i = 0; i < symbols.size(); ++i) cout << "\t" << symbols[i] << endl;
	cout << endl;

	cout << "SYMBOLS PROCESSED" << endl;

	// assemble the machine code and print to the console
	cout << "ASSEMBLING MACHINE CODE" << endl;
	vector<unsigned char> machineCode = assemble(symbols);
	cout << hex << "machine code: " << endl;
	for (int i = 0; i < machineCode.size(); ++i)
	{
		if (machineCode[i] < 16) cout << 0;
		cout << (int)machineCode[i] << " ";
	}
	cout << dec << endl;

	// end program
	return 0;
}