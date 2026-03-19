#ifndef EXTERNAL_SORT_APP_APP_H_
#define EXTERNAL_SORT_APP_APP_H_

#include <cstdint>
#include <string>
#include <vector>

namespace external_sort {
namespace app {

struct ConfigOptions {
  int buffer_size = 1024 * 1024 * 8;  // 设置缓冲区8MB
  int run_size =
      25'000'000;  // 每读入run_size个数的数字后，生成一个.bin文件作为临时存储

  std::string fin_path;
  std::string fout_errors_path;
  std::string fout_result_path;
};

class ExternalSortApp {
 public:
  // 构造函数用于初始化配置信息
  explicit ExternalSortApp(ConfigOptions cfg);
  int Run();

 private:
  ConfigOptions options_;

  std::vector<char> fout_errors_buffer_;
  std::vector<char> fout_result_buffer_;

  std::string fin_path_;
  std::string fout_errors_path_;
  std::string fout_result_path_;

  int buffer_size_;
  int run_size_;

  std::vector<uint64_t> keys_;
};

}  // namespace app
}  // namespace external_sort

#endif
