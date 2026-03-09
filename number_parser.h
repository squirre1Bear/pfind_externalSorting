#ifndef NUMBER_PARSER_H
#define NUMBER_PARSER_H

#include <string>

struct ANum {
	bool sign = 0;  //1为正号，0为负号。
	uint64_t base = 0;
	int exponent = 0;
	bool is_standard_float = 1;
};
bool NumberParser(std::string& num_str, ANum& num_struct);
bool IsNumOrNot(char c);
uint64_t GetKey(ANum& num_struct);
void KeyParser(uint64_t key, ANum& num_struct);
bool RoundBase(ANum& num_struct, uint16_t round);
bool ParseSign(std::string& num_str, ANum& num_struct, int& cur_index, int& decimal_point_index);
void StandardOut(ANum& num_struct);

#endif