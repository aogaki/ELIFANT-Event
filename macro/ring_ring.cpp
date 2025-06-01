#include <TCanvas.h>
#include <TChain.h>
#include <TCutG.h>
#include <TF1.h>
#include <TFile.h>
#include <TH1.h>
#include <TH2.h>
#include <TROOT.h>
#include <TSystem.h>
#include <TTree.h>

#include <chrono>
#include <filesystem>
#include <iostream>
#include <mutex>
#include <nlohmann/json.hpp>
#include <string>
#include <thread>
#include <vector>

#include "ChSettings.hpp"
#include "EventData.hpp"
#include "L2Conditions.hpp"
#include "L2EventBuilder.hpp"

class RingInfo
{
 public:
  RingInfo() = default;
  RingInfo(Int_t si, Int_t ring, Double_t energy)
      : si(si), ring(ring), energy(energy)
  {
  }
  ~RingInfo() = default;

  Int_t si = -1;  // 0 is dE, 1 is E
  Int_t ring = -1;
  Int_t sector = -1;
  Double_t energy = -1.;
};

std::vector<std::string> GetFileList(const std::string dirName)
{
  std::vector<std::string> fileList;

  auto searchKey = Form("L2_");
  for (const auto &entry : std::filesystem::directory_iterator(dirName)) {
    if (entry.path().string().find(searchKey) != std::string::npos) {
      fileList.push_back(entry.path().string());
    }
  }

  return fileList;
}

Double_t GetCalibratedEnergy(const DELILA::ChSettings_t &chSetting,
                             const UShort_t &adc)
{
  return chSetting.p0 + chSetting.p1 * adc + chSetting.p2 * adc * adc +
         chSetting.p3 * adc * adc * adc;
}

Int_t GetRing(Int_t mod, Int_t ch)
{
  constexpr auto errorRingNo = 47;  // Error code for invalid ring number

  if (ch > 14 || ch < 0) {
    return errorRingNo;  // Invalid channel
  }

  if (mod == 0) {
    return errorRingNo;  // Invalid ring for mod 0
  } else if (mod == 4) {
    return errorRingNo;  // Invalid ring for mod 0
  } else if (mod == 1) {
    return 30 + (14 - ch);
  } else if (mod == 2) {
    return (14 - ch);
  } else if (mod == 3) {
    return 15 + (14 - ch);
  } else if (mod == 5) {
    return (14 - ch);
  } else if (mod == 6) {
    return 15 + (14 - ch);
  } else if (mod == 7) {
    return 30 + (14 - ch);
  }

  return errorRingNo;  // Invalid module
}

Int_t GetSector(Int_t mod, Int_t ch)
{
  // Need to check real position

  constexpr auto errorSectorNo = 16;  // Error code for invalid sector number
  if (mod != 0 && mod != 4) {
    return errorSectorNo;  // Invalid module for sector
  }

  if (mod == 0) {
    return ch;  // dE module, sector is the channel number
  } else if (mod == 4) {
    if (ch % 2 == 0) {
      return ch + 1;
    } else {
      return ch - 1;
    }
  }
}

constexpr uint32_t nModules = 11;
constexpr uint32_t nChannels = 32;
TH1D *histADC[nModules][nChannels];
TH1D *histEnergy[nModules][nChannels];
constexpr uint32_t nSectors = 16;
constexpr uint32_t nRings = 48;
TH2D *histSectorSector;
TH2D *histRingRingCorrelation;
TH2D *histRingRing[nRings][nRings];
TH2D *histRingRingSum[nRings];
TH2D *histRingRingSumTotal;
void InitHists()
{
  auto settingsFileName = "./chSettings.json";
  auto chSettingsVec = DELILA::ChSettings::GetChSettings(settingsFileName);

  histSectorSector =
      new TH2D("histSectorSector", "dE Sector - E Sector Correlation", nSectors,
               -0.5, nSectors - 0.5, nSectors, -0.5, nSectors - 0.5);
  histSectorSector->SetXTitle("E Sector");
  histSectorSector->SetYTitle("dE Sector");

  histRingRingCorrelation =
      new TH2D("histRingRingCorrelation", "dE Ring - E Ring Correlation",
               nRings, -0.5, nRings - 0.5, nRings, -0.5, nRings - 0.5);
  histRingRingCorrelation->SetXTitle("E Ring");
  histRingRingCorrelation->SetYTitle("dE Ring");

  for (uint32_t i = 0; i < nRings; i++) {
    for (uint32_t j = 0; j < nRings; j++) {
      histRingRing[i][j] =
          new TH2D(Form("histRingRing_%d_%d", i, j),
                   Form("dE Ring %02d - E Ring %02d Correlation", i, j), 500,
                   0.5, 20000.5, 500, 0.5, 20000.5);
      histRingRing[i][j]->SetXTitle("E Ring");
      histRingRing[i][j]->SetYTitle("dE Ring");
    }
  }

  for (uint32_t i = 0; i < nRings; i++) {
    histRingRingSum[i] =
        new TH2D(Form("histRingRingSum_%d", i),
                 Form("dE Ring %02d - E Ring Sum Correlation", i), 500, 0.5,
                 20000.5, 500, 0.5, 20000.5);
    histRingRingSum[i]->SetXTitle("E Ring");
    histRingRingSum[i]->SetYTitle("dE Ring");
  }
  histRingRingSumTotal =
      new TH2D("histRingRingSumTotal", "dE Ring - E Ring Sum Correlation", 500,
               0.5, 20000.5, 500, 0.5, 20000.5);

  constexpr uint32_t nBins = 32000;
  for (uint32_t i = 0; i < nModules; i++) {
    for (uint32_t j = 0; j < nChannels; j++) {
      histADC[i][j] = new TH1D(Form("histADC_%d_%d", i, j),
                               Form("Energy Module%02d Channel%02d", i, j),
                               nBins, 0.5, nBins + 0.5);
      histADC[i][j]->SetXTitle("ADC channel");
    }
  }

  for (uint32_t i = 0; i < nModules; i++) {
    for (uint32_t j = 0; j < nChannels; j++) {
      if (i >= chSettingsVec.size() || j >= chSettingsVec.at(i).size()) {
        continue;
      }
      auto chSetting = chSettingsVec.at(i).at(j);
      std::array<double_t, nBins + 1> binTable;
      for (uint16_t k = 0; k < nBins + 1; k++) {
        auto nextEdge = GetCalibratedEnergy(chSetting, k);
        if (k != 0) {
          auto previousEdge = binTable.at(k - 1);
          if (nextEdge < previousEdge) {
            nextEdge = previousEdge + 0.1;
          }
        }
        binTable.at(k) = nextEdge;
      }
      histEnergy[i][j] = new TH1D(Form("histEnergy_%d_%d", i, j),
                                  Form("Energy Module%02d Channel%02d", i, j),
                                  nBins, binTable.data());
      histEnergy[i][j]->SetXTitle("Energy [keV]");
    }
  }
}

