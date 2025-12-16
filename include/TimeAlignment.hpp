#ifndef TimeAlignment_hpp
#define TimeAlignment_hpp 1

#include <TFile.h>
#include <TH1.h>
#include <TH2.h>

#include <atomic>
#include <mutex>
#include <string>
#include <vector>

#include "ChSettings.hpp"

namespace DELILA
{

const std::string kTimeAlignmentFileName = "timeAlignment.root";
const std::string kTimeSettingsFileName = "timeSettings.json";

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
  void CalculateTimeAlignment();
  void Cancel() { fCancelled.store(true); }

 private:
  std::vector<std::vector<ChSettings_t>> fChSettingsVec;
  std::vector<std::vector<std::unique_ptr<TH2D>>> fHistoTime;
  std::vector<std::vector<std::unique_ptr<TH1D>>> fHistoADC;
  double_t fTimeWindow = 0.;

  std::atomic<bool> fDataProcessFlag{false};
  std::vector<std::string> fFileList;
  std::atomic<bool> fCancelled{false};
  std::mutex fFileListMutex;
  std::mutex fHistogramMutex;  // For mutex-based approach

  // Thread-local histograms for parallel filling
  struct ThreadHistograms {
    std::vector<std::vector<std::unique_ptr<TH2D>>> histoTime;
    std::vector<std::vector<std::unique_ptr<TH1D>>> histoADC;
  };
  std::vector<ThreadHistograms> fThreadHistograms;

  void DataProcess(int threadID);
  void MergeThreadHistograms();
  void SaveHistograms();

  // For fitting ADC spectrum
  // Have to be a different class
  std::vector<TF1 *> FitHist(TH1D *hist);
  std::vector<double> GetPeaks(TH1D *hist, double sigma = 10,
                               double threshold = 0.1);
  Double_t FitFnc(Double_t *pos, Double_t *par);

};  // TimeAlignment

}  // namespace DELILA

#endif