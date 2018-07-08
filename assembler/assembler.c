#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#pragma warning (disable:4996)

typedef enum {true =1, false = !true}bool;
typedef char* string;
#define BIN 2
#define BASE_FOUR 4
#define	COMSIZE 4
#define WORDSIZ 10
#define BASE_FOUR_WORD_SIZ 4
#define ENTRY_GLOB_ADDRESS -100 /* set as address when there is decleration of entry */
#define REGSIZ 4 
#define DECODSIZ 2
#define MAX_NAME_SIZ 80
#define MAX_LINE_LEN 80
#define SIZ_FOR_REALLOC 5
#define MAIN_REALLOC 1024
#define MAX_ONE_DATA_SIZ 800 


typedef struct command
{
	int commandsSize;
	string commands[];
}comand;

enum type{ ABS, MAT, REG, NON, STR };

typedef struct symbols_struct
{
	struct symbols_struct* next;
	char name[MAX_NAME_SIZ];
	int* address_values;
	bool isExtern;
	bool isCommand;
	bool isEntry;
	bool isData;
	int addresses_size;
}syms, *symp;

typedef struct symbolsList
{
	symp start;
	symp end;
	int size;
}symList, *symListP;

comand coms = { 1, { "mov" } };
int absolute = 0;
int realoc = 1;
int external = 2;

static bool entry_flag = false;
static bool extern_flag = false;
static bool errorFlag = false;
static int line_counter = 0;

void firstMove(string data, symListP symbols, char dataSegment[], char commandSegment[], int* DC, int* IC);
void firstMoveHandler(int argc, string argv[], symListP symbols, char dataSegment[], char commandSegment[], int* DC, int* IC);
void secondMoveHandler(string file_name, symList symbols, string dataSegment, string commandsSegment, int DC, int IC, int START_IC);

bool handleCommand(string insert, string data , int command_index, int *IC);
bool handleDatas(string insert, string type, string args, int* DC);


string increaseArgument(string data, char sep);
bool isString(string data);
void fromDeciFromIndex(char res[], int base, int inputNum, int start_index, int size, bool is_end_of_string);
void fromDeci(string insert, int base,int data, int size);
void setErrorFlag(string errorMsg, bool endProg);
bool is_symbol(string name);
void NotEnoughMemory();


symListP getNewSymbolsList();
void destroySybolsList(symListP symbols);

void printToFileInBaseFour(string data, FILE* fp);
void printSymbols(symList symbols);
void printAllData(symList symbols, string dataSegment, string commandsSegment, int IC, int DC);
string allocateString();
string tryToReallocString(string toRealloc);

/* must be a multipication of 2 */
char getRepresentiveBaseCharFromBinary(string data, int to_base, int* index);

int main(int argc, string argv[])
{
	
	string commandsSegment = allocateString();
	string dataSegment = allocateString();
	int IC = 100, DC = 0, START_IC = IC;
	symListP symbols = getNewSymbolsList();
	
	firstMoveHandler(argc, argv, symbols, dataSegment, commandsSegment, &DC, &IC);
	secondMoveHandler(argv[1], *symbols, dataSegment, commandsSegment, DC, IC, START_IC);

	printAllData(*symbols, dataSegment, commandsSegment, IC, DC);

	destroySybolsList(symbols);
	free(dataSegment);
	free(commandsSegment);

	system("pause");
	return 0;
}

string setStringEnd(string file_name, string end)
{
	int dot_index = strcspn(file_name, ".");
	sscanf(end, "%s", file_name + dot_index + 1);
	return file_name;
}



void writeAddressesToCommandsSegment(symList symbols, string commandsSegment)
{
	symp runner = symbols.start;
	int found_index = 0, i = 0;

	for (runner; runner != NULL; runner = runner->next)
	{
		found_index = strcspn(commandsSegment, runner->name);
		for (i = 0; i < runner->addresses_size, found_index != strlen(commandsSegment); i++, found_index = (strcspn(commandsSegment, runner->name)))
		{
			fromDeciFromIndex(commandsSegment, BIN, runner->address_values[i], found_index, WORDSIZ, false);
		}
	}
}

void increaseDataSegmentAdresses(symList symbols, int max_ic_val)
{
	symp runner = symbols.start;
	int i = 0;
	for (runner; runner != NULL; runner = runner->next)
		if (runner->isData) 
			for (i = 0; i != runner->addresses_size; i++) 
				runner->address_values[i] += max_ic_val;
}

