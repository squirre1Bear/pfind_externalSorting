#ifndef EXTERNAL_SORT_PARSER_NUMBER_PARSER_H_
#define EXTERNAL_SORT_PARSER_NUMBER_PARSER_H_

#include <cstdint>
#include <cstring>
#include <string>
#include <string_view>

#include "pfind_external_sorting/model.h"

namespace external_sort {
namespace parse {

constexpr int kFormattedNumberBytes = 18;
extern char digit_table_3[1000][3];

// 初始化int—>string的表格
void InitDigit3();

// 传入string类型的原始数字，返回解析完的ParsedNumber类型数字
model::ParsedNumber NumberParser(std::string_view origin_number_view);

// 判断字符是否为0~9的数字，如果是则返回true
bool IsDigit(char c);

// 通过key解析出原数字
// 输入key和一个ParsedNumber结构体地址，直接修改ParsedNumber内部的值
void KeyParser(uint64_t key, model::ParsedNumber& parsed_number);

// 初始化ParsedNumber结构体
void InitParsedNumber(model::ParsedNumber& parsed_number);


// 根据key获取标准输出格式的数字
// 结果输出到formatted_num参数中
// 参数: char* formatted_num, 存储格式化后的数字，用于输出
//       uint64_t key. 数字对应的key
inline void GetFormattedNumber(char* formatted_number, uint64_t key) {
  // 在本函数直接进行key的解析，避免声明parsednumber、调用一次函数
  // 从key解析底数、指数
  uint64_t kMaxExpBase = (1ULL << 63) - 1;
  uint64_t kBaseMask = (1ULL << 52) - 1;
  uint64_t exp_mask = ((1ULL << 63) - 1) - kBaseMask;
  uint64_t kMod1E9 = 1'000'000'000;

  bool is_positive = true;
  uint64_t base;
  int exponent;

  if (key >= (1ULL << 63)) {  // 原值为正数
    is_positive = true;
    base = key & kBaseMask;
  } else {
    is_positive = false;
    key = kMaxExpBase - key;
    base = key & kBaseMask;
  }
  exponent = ((key & exp_mask) >> 52) - 1000;

  if (base == 0) {
    memcpy(formatted_number, "+0.000000000E+000\n", 18);  // 单独输出+0E+0
    return;
  } else {
    // 采用格式化后的char数组一次写入最终结果
    uint32_t int_part = base / kMod1E9;     // 底数的整数部分
    uint32_t factor_part = base % kMod1E9;  // 底数的小数部分

    // 格式化后共17个字符，再补上'\n'，总计18个字符。
    formatted_number[0] = (is_positive == 0) ? '-' : '+';
    formatted_number[1] = '0' + int_part;
    formatted_number[2] = '.';

    // 查表获取string类型的小数部分
    uint64_t high3 = factor_part / 1'000'000;  // 高三位
    uint64_t mid3 = (factor_part / 1000) % 1000;
    uint64_t low3 = factor_part % 1000;  // 低三位

    memcpy(formatted_number + 3, digit_table_3[high3], 3);
    memcpy(formatted_number + 6, digit_table_3[mid3], 3);
    memcpy(formatted_number + 9, digit_table_3[low3], 3);

    formatted_number[12] = 'E';
    formatted_number[13] = (exponent >= 0) ? '+' : '-';
    int abs_exp = exponent;
    if (abs_exp < 0)  // 对指数取绝对值，便于写入数组
      abs_exp = -abs_exp;

    // 查表获取string类型的指数
    memcpy(formatted_number + 14, digit_table_3[abs_exp], 3);

    formatted_number[17] = '\n';
  }
  return;
}

// 生成数字的key
// 传入ParsedNumber类型数字，返回uint64_t类型的key
// 这里我用uint64_t存储映射后的值key，保证key越大时，原值越大
/*
        从低位起：1~52位为base、53~63位为exponent、64为sign
        key生成思路：exp、base先整合成一个63位的exp_base。
                                如果符号为+，则exp_base直接放到1~63位。
                                如果符号为-，则MAX_EXP_BASE(63个1) -
   exp_base放到1~63位。
*/
inline uint64_t GetKey(model::ParsedNumber& num_struct) {
  constexpr uint64_t kMaxExpBase = (1ULL << 63) - 1;
  const uint64_t exp_base =
      (static_cast<uint64_t>(num_struct.exponent + 1000) << 52) +
      num_struct.base;  // 从低位起 1~52位为base、53~63位为exponent

  if (num_struct.is_positive) {      // 当前值为正数
    return (1ULL << 63) | exp_base;  // 最高位符号为1
  }
  return kMaxExpBase ^ exp_base;
}

// 直接传入底数、指数，避免生成ParsedNumber结构体
inline uint64_t GetKey(uint64_t& base, int& exponent, bool is_positive) {
  const uint64_t kMaxExpBase = (1ULL << 63) - 1;
  uint64_t exp_base = (static_cast<uint64_t>(exponent + 1000) << 52) +
                      base;  // 从低位起 1~52位为base、53~63位为exponent
  uint64_t key = 0;
  // 当前值为正数
  if (is_positive == 1) {
    key = 1ULL << 63;  // 最高位符号为1
    key = key | exp_base;
  } else {  // 当前值为负数
    // key = 0ULL << 63;
    key = kMaxExpBase ^ exp_base;
  }

  return key;
}

// 四舍五入。当产生进位时会修改parsed_number的底数，并返回true表示指数需要进位
// round为需要舍/入的数字
// 注:后面解析指数时有移位操作，不能直接在本函数中让指数+1。需要在指数解析完成后在+1
inline bool RoundBase(uint64_t& base, uint16_t round) {
  if (round < 5) return false;  // 最后一位数字小于5，直接舍掉

  base++;  // 注意，base已经有10位有效数字了，如果产生指数进位，则base一定会变成1e10
  if (base == 10'000'000'000ULL) {
    // num_struct.exponent++;   //注意！不要在这里加exp，后面读exp的时候会有 *=
    // 10的操作，需要保证exp初始值为0！
    base /= 10;
    return true;
  }
  return false;
}

// 传入string_view类型的原始字符串，解析出key。
// 如果字符串不合法，则返回false，写入errors.txt
inline bool ParseLineToKey(std::string_view sv, uint64_t& key) {
  const char* begin_ptr = sv.data();
  const char* end_ptr = begin_ptr + sv.size();
  const char* cur_ptr = begin_ptr;

  uint64_t base = 0;
  int exponent = 0;

  int decimal_point_index = -1;
  int significant_digit = 0;       // 总的有效位数
  int first_non_zero_index = -1;   // 首个非零元素下标
  bool has_exponent = false;       // 是否已经读到E e
  bool exp_has_carry_bit = false;  // exp是否有进位
  bool is_positive = true;
  bool is_standard_float = true;  // 是否为标准浮点表述
  const int num_str_size = end_ptr - begin_ptr;

  if (begin_ptr == end_ptr) {
    return false;  // 空串
  }
  // 读取符号
  if (*cur_ptr == '-') {
    is_positive = false;
    cur_ptr++;
    // [如果不在这里处理长度为1的情况，则有cur_index >=
    // num_str.size()，无法进入后面的有效位数判断循环，导致无法处理单一符号的情况]
    if (num_str_size == 1 ||
        (!IsDigit(*cur_ptr) &&
         *cur_ptr !=
             '.')) {  // 只读到了一个负号，或是负号之后没有跟数字 / 小数点
      return false;
    }
  } else if (*cur_ptr == '+') {
    is_positive = true;
    cur_ptr++;
    if (num_str_size == 1 ||
        (!IsDigit(*cur_ptr) &&
         *cur_ptr != '.')) {  // 只读到了一个正号，或是之后没有跟数字 / 小数点
      return false;
    }
  } else if (IsDigit(*cur_ptr)) {
    is_positive = 1;
  } else if (*cur_ptr == '.') {  // 省略0的形式，如 .5
    decimal_point_index = 0;
    cur_ptr++;
    is_positive = 1;
    if (num_str_size == 1 ||
        !IsDigit(*cur_ptr)) {  // 只读到了一个小数点，或是小数点之后没有跟数字
      return false;
    }
  } else {
    return false;
  }

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
      if (decimal_point_index != -1) {
        return false;
      }  // 之前出现过了小数点
      decimal_point_index = cur_ptr - begin_ptr;

      // 注意保证小数点前或后面有数字，防止出现单独小数点的情况
      bool is_valid_decimal_point = false;
      if (cur_ptr - begin_ptr >= 1 && IsDigit(*(cur_ptr - 1)))  // 前面有数字
        is_valid_decimal_point = true;
      if (cur_ptr + 1 < end_ptr && IsDigit(*(cur_ptr + 1)))  // 后面有数字
        is_valid_decimal_point = true;

      if (!is_valid_decimal_point) {
        return false;
      }  // 小数点前后均无数字
    }
    // 读到前导零之外的数字，即首个有效数字
    else if (IsDigit(*cur_ptr)) {
      significant_digit++;
      base = static_cast<uint64_t>(*cur_ptr - '0');
      first_non_zero_index = cur_ptr - begin_ptr;
    }
    // 读到E （此时仅包含底数为0的情况。如 0E6、-0e3 ）
    else if (*cur_ptr == 'e' || *cur_ptr == 'E') {
      has_exponent = true;
      base = 0;
      cur_ptr++;
      break;  // 进入指数解析代码
    } else {
      return false;
    }

