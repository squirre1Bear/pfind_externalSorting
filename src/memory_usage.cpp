#include "pfind_external_sorting/monitor/memory_usage.h"

#include <cstdint>
#include <iostream>

#include "pfind_external_sorting/model.h"

#ifdef _WIN32
#include <windows.h>
#include <psapi.h>  //paspi.h需要用到windows.h中的内容，需要在windows.h之后引入
#else
#include <sys/resource.h>
#endif

namespace external_sort {
namespace monitor {
// 获取内存使用情况
void PrintMemoryUsage() {
#ifdef _WIN32
  model::MemoryUsage memory{};

  PROCESS_MEMORY_COUNTERS_EX pmc;

  // 获取当前内存使用情况
  if (GetProcessMemoryInfo(GetCurrentProcess(),
                           reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&pmc),
                           sizeof(pmc))) {
    memory.current_rss = static_cast<uint64_t>(pmc.WorkingSetSize);
    memory.peak_rss = static_cast<uint64_t>(pmc.PeakWorkingSetSize);
  }

  // std::cout << "curRss=" << memory.current_rss / 1024.0 / 1024.0 << "MB
  // peakRss=" << memory.peak_rss / 1024.0 / 1024.0 << "MB" << '\n';
  std::cout << "峰值内存使用量 " << memory.peak_rss / 1024.0 / 1024.0 << "MB"
            << '\n';
#else
  struct rusage usage {};
  if (getrusage(RUSAGE_SELF, &usage) == 0) {
    std::cout << "峰值内存使用量 " << usage.ru_maxrss / 1024.0 << "MB" << '\n';
  }
#endif
  return;
}

}  // namespace monitor
}  // namespace external_sort