FILE* open_file(string file_name,string ending, bool flag)
{
	FILE* ret_file = NULL;
	char out_files[MAX_LINE_LEN] = { 0 };
	if (!flag) return NULL;
	strcpy(out_files, file_name);
	ret_file = fopen(setStringEnd(out_files, ending), "w");
	if (!ret_file) setErrorFlag("Cannot open file for writing!", true);
	return ret_file;
}

void updateEntryAndExternToBaseFour(symList symbols,  string base_four_entry, string base_four_extern)
{
	int counter = 0;
	symp runner = symbols.start;
	for (runner; runner != NULL; runner = runner->next)
	{
		if (runner->isEntry)
			for (counter = 0; counter < runner->addresses_size; counter++)
				fromDeci(base_four_entry, BASE_FOUR, runner->address_values[counter], BASE_FOUR_WORD_SIZ);
		if (runner->isExtern)
			for (counter = 0; counter < runner->addresses_size; counter++)
				fromDeci(base_four_extern, BASE_FOUR, runner->address_values[counter], BASE_FOUR_WORD_SIZ);
	}

}


int convertBinToDec(char first_ch, char sec_ch)
{
	return (first_ch - '0') * 2 + sec_ch - '0';
}

void printToFileInBaseFour(int address, string data, FILE* out_file)
{
	int i = 0;
	char address_representation[BASE_FOUR_WORD_SIZ + 1] = { 0 };
	fromDeci(address_representation, BASE_FOUR, address, BASE_FOUR_WORD_SIZ);
	fprintf(out_file, "\n%s ", address_representation);
	for (i; i < WORDSIZ; i += 2)
		fprintf(out_file, "%c",  convertBinToDec(data[i], data[i + 1])+'a');
}

void writeExternAndEntern(FILE* file_entry, FILE* file_extern, symList symbols )
{
	char base_four_data[MAX_ONE_DATA_SIZ] = { 0 };
	int counter = 0, str_len = 0;
	symp runner = symbols.start;
	FILE* file_ptr = file_entry;
	for (runner; runner != NULL; runner = runner->next)
	{
		if (runner->isEntry || runner->isExtern)
		{
			for (counter = 0; counter < runner->addresses_size; counter++){
				fromDeci(base_four_data, BASE_FOUR, runner->address_values[counter], BASE_FOUR_WORD_SIZ);
				if (runner->addresses_size && counter != (runner->addresses_size - 1))
				{
					base_four_data[str_len = strlen(base_four_data)] = ',';
					base_four_data[str_len + 1] = 0;
				}
			}
			file_ptr = runner->isEntry ? file_entry : file_extern;
			fprintf(file_ptr, "%s %s\n", runner->name, base_four_data);
			strcpy(base_four_data, "");
		}
	}
}

void writeDataToOb(FILE* file_to_write, string commandsSegment, int commands_size,string dataSegment, int data_size,int START_IC)
{
	int index = 0;
	for (index = 0; index < strlen(commandsSegment) / WORDSIZ; index++)
		printToFileInBaseFour(START_IC + index, commandsSegment + index * WORDSIZ, file_to_write);
	
	START_IC += index;

	for (index = 0; index < strlen(dataSegment) / WORDSIZ; index++)
		printToFileInBaseFour(START_IC + index, dataSegment + index * WORDSIZ, file_to_write);
}


void secondMoveHandler(string file_name, symList symbols, string dataSegment, string commandsSegment, int DC, int IC,int START_IC)
{
	FILE* file_entry = open_file(file_name, "ent", entry_flag), *file_extern = open_file(file_name, "ext", entry_flag), *file_commands = open_file(file_name, "ob", true);
	
	increaseDataSegmentAdresses(symbols, IC); 
	writeAddressesToCommandsSegment(symbols,commandsSegment);
	writeDataToOb(file_commands, commandsSegment, strlen(commandsSegment) / WORDSIZ, dataSegment, strlen(dataSegment) / WORDSIZ,START_IC);
	writeExternAndEntern(file_entry, file_extern, symbols);
}

