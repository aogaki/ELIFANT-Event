#ifndef L1EventBuilder_hpp
#define L1EventBuilder_hpp 1

#include <TTree.h>

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

 private:
  std::vector<std::vector<ChSettings_t>> fChSettingsVec;
  std::vector<std::vector<std::vector<std::vector<double_t>>>> fTimeSettingsVec;
  double_t fTimeWindow = 0.;
  double_t fCoincidenceWindow = 0.;
  uint8_t fRefMod = 0;
  uint8_t fRefCh = 0;
  std::vector<std::string> fFileList;
  std::mutex fFileListMutex;

  void DataReader(int threadID, std::vector<std::string> fileList);
};

}  // namespace DELILA

#endif