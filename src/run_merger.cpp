// 使用归并排序合并之前生成的.bin文件
#include "pfind_external_sorting/sort/run_merger.h"

#include <cstdio>
#include <fstream>
#include <iostream>
#include <queue>
#include <vector>

#include "pfind_external_sorting/model.h"
#include "pfind_external_sorting/parser/number_parser.h"

namespace external_sort {
namespace sort {
void MergeSort(int k, uint64_t buffer_size, std::ofstream& fout_result) {
  std::vector<std::ifstream> run_files;  // 存储k个bin文件输入流
  run_files.reserve(k);

  // 记录k个文件的ifstream
  char run_path[12];
  for (int i = 0; i < k; i++) {
    snprintf(run_path, 12, "run_%03d.bin", i + 1);
    run_files.emplace_back(run_path, std::ios::binary);
    if (!run_files.back()) {
      std::cerr << "bin文件打开失败";
      return;
    }
  }

  // 向缓存中读取数据
  std::vector<std::vector<uint64_t>> input_buffer(
      k,
      std::vector<uint64_t>(buffer_size / sizeof(uint64_t)));  // 读取数据的缓冲
  std::vector<int> valid_num(k, 0);  // k个缓冲中，分别读入了多少个数字
  std::vector<int> buffer_cursor(k, 0);  // 指向每一个文件下一个读入的数

  for (int i = 0; i < k; i++) {
    run_files[i].read(reinterpret_cast<char*>(input_buffer[i].data()),
                      buffer_size);
    valid_num[i] = run_files[i].gcount() / sizeof(uint64_t);
  }

  // 初始化优先队列
  std::priority_queue<model::MergeNode, std::vector<model::MergeNode>,
                      model::MergeNodeGreaterComparer>
      min_heap;
  for (int i = 0; i < k; i++) {
    min_heap.push({input_buffer[i][0], i});  // 把每个缓冲区的首个元素放到小根堆
    buffer_cursor[i] = 1;                    // 指向下标为1的元素
  }

  // 初始化输出的缓冲区
  int max_buffer_num = 1024 * 1024;  // 缓冲区最大能存储的数字个数
  std::vector<char> buffer_out(18 *
                               max_buffer_num);  // 18是每个格式化数字所占byte数
  int buffered_count = 0;  // buffer中实际存储的数字个数

  // 排序、输出
  while (!min_heap.empty()) {
    model::MergeNode node = min_heap.top();
    min_heap.pop();
    parse::GetFormattedNumber(buffer_out.data() + 18 * buffered_count,
                              node.key);
    buffered_count++;

    // 如果缓存满了，则进行一次写入
    if (buffered_count >= max_buffer_num) {
      fout_result.write(buffer_out.data(), buffer_out.size());
      buffered_count = 0;
    }

    // 找从哪个文件来的，继续读入一个数
    int file_index = node.from_where;
    model::MergeNode new_node;

    // 如果当前缓存已经读完，则继续读入当前文件至缓存
    if (buffer_cursor[file_index] >= valid_num[file_index]) {
      run_files[file_index].read(
          reinterpret_cast<char*>(input_buffer[file_index].data()),
          buffer_size);
      valid_num[file_index] = run_files[file_index].gcount() / sizeof(uint64_t);

      if (valid_num[file_index] != 0) {  // 读入成功
        buffer_cursor[file_index] = 0;
      } else {  // 当前文件已经读完了，此时没有元素进堆
        continue;
      }
    }

    // 读入下一个数
    min_heap.push(
        {input_buffer[file_index][buffer_cursor[file_index]], file_index});
    buffer_cursor[file_index]++;
  }

  // 把缓存中剩下的部分输出
  if (buffered_count != 0) {
    fout_result.write(buffer_out.data(), 18 * buffered_count);
    buffered_count = 0;
    std::vector<char>().swap(buffer_out);
  }

  return;
}

}  // namespace sort
}  // namespace external_sort
