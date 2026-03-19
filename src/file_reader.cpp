#include "pfind_external_sorting/io/file_reader.h"

#include <fstream>
#include <iostream>
#include <optional>
#include <vector>

#include "pfind_external_sorting/parser/number_parser.h"

namespace external_sort {
namespace io {
InputFileReader::InputFileReader(std::string fin_path,
                                 std::string fout_errors_path,
                                 std::string fout_result_path, int buffer_size)
    : fin_path_(fin_path),
      fout_errors_path_(fout_errors_path),
      fout_result_path_(fout_result_path),
      fin_buffer_(buffer_size),
      fout_errors_buffer_(buffer_size),
      fout_result_buffer_(buffer_size),
      first_half_(""),
      block_begin_(fin_buffer_.data()),
      block_cursor_(fin_buffer_.data()) {}

bool InputFileReader::OpenFiles() {
  fin_.open(fin_path_);
  fout_errors_.open(fout_errors_path_);
  fout_result_.open(fout_result_path_);
  if (!(fin_ && fout_errors_ && fout_result_)) {
    std::cerr << "文件打开失败！\n";
    return false;
  }
  return true;
}

bool InputFileReader::GetBuffer() {
  if (fout_errors_.rdbuf()->pubsetbuf(fout_errors_buffer_.data(),
                                      fout_errors_buffer_.size()) == nullptr) {
    std::cerr << "缓冲区获取失败！\n";
    return false;
  }
  if (fout_result_.rdbuf()->pubsetbuf(fout_result_buffer_.data(),
                                      fout_result_buffer_.size()) == nullptr) {
    std::cerr << "缓冲区获取失败！\n";
    return false;
  }
  return true;
}

int InputFileReader::GetBlock() {  // 读入一块数据，并返回实际读入的字节数
  fin_.read(fin_buffer_.data(), fin_buffer_.size());
  block_begin_ = fin_buffer_.data();
  block_cursor_ = fin_buffer_.data();
  block_end_ = fin_buffer_.data() + fin_.gcount();
  return fin_.gcount();
}

std::optional<model::ParsedNumber> InputFileReader::ParseFirstHalfNumber() {
  while (block_cursor_ != block_end_) {
    if (*block_cursor_ == '\n') {
      std::string second_half;
      std::string complete_num;

      second_half.assign(fin_buffer_.data(), block_cursor_);
      complete_num = first_half_ + second_half;
      first_half_ = "";
      parsed_num_ = parse::NumberParser(complete_num);
      parsed_num_.original_string = complete_num;

      return parsed_num_;
    }
    block_cursor_++;
  }

  // 一整块都没有读到换行符，此时需要将整块的内容拼到first_half后面
  first_half_.append(block_begin_, block_end_);
  return std::nullopt;
}

bool InputFileReader::EndOfBlock() { return block_cursor_ == block_end_; }

// 从in_buffer_中读入一行
std::string InputFileReader::ReadLine() {
  char* p = block_cursor_;
  while (p != block_end_) {
    if (*p == '\n') {
      // 读到空行时，跳过该行
      if (*block_cursor_ == '\n') {
        block_cursor_ = ++p;
        continue;
      }
      std::string str(block_cursor_, p);
      block_cursor_ = ++p;
      return str;
    }
    p++;
  }

  // 当前块读到结尾，需要将后半段存入first_half
  std::string str(block_cursor_, p);
  first_half_ = str;
  return "";
}

std::string InputFileReader::GetFirstHalfNumber() { return first_half_; }

void InputFileReader::WriteToErrors(std::string str) {
  fout_errors_.write(str.data(), str.size());
  fout_errors_.put('\n');
  return;
}

void InputFileReader::WriteToResult() {}

bool InputFileReader::IsFirstHalfEmpty() { return first_half_.empty(); }

// 当输入缓冲区满了，内部排序后写入 .bin文件
bool InputFileReader::GenerateBin(int run_number, std::vector<uint64_t>& keys) {
  if (run_number > 999) {
    std::cerr << "bin文件数量超过999个";
    return false;
  }
  char bin_filename[12];
  snprintf(bin_filename, 12, "run_%03d.bin", run_number);

  std::ofstream fout_bin(bin_filename, std::ios::binary);
  fout_bin.write(reinterpret_cast<const char*>(keys.data()),
                 keys.size() * sizeof(uint64_t));

  std::cout << "  " << bin_filename << "已生成\n";
  return true;
}

std::ofstream& InputFileReader::GetResultStream() { return fout_result_; }

}  // namespace io
}  // namespace external_sort
