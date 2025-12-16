#include "L1EventBuilder.hpp"

#include <TFile.h>
#include <TROOT.h>
#include <TTree.h>

#include <EventData.hpp>
#include <algorithm>
#include <csignal>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

// Global pointer for signal handler access
static DELILA::L1EventBuilder* g_eventBuilder = nullptr;

// Signal handler for Ctrl-C
void signalHandler(int signal) {
  if (signal == SIGINT) {
    std::cout << "\n\nReceived Ctrl-C! Stopping threads gracefully..." << std::endl;
    if (g_eventBuilder != nullptr) {
      g_eventBuilder->Cancel();
    }
  }
}

DELILA::L1EventBuilder::L1EventBuilder() {}
DELILA::L1EventBuilder::~L1EventBuilder() {}

void DELILA::L1EventBuilder::LoadChSettings(const std::string &fileName)
{
  fChSettingsVec = ChSettings::GetChSettings(fileName);
  if (fChSettingsVec.size() == 0) {
    std::cerr << "Error: No channel settings found in file: " << fileName
              << std::endl;
    return;
  }
}

void DELILA::L1EventBuilder::LoadFileList(
    const std::vector<std::string> &fileList)
{
  fFileList = fileList;
  if (fFileList.size() == 0) {
    std::cerr << "Error: No files found." << std::endl;
    return;
  }
}

void DELILA::L1EventBuilder::LoadTimeSettings(const std::string &fileName)
{
  auto jsonFile = std::ifstream(fileName);
  if (!jsonFile) {
    std::cerr << "Error: Could not open time settings file: " << fileName
              << std::endl;
    return;
  }
  nlohmann::json timeJSON;
  jsonFile >> timeJSON;
  if (timeJSON.size() == 0) {
    std::cerr << "Error: No time settings found in file: " << fileName
              << std::endl;
    return;
  }

  fTimeSettingsVec.clear();
  fTimeSettingsVec.resize(timeJSON.size());
  for (size_t iRefMod = 0; iRefMod < timeJSON.size(); iRefMod++) {
    fTimeSettingsVec[iRefMod].resize(timeJSON[iRefMod].size());
    for (size_t iRefCh = 0; iRefCh < timeJSON[iRefMod].size(); iRefCh++) {
      fTimeSettingsVec[iRefMod][iRefCh].resize(
          timeJSON[iRefMod][iRefCh].size());
      for (size_t iMod = 0; iMod < timeJSON[iRefMod][iRefCh].size(); iMod++) {
        fTimeSettingsVec[iRefMod][iRefCh][iMod].resize(
            timeJSON[iRefMod][iRefCh][iMod].size());
        for (size_t iCh = 0; iCh < timeJSON[iRefMod][iRefCh][iMod].size();
             iCh++) {
          fTimeSettingsVec[iRefMod][iRefCh][iMod][iCh] =
              timeJSON[iRefMod][iRefCh][iMod][iCh]["TimeOffset"];
          if (iRefMod == iMod && iRefCh == iCh) {
            fTimeSettingsVec[iRefMod][iRefCh][iMod][iCh] =
                0.;  // Reference channel has no offset
          }
        }
      }
    }
  }

  // Print loaded dimensions for debugging
  std::cout << "Time settings loaded: [" << fTimeSettingsVec.size() << "][";
  if (!fTimeSettingsVec.empty()) {
    std::cout << fTimeSettingsVec[0].size() << "][";
    if (!fTimeSettingsVec[0].empty()) {
      std::cout << fTimeSettingsVec[0][0].size() << "][...]";
    }
  }
  std::cout << std::endl;
}

