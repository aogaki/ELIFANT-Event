#include "TimeAlignment.hpp"

#include <TF1.h>
#include <TFile.h>
#include <TFileRAII.hpp>
#include <TROOT.h>
#include <TSpectrum.h>
#include <TTree.h>

#include <DELILAExceptions.hpp>
#include <algorithm>
#include <csignal>
#include <iostream>
#include <mutex>
#include <nlohmann/json.hpp>
#include <thread>
#include <tuple>

// Define USE_MUTEX_APPROACH to compare performance
// #define USE_MUTEX_APPROACH

// Global pointer for signal handler access
static DELILA::TimeAlignment* g_timeAlignment = nullptr;

// Signal handler for Ctrl-C
void timeAlignmentSignalHandler(int signal) {
  if (signal == SIGINT) {
    std::cout << "\n\nReceived Ctrl-C! Stopping time alignment threads gracefully..." << std::endl;
    if (g_timeAlignment != nullptr) {
      g_timeAlignment->Cancel();
    }
  }
}

DELILA::TimeAlignment::TimeAlignment()
{
  // Constructor
}
DELILA::TimeAlignment::~TimeAlignment()
{
  // Destructor
  fDataProcessFlag.store(false);
  fFileList.clear();
  fChSettingsVec.clear();
  fHistoTime.clear();
}

void DELILA::TimeAlignment::LoadChSettings(const std::string &fileName)
{
  try {
    fChSettingsVec = ChSettings::GetChSettings(fileName);
    if (fChSettingsVec.empty()) {
      throw DELILA::ConfigException("No channel settings found in file: " + fileName);
    }
  } catch (const DELILA::ConfigException &e) {
    throw;  // Re-throw DELILA exceptions as-is
  } catch (const std::exception &e) {
    throw DELILA::ConfigException("Failed to load channel settings from " + fileName +
                                  ": " + e.what());
  }
}

void DELILA::TimeAlignment::LoadFileList(
    const std::vector<std::string> &fileList)
{
  if (fileList.empty()) {
    throw DELILA::ValidationException("File list is empty");
  }
  fFileList = fileList;
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

  fHistoTime.resize(fChSettingsVec.size());
  fHistoADC.resize(fChSettingsVec.size());
  for (size_t i = 0; i < fChSettingsVec.size(); i++) {
    fHistoTime[i].resize(fChSettingsVec[i].size());
    fHistoADC[i].resize(fChSettingsVec[i].size());
    for (size_t j = 0; j < fChSettingsVec[i].size(); j++) {
      int nBins = fTimeWindow;
      TString histName = Form("hTime_%02zu_%02zu", i, j);
      fHistoTime[i][j] =
          std::make_unique<TH2D>(histName, histName, nBins, -fTimeWindow,
                                 fTimeWindow, maxID, 0, maxID);

      histName = Form("hADC_%02zu_%02zu", i, j);
      fHistoADC[i][j] =
          std::make_unique<TH1D>(histName, histName, 32000, 0, 32000);
      fHistoADC[i][j]->SetDirectory(0);
      fHistoTime[i][j]->SetDirectory(0);
    }
  }
}

void DELILA::TimeAlignment::SaveHistograms()
{
  auto file = DELILA::MakeTFile(kTimeAlignmentFileName.c_str(), "RECREATE");
  if (!file || file->IsZombie()) {
    std::cerr << "Error: Could not open file: " << kTimeAlignmentFileName
              << std::endl;
    return;
  }

  for (size_t i = 0; i < fHistoTime.size(); i++) {
    for (size_t j = 0; j < fHistoTime[i].size(); j++) {
      if (!fHistoTime[i][j]) {
        std::cerr << "Error: Histogram not initialized for module " << i
                  << ", channel " << j << std::endl;
        continue;
      }
      if (fHistoTime[i][j]->GetEntries() > 0) {
        fHistoTime[i][j]->Write();
      }
    }
  }

  for (size_t i = 0; i < fHistoADC.size(); i++) {
    for (size_t j = 0; j < fHistoADC[i].size(); j++) {
      if (!fHistoADC[i][j]) {
        std::cerr << "Error: Histogram not initialized for module " << i
                  << ", channel " << j << std::endl;
        continue;
      }
      auto hist = fHistoADC[i][j].get();  // crazy!
      auto fitVec = FitHist(hist);
      fHistoADC[i][j]->Write();
      for (auto fit : fitVec) {
        if (fit) {
          fit->Write();
        }
      }
    }
  }

  // file->Close() and delete will be called automatically by TFilePtr destructor
  std::cout << "Histograms saved to: " << kTimeAlignmentFileName << std::endl;
}

