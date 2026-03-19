#ifndef EXTERNAL_SORT_SORT_RADIX_SORT_H_
#define EXTERNAL_SORT_SORT_RADIX_SORT_H_

#include <cstdint>
#include <vector>

namespace external_sort {
namespace sort {

// 进行uint64_t类型容器的基数排序
// 传入待排序的vector<uint64_t>指针，无返回值
void RadixSort64(std::vector<uint64_t>& keys);

}  // namespace sort
}  // namespace external_sort
#endif