void DELILA::L1EventBuilder::BuildEvent(const uint32_t nThreads)
{
  // For sequential data read and multi threading by user code.
  // Disable implicit multi-threading is faster now. ROOT 6.34.08
  ROOT::DisableImplicitMT();
  ROOT::EnableThreadSafety();

  // Setup signal handler for Ctrl-C
  g_eventBuilder = this;
  fCancelled.store(false);
  ::signal(SIGINT, signalHandler);

  // Validate reference channel configuration
  if (fTimeSettingsVec.empty()) {
    std::cerr << "Error: Time settings not loaded!" << std::endl;
    return;
  }
  if (fRefMod >= fTimeSettingsVec.size()) {
    std::cerr << "Error: TimeReferenceMod (" << static_cast<int>(fRefMod)
              << ") is out of bounds! Time settings has "
              << fTimeSettingsVec.size() << " modules." << std::endl;
    std::cerr << "Please check your settings.json and timeSettings.json files."
              << std::endl;
    return;
  }
  if (fRefCh >= fTimeSettingsVec[fRefMod].size()) {
    std::cerr << "Error: TimeReferenceCh (" << static_cast<int>(fRefCh)
              << ") is out of bounds! Time settings for module " << static_cast<int>(fRefMod)
              << " has " << fTimeSettingsVec[fRefMod].size() << " channels."
              << std::endl;
    std::cerr << "Please check your settings.json and timeSettings.json files."
              << std::endl;
    std::cerr << "Either regenerate timeSettings.json with './eve-builder -t'"
              << std::endl;
    std::cerr << "or set TimeReferenceCh to 0 in settings.json." << std::endl;
    return;
  }

  std::cout << "Using reference: Module " << static_cast<int>(fRefMod)
            << ", Channel " << static_cast<int>(fRefCh) << std::endl;

  // make local file list
  std::vector<std::vector<std::string>> localFileList;
  localFileList.resize(nThreads);
  for (auto iThread = 0, iFile = 0; iThread < nThreads; iThread++) {
    auto nFiles = fFileList.size() / nThreads;
    auto nFilesMod = fFileList.size() % nThreads;
    auto nFilesPerThread = nFiles + (iThread < nFilesMod ? 1 : 0);
    for (auto iFilePerThread = 0; iFilePerThread < nFilesPerThread;
         iFilePerThread++) {
      localFileList[iThread].emplace_back(fFileList[iFile]);
      iFile++;
    }
  }

  std::vector<std::thread> readerThreads;
  for (uint32_t i = 0; i < nThreads; i++) {
    readerThreads.emplace_back(&DELILA::L1EventBuilder::DataReader, this, i,
                               localFileList[i]);
  }

  for (auto &thread : readerThreads) {
    thread.join();
  }
}