std::mutex counterMutex;
uint64_t totalEvents = 0;
uint64_t processedEvents = 0;
std::vector<bool> IsFinished;
void AnalysisThread(TString fileName, uint32_t threadID)
{
  ROOT::EnableThreadSafety();

  counterMutex.lock();
  auto settingsFileName = "./chSettings.json";
  auto chSettingsVec = DELILA::ChSettings::GetChSettings(settingsFileName);
  counterMutex.unlock();

  auto file = TFile::Open(fileName, "READ");
  if (!file) {
    std::cerr << "File not found: events.root" << std::endl;
    return;
  }
  auto tree = dynamic_cast<TTree *>(file->Get("L2EventData"));
  if (!tree) {
    std::cerr << "Tree not found: Event_Tree" << std::endl;
    return;
  }

  DELILA::EventData eventData;
  tree->SetBranchAddress("TriggerTime", &eventData.triggerTime);
  tree->SetBranchAddress("EventDataVec", &eventData.eventDataVec);

  ULong64_t ESectorCounter = 0;
  tree->SetBranchAddress("E_Sector_Counter", &ESectorCounter);
  ULong64_t dESectorCounter = 0;
  tree->SetBranchAddress("dE_Sector_Counter", &dESectorCounter);

  auto const nEntries = tree->GetEntries();
  {
    std::lock_guard<std::mutex> lock(counterMutex);
    totalEvents += nEntries;
  }

  //   for (auto iEve = 0; iEve < 10000; iEve++) {
  for (auto iEve = 0; iEve < nEntries; iEve++) {
    tree->GetEntry(iEve);
    constexpr auto nProcess = 1000;
    if (iEve % nProcess == 0) {
      std::lock_guard<std::mutex> lock(counterMutex);
      processedEvents += nProcess;
    }

    std::vector<uint32_t> sectorVec(nSectors, 0);
    std::vector<uint32_t> dESectorVec(nSectors, 0);

    std::vector<RingInfo> ringVec;
    std::vector<RingInfo> dERingVec;

    for (auto &event : *eventData.eventDataVec) {
      if (event.mod >= nModules || event.ch >= nChannels) {
        continue;  // Skip invalid channels
      }

      auto chSetting = chSettingsVec.at(event.mod).at(event.ch);
      // if (event.isWithAC) {
      //   continue;  // Skip events with AC data
      // }
      histADC[event.mod][event.ch]->Fill(event.chargeLong);
      auto ene = GetCalibratedEnergy(chSetting, event.chargeLong);
      histEnergy[event.mod][event.ch]->Fill(ene);

      if (event.mod == 0) {
        auto sector = GetSector(event.mod, event.ch);
        dESectorVec.at(sector)++;
      } else if (event.mod == 4) {
        auto sector = GetSector(event.mod, event.ch);
        sectorVec.at(sector)++;
      } else if (event.mod >= 1 && event.mod <= 3) {
        auto ring = GetRing(event.mod, event.ch);
        auto ene = GetCalibratedEnergy(chSetting, event.chargeLong);
        dERingVec.push_back({0, ring, ene});
      } else if (event.mod >= 5 && event.mod <= 7) {
        auto ring = GetRing(event.mod, event.ch);
        auto ene = GetCalibratedEnergy(chSetting, event.chargeLong);
        ringVec.push_back({1, ring, ene});
      }
    }

    for (uint32_t i = 0; i < nSectors; i++) {
      for (uint32_t j = 0; j < nSectors; j++) {
        if (sectorVec.at(i) > 0 && dESectorVec.at(j) > 0) {
          auto e = i;
          auto dE = j;
          histSectorSector->Fill(e, dE);
        }
      }
    }

    for (auto &eRing : ringVec) {
      for (auto &dERing : dERingVec) {
        histRingRing[dERing.ring][eRing.ring]->Fill(eRing.energy,
                                                    dERing.energy);
        histRingRingCorrelation->Fill(eRing.ring, dERing.ring);
      }
    }
  }

  IsFinished.at(threadID) = true;
  file->Close();
}