void firstMoveHandler(int argc, string argv[], symListP symbols, string dataSegment, string commandsSegment, int* DC, int* IC)
{
	FILE* reading_file = NULL;
	static char line[] = { 0 };
	if (argc < 2) setErrorFlag("Usage: .\\assembler file_arg", true);
	reading_file = fopen(argv[1], "r");
	if (!reading_file) setErrorFlag("Cannot open argument file", true);

	while (!feof(reading_file) && (int)(fgets(line, MAX_LINE_LEN, reading_file)) != EOF)
	{
		line[strlen(line) - 1] = 0;
		firstMove(line, symbols, dataSegment, commandsSegment, DC, IC);
		dataSegment = tryToReallocString(dataSegment);
		commandsSegment = tryToReallocString(commandsSegment);
		strcpy(line, "");
	}
	fclose(reading_file); 
}

void printAllData(symList symbols, string dataSegment, string commandsSegment, int IC, int DC)
{
	printSymbols(symbols);
	puts("dataSegment:");
	puts(dataSegment);
	puts("");
	puts("commandsSegment:");
	puts(commandsSegment);

	printf("\nIC: %d\n", IC);
	printf("DC: %d\n", DC);

}
string allocateString()
{
	string ret = (string)malloc(sizeof(char) * MAIN_REALLOC);
	if (!ret) NotEnoughMemory(); ret[0] = 0;
	return ret;
}

string tryToReallocString(string toRealloc)
{
	if ((MAIN_REALLOC - MAX_ONE_DATA_SIZ) < strlen(toRealloc))
	{
		toRealloc = (string)realloc(toRealloc, strlen(toRealloc) + MAIN_REALLOC);
		if (!toRealloc) NotEnoughMemory();
	}
	return toRealloc;
}

string increaseArgument(string data, char sep){
	string index = strchr(data, sep);
	if (index) {
		*index = 0;
		index++;
	}
	return index;
};


int getAdressSize(int type)
{
	if (type == MAT) return 2;
	return 1;
}

bool addressesAreRegisters(int src, int dst)
{
	return ((src == ABS || src == REG) && (dst == ABS || dst == REG));
}

int getICValue(int src, int dst)
{
	if (addressesAreRegisters(src, dst)) return 1;
	else return getAdressSize(src) + getAdressSize(dst);
}

void writeAdressing(string insert, int src, int dst)
{
	fromDeci(insert, BIN, src % 3, DECODSIZ);
	fromDeci(insert, BIN, dst % 3, DECODSIZ);
	if (addressesAreRegisters(src, dst)) fromDeci(insert, BIN, absolute, DECODSIZ);
	else fromDeci(insert, BIN, realoc, DECODSIZ);
}


int getOperandIndex(string data){
	if (data[0] == '#') return ABS;
	if (strchr(data, '[')) return MAT;
	if (data[0] == 'r' && isdigit(data[1])) return REG;
	if (isString(data)) return STR;
	else setErrorFlag("Must be a valid string, numbers must be avoided!", false);
	return NON;
};


bool isString(string data)
{
	int i = 0;
	for (i; i < strlen(data); i++) if (!isalnum(data[i]) || isdigit(data[i])){
		setErrorFlag("Bad String name!", false);
		return false;
	}
	return true;
}

bool insertString(char addData[], string data)
{
	if (!isString(data)) return false; 
	sprintf(addData + strlen(addData), "%s%.*s", data, WORDSIZ - strlen(data), "??????????");
	return true;
}
bool insertNum(char addData[], string data)
{
	if (data[0] != '#') return false;
	int num = 0;

	if (!sscanf(data + 1, "%d", &num)){
		setErrorFlag("bad number", false);
		return false;
	}
	fromDeci(addData, BIN, num, WORDSIZ);
	return true;

}
//bool insertReg(char addData[], string data);
bool insertMat(char addData[], string data)
{
	bool bad_mat = false;
	int r1 = 0, r2 = 0, index = 0;
	if (!strchr(data, '[')) return false;
	index = (strchr(data, '[') - data);
	if (index < 0 || index > strlen(data)) return false;
	if (sprintf(addData + strlen(addData), "\n%.*s\n", index, data)<0) return false;
	if (sscanf(data + index, "[r%d][r%d]", &r1, &r2) != 2) {
		setErrorFlag("Bad mat construction!", false);
		bad_mat = true;
	}
	fromDeci(addData, BIN, r1, REGSIZ);
	fromDeci(addData, BIN, r2, REGSIZ);
	fromDeci(addData, BIN, realoc, DECODSIZ);
	return !bad_mat;
}