void DELILA::TimeAlignment::FillHistograms(const int nThreads)
{
  // For sequential data read and multi threading by user code.
  // Disable implicit multi-threading is faster now. ROOT 6.34.08
  ROOT::DisableImplicitMT();
  ROOT::EnableThreadSafety();

  // Setup signal handler for Ctrl-C
  g_timeAlignment = this;
  fCancelled.store(false);
  ::signal(SIGINT, timeAlignmentSignalHandler);

  // Initialize thread-local histograms
  auto maxID = 0;
  for (size_t i = 0; i < fChSettingsVec.size(); i++) {
    for (size_t j = 0; j < fChSettingsVec[i].size(); j++) {
      maxID = std::max(maxID, static_cast<int>(fChSettingsVec[i][j].ID));
    }
  }
  maxID += 1;

#ifndef USE_MUTEX_APPROACH
  fThreadHistograms.resize(nThreads);
  for (int t = 0; t < nThreads; ++t) {
    fThreadHistograms[t].histoTime.resize(fChSettingsVec.size());
    fThreadHistograms[t].histoADC.resize(fChSettingsVec.size());
    for (size_t i = 0; i < fChSettingsVec.size(); i++) {
      fThreadHistograms[t].histoTime[i].resize(fChSettingsVec[i].size());
      fThreadHistograms[t].histoADC[i].resize(fChSettingsVec[i].size());
      for (size_t j = 0; j < fChSettingsVec[i].size(); j++) {
        int nBins = fTimeWindow;
        TString histName = Form("hTime_%02zu_%02zu_thread%d", i, j, t);
        fThreadHistograms[t].histoTime[i][j] =
            std::make_unique<TH2D>(histName, histName, nBins, -fTimeWindow,
                                   fTimeWindow, maxID, 0, maxID);
        fThreadHistograms[t].histoTime[i][j]->SetDirectory(0);

        histName = Form("hADC_%02zu_%02zu_thread%d", i, j, t);
        fThreadHistograms[t].histoADC[i][j] =
            std::make_unique<TH1D>(histName, histName, 32000, 0, 32000);
        fThreadHistograms[t].histoADC[i][j]->SetDirectory(0);
      }
    }
  }
#endif

  std::vector<std::thread> threads;
  fDataProcessFlag.store(true);
  for (int i = 0; i < nThreads; ++i) {
    threads.emplace_back(&DELILA::TimeAlignment::DataProcess, this, i);
  }

  for (auto &thread : threads) {
    if (thread.joinable()) {
      thread.join();
    }
  }

#ifndef USE_MUTEX_APPROACH
  MergeThreadHistograms();
#endif
  SaveHistograms();
}

