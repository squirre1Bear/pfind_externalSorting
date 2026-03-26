#ifndef EXTERNAL_SORT_SORT_RUN_MERGER_H_
#define EXTERNAL_SORT_SORT_RUN_MERGER_H_

#include <cstdint>
#include <cstdio>
#include <fstream>
namespace external_sort {
namespace sort {

// 对.bin文件进行归并排序
// 参数:
//	  k: 总.bin文件个数
//    buffer_size: 读入缓冲器的大小，单位为byte
//    fout_result: 待写入文件的流
void MergeSort(int k, uint64_t total_run_buffer_size,
               FILE* fout_result);

// 使用败者树进行排序
void LoserTreeSort(int k, uint64_t total_run_buffer_size, FILE* fout_result);
}  // namespace sort
}  // namespace external_sort
#endif