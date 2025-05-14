#include "TimeAlignment.hpp"

#include <TFile.h>
#include <TROOT.h>
#include <TTree.h>

#include <algorithm>
#include <mutex>
#include <nlohmann/json.hpp>
#include <thread>
#include <tuple>

DELILA::TimeAlignment::TimeAlignment()
{
  // Constructor
}
DELILA::TimeAlignment::~TimeAlignment()
{
  // Destructor
  fDataProcessFlag = false;
  fFileList.clear();
  fChSettingsVec.clear();
  fIDTable.clear();
  fTriggerTable.clear();
  fHistograms.clear();
}

void DELILA::TimeAlignment::LoadChSettings(const std::string &fileName)
{
  fChSettingsVec = ChSettings::GetChSettings(fileName);
  if (fChSettingsVec.size() == 0) {
    std::cerr << "Error: No channel settings found in file: " << fileName
              << std::endl;
    return;
  }

  fIDTable.resize(fChSettingsVec.size());
  for (size_t i = 0; i < fChSettingsVec.size(); i++) {
    fIDTable[i].resize(fChSettingsVec[i].size());
    for (size_t j = 0; j < fChSettingsVec[i].size(); j++) {
      fIDTable[i][j] = fChSettingsVec[i][j].ID;
    }
  }

  fTriggerTable.resize(fChSettingsVec.size());
  for (size_t i = 0; i < fChSettingsVec.size(); i++) {
    fTriggerTable[i].resize(fChSettingsVec[i].size());
    for (size_t j = 0; j < fChSettingsVec[i].size(); j++) {
      fTriggerTable[i][j] = fChSettingsVec[i][j].isEventTrigger;
    }
  }

  fDetectorTypeTable.resize(fChSettingsVec.size());
  for (size_t i = 0; i < fChSettingsVec.size(); i++) {
    fDetectorTypeTable[i].resize(fChSettingsVec[i].size());
    for (size_t j = 0; j < fChSettingsVec[i].size(); j++) {
      fDetectorTypeTable[i][j] =
          ChSettings::GetDetectorType(fChSettingsVec[i][j].detectorType);
    }
  }
}

void DELILA::TimeAlignment::LoadFileList(
    const std::vector<std::string> &fileList)
{
  fFileList = fileList;
  if (fFileList.size() == 0) {
    std::cerr << "Error: No files found." << std::endl;
    return;
  }
}

void DELILA::TimeAlignment::InitHistograms()
{
  auto maxID = 0;
  for (size_t i = 0; i < fChSettingsVec.size(); i++) {
    for (size_t j = 0; j < fChSettingsVec[i].size(); j++) {
      maxID = std::max(maxID, static_cast<int>(fChSettingsVec[i][j].ID));
    }
  }
  maxID += 1;  // Adjust for zero-based indexing

  fHistograms.resize(fChSettingsVec.size());
  for (size_t i = 0; i < fChSettingsVec.size(); i++) {
    fHistograms[i].resize(fChSettingsVec[i].size());
    for (size_t j = 0; j < fChSettingsVec[i].size(); j++) {
      int nBins = 20 * fTimeWindow;
      TString histName = Form("h_%02zu_%02zu", i, j);
      fHistograms[i][j] =
          std::make_unique<TH2D>(histName, histName, nBins, -fTimeWindow,
                                 fTimeWindow, maxID, 0, maxID);
    }
  }
}

void DELILA::TimeAlignment::SaveHistograms()
{
  auto file = new TFile(kTimeAlignmentFileName.c_str(), "RECREATE");
  if (!file || file->IsZombie()) {
    std::cerr << "Error: Could not open file: " << kTimeAlignmentFileName
              << std::endl;
    return;
  }

  for (size_t i = 0; i < fHistograms.size(); i++) {
    for (size_t j = 0; j < fHistograms[i].size(); j++) {
      if (!fHistograms[i][j]) {
        std::cerr << "Error: Histogram not initialized for module " << i
                  << ", channel " << j << std::endl;
        continue;
      }
      fHistograms[i][j]->Write();
    }
  }

  file->Close();
  delete file;
  std::cout << "Histograms saved to: " << kTimeAlignmentFileName << std::endl;
}

void DELILA::TimeAlignment::FillHistograms(const int nThreads)
{
  // For sequential data read and multi threading by user code.
  // Disable implicit multi-threading is faster now. ROOT 6.34.08
  ROOT::DisableImplicitMT();
  ROOT::EnableThreadSafety();

  // check kTimeAlignmentFileName exists
  auto file = new TFile(kTimeAlignmentFileName.c_str(), "READ");
  if (file->IsOpen()) {
    std::cout << "Time histogram file already exists: "
              << kTimeAlignmentFileName << std::endl;
    std::cout << "Skip to generate." << std::endl;
    file->Close();
    delete file;
    return;
  } else {
    std::vector<std::thread> threads;
    fDataProcessFlag = true;
    for (int i = 0; i < nThreads; ++i) {
      threads.emplace_back(&DELILA::TimeAlignment::DataProcess, this, i);
    }

    for (auto &thread : threads) {
      if (thread.joinable()) {
        thread.join();
      }
    }
    SaveHistograms();
  }
}