bool insertReg(char addData[], string data, int size, bool decode)
{
	int num = 0;
	bool bad_register = false;  

	sscanf(data + 1, "%d", &num);
	bad_register = (strlen(data) != 2 || num < 1 || num > 8);
	if (bad_register) setErrorFlag("Bad register name!", false);
	
	fromDeci(addData, BIN, num, size);

	if (decode) fromDeci(addData, BIN, absolute, DECODSIZ);
	return !bad_register;
}


bool chooseAndInsertOperand(string insert, string data, int type)
{
	if (type == STR) return insertString(insert, data);
	else if (type == ABS) return insertNum(insert, data);
	else if (type == MAT) return insertMat(insert, data);
	return true;
}

bool getData(string insert, string arg1, string arg2, int src, int dst)
{
	bool ok = false;
	ok = chooseAndInsertOperand(insert, arg1, src);
	ok = chooseAndInsertOperand(insert, arg2, dst);
	if (src == REG && dst == REG)
	{
		ok = insertReg(insert, arg1, REGSIZ, false);
		ok = insertReg(insert, arg2, REGSIZ, true);
	}
	else if (src == REG) ok = insertReg(insert, arg1, REGSIZ * 2, true);
	else if (dst == REG) ok = insertReg(insert, arg2, REGSIZ * 2, true);
	return ok;
}


int getCommandIndex(string data){
	int i = 0;
	for (; i < coms.commandsSize; i++)
		if (!strcmp(data, coms.commands[i]))
			return i;
	return EOF;
}

bool handleCommand(string insert,string args, int command_index,  int *IC){

	string args2 = NULL, none = NULL;
	int src, dst;
	if (command_index == EOF) return false;
	args2 = increaseArgument(args, ',');
	none = increaseArgument(args2, ',');
	src =  getOperandIndex(args);
	dst = getOperandIndex(args2);
	if (src == NON || dst == NON) return false;

	fromDeci(insert, BIN, command_index, COMSIZE);
	writeAdressing(insert, src, dst);
	
	if (!getData(insert, args, args2, src, dst)) return false;
	*IC += getICValue(src, dst) + 1; //////
	return true;
};


bool getDataData(string insert, string data, int *DC, bool matrix, bool empty_matrix, int expected_size_opt)
{
	
	bool bad_length = false, bad_data = false;
	int value = 0, size = 0;
	string args = NULL;

	while (true)
	{
		if (!empty_matrix )
		{
			if (!data) break;
			args = increaseArgument(data, ',');
			if ((bad_data = !(value = atoi(data))))break;
		}
		else if (matrix && size == expected_size_opt) break;
		fromDeci(insert, BIN, value, WORDSIZ);
		data = args;
		size++;
	}
	bad_length = (matrix && expected_size_opt != size);
	if (bad_length || bad_data)
	{
		setErrorFlag("Error occurd! getDataData", false);
		insert[strlen(insert) - WORDSIZ*size] = 0;
		return false;
	}
	*DC += size;
	return true;
}

bool getMatData(string insert, string size_data, int *DC)
{
	int counter = 0, excepted_size= 0, x, y, value;
	string args = increaseArgument(size_data, ' ');
	if (sscanf(size_data, "[%d][%d]", &x, &y) != 2)
	{
		setErrorFlag("Error in matrix size construction!", false);
		return;
	}
	excepted_size = x*y;
	if (!getDataData(insert, args, DC, true, !args, excepted_size)) return false;
	return true;
}

bool getStringData(string insert, string data, int* DC)
{
	bool no_data = !data, no_quotes = false;
	string first_quotes_seperator = NULL, second_quotes_seperator = NULL;
	int size = 0, value = 0;

	if (no_data)
	{
		first_quotes_seperator = strchr(data, '\"');
		second_quotes_seperator = strchr(first_quotes_seperator, '\"');
		no_quotes = !first_quotes_seperator && !second_quotes_seperator;
		if (!no_quotes) no_quotes = first_quotes_seperator != data && strlen(second_quotes_seperator);
	}

	if (no_data || no_quotes)
	{
		setErrorFlag("Error in string data construction", false);
		return false;
	}
	data++;
	for (size; size < strlen(data)-1; size++)
	{
		if (!isalnum(data[size]) && data[size] != ' ')
		{
			insert[strlen(insert) - size*WORDSIZ] = 0;
			setErrorFlag("string must consist of valid chars", false);
			return false;
		}
		fromDeci(insert, BIN, data[size], WORDSIZ);
	}
	*DC += size;
	return true;
}


