#include <TCanvas.h>
#include <TChain.h>
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

constexpr uint32_t nModules = 11;
constexpr uint32_t nChannels = 32;
TH1D *histADC[nModules][nChannels];
TH1D *histEnergy[nModules][nChannels];
constexpr uint32_t nSectors = 16;
TH2D *histSectorCorrelation[nSectors][nSectors];
TH2D *histSectorCorrelationSum;
constexpr uint32_t nRings = 48;
TH2D *histRingCorrelation[nRings][nRings];
TH2D *histRingCorrelationSum;
TH2D *histDERingESectorCorrelation[nRings];
TH2D *histDERingESectorCorrelationSum;
void InitHists()
{
  auto settingsFileName = "./chSettings.json";
  auto chSettingsVec = DELILA::ChSettings::GetChSettings(settingsFileName);

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

  for (uint32_t i = 0; i < nSectors; i++) {
    for (uint32_t j = 0; j < nSectors; j++) {
      histSectorCorrelation[i][j] =
          new TH2D(Form("histSectorCorrelation_%02d_%02d", i, j),
                   Form("Sector Correlation dE %02d vs E %02d", i, j), 2000, 0,
                   20000, 2000, 0, 20000);
      histSectorCorrelation[i][j]->SetXTitle("[keV]");
      histSectorCorrelation[i][j]->SetYTitle("[keV]");
    }
  }
  histSectorCorrelationSum =
      new TH2D("histSectorCorrelationSum", "Sector Correlation Sum", 2000, 0,
               20000, 2000, 0, 20000);
  histSectorCorrelationSum->SetXTitle("[keV]");
  histSectorCorrelationSum->SetYTitle("[keV]");

  for (uint32_t i = 0; i < nRings; i++) {
    for (uint32_t j = 0; j < nRings; j++) {
      histRingCorrelation[i][j] =
          new TH2D(Form("histRingCorrelation_%02d_%02d", i, j),
                   Form("Ring Correlation dE %02d vs E %02d", i, j), 2000, 0,
                   20000, 2000, 0, 20000);
      histRingCorrelation[i][j]->SetXTitle("[keV]");
      histRingCorrelation[i][j]->SetYTitle("[keV]");
    }
  }
  histRingCorrelationSum =
      new TH2D("histRingCorrelationSum", "Ring Correlation Sum", 2000, 0, 20000,
               2000, 0, 20000);
  histRingCorrelationSum->SetXTitle("[keV]");
  histRingCorrelationSum->SetYTitle("[keV]");

  for (uint32_t i = 0; i < nRings; i++) {
    histDERingESectorCorrelation[i] =
        new TH2D(Form("histDERingESectorCorrelation_%02d", i),
                 Form("dE Ring %02d vs E All Sector", i), 2000, 0, 20000, 2000,
                 0, 20000);
    histDERingESectorCorrelation[i]->SetXTitle("[keV]");
    histDERingESectorCorrelation[i]->SetYTitle("[keV]");
  }
  histDERingESectorCorrelationSum =
      new TH2D("histDERingESectorCorrelationSum", "dE Ring vs E Sector Sum",
               2000, 0, 20000, 2000, 0, 20000);
  histDERingESectorCorrelationSum->SetXTitle("[keV]");
  histDERingESectorCorrelationSum->SetYTitle("[keV]");
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

  Double_t eneE = 0.;
  Double_t eneDE = 0.;
  UInt_t sectorE = 0;
  UInt_t sectorDE = 0;
  UInt_t ringE = 0;
  UInt_t ringDE = 0;

  auto const nEntries = tree->GetEntries();
  {
    std::lock_guard<std::mutex> lock(counterMutex);
    totalEvents += nEntries;
  }
  for (auto iEve = 0; iEve < nEntries; iEve++) {
    tree->GetEntry(iEve);
    constexpr auto nProcess = 1000;
    if (iEve % nProcess == 0) {
      std::lock_guard<std::mutex> lock(counterMutex);
      processedEvents += nProcess;
    }

    for (auto &event : *eventData.eventDataVec) {
      if (event.isWithAC) {
        continue;  // Skip events with AC data
      }

      if (event.mod >= nModules || event.ch >= nChannels) {
        continue;  // Skip invalid channels
      }

      auto chSetting = chSettingsVec.at(event.mod).at(event.ch);
      histADC[event.mod][event.ch]->Fill(event.chargeLong);
      histEnergy[event.mod][event.ch]->Fill(
          GetCalibratedEnergy(chSetting, event.chargeLong));

      if (event.mod == 4) {  // Find E
        eneE = GetCalibratedEnergy(chSetting, event.chargeLong);
        sectorE = event.ch;
        for (auto &dEEvent : *eventData.eventDataVec) {  // all combinations
          if (dEEvent.mod == 0) {                        // Find dE
            eneDE = GetCalibratedEnergy(chSetting, dEEvent.chargeLong);
            sectorDE = dEEvent.ch;
            histSectorCorrelation[sectorDE][sectorE]->Fill(eneE, eneDE);
            histSectorCorrelationSum->Fill(eneE, eneDE);
          } else if (dEEvent.mod == 1 || dEEvent.mod == 2 ||
                     dEEvent.mod == 3) {  // Find dE
            eneDE = GetCalibratedEnergy(chSetting, dEEvent.chargeLong);
            ringDE = (dEEvent.mod - 1) * 16 + dEEvent.ch;  // 0-47
            histDERingESectorCorrelation[ringDE]->Fill(eneE, eneDE);
            histDERingESectorCorrelationSum->Fill(eneE, eneDE);
          }
        }
      }

      if (event.mod == 5 || event.mod == 6 || event.mod == 7) {  // Find E
        eneE = GetCalibratedEnergy(chSetting, event.chargeLong);
        ringE = (event.mod - 5) * 16 + event.ch;         // 0-47
        for (auto &dEEvent : *eventData.eventDataVec) {  // all combinations
          if (dEEvent.mod == 1 || dEEvent.mod == 2 || dEEvent.mod == 3) {
            eneDE = GetCalibratedEnergy(chSetting, dEEvent.chargeLong);
            ringDE = (dEEvent.mod - 1) * 16 + dEEvent.ch;  // 0-47
            histRingCorrelation[ringDE][ringE]->Fill(eneE, eneDE);
            histRingCorrelationSum->Fill(eneE, eneDE);
          }
        }
      }
    }
  }

  IsFinished.at(threadID) = true;
  file->Close();
}

