#include "pfind_external_sorting/sort/radix_sort.h"

namespace external_sort {
namespace sort {
// 64位基数排序
// keys: 待排序元素数组。temp: 记录中间结果用的桶
void RadixSort64(std::vector<uint64_t>& keys, std::vector<uint64_t>& temp) {
  // 后面会进行keys.swap(temp)，如果长度不等，则交换后再遍历temp时会越界
  if (temp.size() != keys.size()) temp.resize(keys.size());

  const int kBitsPerPass = 16;  // 每个桶包括了几位数字
  const int kPassNum = (64 + kBitsPerPass - 1) /
                       kBitsPerPass;  // 总共进行几趟基数排序，进行了向上取整
  const uint64_t kRadix = 1ULL << kBitsPerPass;  // 桶的个数
  const uint64_t mask = kRadix - 1;  // 用于取出特定段的数字

  //std::vector<uint64_t> temp(keys.size());  // 排序的桶

  for (int pass = 0; pass < kPassNum; pass++) {
    std::vector<int> count_index(kRadix + 1);
    int shift = kBitsPerPass * pass;
    for (auto key : keys) {
      uint64_t key_masked = (key >> shift) & mask;
      count_index
          [key_masked +
           1]++;  // 记录每个值分别有多少个，值为key的存到下标(key+1)位置，方便后面求每个值的起始下标
    }

    // 统计各值起始位置
    for (uint64_t i = 1; i < kRadix; i++) {
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

}  // namespace sort
}  // namespace external_sort