bool handleDatas(string insert, string type, string args, int* DC)
{
	int value = 0, last_size = strlen(insert);
	bool integers, str, mat, empty = false;

	integers = !strcmp(type, ".data");
	str = !strcmp(type, ".string");
	mat = !strcmp(type, ".mat");
	if (!integers && !mat && !str)
	{
		setErrorFlag("data construction is uknown, must be:.data\.string\.mat", false);
		return false;
	}

	if (integers) integers = getDataData(insert, args, DC, false, false, EOF);
	if (mat) mat = getMatData(insert, args, DC);
	if (str) str = getStringData(insert, args, DC);
	if (!integers && !mat && !str)
	{
		insert[last_size] = 0;
		return false;
	}
	return true;
}


bool is_symbol(string name)
{
	string end = strchr(name, ':');
	if (end) end[strlen(end) - 1] = 0;
	return end != NULL;
}

symListP getNewSymbolsList()
{
	symListP symbols = (symListP)malloc(sizeof(symList));
	if (!symbols) NotEnoughMemory();

	symbols->start = NULL;
	symbols->end = NULL;
	
	symbols->size = 0;
	return symbols;
}
void destroySybolsList(symListP symbols)
{
	symp runner = symbols->start, tmp = runner;
	for (tmp = tmp != NULL ? tmp->next : NULL; runner != NULL; runner = tmp, tmp = tmp != NULL ? tmp->next : NULL)
	{
		free(runner->address_values);
		free(runner);
	}
	free(symbols);
	
}

void printSymbols(symList symbols)
{
	int i = 0;
	symp runner = symbols.start;
	puts("Symbols:");
	for (; runner != NULL; runner = runner->next)
	{
		printf("name: %s\n", runner->name);
		puts("addresses values are:");
		for (i = 0; i < runner->addresses_size; i++) printf("%d,", runner->address_values[i]);
		puts("");
		if (runner->isCommand) puts("isCommand True");
		if (runner->isData) puts("isData True");
		if (runner->isExtern) puts("isExtern True");
		if (runner->isEntry) puts("isEntry True");
		puts(""); 
	}
		
}

symp nameExistsInSymb(symList symbols, string name)
{
	symp found = symbols.start;

	for (found; found != NULL; found = found->next)
		if (!strcmp(found->name, name)) return found;
	return NULL;
}

/* Returns true if name already exists
returns flase if doesn't exist
*/
bool insertAddressInSymbols(symListP symbols, string name, int address_value)
{
	symp found = nameExistsInSymb(*symbols, name);
	if (found){

		if (!found->isEntry && !((++(found->addresses_size)) % SIZ_FOR_REALLOC)){

			found->address_values = (int)realloc(found->address_values, sizeof(int)* (found->addresses_size + SIZ_FOR_REALLOC));
			if (!found->address_values) NotEnoughMemory();

		}
		if (found->isEntry)
			if (found->address_values[0] == ENTRY_GLOB_ADDRESS)
				found->address_values[0] = address_value;
			else setErrorFlag("You can't define a symbol twice", false);
		return true;
	}
	return false;
}

void insertSymbol(symListP symbols, string name, int address_value, bool is_extern, bool is_entry, bool is_command, bool is_data)
{
	symp insert = (symp)malloc(sizeof(syms));
	if (!insert) NotEnoughMemory();

	/* if symbol already exists inserts one more address, in case of entry sets the address */
	if (insertAddressInSymbols(symbols, name, address_value)) return;

	insert->addresses_size = 0;
	insert->address_values = malloc(sizeof(int)* SIZ_FOR_REALLOC);
	if (!insert->address_values) NotEnoughMemory();
	insert->address_values[insert->addresses_size++] = address_value;
	strcpy(&(insert->name), name);
	insert->isExtern = is_extern;
	insert->isCommand = is_command;
	insert->isEntry = is_entry;
	insert->isData = is_data; 

	if (symbols->start == NULL)
	{
		symbols->start = insert;
		symbols->start->next = NULL;
		symbols->end = insert;
	}
	else
	{
		symbols->end->next = insert;
		symbols->end = symbols->end->next;
		symbols->end->next = NULL;
	}
	symbols->size++;

}

bool isDatas(string command)
{
	return (!strcmp(command, ".data") || !strcmp(command, ".string") || !strcmp(command, ".mat") );
}

