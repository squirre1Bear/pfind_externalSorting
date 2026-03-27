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
void MergeSort(int k, uint64_t total_run_buffer_size, FILE* fout_result) {
  std::vector<std::ifstream> run_files;  // 存储k个bin文件输入流
  run_files.reserve(k);

  // 记录k个文件的ifstream
  char run_path[16];
  for (int i = 0; i < k; i++) {
    snprintf(run_path, 16, "tmp/run_%03d.bin", i + 1);
    run_files.emplace_back(run_path, std::ios::binary);
    if (!run_files.back()) {
      std::cerr << "bin文件打开失败！\n";
      return;
    }
  }

  if (k == 0) {
    std::cout << "所有数字均不合法！\n";
    return;
  }
  uint64_t bytes_pre_run = total_run_buffer_size / k;

  // 向缓存中读取数据
  std::vector<std::vector<uint64_t>> input_buffer(
      k,
      std::vector<uint64_t>(bytes_pre_run /
                            sizeof(uint64_t)));  // 读取数据的缓冲
  std::vector<int> valid_num(k, 0);  // k个缓冲中，分别读入了多少个数字
  std::vector<int> buffer_cursor(k, 0);  // 指向每一个文件下一个读入的数

  for (int i = 0; i < k; i++) {
    run_files[i].read(reinterpret_cast<char*>(input_buffer[i].data()),
                      bytes_pre_run);
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
      if (fwrite(buffer_out.data(), 1, buffer_out.size(), fout_result) !=
          buffer_out.size()) {
        std::cerr << "result文件写入失败！\n";
        return;
      }
      buffered_count = 0;
    }

    // 找从哪个文件来的，继续读入一个数
    int file_index = node.from_where;
    model::MergeNode new_node;

    // 如果当前缓存已经读完，则继续读入当前文件至缓存
    if (buffer_cursor[file_index] >= valid_num[file_index]) {
      run_files[file_index].read(
          reinterpret_cast<char*>(input_buffer[file_index].data()),
          bytes_pre_run);
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
    if (fwrite(buffer_out.data(), 1, 18 * buffered_count, fout_result) !=
        18 * buffered_count) {  // 注意buffer_out没有满，写入量不是
                                // buffer_out.size()！
      std::cerr << "result文件写入失败！\n";
      return;
    }

    buffered_count = 0;
    std::vector<char>().swap(buffer_out);
  }

  return;
}

