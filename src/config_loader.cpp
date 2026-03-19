#include "pfind_external_sorting/config_loader.h"

#include <fstream>
#include <iostream>
#include <sstream>

#include "pfind_external_sorting/app.h"

namespace external_sort {
namespace config_loader {

app::ConfigOptions ConfigLoader(std::string config_path) {
  std::ifstream fin_config(config_path);
  if (!fin_config) throw std::runtime_error("配置文件打开失败！");

  // 解析配置文件读取结果
  app::ConfigOptions config;
  std::string line = "";
  while (std::getline(fin_config, line)) {
    std::istringstream line_stream(line);
    std::string key, value;
    if (std::getline(line_stream, key, '=') &&
        getline(line_stream, value, '=')) {
      if (key == "buffer_size") {
        config.buffer_size = std::stoi(value);
      } else if (key == "run_size") {
        config.run_size = std::stoi(value);
      } else if (key == "fin_path") {
        config.fin_path = value;
      } else if (key == "fout_errors_path") {
        config.fout_errors_path = value;
      } else if (key == "fout_result_path") {
        config.fout_result_path = value;
      }
    }
  }
  return config;
}
}  // namespace config_loader
}  // namespace external_sort