void DELILA::TimeAlignment::DataProcess(int threadID)
{
  while (fDataProcessFlag) {
    auto fileName = std::string();
    {
      std::lock_guard<std::mutex> lock(fFileListMutex);
      if (fFileList.empty()) {
        fDataProcessFlag = false;
        break;
      }
      fileName = fFileList.front();
      fFileList.erase(fFileList.begin());
      std::cout << "Processing file: " << fileName << std::endl;
    }
    auto file = new TFile(fileName.c_str(), "READ");
    if (!file || file->IsZombie()) {
      std::cerr << "Error: Could not open file: " << fileName << std::endl;
      continue;
    }
    auto tree = static_cast<TTree *>(file->Get("ELIADE_Tree"));
    tree->SetBranchStatus("*", kFALSE);

    UChar_t mod;
    tree->SetBranchStatus("Mod", kTRUE);
    tree->SetBranchAddress("Mod", &mod);
    UChar_t ch;
    tree->SetBranchStatus("Ch", kTRUE);
    tree->SetBranchAddress("Ch", &ch);

    Double_t fineTS;
    tree->SetBranchStatus("FineTS", kTRUE);
    tree->SetBranchAddress("FineTS", &fineTS);

    const uint64_t nEvents = tree->GetEntries();
    std::vector<std::tuple<UChar_t, UChar_t, Double_t>> dataVec;
    dataVec.reserve(nEvents);
    for (int64_t iEve = 0; iEve < nEvents; iEve++) {
      tree->GetEntry(iEve);
      dataVec.emplace_back(mod, ch, fineTS / 1000.);  // ps -> ns
    }
    file->Close();
    delete file;

    std::sort(dataVec.begin(), dataVec.end(), [](const auto &a, const auto &b) {
      return std::get<2>(a) < std::get<2>(b);
    });

    for (int64_t iEve = 0; iEve < nEvents; iEve++) {
      auto mod = std::get<0>(dataVec[iEve]);
      auto ch = std::get<1>(dataVec[iEve]);
      auto fineTS = std::get<2>(dataVec[iEve]);

      if (fTriggerTable[mod][ch]) {
        auto time0 = fineTS;
        int origMod = mod;
        int origCh = ch;

        for (auto i = iEve + 1; i < nEvents; i++) {
          auto mod = std::get<0>(dataVec[i]);
          auto ch = std::get<1>(dataVec[i]);
          auto fineTS = std::get<2>(dataVec[i]);

          auto timeDiff = fineTS - time0;
          if (timeDiff > fTimeWindow) {
            break;
          }
          fHistograms[origMod][origCh]->Fill(timeDiff, fIDTable[mod][ch]);
        }
        for (auto i = iEve - 1; i >= 0; i--) {
          auto mod = std::get<0>(dataVec[i]);
          auto ch = std::get<1>(dataVec[i]);
          auto fineTS = std::get<2>(dataVec[i]);

          auto timeDiff = fineTS - time0;
          if (timeDiff < -fTimeWindow) {
            break;
          }
          fHistograms[origMod][origCh]->Fill(timeDiff, fIDTable[mod][ch]);
        }
      }
    }
    {
      std::lock_guard<std::mutex> lock(fFileListMutex);
      std::cout << "Processed file: " << fileName << std::endl;
    }
  }

  {
    std::lock_guard<std::mutex> lock(fFileListMutex);
    std::cout << "Thread " << threadID << " finished." << std::endl;
  }
}

