#include "pfind_external_sorting/app.h"
#include "pfind_external_sorting/config_loader.h"

int main() {
  std::string config_path = "config.txt";
  external_sort::app::ConfigOptions cfg =
      external_sort::config_loader::ConfigLoader(config_path);

  external_sort::app::ExternalSortApp app(cfg);
  app.Run();
  return 0;
}