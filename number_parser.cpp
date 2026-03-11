#include<iostream>
#include<bitset>
#include<algorithm>
#include<iomanip>
#include<vector>
#include<cstdint>

#include "number_parser.h"


inline bool IsNumOrNot(char c) {
	return c - '0' >= 0 && c - '0' <= 9;
}

bool NumberParser(const char* begin_ptr, const char* end_ptr, ANum& num_struct) {
	const char* cur_ptr = begin_ptr;
	int decimal_point_index = -1;
	int significant_digit = 0;   //总的有效位数
	int first_non_zero_index = -1;   //首个非零元素下标
	bool has_exponent = false;     //是否已经读到E e 
	bool exp_has_carry_bit = false;   //exp是否有进位
	const int num_str_size = end_ptr - begin_ptr;

	if (begin_ptr == end_ptr)  return false;   //空串

	// 读取符号
	if (*cur_ptr == '-') {
		num_struct.sign = 0;
		cur_ptr++;
		// [如果不在这里处理长度为1的情况，则有cur_index >= num_str.size()，无法进入后面的有效位数判断循环，导致无法处理单一符号的情况]
		if (num_str_size == 1 || (!IsNumOrNot(*cur_ptr) && *cur_ptr != '.'))   //只读到了一个负号，或是负号之后没有跟数字 / 小数点
			return false;
	}
	else if (*cur_ptr == '+') {
		num_struct.sign = 1;
		cur_ptr++;
		if (num_str_size == 1 || (!IsNumOrNot(*cur_ptr) && *cur_ptr != '.'))   //只读到了一个正号，或是之后没有跟数字 / 小数点
			return false;
	}
	else if (IsNumOrNot(*cur_ptr)) {
		num_struct.sign = 1;
	}
	else if (*cur_ptr == '.') {   //省略0的形式，如 .5
		decimal_point_index = 0;
		cur_ptr++;
		num_struct.sign = 1;
		if (num_str_size == 1 || !IsNumOrNot(*cur_ptr))   //只读到了一个小数点，或是小数点之后没有跟数字
			return false;
	}
	else return false;


	// 解析底数
	// 先去除前导零
	while (cur_ptr < end_ptr && significant_digit == 0) {
		// 读到前导零
		if (significant_digit == 0 && *cur_ptr - '0' == 0) {
			cur_ptr++;
			continue;
		}
		// 读到小数点
		else if (*cur_ptr == '.') {
			if (decimal_point_index != -1) return false; //之前出现过了小数点
			decimal_point_index = cur_ptr - begin_ptr;
			
			//注意保证小数点前或后面有数字，防止出现单独小数点的情况
			bool is_valid_decimal_point = false;
			if (cur_ptr - begin_ptr >= 1 && IsNumOrNot(*(cur_ptr - 1)))  //前面有数字
				is_valid_decimal_point = true;
			if (cur_ptr + 1 < end_ptr && IsNumOrNot(*(cur_ptr + 1)))   //后面有数字
				is_valid_decimal_point = true;
			
			if (!is_valid_decimal_point)
				return false;    //小数点前后均无数字
		}
		// 读到前导零之外的数字，即首个有效数字
		else if (IsNumOrNot(*cur_ptr)) {
			significant_digit++;
			num_struct.base = (uint64_t)(*cur_ptr - '0');
			first_non_zero_index = cur_ptr - begin_ptr;
		}
		// 读到E （此时仅包含底数为0的情况。如 0E6、-0e3 ）
		else if (*cur_ptr == 'e' || *cur_ptr == 'E') {
			has_exponent = true;
			num_struct.base = 0;
			cur_ptr++;
			break;   //进入指数解析代码
		}
		else
			return false;

		cur_ptr++;
	} //此时cur_index指向第一个有效数字的下一位

	// 去除前导零后，解析底数
	while (cur_ptr < end_ptr && has_exponent == false) {
		//读到小数点
		if (*cur_ptr == '.') {
			if (decimal_point_index != -1)    //读入多个小数点，报错
				return false;
			decimal_point_index = cur_ptr - begin_ptr;
			cur_ptr++;
		}
		//读到数字
		else if (IsNumOrNot(*cur_ptr)) {
			if (significant_digit == 10) {   //此时已经读到10个有效位数，再读入数字需要进行四舍五入判断
				if(RoundBase(num_struct, (*cur_ptr) - '0'))
					exp_has_carry_bit = true;
			}
			else if (significant_digit < 10) {
				num_struct.base *= 10;
				num_struct.base += (uint64_t)((*cur_ptr) - '0');
			}

			// 注：有效位数 > 10的时候也需要继续读入，以防后面出现非法字符

			cur_ptr++;
			significant_digit++;
		}
		//读到E或e
		else if (*cur_ptr == 'E' || *cur_ptr == 'e') {
			if (has_exponent == true)   return false;   //之前已经读到了e
			has_exponent = true;   
			cur_ptr++;
			num_struct.is_standard_float = 0;

			if (cur_ptr >= end_ptr)  return false;
			
			break;   //进入指数解析部分
		}
		else return false;  //读到了 数字、小数点、E e之外的内容
	}


	// 解析指数
	if (has_exponent == true) {
		// 指数是负数，记录指数的时候需要用减法
		if (cur_ptr < end_ptr && *cur_ptr == '-') {
			cur_ptr++;
			if (cur_ptr >= end_ptr)  return false;   //读到负号之后没有跟数字
			while (cur_ptr < end_ptr) {
				if (IsNumOrNot(*cur_ptr)) {
					num_struct.exponent *= 10;
					num_struct.exponent -= (uint64_t)((*cur_ptr) - '0');
					cur_ptr++;
				}
				else return false;
			}
		}
		// 指数是正数（读入正号，或数字），记录指数的时候需要用加法
		else if (cur_ptr < end_ptr && (IsNumOrNot(*cur_ptr) || *cur_ptr == '+')) {
			if (*cur_ptr == '+') cur_ptr++;
			if (cur_ptr >= end_ptr)  return false;   //读到正号之后没有跟数字
			while (cur_ptr < end_ptr) {
				if (IsNumOrNot(*cur_ptr)) {
					num_struct.exponent *= 10;
					num_struct.exponent += (uint64_t)((*cur_ptr) - '0');
					cur_ptr++;
				}
				else return false;
			}
		}
		else return false;   //指数部分读到 数字、正负号之外的内容
	}

	// 只读到了0，那有效数字就是0
	if (significant_digit == 0) {
		num_struct.sign = 1;
		num_struct.base = 0;
		num_struct.exponent = -1000;   //由于0被设置为了正数，且正数比大小时先比exp，exp越大的数越大。0作为正数中最小的，设置exp为最小值
		return true;
	}


	//改变exp，将底数化为 x.xxx 的形式

	if (decimal_point_index != -1) {
		if (decimal_point_index < first_non_zero_index) {
			num_struct.exponent += (decimal_point_index - first_non_zero_index);
		}
		else if (decimal_point_index > first_non_zero_index) {
			num_struct.exponent += (decimal_point_index - first_non_zero_index - 1);
		}
	}
	else {   //没有小数点
		num_struct.exponent += significant_digit - 1;
	}
	// 指数的进位
	if (exp_has_carry_bit)
		num_struct.exponent++;

	//底数补齐10位             //1e9
	while (num_struct.base < 1'000'000'000ULL && num_struct.base != 0) {
		num_struct.base *= 10;
	}

	return true;
}


