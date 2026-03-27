#ifndef EXTERNAL_SORT_SORT_RADIX_SORT_H_
#define EXTERNAL_SORT_SORT_RADIX_SORT_H_

#include <cstdint>
#include <vector>

namespace external_sort {
namespace sort {

// 进行uint64_t类型容器的基数排序
// 传入待排序的vector<uint64_t>指针，和大小为keys.size()的temp用于记录中间结果。无返回值
void RadixSort64(std::vector<uint64_t>& keys, std::vector<uint64_t>& temp);
void Scatter(std::vector<int>& count_index, std::vector<uint64_t>& keys,
             std::vector<uint64_t>& tmp, const uint64_t kRadix, int shift);
}  // namespace sort
}  // namespace external_sort
#endif