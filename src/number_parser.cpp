#include "pfind_external_sorting/parser/number_parser.h"

#include <cstring>


namespace external_sort {
namespace parse {
inline bool IsDigit(char c) { return c - '0' >= 0 && c - '0' <= 9; }
char digit_table_3[1000][3];
void InitDigit3() {
  for (int i = 0; i < 1000; i++) {
    digit_table_3[i][0] = '0' + (i / 100);
    digit_table_3[i][1] = '0' + (i / 10) % 10;
    digit_table_3[i][2] = '0' + i % 10;
  }
}

void InitParsedNumber(model::ParsedNumber& parsed_number) {
  parsed_number.is_positive = false;
  parsed_number.base = 0;
  parsed_number.exponent = 0;
  parsed_number.is_standard_float = true;
  parsed_number.is_legal = true;
  return;
}

model::ParsedNumber NumberParser(std::string_view origin_number_view) {
  model::ParsedNumber parsed_number{};
  const char* begin_ptr = origin_number_view.data();
  const char* end_ptr = begin_ptr + origin_number_view.size();

  const char* cur_ptr = begin_ptr;
  int decimal_point_index = -1;
  int significant_digit = 0;       // 总的有效位数
  int first_non_zero_index = -1;   // 首个非零元素下标
  bool has_exponent = false;       // 是否已经读到E e
  bool exp_has_carry_bit = false;  // exp是否有进位
  const int num_str_size = end_ptr - begin_ptr;

  if (begin_ptr == end_ptr) {
    parsed_number.is_legal = false;
    return parsed_number;  // 空串
  }
  // 读取符号
  if (*cur_ptr == '-') {
    parsed_number.is_positive = false;
    cur_ptr++;
    // [如果不在这里处理长度为1的情况，则有cur_index >=
    // num_str.size()，无法进入后面的有效位数判断循环，导致无法处理单一符号的情况]
    if (num_str_size == 1 ||
        (!IsDigit(*cur_ptr) &&
         *cur_ptr !=
             '.')) {  // 只读到了一个负号，或是负号之后没有跟数字 / 小数点
      parsed_number.is_legal = false;
      return parsed_number;
    }
  } else if (*cur_ptr == '+') {
    parsed_number.is_positive = true;
    cur_ptr++;
    if (num_str_size == 1 ||
        (!IsDigit(*cur_ptr) &&
         *cur_ptr != '.')) {  // 只读到了一个正号，或是之后没有跟数字 / 小数点
      parsed_number.is_legal = false;
      return parsed_number;
    }
  } else if (IsDigit(*cur_ptr)) {
    parsed_number.is_positive = 1;
  } else if (*cur_ptr == '.') {  // 省略0的形式，如 .5
    decimal_point_index = 0;
    cur_ptr++;
    parsed_number.is_positive = 1;
    if (num_str_size == 1 ||
        !IsDigit(*cur_ptr)) {  // 只读到了一个小数点，或是小数点之后没有跟数字
      parsed_number.is_legal = false;
      return parsed_number;
    }
  } else {
    parsed_number.is_legal = false;
    return parsed_number;
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
        parsed_number.is_legal = false;
        return parsed_number;
      }  // 之前出现过了小数点
      decimal_point_index = cur_ptr - begin_ptr;

      // 注意保证小数点前或后面有数字，防止出现单独小数点的情况
      bool is_valid_decimal_point = false;
      if (cur_ptr - begin_ptr >= 1 && IsDigit(*(cur_ptr - 1)))  // 前面有数字
        is_valid_decimal_point = true;
      if (cur_ptr + 1 < end_ptr && IsDigit(*(cur_ptr + 1)))  // 后面有数字
        is_valid_decimal_point = true;

      if (!is_valid_decimal_point) {
        parsed_number.is_legal = false;
        return parsed_number;
      }  // 小数点前后均无数字
    }
    // 读到前导零之外的数字，即首个有效数字
    else if (IsDigit(*cur_ptr)) {
      significant_digit++;
      parsed_number.base = static_cast<uint64_t>(*cur_ptr - '0');
      first_non_zero_index = cur_ptr - begin_ptr;
    }
    // 读到E （此时仅包含底数为0的情况。如 0E6、-0e3 ）
    else if (*cur_ptr == 'e' || *cur_ptr == 'E') {
      has_exponent = true;
      parsed_number.base = 0;
      cur_ptr++;
      break;  // 进入指数解析代码
    } else {
      parsed_number.is_legal = false;
      return parsed_number;
    }

    cur_ptr++;
  }  // 此时cur_index指向第一个有效数字的下一位

