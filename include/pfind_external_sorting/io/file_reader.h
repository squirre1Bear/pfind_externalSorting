#ifndef EXTERNAL_SORT_IO_FILE_READER_H_
#define EXTERNAL_SORT_IO_FILE_READER_H_

#include <fstream>
#include <optional>
#include <string>
#include <vector>

#include "pfind_external_sorting/model.h"
namespace external_sort {
namespace io {

class InputFileReader {
 public:
  explicit InputFileReader(std::string fin_path, std::string fout_errors_path,
                           std::string fout_result_path, int buffer_size);

  // 读入一块数据，并返回实际读入的字节数
  int GetBlock();

  // 打开待读入、写入的文件，并返回是否打开成功
  bool OpenFiles();
  bool GetBuffer();
  std::optional<model::ParsedNumber> ParseFirstHalfNumber();
  bool EndOfBlock();
  std::string ReadLine();
  bool IsFirstHalfEmpty();
  void WriteToErrors(std::string str);
  void WriteToResult();
  bool GenerateBin(int run_number, std::vector<uint64_t>& keys);
  std::ofstream& GetResultStream();
  std::string GetFirstHalfNumber();

 private:
  int buffer_size;
  std::vector<char> fin_buffer_;
  std::vector<char> fout_errors_buffer_;
  std::vector<char> fout_result_buffer_;

  std::ifstream fin_;
  std::ofstream fout_errors_;
  std::ofstream fout_result_;

  std::string fin_path_;
  std::string fout_errors_path_;
  std::string fout_result_path_;

  model::ParsedNumber parsed_num_;
  std::vector<uint64_t> keys_;

  std::string first_half_;  // 存储被块截断的数字
  char* block_begin_;
  char* block_cursor_;  // 利用指针遍历读入的块
  char* block_end_;
};

}  // namespace io
}  // namespace external_sort

#endif