void ring_ring()
{
  gSystem->Load("libEveBuilder.dylib");  // For macOS
  // gSystem->Load("libEveBuilder.so"); // For Linux

  ROOT::EnableThreadSafety();

  std::cout << "Initializing..." << std::endl;
  InitHists();

  auto fileList = GetFileList("./sum/");

  auto startTime = std::chrono::high_resolution_clock::now();
  auto lastTime = startTime;
  std::vector<std::thread> threads;
  for (uint32_t i = 0; i < fileList.size(); i++) {
    threads.emplace_back(AnalysisThread, fileList.at(i), i);
    IsFinished.push_back(false);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  while (true) {
    if (std::all_of(IsFinished.begin(), IsFinished.end(),
                    [](bool b) { return b; })) {
      break;
    }
    auto now = std::chrono::high_resolution_clock::now();
    auto duration =
        std::chrono::duration_cast<std::chrono::milliseconds>(now - lastTime);
    if (duration.count() > 1000) {
      counterMutex.lock();
      auto finishedEvents = processedEvents;
      counterMutex.unlock();
      auto elapsed =
          std::chrono::duration_cast<std::chrono::milliseconds>(now - startTime)
              .count();
      auto remainingTime =
          (totalEvents - finishedEvents) * elapsed / finishedEvents / 1.e3;

      std::cout << "\b\r" << "Processing event " << finishedEvents << " / "
                << totalEvents << ", " << int(remainingTime) << "s  \b\b"
                << std::flush;
      lastTime = now;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  for (auto &thread : threads) {
    thread.join();
  }

  auto endTime = std::chrono::high_resolution_clock::now();
  auto elapsed =
      std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime)
          .count();

  std::cout << "\b\r" << "Processing event " << totalEvents << " / "
            << totalEvents << ", spent " << int(elapsed / 1.e3) << "s  \b\b"
            << std::endl;

  for (auto i = 0; i < nRings; i++) {
    auto hist = histRingRingCorrelation->ProjectionX(
        Form("histRingRingCorrelation_%d", i), i + 1, i + 1);
    auto max = hist->GetMaximum();
    std::vector<int> ringIndices;
    for (auto j = 0; j < nRings; j++) {
      constexpr auto thRate = 0.5;
      if (hist->GetBinContent(j + 1) > thRate * max) {
        histRingRingSum[i]->Add(histRingRing[i][j]);
        ringIndices.push_back(j);
      }
    }
    std::string title = Form("dE Ring %02d - E Ring ", i);
    for (auto j = 0; j < ringIndices.size(); j++) {
      title += std::string(Form("%02d", ringIndices.at(j)));
      if (j < ringIndices.size() - 1) {
        title += " + ";
      }
    }
    histRingRingSum[i]->SetTitle(title.c_str());
  }
  histRingRingSumTotal->Reset();
  for (auto i = 0; i < nRings; i++) {
    histRingRingSumTotal->Add(histRingRingSum[i]);
  }
  histRingRingSumTotal->SetTitle("dE Ring - E Ring Sum Correlation");

  std::cout << "Writing results to file..." << std::endl;
  TFile outFile("ring-results.root", "RECREATE");
  histSectorSector->Write();
  histRingRingCorrelation->Write();
  histRingRingSumTotal->Write();
  outFile.cd();
  outFile.mkdir("RingRingSum");
  outFile.cd("RingRingSum");
  for (auto j = 0; j < nRings; j++) {
    if (histRingRingSum[j]) histRingRingSum[j]->Write();
  }
  outFile.mkdir("RingRing");
  outFile.cd("RingRing");
  for (auto i = 0; i < nRings; i++) {
    for (auto j = 0; j < nRings; j++) {
      if (histRingRing[i][j]) histRingRing[i][j]->Write();
    }
  }
  outFile.mkdir("ADC");
  outFile.mkdir("Energy");
  for (uint32_t i = 0; i < nModules; i++) {
    for (uint32_t j = 0; j < nChannels; j++) {
      outFile.cd("ADC");
      if (histADC[i][j]) histADC[i][j]->Write();
      outFile.cd("Energy");
      if (histEnergy[i][j]) histEnergy[i][j]->Write();
    }
  }
  outFile.Close();
}