  // 去除前导零后，解析底数。若已经进入指数处理部分，则跳过底数解析
  while (cur_ptr < end_ptr && has_exponent == false) {
    // 读到小数点
    if (*cur_ptr == '.') {
      if (decimal_point_index != -1) {  // 读入多个小数点，报错
        parsed_number.is_legal = false;
        return parsed_number;
      }
      decimal_point_index = cur_ptr - begin_ptr;
      cur_ptr++;
    }
    // 读到数字
    else if (IsDigit(*cur_ptr)) {
      if (significant_digit ==
          10) {  // 此时已经读到10个有效位数，再读入数字需要进行四舍五入判断
        if (RoundBase(parsed_number.base, static_cast<uint16_t>((*cur_ptr) - '0')))
          exp_has_carry_bit = true;
      } else if (significant_digit < 10) {
        parsed_number.base *= 10;
        parsed_number.base += static_cast<uint64_t>((*cur_ptr) - '0');
      }

      // 注：有效位数 > 10的时候也需要继续读入，以防后面出现非法字符

      cur_ptr++;
      significant_digit++;
    }
    // 读到E或e
    else if (*cur_ptr == 'E' || *cur_ptr == 'e') {
      if (has_exponent == true) {
        parsed_number.is_legal = false;
        return parsed_number;
      }  // 之前已经读到了e
      has_exponent = true;
      cur_ptr++;
      parsed_number.is_standard_float = false;

      if (cur_ptr >= end_ptr) {
        parsed_number.is_legal = false;
        return parsed_number;
      }

      break;  // 进入指数解析部分
    } else {  // 读到了 数字、小数点、E e之外的内容
      parsed_number.is_legal = false;
      return parsed_number;
    }
  }

  // 解析指数
  if (has_exponent == true) {
    // 指数是负数，记录指数的时候需要用减法
    if (cur_ptr < end_ptr && *cur_ptr == '-') {
      cur_ptr++;
      if (cur_ptr >= end_ptr) {
        parsed_number.is_legal = false;
        return parsed_number;
      }  // 读到负号之后没有跟数字
      while (cur_ptr < end_ptr) {
        if (IsDigit(*cur_ptr)) {
          parsed_number.exponent *= 10;
          parsed_number.exponent -= static_cast<int>((*cur_ptr) - '0');
          cur_ptr++;
        } else {
          parsed_number.is_legal = false;
          return parsed_number;
        }
      }
    }
    // 指数是正数（读入正号，或数字），记录指数的时候需要用加法
    else if (cur_ptr < end_ptr && (IsDigit(*cur_ptr) || *cur_ptr == '+')) {
      if (*cur_ptr == '+') cur_ptr++;
      if (cur_ptr >= end_ptr) {  // 读到正号之后没有跟数字
        parsed_number.is_legal = false;
        return parsed_number;
      }
      while (cur_ptr < end_ptr) {
        if (IsDigit(*cur_ptr)) {
          parsed_number.exponent *= 10;
          parsed_number.exponent += static_cast<int>((*cur_ptr) - '0');
          cur_ptr++;
        } else {
          parsed_number.is_legal = false;
          return parsed_number;
        }
      }
    } else {  // 指数部分读到 数字、正负号之外的内容
      parsed_number.is_legal = false;
      return parsed_number;
    }
  }

  // 只读到了0，那有效数字就是0
  if (significant_digit == 0) {
    parsed_number.is_positive = true;
    parsed_number.base = 0;
    parsed_number.exponent =
        -1000;  // 由于0被设置为了正数，且正数比大小时先比exp，exp越大的数越大。0作为正数中最小的，设置exp为最小值
    parsed_number.is_legal = true;
    return parsed_number;
  }

  // 改变exp，将底数化为 x.xxx 的形式
  if (decimal_point_index != -1) {
    if (decimal_point_index < first_non_zero_index) {  // 底数为0.xxx
      parsed_number.exponent += (decimal_point_index - first_non_zero_index);
    } else if (decimal_point_index > first_non_zero_index) {  // 底数为xx.xxx
      parsed_number.exponent +=
          (decimal_point_index - first_non_zero_index - 1);
    }
  } else {  // 没有小数点
    parsed_number.exponent += significant_digit - 1;
  }
  // 指数的进位
  if (exp_has_carry_bit) parsed_number.exponent++;

  // 底数补齐10位             //1e9
  while (parsed_number.base < 1'000'000'000ULL && parsed_number.base != 0) {
    parsed_number.base *= 10;
  }

  // 指数合法性检查
  if (parsed_number.exponent < -999 || parsed_number.exponent > 999) {
    parsed_number.is_legal = false;
    return parsed_number;
  }

  return parsed_number;
}



// 从key反推原值
void KeyParser(uint64_t key, model::ParsedNumber& num_struct) {
  uint64_t kMaxExpBase = (1ULL << 63) - 1;
  uint64_t kBaseMask = (1ULL << 52) - 1;
  uint64_t exp_mask = ((1ULL << 63) - 1) - kBaseMask;

  if (key >= (1ULL << 63)) {  // 原值为正数
    num_struct.is_positive = true;
    num_struct.base = key & kBaseMask;
    num_struct.exponent = ((key & exp_mask) >> 52) - 1000;
  } else {
    num_struct.is_positive = false;
    key = kMaxExpBase - key;
    num_struct.base = key & kBaseMask;
    num_struct.exponent = ((key & exp_mask) >> 52) - 1000;
  }

  return;
}
}  // namespace parse
}  // namespace external_sort