void DELILA::TimeAlignment::DataProcess(int threadID)
{
  while (fDataProcessFlag.load()) {
    // Check if cancelled
    if (fCancelled.load()) {
      std::lock_guard<std::mutex> lock(fFileListMutex);
      std::cout << "Thread " << threadID << " cancelled by user." << std::endl;
      break;
    }

    auto fileName = std::string();
    {
      std::lock_guard<std::mutex> lock(fFileListMutex);
      if (fFileList.empty()) {
        fDataProcessFlag.store(false);
        break;
      }
      fileName = fFileList.front();
      fFileList.erase(fFileList.begin());
      std::cout << "Processing file: " << fileName << std::endl;
    }
    auto file = DELILA::MakeTFile(fileName.c_str(), "READ");
    if (!file || file->IsZombie()) {
      std::cerr << "Error: Could not open file: " << fileName << std::endl;
      continue;  // Now safe - file will be automatically closed and deleted
    }
    auto tree = static_cast<TTree *>(file->Get("ELIADE_Tree"));
    tree->SetBranchStatus("*", kFALSE);

    UChar_t mod;
    tree->SetBranchStatus("Mod", kTRUE);
    tree->SetBranchAddress("Mod", &mod);
    UChar_t ch;
    tree->SetBranchStatus("Ch", kTRUE);
    tree->SetBranchAddress("Ch", &ch);

    UShort_t chargeLong;
    tree->SetBranchStatus("ChargeLong", kTRUE);
    tree->SetBranchAddress("ChargeLong", &chargeLong);

    Double_t fineTS;
    tree->SetBranchStatus("FineTS", kTRUE);
    tree->SetBranchAddress("FineTS", &fineTS);

    const int64_t nEvents = tree->GetEntries();

    // CHUNKED PROCESSING: Process file in chunks to limit memory usage
    // Instead of loading all entries at once, process 10M at a time
    const int64_t numChunks = (nEvents + CHUNK_SIZE - 1) / CHUNK_SIZE;

    for (int64_t chunkStart = 0; chunkStart < nEvents; chunkStart += CHUNK_SIZE) {
      // Check if cancelled
      if (fCancelled.load()) {
        std::lock_guard<std::mutex> lock(fFileListMutex);
        std::cout << "Thread " << threadID << " cancelled during chunked processing." << std::endl;
        return;
      }

      int64_t chunkEnd = std::min(nEvents, chunkStart + CHUNK_SIZE);

      // Load this chunk from file
      std::vector<std::tuple<UChar_t, UChar_t, Double_t>> dataVec;
      dataVec.reserve(chunkEnd - chunkStart);

      for (int64_t iEve = chunkStart; iEve < chunkEnd; iEve++) {
        tree->GetEntry(iEve);

        // Bounds checking to prevent segmentation fault
        if (mod >= fChSettingsVec.size() || ch >= fChSettingsVec[mod].size()) {
          continue;
        }

        auto threshold = fChSettingsVec[mod][ch].thresholdADC;
        if (chargeLong > threshold) {
#ifdef USE_MUTEX_APPROACH
          {
            std::lock_guard<std::mutex> lock(fHistogramMutex);
            fHistoADC[mod][ch]->Fill(chargeLong);
          }
#else
          fThreadHistograms[threadID].histoADC[mod][ch]->Fill(chargeLong);
#endif
          dataVec.emplace_back(mod, ch, fineTS / 1000.);  // ps -> ns
        }
      }

      // Sort this chunk
      std::sort(dataVec.begin(), dataVec.end(), [](const auto &a, const auto &b) {
        return std::get<2>(a) < std::get<2>(b);
      });

      const auto nGoodEvents = dataVec.size();
    for (int64_t iEve = 0; iEve < nGoodEvents; iEve++) {
      auto mod = std::get<0>(dataVec[iEve]);
      auto ch = std::get<1>(dataVec[iEve]);
      auto fineTS = std::get<2>(dataVec[iEve]);

      if (fChSettingsVec[mod][ch].isEventTrigger) {
        auto time0 = fineTS;
        int origMod = mod;
        int origCh = ch;

        for (auto i = iEve + 1; i < nGoodEvents; i++) {
          auto mod = std::get<0>(dataVec[i]);
          auto ch = std::get<1>(dataVec[i]);
          auto fineTS = std::get<2>(dataVec[i]);

          auto timeDiff = fineTS - time0;
          if (timeDiff > fTimeWindow) {
            break;
          }
#ifdef USE_MUTEX_APPROACH
          {
            std::lock_guard<std::mutex> lock(fHistogramMutex);
            fHistoTime[origMod][origCh]->Fill(timeDiff,
                                              fChSettingsVec[mod][ch].ID);
          }
#else
          fThreadHistograms[threadID].histoTime[origMod][origCh]->Fill(
              timeDiff, fChSettingsVec[mod][ch].ID);
#endif
        }
        for (auto i = iEve - 1; i >= 0; i--) {
          auto mod = std::get<0>(dataVec[i]);
          auto ch = std::get<1>(dataVec[i]);
          auto fineTS = std::get<2>(dataVec[i]);

          auto timeDiff = fineTS - time0;
          if (timeDiff < -fTimeWindow) {
            break;
          }
#ifdef USE_MUTEX_APPROACH
          {
            std::lock_guard<std::mutex> lock(fHistogramMutex);
            fHistoTime[origMod][origCh]->Fill(timeDiff,
                                              fChSettingsVec[mod][ch].ID);
          }
#else
          fThreadHistograms[threadID].histoTime[origMod][origCh]->Fill(
              timeDiff, fChSettingsVec[mod][ch].ID);
#endif
        }
      }
    }

      // Clear this chunk's data and release memory
      dataVec.clear();
      dataVec.shrink_to_fit();
    }  // End chunk loop
    // file will be automatically closed and deleted at end of scope
  }

  {
    std::lock_guard<std::mutex> lock(fFileListMutex);
    std::cout << "Thread " << threadID << " finished." << std::endl;
  }
}

