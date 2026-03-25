// 记录所需要的数据结构
#ifndef EXTERNAL_SORT_MODEL_H_
#define EXTERNAL_SORT_MODEL_H_
#include <cstdint>
#include <string>

namespace external_sort {
namespace model {

struct ParsedNumber {
  // 当出现错误时才需要记录原字符串，在ParsedNumber中直接记录会产生2.5亿个字符串开销
  // std::string original_string = "";
  bool is_positive = false;
  uint64_t base = 0;
  int exponent = 0;
  bool is_standard_float = true;
  bool is_legal = true;
};

struct MergeNode {  // 用于归并排序
  uint64_t key;
  int from_where;  // 来自于哪个文件
};

// 用于维护小根堆。数值小的优先级更高
struct MergeNodeGreaterComparer {
  bool operator()(const MergeNode& a, const MergeNode& b) {
    return a.key > b.key;
  }
};

struct MemoryUsage {
  uint64_t current_rss = 0;  // 当前内存使用字节数
  uint64_t peak_rss = 0;     // 峰值内存使用字节数
};

}  // namespace model
}  // namespace external_sort
#endif