// 使用败者树进行归并
// k为run的数量
void LoserTreeSort(int k, uint64_t total_run_buffer_size, FILE* fout_result) {
  if (k == 0) {
    std::cout << "数据集中没有数字合法！\n";
    return;
  }


  std::vector<std::ifstream> run_files;  // 存储k个bin文件输入流
  run_files.reserve(k);

  // 记录k个文件的ifstream
  char run_path[16];
  for (int i = 0; i < k; i++) {
    snprintf(run_path, 16, "tmp/run_%03d.bin", i + 1);
    run_files.emplace_back(run_path, std::ios::binary);
    if (!run_files.back()) {
      std::cerr << "bin文件打开失败！\n";
      return;
    }
  }

  uint64_t bytes_per_run = total_run_buffer_size / k;
  bytes_per_run = (bytes_per_run / sizeof(uint64_t)) * sizeof(uint64_t);  // 确保bytes_per_run为8的倍数，防止后面读取时越界
  if (bytes_per_run < sizeof(uint64_t)) {
    std::cerr << "total_run_buffer_size 太小，无法给每个 run 分到至少一个 uint64_t\n";
    return;
  }


  // 初始化输入缓存，并读取数据
  std::vector<std::vector<uint64_t>> input_buffer(
      k,
      std::vector<uint64_t>(bytes_per_run /
                            sizeof(uint64_t)));  // 读取数据的缓冲
  std::vector<int> valid_num(k, 0);  // k个缓冲中，分别读入了多少个数字
  std::vector<int> buffer_cursor(k, 0);  // 指向每一个文件下一个读入的数
  std::vector<uint64_t> current_number(k+1);  // 记录每个run当前在处理的元素。current_number[k]=0作为哨兵，仅用于初始化树。

  for (int i = 0; i < k; i++) {
    run_files[i].read(reinterpret_cast<char*>(input_buffer[i].data()),
                      bytes_per_run);
    valid_num[i] = run_files[i].gcount() / sizeof(uint64_t);
    if (valid_num[i] == 0) {  // 当前run为空，节点设置为UINT64_MAX，不参与败者树比较
      current_number[i] = UINT64_MAX;
    } else {
      current_number[i] = input_buffer[i][0];
    }
  }
  current_number[k] = 0;
  
  
  // 初始化输出的缓冲区
  int max_buffer_num = 1024 * 1024;  // 缓冲区最大能存储的数字个数
  std::vector<char> buffer_out(18 *
                               max_buffer_num);  // 18是每个格式化数字所占byte数
  int buffered_count = 0;  // buffer中实际存储的数字个数

  // 初始化败者树
  // loser_tree中存储败者来自几号文件
  // 所有节点初始化为 k，表示指向值为0的哨兵
  std::vector<int> loser_tree(k, k);
  for (int i = k-1; i >= 0; i--) {
    uint64_t father = (i + k) / 2;
    uint64_t winner_index = i;
    while (father > 0) {
      // 当前节点对应的值大于父节点对应的值，则winner为父节点
      if (current_number[winner_index] >
          current_number[loser_tree[father]]) {    
        uint64_t tmp = loser_tree[father];
        loser_tree[father] = winner_index;
        winner_index = tmp;  
      }
      father >>= 1;  // father/2，用于找再上一级的父节点
    }
    // father==0，此时为根节点再往上的节点赋值
    loser_tree[0] = winner_index;

    //buffer_cursor[i]++;  每个run的首元素只进行了建树，还未输出，不能移动游标
  }

  // 输出结果 + 败者树调整
  uint64_t file_index = loser_tree[0];  // 记录树顶元素来自哪个文件
  // 当某个run全部读完后，设置节点为UINT64_MAX
  while (true) {
    if (current_number[file_index] == UINT64_MAX) break;

    parse::GetFormattedNumber(
        buffer_out.data() + 18 * buffered_count,
        current_number[file_index]);
    buffered_count++;
    buffer_cursor[file_index]++;

    // 如果缓存满了，则进行一次写入
    if (buffered_count >= max_buffer_num) {
      if (fwrite(buffer_out.data(), 1, buffer_out.size(), fout_result) !=
          buffer_out.size()) {
        std::cerr << "result文件写入失败！\n";
        return;
      }
      buffered_count = 0;
    }

    // 输入缓冲读完后，重新从run读入数据
    if (buffer_cursor[file_index] >= valid_num[file_index]) {
      run_files[file_index].read(
          reinterpret_cast<char*>(input_buffer[file_index].data()),
          bytes_per_run);
      valid_num[file_index] = run_files[file_index].gcount() / sizeof(uint64_t);
      buffer_cursor[file_index] = 0;
    }
    
    // 如果没元素可读入，则塞入UINT64_MAX，表示该路排序结束
    // 保证只在整棵树只有UINT64_MAX的时候才输出，此时表示排序结束
    if (valid_num[file_index] == 0) {
      current_number[file_index] = UINT64_MAX;
    } else {
      current_number[file_index] =
          input_buffer[file_index][buffer_cursor[file_index]];
    }


    // 败者树调整
    uint64_t father = (file_index + k) / 2;
    uint64_t winner_index = file_index;   // 记录当前胜者来自几号文件
    while (father > 0) {
      if (current_number[winner_index] > current_number[loser_tree[father]]) {
        uint64_t tmp = loser_tree[father];
        loser_tree[father] = winner_index;
        winner_index = tmp;  
      }
      father >>= 1;
    }
    // father为零的时候，调整根节点再往上的那个节点
    loser_tree[0] = winner_index;
    file_index = loser_tree[0];
  }

    // 把缓存中剩下的部分输出
  if (buffered_count != 0) {
    if (fwrite(buffer_out.data(), 1, 18 * buffered_count, fout_result) !=
        18 * buffered_count) {  // 注意buffer_out没有满，写入量不是 buffer_out.size()！
      std::cerr << "result文件写入失败！\n";
      return;
    }

    buffered_count = 0;
    std::vector<char>().swap(buffer_out);
  }

  return;
}

}  // namespace sort
}  // namespace external_sort