void DELILA::TimeAlignment::MergeThreadHistograms()
{
  std::cout << "Merging thread histograms..." << std::endl;

  // Merge all thread-local histograms into the main histograms
  for (size_t i = 0; i < fChSettingsVec.size(); i++) {
    for (size_t j = 0; j < fChSettingsVec[i].size(); j++) {
      // Move first thread's histogram as base
      if (fThreadHistograms.size() > 0 &&
          fThreadHistograms[0].histoTime[i][j]) {
        fHistoTime[i][j] = std::move(fThreadHistograms[0].histoTime[i][j]);
        fHistoADC[i][j] = std::move(fThreadHistograms[0].histoADC[i][j]);

        // Add all other threads' histograms
        for (size_t t = 1; t < fThreadHistograms.size(); t++) {
          if (fThreadHistograms[t].histoTime[i][j]) {
            fHistoTime[i][j]->Add(fThreadHistograms[t].histoTime[i][j].get());
          }
          if (fThreadHistograms[t].histoADC[i][j]) {
            fHistoADC[i][j]->Add(fThreadHistograms[t].histoADC[i][j].get());
          }
        }

        // Rename to final names
        TString histName = Form("hTime_%02zu_%02zu", i, j);
        fHistoTime[i][j]->SetName(histName);
        fHistoTime[i][j]->SetTitle(histName);

        histName = Form("hADC_%02zu_%02zu", i, j);
        fHistoADC[i][j]->SetName(histName);
        fHistoADC[i][j]->SetTitle(histName);
      }
    }
  }

  // Clear thread histograms to free memory
  fThreadHistograms.clear();
  std::cout << "Histogram merging completed." << std::endl;
}