    cur_ptr++;
  }  // 此时cur_index指向第一个有效数字的下一位

  // 去除前导零后，解析底数。若已经进入指数处理部分，则跳过底数解析
  while (cur_ptr < end_ptr && has_exponent == false) {
    // 读到小数点
    if (*cur_ptr == '.') {
      if (decimal_point_index != -1) {  // 读入多个小数点，报错
        return false;
      }
      decimal_point_index = cur_ptr - begin_ptr;
      cur_ptr++;
    }
    // 读到数字
    else if (IsDigit(*cur_ptr)) {
      if (significant_digit ==
          10) {  // 此时已经读到10个有效位数，再读入数字需要进行四舍五入判断
        if (RoundBase(base, static_cast<uint16_t>((*cur_ptr) - '0')))
          exp_has_carry_bit = true;
      } else if (significant_digit < 10) {
        base *= 10;
        base += static_cast<uint64_t>((*cur_ptr) - '0');
      }

      // 注：有效位数 > 10的时候也需要继续读入，以防后面出现非法字符

      cur_ptr++;
      significant_digit++;
    }
    // 读到E或e
    else if (*cur_ptr == 'E' || *cur_ptr == 'e') {
      if (has_exponent == true) {
        return false;
      }  // 之前已经读到了e
      has_exponent = true;
      cur_ptr++;
      is_standard_float = false;

      if (cur_ptr >= end_ptr) {
        return false;
      }

      break;  // 进入指数解析部分
    } else {  // 读到了 数字、小数点、E e之外的内容
      return false;
    }
  }

