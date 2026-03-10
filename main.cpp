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
	// 注意，C++中后定义的对象先释放内存。如果in_buffer在fin之后定义，则会在fin关闭前就被释放，fin会找不到buffer地址报错！
	int buffer_size = 1024 * 1024 * 32;  //设置缓冲区32MB
	std::vector<char> in_buffer(buffer_size);
	std::vector<char> out_result_buffer(buffer_size);
	std::vector<char> out_errors_buffer(buffer_size);

	std::string test_path = "data_10m.txt";
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
	fin.rdbuf()->pubsetbuf(in_buffer.data(), in_buffer.size());
	fout_result.rdbuf()->pubsetbuf(out_result_buffer.data(), out_result_buffer.size());
	fout_errors.rdbuf()->pubsetbuf(out_errors_buffer.data(), out_errors_buffer.size());

	auto time_begin = std::chrono::steady_clock::now();
	auto time_end = std::chrono::steady_clock::now();

	std::vector<uint64_t> keys;  //存储每个数映射后的结果。保证key越大时，原数值越大，且可通过key倒推原值。
	keys.reserve(10000000);
	std::string line;
	ANum num_struct;
	while (std::getline(fin, line)) {
		if (line.empty()) continue;
		 
		num_struct.sign = 0;  //1为正号，0为负号
		num_struct.base = 0;
		num_struct.exponent = 0;
		num_struct.is_standard_float = 1;

		//数字不合法，写入errors.txt文件
		if (!NumberParser(line, num_struct)) {
			fout_errors << line <<'\n'; 
		}
		//数字合法，输出标准化后的结果。并计算key
		else {
			keys.push_back(GetKey(num_struct));
		}
	}

	time_end = std::chrono::steady_clock::now();
	std::cout << "[" << std::chrono::duration_cast<std::chrono::milliseconds>(time_end - time_begin).count() << "ms] key映射完成 \n";
	
	//排序
	std::sort(keys.begin(), keys.end());
	time_end = std::chrono::steady_clock::now();
	std::cout << "[" << std::chrono::duration_cast<std::chrono::milliseconds>(time_end - time_begin).count() << "ms] key排序完成 \n";



	for (auto& key : keys) {
		KeyParser(key, num_struct);

		int mod_1E9 = 1000000000;
		if (num_struct.base == 0) {
			fout_result << "0.000000000E+000\n";   //单独输出+0E+0
		}
		else {
			// 采用格式化后的char数组一次写入最终结果
			uint64_t int_part = num_struct.base / mod_1E9;   //底数的整数部分
			uint64_t factor_part = num_struct.base % mod_1E9;   //底数的小数部分

			char buffer[64];
			snprintf(buffer, sizeof(buffer), "%c%1llu.%09lluE%+03d\n",
				(num_struct.sign == 0 ? '-' : '+'),  //写入符号
				int_part,
				factor_part,   //写入底数的小数部分，共9位，不足用0补齐
				num_struct.exponent   //指数。共3位，不足的用0补齐，并强制显示符号(%+)
			);

			fout_result << buffer;
		}

	}

	time_end = std::chrono::steady_clock::now();
	std::cout << "[" << std::chrono::duration_cast<std::chrono::milliseconds>(time_end - time_begin).count() << "ms] 结果输出完成";

	return 0;
}