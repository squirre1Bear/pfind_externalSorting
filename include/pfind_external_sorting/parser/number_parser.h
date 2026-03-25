#ifndef EXTERNAL_SORT_PARSER_NUMBER_PARSER_H_
#define EXTERNAL_SORT_PARSER_NUMBER_PARSER_H_

#include <cstdint>
#include <string>

#include "pfind_external_sorting/model.h"

namespace external_sort {
namespace parse {
// 初始化int—>string的表格
void InitDigit3();  

// 传入string类型的原始数字，返回解析完的ParsedNumber类型数字
model::ParsedNumber NumberParser(std::string_view origin_number_view);

// 判断字符是否为0~9的数字，如果是则返回true
bool IsDigit(char c);

// 生成数字的key
// 传入ParsedNumber类型数字，返回uint64_t类型的key
uint64_t GetKey(model::ParsedNumber& parsed_number);

// 通过key解析出原数字
// 输入key和一个ParsedNumber结构体地址，直接修改ParsedNumber内部的值
void KeyParser(uint64_t key, model::ParsedNumber& parsed_number);

// 四舍五入。当产生进位时会修改parsed_number的底数，并返回true表示指数需要进位
// round为需要舍/入的数字
// 注:
// 后面解析指数时有移位操作，不能直接在本函数中让指数+1。需要在指数解析完成后在+1
bool RoundBase(model::ParsedNumber& parsed_number, uint16_t round);

// 根据key获取标准输出格式的数字
// 结果输出到formatted_num参数中
void GetFormattedNumber(char* formatted_num, uint64_t key);

// 初始化ParsedNumber结构体
void InitParsedNumber(model::ParsedNumber& parsed_number);

}  // namespace parse
}  // namespace external_sort

#endif