// 在exp有进位的时候，返回true，表示exp需要+1。
bool RoundBase(ANum& num_struct, uint16_t round) {  //四舍五入base
	if (round < 5) return false;   //最后一位数字小于5，直接舍掉

	num_struct.base++;    //注意，base已经有10位有效数字了，如果产生指数进位，则base一定会变成1e10
	if (num_struct.base == 10'000'000'000ULL) {   //1e10
		//num_struct.exponent++;   //注意！不要在这里加exp，后面读exp的时候会有 *= 10的操作，需要保证exp初始值为0！
		num_struct.base /= 10;
		return true;
	}
	return false;
}

// 这里我用uint64_t存储映射后的值key，保证key越大时，原值越大
/*
	从低位起：1~52位为base、53~63位为exponent、64为sign
	key生成思路：exp、base先整合成一个63位的exp_base。
				如果符号为+，则exp_base直接放到1~63位。
				如果符号为-，则MAX_EXP_BASE(63个1) - exp_base放到1~63位。
*/
uint64_t GetKey(ANum& num_struct) {
	uint64_t MAX_EXP_BASE = (1ULL << 63) - 1;
	uint64_t exp_base = ((uint64_t)(num_struct.exponent + 1000) << 52) + num_struct.base;   // 从低位起 1~52位为base、53~63位为exponent
	uint64_t key = 0;
	//当前值为正数
	if (num_struct.sign == 1) {
		key = 1ULL << 63;  //最高位符号为1
		key = key | exp_base;
	}
	else {   //当前值为负数
		//key = 0ULL << 63;    
		key = MAX_EXP_BASE ^ exp_base;
	}

	return key;
}


// 从key反推原值
void KeyParser(uint64_t key, ANum& num_struct) {
	uint64_t MAX_EXP_BASE = (1ULL << 63) - 1;
	uint64_t base_mask = (1ULL << 52) - 1;
	uint64_t exp_mask = ((1ULL << 63) - 1) - base_mask;

	if (key >= (1ULL << 63)) {   //原值为正数
		num_struct.sign = 1;
		num_struct.base = key & base_mask;
		num_struct.exponent = ((key & exp_mask) >> 52) - 1000;
	}
	else {
		num_struct.sign = 0;
		key = MAX_EXP_BASE - key;
		num_struct.base = key & base_mask;
		num_struct.exponent = ((key & exp_mask) >> 52) - 1000;
	}

	return;
}


void RadixSort64(std::vector<uint64_t>& keys) {
	const int bits_per_pass = 16;  //每个桶包括了几位数字
	const int pass_num = (64 + bits_per_pass -1) / bits_per_pass;    //总共进行几趟基数排序，进行了向上取整
	const uint64_t radix = 1ULL << bits_per_pass;   //桶的个数
	const uint64_t mask = radix - 1;   //用于取出特定段的数字

	std::vector<uint64_t> temp(keys.size());    // 排序的桶

	for (int pass = 0; pass < pass_num;pass++) {
		std::vector<int> count_index(radix + 1);
		int shift = bits_per_pass * pass;
		for (auto key : keys) {
			uint64_t key_masked = (key >> shift) & mask;
			count_index[key_masked + 1]++;   //记录每个值分别有多少个，值为key的存到下标(key+1)位置，方便后面求每个值的起始下标
		}

		// 统计各值起始位置
		for (uint64_t i = 1;i < radix;i++) {
			count_index[i] += count_index[i - 1];
		}

		// 把数字放进temp数组中，完成一趟基数排序
		for (auto key : keys) {
			uint64_t key_masked = (key >> shift) & mask;
			temp[count_index[key_masked]] = key;
			count_index[key_masked]++;
		}

		keys.swap(temp);
	}
	return;
}