  // 解析指数
  if (has_exponent == true) {
    // 指数是负数，记录指数的时候需要用减法
    if (cur_ptr < end_ptr && *cur_ptr == '-') {
      cur_ptr++;
      if (cur_ptr >= end_ptr) {
        return false;
      }  // 读到负号之后没有跟数字
      while (cur_ptr < end_ptr) {
        if (IsDigit(*cur_ptr)) {
          exponent *= 10;
          exponent -= static_cast<int>((*cur_ptr) - '0');
          cur_ptr++;
        } else {
          return false;
        }
      }
    }
    // 指数是正数（读入正号，或数字），记录指数的时候需要用加法
    else if (cur_ptr < end_ptr && (IsDigit(*cur_ptr) || *cur_ptr == '+')) {
      if (*cur_ptr == '+') cur_ptr++;
      if (cur_ptr >= end_ptr) {  // 读到正号之后没有跟数字
        return false;
      }
      while (cur_ptr < end_ptr) {
        if (IsDigit(*cur_ptr)) {
          exponent *= 10;
          exponent += static_cast<int>((*cur_ptr) - '0');
          cur_ptr++;
        } else {
          return false;
        }
      }
    } else {  // 指数部分读到 数字、正负号之外的内容
      return false;
    }
  }

  // 只读到了0，那有效数字就是0
  if (significant_digit == 0) {
    is_positive = true;
    base = 0;
    exponent =
        -1000;  // 由于0被设置为了正数，且正数比大小时先比exp，exp越大的数越大。0作为正数中最小的，设置exp为最小值

    key = GetKey(base, exponent, is_positive);

    return true;
  }

  // 改变exp，将底数化为 x.xxx 的形式
  if (decimal_point_index != -1) {
    if (decimal_point_index < first_non_zero_index) {  // 底数为0.xxx
      exponent += (decimal_point_index - first_non_zero_index);
    } else if (decimal_point_index > first_non_zero_index) {  // 底数为xx.xxx
      exponent += (decimal_point_index - first_non_zero_index - 1);
    }
  } else {  // 没有小数点
    exponent += significant_digit - 1;
  }
  // 指数的进位
  if (exp_has_carry_bit) exponent++;

  // 底数补齐10位             //1e9
  while (base < 1'000'000'000ULL && base != 0) {
    base *= 10;
  }

  // 指数合法性检查
  if (exponent < -999 || exponent > 999) {
    return false;
  }

  key = GetKey(base, exponent, is_positive);

  return true;
}
}  // namespace parse
}  // namespace external_sort

#endif