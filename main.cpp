#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <cstdint>
#include <iomanip>
#include <chrono>

#include "number_parser.h"

int main() {
	// 注意，C++中后定义的对象先释放内存。如果out_buffer在fout之后定义，则会在fout关闭前就被释放，fout会因找不到buffer地址而报错！
	const int buffer_size = 1024 * 1024 * 8;  //设置缓冲区8MB
	std::vector<char> out_result_buffer(buffer_size);
	std::vector<char> out_errors_buffer(buffer_size);

	std::string test_path = "data_250m.txt";
	std::ifstream fin(test_path);
	std::ofstream fout_errors("errors.txt");
	std::ofstream fout_result("result.txt");
	if (!fin) {
		std::cerr << "待排序文件打开失败";
		return 1;
	}
	if (!fout_errors || !fout_result) {
		std::cerr << "待写入文件打开失败";
		return 1;
	}

	// 指定缓冲区大小
	fout_result.rdbuf()->pubsetbuf(out_result_buffer.data(), out_result_buffer.size());
	fout_errors.rdbuf()->pubsetbuf(out_errors_buffer.data(), out_errors_buffer.size());

	auto time_begin = std::chrono::steady_clock::now();
	auto time_end = std::chrono::steady_clock::now();

	std::vector<uint64_t> keys;  //存储每个数映射后的结果。保证key越大时，原数值越大，且可通过key倒推原值。
	keys.reserve(10000000);
	ANum num_struct;

	bool is_eof = false;    // 是否读到文件结尾
	std::string before_half;   // 记录被块截断的半行字符
	std::vector<char> block_buffer(buffer_size);

	// 按块循环读入数据
	while (!is_eof) {
		//读入一块数据
		fin.read(block_buffer.data(), buffer_size);    // 读入数据到block_buffer中

		uint64_t bytes_read = fin.gcount();   // 获取实际读入的数据量
		const char* block_begin = block_buffer.data();
		const char* block_end = block_buffer.data() + bytes_read;   // 设置起止指针
		const char* str_begin = block_buffer.data();   // 当前读入字符串的初始位置
		const char* p = block_buffer.data();   // 遍历指针

		if (bytes_read < buffer_size) {   //文件读入完成
			is_eof = true;
		}

		// 先处理之前被截断的字符
		// 找'\n' 把之前的半个数字拼全
		if (!before_half.empty()) {
			while (p != block_end) {
				if (*p != '\n') {
					++p;
					continue;
				}
				break;    //p指向 \n
			}

			// 整个块都没有读到换行符（有超超超超长数字串）
			if (p == block_end) {
				before_half.append(block_begin, block_end);
				continue;
			}

			std::string later_half;
			later_half.assign(block_begin, p);
			std::string complete_num = before_half + later_half;  // 拼接成完整数字

			// 把当前值放进去
			num_struct.sign = 0;  //1为正号，0为负号
			num_struct.base = 0;
			num_struct.exponent = 0;
			num_struct.is_standard_float = 1;

			//数字不合法，写入errors.txt文件
			if (!NumberParser(complete_num.data(), complete_num.data() + complete_num.size(), num_struct)) {
				fout_errors.write(complete_num.data(), complete_num.size());    //写入有误的字符串
				fout_errors.put('\n');
			}
			//数字合法，计算key并插入keys待排序
			else {
				keys.push_back(GetKey(num_struct));
			}

			++p;
			str_begin = p;
			before_half.clear();
		}

		// 处理整行的数字串
		while (p != block_end) {
			if (*p != '\n') {
				++p;
				continue;
			} 
			if (str_begin == p) {    //当前是空行
				++p;
				str_begin = p;
				continue;
			}

			// 初始化数字结构体
			num_struct.sign = 0;  //1为正号，0为负号
			num_struct.base = 0;
			num_struct.exponent = 0;
			num_struct.is_standard_float = 1;

			//数字不合法，写入errors.txt文件
			if (!NumberParser(str_begin, p, num_struct)) {
				fout_errors.write(str_begin, p - str_begin);    //写入有误的字符串
				fout_errors.put('\n');
			}
			//数字合法，计算key并插入keys待排序
			else {
				keys.push_back(GetKey(num_struct));
			}

			++p;
			str_begin = p;
		}
		// 记录下被截断了的半个数
		if (str_begin != block_end) {
			before_half.assign(str_begin, block_end);
		}
	}

	// 如果文件结尾没有'\n'，则最后一个数字没有被处理
	if (!before_half.empty()) {
		num_struct.sign = 0;  //1为正号，0为负号
		num_struct.base = 0;
		num_struct.exponent = 0;
		num_struct.is_standard_float = 1;

		//数字不合法，写入errors.txt文件
		if (!NumberParser(before_half.data(), before_half.data() + before_half.size(), num_struct)) {
			fout_errors.write(before_half.data(), before_half.size());    //写入有误的字符串
			fout_errors.put('\n');
		}
		//数字合法，计算key并插入keys待排序
		else {
			keys.push_back(GetKey(num_struct));
		}
	}


	time_end = std::chrono::steady_clock::now();
	std::cout << "[" << std::chrono::duration_cast<std::chrono::milliseconds>(time_end - time_begin).count() << "ms] key映射完成 \n";
	
	//排序
	//std::sort(keys.begin(), keys.end());
	RadixSort64(keys);

	time_end = std::chrono::steady_clock::now();
	std::cout << "[" << std::chrono::duration_cast<std::chrono::milliseconds>(time_end - time_begin).count() << "ms] key排序完成 \n";


	for (auto& key : keys) {
		KeyParser(key, num_struct);

		int kMod1E9 = 1'000'000'000;
		if (num_struct.base == 0) {
			fout_result << "0.000000000E+000\n";   //单独输出+0E+0
		}
		else {
			// 采用格式化后的char数组一次写入最终结果
			short int_part = num_struct.base / kMod1E9;   //底数的整数部分
			uint64_t factor_part = num_struct.base % kMod1E9;   //底数的小数部分

			// 格式化后共17个字符，再补上'\n'，总计18个字符。
			char buffer[18];
			buffer[0] = (num_struct.sign == 0) ? '-' : '+';
			buffer[1] = '0' + int_part;
			buffer[2] = '.';
			for (int i = 11;i >= 3;i--) {   // 下标3~11为小数部分
				buffer[i] = '0' + (factor_part % 10);
				factor_part /= 10;
			}
			buffer[12] = 'E';
			buffer[13] = (num_struct.exponent >= 0) ? '+' : '-';
			int abs_exp = num_struct.exponent;
			if (abs_exp < 0)   // 对指数取绝对值，便于写入数组
				abs_exp = -abs_exp;
			for (int i = 16;i >= 14;i--) {   // 下标14~16为指数部分
				buffer[i] = '0' + (abs_exp % 10);
				abs_exp /= 10;
			}
			buffer[17] = '\n';
		
			fout_result.write(buffer, 18);
		}

	}

	time_end = std::chrono::steady_clock::now();
	std::cout << "[" << std::chrono::duration_cast<std::chrono::milliseconds>(time_end - time_begin).count() << "ms] 结果输出完成";

	return 0;
}