void DELILA::TimeAlignment::CalculateTimeAlignment(const double_t thFactor)
{
  TString fileName = kTimeAlignmentFileName;
  auto file = new TFile(fileName, "READ");
  if (!file || file->IsZombie()) {
    std::cerr << "Error: Could not open file: " << fileName << std::endl;
    return;
  }

  std::vector<std::vector<std::vector<std::vector<TimeSettings_t>>>>
      timeSettingsVec;

  timeSettingsVec.resize(fChSettingsVec.size());
  for (auto iMod = 0; iMod < fChSettingsVec.size(); iMod++) {
    timeSettingsVec[iMod].resize(fChSettingsVec[iMod].size());
    for (auto iCh = 0; iCh < fChSettingsVec[iMod].size(); iCh++) {
      // Read the histogram from the file
      auto histName = Form("h_%02d_%02d", iMod, iCh);
      auto hist2D = static_cast<TH2D *>(file->Get(histName));
      if (!hist2D) {
        std::cerr << "Error: Could not find histogram: " << histName
                  << std::endl;
        file->Close();
        delete file;
      }
      auto histVec = std::vector<std::vector<TH1D *>>();
      histVec.resize(fChSettingsVec.size());
      for (size_t i = 0; i < fChSettingsVec.size(); i++) {
        histVec[i].resize(fChSettingsVec[i].size());
        for (size_t j = 0; j < fChSettingsVec[i].size(); j++) {
          auto id = fIDTable[i][j];
          auto bin = id + 1;
          histVec[i][j] = hist2D->ProjectionX(Form("hpx_%04d", id), bin, bin);
          auto detectorType =
              ChSettings::GetDetectorType(fChSettingsVec[i][j].detectorType);
          if (detectorType == DetectorType::AC) {
            histVec[i][j]->Rebin(10);
          } else if (detectorType == DetectorType::HPGe) {
            histVec[i][j]->Rebin(100);
          } else if (detectorType == DetectorType::PMT) {
            // histVec[i][j]->Rebin(2);
          }
        }
      }

      std::vector<std::vector<TimeSettings_t>> localVec;
      localVec.resize(fChSettingsVec.size());
      for (size_t i = 0; i < fChSettingsVec.size(); i++) {
        localVec[i].resize(fChSettingsVec[i].size());
        for (size_t j = 0; j < fChSettingsVec[i].size(); j++) {
          auto hist = histVec[i][j];
          auto timeOffset = 0.;
          auto leftEdge = 0.;
          auto rightEdge = 0.;
          if (hist->GetEntries() > 0) {
            auto baseLine = 0.;
            constexpr auto sampleBins = 32;
            for (auto i = 0; i < sampleBins; i++) {
              baseLine += hist->GetBinContent(i);
            }
            baseLine /= sampleBins;

            const auto maxBin = hist->GetMaximumBin();
            const auto maxBinCenter = hist->GetBinCenter(maxBin);
            timeOffset = maxBinCenter;

            const auto maxBinContent = hist->GetBinContent(maxBin);
            const auto height = maxBinContent - baseLine;
            const auto th = baseLine + thFactor * height;
            auto leftBin = maxBin;
            auto rightBin = maxBin;
            while (leftBin > 0 && hist->GetBinContent(leftBin) > th) {
              leftBin--;
            }
            while (rightBin < hist->GetNbinsX() &&
                   hist->GetBinContent(rightBin) > th) {
              rightBin++;
            }
            leftEdge = hist->GetBinLowEdge(leftBin);
            rightEdge = hist->GetBinLowEdge(rightBin + 1);

            localVec[i][j].TimeOffset = timeOffset;
            localVec[i][j].TimeWindowLeftEdge = leftEdge;
            localVec[i][j].TimeWindowRightEdge = rightEdge;
          }
        }

        timeSettingsVec[iMod][iCh] = localVec;
      }
    }
  }

  file->Close();
  delete file;

  nlohmann::json jsonData = nlohmann::json::array();
  for (auto iRefMod = 0; iRefMod < fChSettingsVec.size(); iRefMod++) {
    nlohmann::json refModData = nlohmann::json::array();
    for (auto iRefCh = 0; iRefCh < fChSettingsVec[iRefMod].size(); iRefCh++) {
      nlohmann::json refChData = nlohmann::json::array();
      for (auto iMod = 0; iMod < fChSettingsVec.size(); iMod++) {
        nlohmann::json modData = nlohmann::json::array();
        for (auto iCh = 0; iCh < fChSettingsVec[iMod].size(); iCh++) {
          auto timeOffset =
              timeSettingsVec[iRefMod][iRefCh][iMod][iCh].TimeOffset;
          if (timeOffset != 0.) {
            std::cout << iRefMod << " " << iRefCh << " " << iMod << " " << iCh
                      << " TimeOffset: " << timeOffset << std::endl;
          }
          nlohmann::json chData;
          chData["TimeOffset"] = timeOffset;
          chData["TimeWindowLeftEdge"] =
              timeSettingsVec[iRefMod][iRefCh][iMod][iCh].TimeWindowLeftEdge;
          chData["TimeWindowRightEdge"] =
              timeSettingsVec[iRefMod][iRefCh][iMod][iCh].TimeWindowRightEdge;
          modData.push_back(chData);
        }
        refChData.push_back(modData);
      }
      refModData.push_back(refChData);
    }
    jsonData.push_back(refModData);
  }
  std::ofstream ofs("timeSettings.json");
  ofs << jsonData.dump(4) << std::endl;
  ofs.close();
  std::cout << "timeSettings.json generated." << std::endl;
}