#ifndef NUMBER_PARSER_H
#define NUMBER_PARSER_H

#include <string>

struct ANum {
	bool sign = false;  //true为正号，false为负号。
	uint64_t base = 0;
	int exponent = 0;
	bool is_standard_float = 1;
};
bool NumberParser(const char* begin_ptr, const char* end_ptr , ANum& num_struct);
bool IsNumOrNot(char c);
uint64_t GetKey(ANum& num_struct);
void KeyParser(uint64_t key, ANum& num_struct);
bool RoundBase(ANum& num_struct, uint16_t round);
void RadixSort64(std::vector<uint64_t>& keys);

#endif