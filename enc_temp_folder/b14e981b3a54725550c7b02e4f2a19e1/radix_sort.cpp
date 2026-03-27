#include "pfind_external_sorting/sort/radix_sort.h"

namespace external_sort {
namespace sort {
// 64位基数排序
// keys: 待排序元素数组。tmp: 记录中间结果用的桶
void RadixSort64(std::vector<uint64_t>& keys, std::vector<uint64_t>& tmp) {
  if (keys.empty()) return;
  // 后面会进行keys.swap(tmp)，如果长度不等，则交换后再遍历temp时会越界
  if (tmp.size() != keys.size()) tmp.resize(keys.size());

  const int kBitsPerPass = 16;  // 每个桶包括了几位数字
  const int kPassNum = (64 + kBitsPerPass - 1) /
                       kBitsPerPass;  // 总共进行几趟基数排序，进行了向上取整
  const uint64_t kRadix = 1ULL << kBitsPerPass;  // 一段可以表示的数字个数
  const uint64_t mask = kRadix - 1;  // 用于取出特定段的数字

  // std::vector<uint64_t> temp(keys.size());  // 排序的桶
  static std::vector<int> count_index0(kRadix);
  static std::vector<int> count_index1(kRadix);
  static std::vector<int> count_index2(kRadix);
  static std::vector<int> count_index3(kRadix);

  // 一次遍历完成计数
  for (int i = 0; i < keys.size(); i++) {
    count_index0[keys[i] & 0xFFFFu]++;
    count_index1[(keys[i] >> 16) & 0xFFFFu]++;
    count_index2[(keys[i] >> 32) & 0xFFFFu]++;
    count_index3[(keys[i] >> 48) & 0xFFFFu]++;  // 记录每个值分别有多少个
  }

  Scatter(count_index0, keys, tmp, kRadix, 0);
  Scatter(count_index1, keys, tmp, kRadix, 16);
  Scatter(count_index2, keys, tmp, kRadix, 32);
  Scatter(count_index3, keys, tmp, kRadix, 48);

  return;
}

// 将16位数塞入桶中进行一轮排序
void Scatter(std::vector<int>& count_index, std::vector<uint64_t>& keys,
             std::vector<uint64_t>& tmp, const uint64_t kRadix, int shift) {
  // 记录每个值存放的末尾下标。count_index[i]为 <= i的元素个数
  for (int i = 1; i < kRadix; i++) {
    count_index[i] += count_index[i - 1];
  }

  // 分配到桶中
  for (int i = keys.size() - 1; i >= 0; i--) {
    uint64_t key = keys[i];
    int digit = (key >> shift) & 0xFFFFu;
    count_index[digit]--;
    tmp[count_index[digit]] = key;
  }

  keys.swap(tmp);

  return;
}

}  // namespace sort
}  // namespace external_sort