void DELILA::L1EventBuilder::DataReader(int threadID,
                                        std::vector<std::string> fileList)
{
  TString outputName = TString::Format("L1_%d.root", threadID);
  auto outputFile = new TFile(outputName, "RECREATE");
  auto outputTree = new TTree("L1EventData", "L1EventData");
  DELILA::EventData eventData;
  outputTree->Branch("TriggerTime", &eventData.triggerTime, "TriggerTime/D");
  outputTree->Branch("EventDataVec", &eventData.eventDataVec);
  outputTree->SetDirectory(outputFile);

  for (auto iFile = 0; iFile < fileList.size(); iFile++) {
    // Check if cancelled
    if (fCancelled.load()) {
      std::lock_guard<std::mutex> lock(fFileListMutex);
      std::cout << "Thread " << threadID << " cancelled by user." << std::endl;
      break;
    }

    std::string fileName = fileList[iFile];
    {
      std::lock_guard<std::mutex> lock(fFileListMutex);
      std::cout << "Thread " << threadID << " reading file: " << fileName
                << " (" << iFile + 1 << "/" << fileList.size() << ")"
                << std::endl;
    }

    auto file = new TFile(fileName.c_str(), "READ");
    if (!file || file->IsZombie()) {
      std::cerr << "Error: Could not open file: " << fileName << std::endl;
      continue;
    }
    auto tree = static_cast<TTree *>(file->Get("ELIADE_Tree"));
    if (!tree) {
      std::cerr << "Error: Could not find tree in file: " << fileName
                << std::endl;
      file->Close();
      continue;
    }
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

    UShort_t chargeLong;
    tree->SetBranchStatus("ChargeLong", kTRUE);
    tree->SetBranchAddress("ChargeLong", &chargeLong);

    UShort_t chargeShort;
    tree->SetBranchStatus("ChargeShort", kTRUE);
    tree->SetBranchAddress("ChargeShort", &chargeShort);

    const auto nEntries = tree->GetEntries();
    std::vector<std::unique_ptr<RawData_t>> rawDataVec;
    rawDataVec.reserve(nEntries);
    for (Long64_t iEve = 0; iEve < nEntries; iEve++) {
      tree->GetEntry(iEve);
      if (chargeLong > fChSettingsVec[mod][ch].thresholdADC) {
        auto ts = fineTS / 1000.;  // ps to ns
        ts -= fTimeSettingsVec[fRefMod][fRefCh][mod][ch];
        auto rawData =
            new RawData_t(false, mod, ch, chargeLong, chargeShort, ts);
        rawDataVec.emplace_back(rawData);
      }
    }
    file->Close();
    std::sort(rawDataVec.begin(), rawDataVec.end(),
              [](const std::unique_ptr<RawData_t> &a,
                 const std::unique_ptr<RawData_t> &b) {
                return a->fineTS < b->fineTS;
              });

    const auto nRawData = rawDataVec.size();

    for (auto iEve = 0; iEve < nRawData; iEve++) {
      auto &rawData = rawDataVec[iEve];
      auto trgMod = rawData->mod;
      auto trgCh = rawData->ch;

      if (fChSettingsVec[trgMod][trgCh].isEventTrigger) {
        auto triggerID = fChSettingsVec[trgMod][trgCh].ID;
        eventData.Clear();
        eventData.triggerTime = rawData->fineTS;
        eventData.eventDataVec->emplace_back(
            rawData->isWithAC, trgMod, trgCh, rawData->chargeLong,
            rawData->chargeShort, rawData->fineTS - eventData.triggerTime);
        bool fillFlag = true;

        for (auto jEve = iEve + 1; (jEve < nRawData) && fillFlag; jEve++) {
          auto &rawData2 = rawDataVec[jEve];
          auto ts = rawData2->fineTS - eventData.triggerTime;
          if (ts > fCoincidenceWindow) {
            break;
          }
          auto mod = rawData2->mod;
          auto ch = rawData2->ch;
          auto id = fChSettingsVec[mod][ch].ID;
          auto isEventTrigger = fChSettingsVec[mod][ch].isEventTrigger;
          if (isEventTrigger && id >= triggerID && ts < fCoincidenceWindow) {
            // skip this event
            fillFlag = false;
            break;
          }
          auto hit = *rawData2;
          hit.fineTS -= eventData.triggerTime;
          eventData.eventDataVec->emplace_back(hit.isWithAC, mod, ch,
                                               hit.chargeLong, hit.chargeShort,
                                               hit.fineTS);
        }
        for (auto jEve = iEve - 1; (jEve >= 0) && fillFlag; jEve--) {
          auto &rawData2 = rawDataVec[jEve];
          auto ts = rawData2->fineTS - eventData.triggerTime;
          if (ts < -fCoincidenceWindow) {
            break;
          }
          auto mod = rawData2->mod;
          auto ch = rawData2->ch;
          auto id = fChSettingsVec[mod][ch].ID;
          auto isEventTrigger = fChSettingsVec[mod][ch].isEventTrigger;
          if (isEventTrigger && id >= triggerID && ts > -fCoincidenceWindow) {
            // skip this event
            fillFlag = false;
            break;
          }
          auto hit = *rawData2;
          hit.fineTS -= eventData.triggerTime;
          eventData.eventDataVec->emplace_back(hit.isWithAC, mod, ch,
                                               hit.chargeLong, hit.chargeShort,
                                               hit.fineTS);
        }

        if (fillFlag) {
          std::sort(eventData.eventDataVec->begin() + 1,
                    eventData.eventDataVec->end(),
                    [](const RawData_t &a, const RawData_t &b) {
                      return a.fineTS < b.fineTS;
                    });

          // Check AC
          for (auto &hit : *(eventData.eventDataVec)) {
            auto mod = hit.mod;
            auto ch = hit.ch;
            if (fChSettingsVec[mod][ch].hasAC) {
              auto acMod = fChSettingsVec[mod][ch].ACMod;
              auto acCh = fChSettingsVec[mod][ch].ACCh;
              for (auto &ac : *(eventData.eventDataVec)) {
                if (ac.mod == acMod && ac.ch == acCh &&
                    fabs(ac.fineTS) < fCoincidenceWindow) {
                  hit.isWithAC = true;
                  break;
                }
              }
            }
          }

          outputTree->Fill();
        }
        eventData.Clear();
      }
    }

    rawDataVec.clear();
  }

  outputFile->cd();
  outputTree->Write();
  outputFile->Close();
  delete outputFile;
  {
    std::lock_guard<std::mutex> lock(fFileListMutex);
    std::cout << "Thread " << threadID << " finished writing data."
              << std::endl;
    std::cout << "Thread " << threadID << " finished." << std::endl;
  }
}