#ifndef EXTERNAL_SORT_CONFIG_LOADER_H_
#define EXTERNAL_SORT_CONFIG_LOADER_H_

#include <string>

#include "pfind_external_sorting/app.h"

namespace external_sort {
namespace config_loader {
app::ConfigOptions ConfigLoader(std::string config_path);
}  // namespace config_loader
}  // namespace external_sort

#endif