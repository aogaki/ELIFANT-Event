#ifndef L1EventBuilder_hpp
#define L1EventBuilder_hpp 1

#include <TTree.h>

#include <atomic>
#include <cmath>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <tuple>
#include <vector>

#include "ChSettings.hpp"
#include "EventData.hpp"

namespace DELILA
{

class L1EventBuilder
{
 public:
  L1EventBuilder();
  ~L1EventBuilder();

  void LoadChSettings(const std::string &fileName);
  void LoadFileList(const std::vector<std::string> &fileList);
  void LoadTimeSettings(const std::string &fileName);
  void SetTimeWindow(double_t timeWindow) { fTimeWindow = timeWindow; }
  void SetCoincidenceWindow(double_t coincidenceWindow)
  {
    fCoincidenceWindow = coincidenceWindow;
  }
  void SetRefMod(uint8_t mod) { fRefMod = mod; }
  void SetRefCh(uint8_t ch) { fRefCh = ch; }

  void BuildEvent(const uint32_t nThreads);
  void Cancel() { fCancelled.store(true); }

 private:
  std::vector<std::vector<ChSettings_t>> fChSettingsVec;
  std::vector<std::vector<std::vector<std::vector<double_t>>>> fTimeSettingsVec;
  double_t fTimeWindow = 0.;
  double_t fCoincidenceWindow = 0.;
  uint8_t fRefMod = 0;
  uint8_t fRefCh = 0;
  std::vector<std::string> fFileList;
  std::mutex fFileListMutex;
  std::atomic<bool> fCancelled{false};

  // Chunked processing configuration to limit memory usage
  static constexpr Long64_t CHUNK_SIZE = 10000000;  // 10M entries per chunk
  static constexpr Long64_t OVERLAP_SIZE = 10000;    // 10k entries overlap for coincidence window

  void DataReader(int threadID, std::vector<std::string> fileList);
};

}  // namespace DELILA

#endif