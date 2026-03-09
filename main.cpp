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
  
	auto time_begin = std::chrono::steady_clock::now();
	auto time_end = std::chrono::steady_clock::now();

	std::vector<uint64_t> keys;  //存储每个数映射后的结果。保证key越大时，原数值越大，且可通过key倒推原值。
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

		//StandardOut(mynum);
		int mod_1E9 = 1000000000;
		// 输出符号
		if (num_struct.sign == 0)
			fout_result << '-';
		else
			fout_result << '+';

		// 输出底数、指数
		if (num_struct.base == 0) {
			fout_result << "0.000000000E+000";   //单独输出+0E+0
		}
		else {
			fout_result << std::fixed << std::setprecision(9) << num_struct.base / (double)mod_1E9;
			fout_result << "E";
			if (num_struct.exponent >= 0)  fout_result << "+";
			else fout_result << "-";
			if (num_struct.exponent > -10 && num_struct.exponent < 10) {
				fout_result << "00" << std::max(num_struct.exponent, -num_struct.exponent);
			}
			else if (num_struct.exponent > -100 && num_struct.exponent < 100) {
				fout_result << "0" << std::max(num_struct.exponent, -num_struct.exponent);
			}
			else {
				fout_result << std::max(num_struct.exponent, -num_struct.exponent);
			}
		}

		fout_result << '\n';
	}

	time_end = std::chrono::steady_clock::now();
	std::cout << "[" << std::chrono::duration_cast<std::chrono::milliseconds>(time_end - time_begin).count() << "ms] 结果输出完成";



	return 0;
}