void reader()
{
  gSystem->Load("libEveBuilder.dylib");

  ROOT::EnableThreadSafety();

  InitHists();
  std::cout << "Initialized histograms." << std::endl;

  auto fileList = GetFileList("./");

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

  // Printout event statistics of correlation histograms
  std::cout << "Correlation histograms:" << std::endl;
  for (uint32_t i = 0; i < nSectors; i++) {
    for (uint32_t j = 0; j < nSectors; j++) {
      if (histSectorCorrelation[i][j]) {
        std::cout << "Sector " << i << " vs Sector " << j << ": "
                  << histSectorCorrelation[i][j]->GetEntries() << " entries"
                  << std::endl;
      }
    }
  }
  std::cout << "Sector Correlation Sum: "
            << histSectorCorrelationSum->GetEntries() << " entries"
            << std::endl;
  for (uint32_t i = 0; i < nRings; i++) {
    for (uint32_t j = 0; j < nRings; j++) {
      if (histRingCorrelation[i][j]) {
        std::cout << "Ring " << i << " vs Ring " << j << ": "
                  << histRingCorrelation[i][j]->GetEntries() << " entries"
                  << std::endl;
      }
    }
  }
  std::cout << "Ring Correlation Sum: " << histRingCorrelationSum->GetEntries()
            << " entries" << std::endl;

  for (uint32_t i = 0; i < nRings; i++) {
    if (histDERingESectorCorrelation[i]) {
      std::cout << "dE Ring " << i << " vs E Sector" << ": "
                << histDERingESectorCorrelation[i]->GetEntries() << " entries"
                << std::endl;
    }
  }
  std::cout << "dE Ring vs E Sector Sum: "
            << histDERingESectorCorrelationSum->GetEntries() << " entries"
            << std::endl;

  std::cout << "Writing results to file..." << std::endl;
  TFile outFile("results.root", "RECREATE");
  outFile.cd();
  outFile.mkdir("SectorSector");
  outFile.mkdir("RingRing");
  outFile.mkdir("DERingESector");
  // Write corrilation histograms
  outFile.cd("SectorSector");
  for (uint32_t i = 0; i < nSectors; i++) {
    for (uint32_t j = 0; j < nSectors; j++) {
      if (histSectorCorrelation[i][j]) histSectorCorrelation[i][j]->Write();
    }
  }
  if (histSectorCorrelationSum) histSectorCorrelationSum->Write();
  outFile.cd("RingRing");
  for (uint32_t i = 0; i < nRings; i++) {
    for (uint32_t j = 0; j < nRings; j++) {
      if (histRingCorrelation[i][j]) histRingCorrelation[i][j]->Write();
    }
  }
  if (histRingCorrelationSum) histRingCorrelationSum->Write();
  outFile.cd("DERingESector");
  for (uint32_t i = 0; i < nRings; i++) {
    if (histDERingESectorCorrelation[i]) {
      histDERingESectorCorrelation[i]->Write();
    }
  }
  if (histDERingESectorCorrelationSum) histDERingESectorCorrelationSum->Write();
  // Write ADC and energy histograms
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
}
