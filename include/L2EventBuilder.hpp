#ifndef L2EventBuilder_hpp
#define L2EventBuilder_hpp 1

#include <atomic>
#include <cmath>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "ChSettings.hpp"
#include "EventData.hpp"
#include "L2Conditions.hpp"

namespace DELILA
{

class L2EventBuilder
{
 public:
  L2EventBuilder();
  ~L2EventBuilder();

  void LoadChSettings(const std::string &fileName);
  void LoadL2Settings(const std::string &fileName);
  void SetCoincidenceWindow(double_t coincidenceWindow)
  {
    fCoincidenceWindow = coincidenceWindow;
  }

  void BuildEvent(uint32_t nThreads);
  void Cancel() { fCancelled.store(true); }

 private:
  std::vector<std::vector<ChSettings_t>> fChSettingsVec;
  double_t fCoincidenceWindow = 0.;

  std::vector<std::string> fFileList;
  std::atomic<bool> fCancelled{false};
  void GetFileList(std::string key);
  void ProcessData(const uint32_t threadID, const std::string &fileName,
                   std::vector<L2Counter> localCounterVec,
                   std::vector<L2Flag> localFlagVec,
                   std::vector<L2DataAcceptance> localDataAcceptanceVec);
  std::mutex fMutex;

  std::vector<L2Counter> fCounterVec;
  std::vector<L2Flag> fFlagVec;
  std::vector<L2DataAcceptance> fDataAcceptanceVec;

  void MergeFiles();
};

}  // namespace DELILA

#endif