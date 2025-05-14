#ifndef TimeAlignment_hpp
#define TimeAlignment_hpp 1

#include <TFile.h>
#include <TH2.h>

#include <mutex>
#include <string>
#include <vector>

#include "ChSettings.hpp"

namespace DELILA
{

const std::string kTimeAlignmentFileName = "timeAlignment.root";

class TimeAlignment
{
 public:
  TimeAlignment();
  ~TimeAlignment();

  void LoadChSettings(const std::string &fileName);
  void LoadFileList(const std::vector<std::string> &fileList);
  void SetTimeWindow(double_t timeWindow) { fTimeWindow = timeWindow; }
  void InitHistograms();
  void FillHistograms(const int nThreads);
  void CalculateTimeAlignment(const double_t thFactor = 0.05);

 private:
  std::vector<std::vector<ChSettings_t>> fChSettingsVec;
  std::vector<std::vector<int32_t>> fIDTable;
  std::vector<std::vector<bool>> fTriggerTable;
  std::vector<std::vector<DetectorType>> fDetectorTypeTable;
  std::vector<std::vector<std::unique_ptr<TH2D>>> fHistograms;
  double_t fTimeWindow = 0.;

  bool fDataProcessFlag = false;
  std::vector<std::string> fFileList;
  std::mutex fFileListMutex;
  void DataProcess(int threadID);
  void SaveHistograms();

};  // TimeAlignment

}  // namespace DELILA

#endif