void DELILA::TimeAlignment::CalculateTimeAlignment()
{
  TString fileName = kTimeAlignmentFileName;
  auto file = DELILA::MakeTFile(fileName, "READ");
  if (!file || file->IsZombie()) {
    std::cerr << "Error: Could not open file: " << fileName << std::endl;
    return;
  }

  std::vector<std::vector<std::vector<std::vector<double_t>>>> timeSettingsVec;

  timeSettingsVec.resize(fChSettingsVec.size());
  for (auto iMod = 0; iMod < fChSettingsVec.size(); iMod++) {
    timeSettingsVec[iMod].resize(fChSettingsVec[iMod].size());
    for (auto iCh = 0; iCh < fChSettingsVec[iMod].size(); iCh++) {
      // Read the histogram from the file
      auto histName = Form("hTime_%02d_%02d", iMod, iCh);
      auto hist2D = static_cast<TH2D *>(file->Get(histName));
      if (!hist2D) {
        std::cerr << "Error: Could not find histogram: " << histName
                  << std::endl;
        // file->Close();
        // delete file;
        continue;
      }
      auto histVec = std::vector<std::vector<TH1D *>>();
      histVec.resize(fChSettingsVec.size());
      for (size_t i = 0; i < fChSettingsVec.size(); i++) {
        histVec[i].resize(fChSettingsVec[i].size());
        for (size_t j = 0; j < fChSettingsVec[i].size(); j++) {
          auto id = fChSettingsVec[i][j].ID;
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

      std::vector<std::vector<double_t>> localVec;
      localVec.resize(fChSettingsVec.size());
      for (size_t i = 0; i < fChSettingsVec.size(); i++) {
        localVec[i].resize(fChSettingsVec[i].size());
        for (size_t j = 0; j < fChSettingsVec[i].size(); j++) {
          auto hist = histVec[i][j];
          auto timeOffset = 0.;
          auto leftEdge = 0.;
          auto rightEdge = 0.;
          if (hist->GetEntries() > 0) {
            const auto maxBin = hist->GetMaximumBin();
            const auto maxBinCenter = hist->GetBinCenter(maxBin);
            timeOffset = maxBinCenter;

            localVec[i][j] = timeOffset;
          }
        }

        timeSettingsVec[iMod][iCh] = localVec;
      }
    }
  }
  // file will be automatically closed and deleted

  nlohmann::json jsonData = nlohmann::json::array();
  for (auto iRefMod = 0; iRefMod < fChSettingsVec.size(); iRefMod++) {
    nlohmann::json refModData = nlohmann::json::array();
    for (auto iRefCh = 0; iRefCh < fChSettingsVec[iRefMod].size(); iRefCh++) {
      nlohmann::json refChData = nlohmann::json::array();
      if (timeSettingsVec[iRefMod][iRefCh].size() == 0) {
        continue;
      }
      for (auto iMod = 0; iMod < fChSettingsVec.size(); iMod++) {
        nlohmann::json modData = nlohmann::json::array();
        for (auto iCh = 0; iCh < fChSettingsVec[iMod].size(); iCh++) {
          auto timeOffset = timeSettingsVec[iRefMod][iRefCh][iMod][iCh];
          if (timeOffset != 0.) {
            if (iRefMod == iMod && iRefCh == iCh) {
              timeOffset = 0.;  // Reference channel has no offset
            }
            std::cout << iRefMod << " " << iRefCh << " " << iMod << " " << iCh
                      << " TimeOffset: " << timeOffset << std::endl;
          }
          nlohmann::json chData;
          chData["TimeOffset"] = timeOffset;
          modData.push_back(chData);
        }
        refChData.push_back(modData);
      }
      refModData.push_back(refChData);
    }
    jsonData.push_back(refModData);
  }

  std::ofstream ofs(kTimeSettingsFileName);
  ofs << jsonData.dump(4) << std::endl;
  ofs.close();
  std::cout << kTimeSettingsFileName << " generated." << std::endl;
}

std::vector<TF1 *> DELILA::TimeAlignment::FitHist(TH1D *hist)
{
  // Fit the histogram to find the peaks

  std::vector<TF1 *> fitVec;
  auto peaks = GetPeaks(hist, 50, 0.2);
  TF1 *simpleGaus = new TF1("simpleGaus", "gaus");
  auto histName = hist->GetName();
  for (int i = 0; i < peaks.size(); i++) {
    // Get peak information
    auto peakPos = peaks[i];
    auto peakHeight = hist->GetBinContent(hist->FindBin(peakPos));
    simpleGaus->SetRange(peakPos - 10, peakPos + 10);
    simpleGaus->SetParameters(peakHeight, peakPos, 1);
    hist->Fit(simpleGaus, "RQ");
    auto peakSigma = simpleGaus->GetParameter(2);

    // Get BG information
    auto bgLeft = hist->GetBinContent(hist->FindBin(peakPos - 2 * peakSigma));
    auto bgRight = hist->GetBinContent(hist->FindBin(peakPos + 2 * peakSigma));
    auto bgSlope = (bgRight - bgLeft) / (4 * peakSigma);
    auto bgIntercept = bgLeft - bgSlope * (peakPos - 4 * peakSigma);

    auto fncName = TString(histName) + Form("_f%d", i);
    auto fit = new TF1(fncName, "gaus(0)+pol1(3)", peakPos - 2 * peakSigma,
                       peakPos + 2 * peakSigma);
    fit->SetParNames("height", "mean", "sigma", "bgIntercept", "bgSlope");
    fit->SetParameters(peakHeight, peakPos, peakSigma, bgIntercept, bgSlope);
    hist->Fit(fit, "RQ");
    hist->Fit(fit, "RQ");
    fitVec.push_back(fit);
  }

  return fitVec;
}

std::vector<double> DELILA::TimeAlignment::GetPeaks(TH1D *hist, double sigma,
                                                    double threshold)
{
  TSpectrum *s = new TSpectrum(20);
  int n = s->Search(hist, sigma, "", threshold);
  std::vector<double> peaks;

  double *p = s->GetPositionX();
  for (int i = 0; i < n; i++) {
    peaks.push_back(p[i]);
  }

  std::sort(peaks.begin(), peaks.end());
  return peaks;
}