bool isExtern(string command)
{
	return !strcmp(command, ".extern");
}

bool isEntry(string command)
{
	return !strcmp(command, ".entry");
}

bool isSymoblExtern(symList symbols, string name)
{
	symp runner = symbols.start;
	for (; runner != NULL; runner = runner->next) if (runner->isExtern) return true;
	return false;
}

void firstMove(string data, symListP symbols, string dataSegment, string commandSegment, int* DC, int* IC)
{
	int last_length_data = strlen(dataSegment), last_length_commands = strlen(commandSegment);
	string command = data;
	string args = increaseArgument(command, ' ');
	string args2 = NULL;
	int LAST_IC = *IC, LAST_DC = *DC, L = 0, EXTERN_ADDRESS = 0;

	if (is_symbol(command))
	{
		args2 = increaseArgument(args, ' ');
		if (isSymoblExtern(*symbols, command)) puts("Cannot set extern symbol data");
		if (isDatas(args) && handleDatas(dataSegment, args, args2, DC) && ++(*IC))
			insertSymbol(symbols, command, LAST_IC, false, false, false, true);
		else if (isEntry(args)) 
			setErrorFlag("Warning: label can't define an entry", false); /* Mustn't be ! */
		else if (isExtern(args)) 
			setErrorFlag("Warning: label can't define an extern", false);
		else if (handleCommand(commandSegment, args2, getCommandIndex(args), IC))
			insertSymbol(symbols, command, LAST_IC, false, false, true,false);
	}
	else
	{
		if (isDatas(command) && ++(IC)) handleDatas(dataSegment, command, args, DC);
		else if (isEntry(command))
		{
			entry_flag = true;
			insertSymbol(symbols, args, ENTRY_GLOB_ADDRESS, false, true, false, false);
		}
		else if (isExtern(command) && ++EXTERN_ADDRESS)
		{
			extern_flag = true;
			insertSymbol(symbols, args, &EXTERN_ADDRESS, true, false, false, false);
		}
		else handleCommand(commandSegment, args, getCommandIndex(command), IC);
	}
	if (errorFlag)
	{
		if (last_length_commands != strlen(commandSegment)) commandSegment[last_length_commands] = 0;
		if (last_length_data != strlen(dataSegment)) dataSegment[last_length_data] = 0;
		errorFlag = false;
	}
}


char reVal(int num)
{
	if (num >= 0 && num <= 9)
		return (char)(num + '0');
	else
		return (char)(num - 10 + 'A');
}

// Utility function to reverse a string
void strev(char *str, int size, int last_index)
{
	int i;
	for (i = 0; i < (size / 2); i++)
	{
		char temp = str[last_index + i];
		str[last_index + i] = str[last_index + size - i - 1];
		str[last_index + size - i - 1] = temp;
	}
}


// Function to convert a given decimal number
// to a base 'base' and
void fromDeci(char res[], int base, int inputNum, int size)
{
	fromDeciFromIndex(res, base, inputNum, strlen(res), size, true);
}

void fromDeciFromIndex(char res[], int base, int inputNum, int start_index,int size, bool is_end_of_string)
{
	int last_index = start_index;
	/* dividing it by base and taking remainder */
	while (inputNum > 0)
	{
		res[start_index++] = reVal(inputNum % base);
		inputNum /= base;
		if (base == BASE_FOUR) res[start_index - 1] = (res[start_index - 1] - '0' + 'a');
	}
	while ((start_index - last_index) != size) {
		if (base == BASE_FOUR) res[start_index] = 'a';
		res[start_index++] = '0';
	}
	

	if (is_end_of_string) res[start_index] = '\0';
	strev(res, size, last_index); // Reverse the result

}


void setErrorFlag(string errorMsg, bool endProg)
{
	fprintf(stderr, "%s in line: %d\n", errorMsg, line_counter);
	errorFlag = true;
	if (endProg)
	{
		system("pause");
		exit(1);
	}
}
void NotEnoughMemory()
{
	setErrorFlag("Not enough memory. Cannot allocate", true);
}

char getRepresentiveBaseCharFromBinary(string data, int to_base, int* index)
{
	int max_mult_2 = to_base / 2;
	char calc_res = 'a';
	while (max_mult_2)
	{
		calc_res += ((data[*index] - '0')*max_mult_2);
		max_mult_2 /= 2;
		(*index)++;
	}
	return calc_res;
}