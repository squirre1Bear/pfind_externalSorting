#define _CRT_SECURE_NO_WARNINGS
#include "pfind_external_sorting/io/file_reader.h"

#include <cstdio>
#include <cstring>
#include <fstream>
#include <iostream>
#include <optional>
#include <vector>

#include "pfind_external_sorting/parser/number_parser.h"

namespace external_sort {
namespace io {
InputFileReader::InputFileReader(std::string fin_path,
                                 std::string fout_errors_path,
                                 std::string fout_result_path, uint64_t buffer_size)
    : fin_path_(fin_path),
      fout_errors_path_(fout_errors_path),
      fout_result_path_(fout_result_path),
      fin_buffer_(buffer_size),
      fout_errors_buffer_(buffer_size),
      fout_result_buffer_(buffer_size),
      first_half_number_(""),
      block_begin_(fin_buffer_.data()),
      block_cursor_(fin_buffer_.data()),
      fout_errors_(nullptr),
      fout_result_(nullptr),
      block_end_(fin_buffer_.data()) {}

InputFileReader::~InputFileReader() {
  if (fout_errors_) {
    fflush(fout_errors_);
    fclose(fout_errors_);
    fout_errors_ = nullptr;
  }
  if (fout_result_) {
    fflush(fout_result_);
    fclose(fout_result_);
    fout_result_ = nullptr;
  }
}

bool InputFileReader::OpenFiles() {
  fin_.open(fin_path_);
    // 采用c中FILE*加速文件读写
  fout_errors_ = fopen(fout_errors_path_.c_str(), "wb");
  fout_result_ = fopen(fout_result_path_.c_str(), "wb");
  if (!(fin_ && fout_errors_ && fout_result_)) {
    std::cerr << "文件打开失败！\n";
    return false;
  }
  return true;
}

bool InputFileReader::GetBuffer() {
  if (setvbuf(fout_errors_, fout_errors_buffer_.data(), _IOFBF,  // 采用全缓冲_IOFBF，缓冲区满了再进行写入操作
              fout_errors_buffer_.size()) != 0) {
    std::cerr << "缓冲区获取失败！\n";
    return false;
  }
  if (setvbuf(fout_result_, fout_result_buffer_.data(), _IOFBF,
              fout_result_buffer_.size()) != 0) {
    std::cerr << "缓冲区获取失败！\n";
    return false;
  }
  return true;
}

// 读入一块数据，并返回实际读入的字节数
uint64_t InputFileReader::GetBlock() {
  fin_.read(fin_buffer_.data(), fin_buffer_.size());
  block_begin_ = fin_buffer_.data();
  block_cursor_ = fin_buffer_.data();
  block_end_ = fin_buffer_.data() + fin_.gcount();
  return fin_.gcount();
}

std::optional<model::ParsedNumber> InputFileReader::ParseFirstHalfNumber() {
  while (block_cursor_ != block_end_) {
    if (*block_cursor_ == '\n') {
      // 直接对complete_first_half_number_赋值，避免声明新的字符串
      complete_first_half_number_.clear();
      complete_first_half_number_.reserve(first_half_number_.size() +
                                          (block_cursor_ - block_begin_));
      complete_first_half_number_.append(first_half_number_);
      complete_first_half_number_.append(block_begin_, block_cursor_);
     
      first_half_number_.clear();
      parsed_num_ = parse::NumberParser(complete_first_half_number_);
      ++block_cursor_;  // 跳过'\n'

      return parsed_num_;
    }
    ++block_cursor_;
  }

  // 一整块都没有读到换行符，此时需要将整块的内容拼到first_half后面
  first_half_number_.append(block_begin_, block_end_);
  return std::nullopt;
}

bool InputFileReader::EndOfBlock() { return block_cursor_ == block_end_; }

// 从in_buffer_中读入一行，使用memchr代替指针手动遍历
std::string_view InputFileReader::ReadLine() {
  char* p = static_cast<char*>(
      memchr(block_cursor_, '\n', block_end_ - block_cursor_));

  if (p != nullptr) {
    std::string_view sv(block_cursor_, p - block_cursor_);
    block_cursor_ = ++p;
    return sv;
  } else {  // in_buffer_中没有下一个换行符了，需要将后半段存入first_half
    std::string_view sv(block_cursor_, block_end_ - block_cursor_);   // 注意此时 p为空指针，长度不能用p-block_cursor_
    first_half_number_ = sv;
    block_cursor_ = block_end_;
    return {};
  }
}

std::string& InputFileReader::GetFirstHalfNumber() { 
    return first_half_number_; 
}

std::string& InputFileReader::GetCompelteFirstHalfNumber() {
  return complete_first_half_number_;
}

void InputFileReader::WriteToErrors(std::string_view str_view) {
  fwrite(str_view.data(), 1, str_view.size(), fout_errors_);
  fputc('\n', fout_errors_);
  return;
}

bool InputFileReader::IsFirstHalfEmpty() { return first_half_number_.empty(); }

// 当输入缓冲区满了，内部排序后写入 .bin文件
bool InputFileReader::GenerateBin(int run_number, std::vector<uint64_t>& keys) {
  if (run_number > 999) {
    std::cerr << "bin文件数量超过999个";
    return false;
  }
  char bin_filename[16];
  snprintf(bin_filename, 16, "tmp/run_%03d.bin", run_number);
  
  std::ofstream fout_bin(bin_filename, std::ios::binary);
  if (!fout_bin) {
    std::cerr << "bin文件打开失败！\n";
    return false;
  }
  fout_bin.write(reinterpret_cast<const char*>(keys.data()),
                 keys.size() * sizeof(uint64_t));

  std::cout << "  " << bin_filename << "已生成\n";
  return true;
}

FILE* InputFileReader::GetResultStream() { return fout_result_; }

}  // namespace io
}  // namespace external_sort
