#include "pfind_external_sorting/app.h"

#include <chrono>
#include <iostream>

#include "pfind_external_sorting/io/file_reader.h"
#include "pfind_external_sorting/monitor/memory_usage.h"
#include "pfind_external_sorting/parser/number_parser.h"
#include "pfind_external_sorting/sort/radix_sort.h"
#include "pfind_external_sorting/sort/run_merger.h"

namespace external_sort {
namespace app {
ExternalSortApp::ExternalSortApp(ConfigOptions cfg)
    : options_(cfg),
      io_buffer_size_(cfg.io_buffer_size),
      total_run_buffer_size_(cfg.total_run_buffer_size),
      run_size_(cfg.run_size),
      fin_path_(cfg.fin_path),
      fout_errors_path_(cfg.fout_errors_path),
      fout_result_path_(cfg.fout_result_path) {}

int ExternalSortApp::Run() {
  // 注意，C++中后定义的对象先释放内存。如果out_buffer在fout之后定义，则会在fout关闭前就被释放，fout会因找不到buffer地址而报错！

  int total_run = 0;  // 记录生成了多少个run

  io::InputFileReader file_reader(fin_path_, fout_errors_path_,
                                  fout_result_path_, io_buffer_size_);

  if (!file_reader.OpenFiles()) return 1;
  if (!file_reader.GetBuffer()) return 1;

  auto time_begin = std::chrono::steady_clock::now();
  auto time_end = std::chrono::steady_clock::now();

  std::vector<uint64_t>
      keys;  // 存储每个数映射后的结果。保证key越大时，原数值越大，且可通过key倒推原值。
  keys.reserve(run_size_);
  std::vector<uint64_t> radix_sort_buffer(run_size_);

  std::string first_half;  // 记录被块截断的半行字符

  time_end = std::chrono::steady_clock::now();
  std::cout << "["
            << std::chrono::duration_cast<std::chrono::milliseconds>(time_end -
                                                                     time_begin)
                   .count()
            << "ms] 开始生成.bin文件\n";

  // 按块循环读入数据
  while (true) {
    // 读入一块数据
    uint64_t bytes_get = file_reader.GetBlock();
    if (bytes_get == 0) break;

    // 处理前一块留下的半个数字
    if (file_reader.GetFirstHalfNumber() != "") {
      auto opt_parsed_number = file_reader.ParseFirstHalfNumber();
      if (!opt_parsed_number) continue;  // 整个块都是同一个数字（没有读到\n）

      model::ParsedNumber parsed_number =
          *opt_parsed_number;  // opt_parsed_number为
                               // std::optional<ParsedNumber>类型，需要先用*提出里面的值
      if (!parsed_number.is_legal) {
        file_reader.WriteToErrors(file_reader.GetCompelteFirstHalfNumber());
      } else {
        uint64_t key = parse::GetKey(parsed_number);
        keys.push_back(key);

        // 如果缓冲区满了，则排序后写入 .bin文件
        if (keys.size() >= run_size_) {
          sort::RadixSort64(keys, radix_sort_buffer);
          if (!file_reader.GenerateBin(++total_run, keys)) return 1;
          keys.clear();
        }
      }
    }

    // 按行处理块内剩余的数字
    std::string_view original_string_view;
    while (!file_reader.EndOfBlock()) {
      // [修改] 直接返回字符串时，每次都会复制一次字符串。需要改为零拷贝读取
      // std::string original_string = file_reader.ReadLine();
      bool is_empty_line = false;   // 不加该判断时，original_string_view.empty()会将 空行 也识别为块结束
      original_string_view = file_reader.ReadLine(is_empty_line);

      // 已经读完整个块的内容。（没处理的半段数字已在ReadLine()中被存入first_half_）
      if (original_string_view.empty() && !is_empty_line) break;

      // 直接通过当前行解析出key，避免构造ParsedNumber
      uint64_t key;
      if (!parse::ParseLineToKey(original_string_view, key)) {
        file_reader.WriteToErrors(original_string_view);
      } else {
        keys.push_back(key);
        // 如果缓冲区满了，则排序后写入 .bin文件
        if (keys.size() >= run_size_) {
          sort::RadixSort64(keys, radix_sort_buffer);
          if (!file_reader.GenerateBin(++total_run, keys)) return 1;
          keys.clear();
        }
      }
    }
  }

  // 处理读完所有内容后剩下的半段数字
  first_half = file_reader.GetFirstHalfNumber();
  if (!first_half.empty()) {
    model::ParsedNumber parsed_number;
    parsed_number = parse::NumberParser(first_half);

    if (!parsed_number.is_legal) {
      file_reader.WriteToErrors(first_half);
    } else {
      uint64_t key = parse::GetKey(parsed_number);
      keys.push_back(key);
    }
  }

  // 将最后一组数据 排序后 写入 .bin文件
  if (!keys.empty()) {
    sort::RadixSort64(keys, radix_sort_buffer);
    if (!file_reader.GenerateBin(++total_run, keys)) return 1;

    std::vector<uint64_t>().swap(keys);  // 释放keys空间
  }

  // 释放空间
  std::vector<uint64_t>().swap(radix_sort_buffer);

  time_end = std::chrono::steady_clock::now();
  std::cout << "["
            << std::chrono::duration_cast<std::chrono::milliseconds>(time_end -
                                                                     time_begin)
                   .count()
            << "ms] 开始归并排序\n";

  // 初始化从int->string的表格
  external_sort::parse::InitDigit3();

  // 归并排序并写入result.txt
  //external_sort::sort::MergeSort(total_run, total_run_buffer_size_,
  //                               file_reader.GetResultStream());
  external_sort::sort::LoserTreeSort(total_run, total_run_buffer_size_,
                                      file_reader.GetResultStream());

  time_end = std::chrono::steady_clock::now();
  std::cout << "["
            << std::chrono::duration_cast<std::chrono::milliseconds>(time_end -
                                                                     time_begin)
                   .count()
            << "ms] 结果输出完成 \n";

  external_sort::monitor::PrintMemoryUsage();
  return 0;
}

}  // namespace app
}  // namespace external_sort