#include "L1EventBuilder.hpp"

#include <TFile.h>
#include <TFileRAII.hpp>
#include <TROOT.h>
#include <TTree.h>

#include <DELILAExceptions.hpp>
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
  try {
    fChSettingsVec = ChSettings::GetChSettings(fileName);
    if (fChSettingsVec.size() == 0) {
      throw DELILA::ConfigException("No channel settings found in file: " + fileName);
    }
  } catch (const DELILA::ConfigException &e) {
    throw;  // Re-throw DELILA exceptions as-is
  } catch (const std::exception &e) {
    throw DELILA::ConfigException("Failed to load channel settings from " + fileName +
                          ": " + e.what());
  }
}

void DELILA::L1EventBuilder::LoadFileList(
    const std::vector<std::string> &fileList)
{
  if (fileList.empty()) {
    throw DELILA::ValidationException("File list is empty");
  }
  fFileList = fileList;
}

void DELILA::L1EventBuilder::LoadTimeSettings(const std::string &fileName)
{
  auto jsonFile = std::ifstream(fileName);
  if (!jsonFile) {
    throw DELILA::FileException("Could not open time settings file: " + fileName);
  }

  nlohmann::json timeJSON;
  try {
    jsonFile >> timeJSON;
  } catch (const nlohmann::json::exception &e) {
    throw DELILA::JSONException("Invalid JSON in time settings file " + fileName +
                                ": " + e.what());
  }

  if (timeJSON.empty()) {
    throw DELILA::ConfigException("No time settings found in file: " + fileName);
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
  // Validate inputs
  if (nThreads == 0 || nThreads > 128) {
    throw DELILA::ValidationException("Thread count must be between 1 and 128, got: " +
                                      std::to_string(nThreads));
  }

  if (fFileList.empty()) {
    throw DELILA::ValidationException("File list is empty. Call LoadFileList first.");
  }

  if (fChSettingsVec.empty()) {
    throw DELILA::ConfigException(
        "Channel settings not loaded. Call LoadChSettings first.");
  }

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
    throw DELILA::ConfigException("Time settings not loaded. Call LoadTimeSettings first.");
  }

  if (fRefMod >= fTimeSettingsVec.size()) {
    throw DELILA::RangeException(
        "TimeReferenceMod (" + std::to_string(fRefMod) +
        ") is out of bounds! Time settings has " +
        std::to_string(fTimeSettingsVec.size()) +
        " modules. Please check your settings.json and timeSettings.json files.");
  }

  if (fRefCh >= fTimeSettingsVec[fRefMod].size()) {
    throw DELILA::RangeException(
        "TimeReferenceCh (" + std::to_string(fRefCh) +
        ") is out of bounds! Time settings for module " +
        std::to_string(fRefMod) + " has " +
        std::to_string(fTimeSettingsVec[fRefMod].size()) +
        " channels. Either regenerate timeSettings.json with './eve-builder -t' " +
        "or set TimeReferenceCh to 0 in settings.json.");
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
  auto outputFile = DELILA::MakeTFile(outputName, "RECREATE");
  auto outputTree = new TTree("L1EventData", "L1EventData");
  DELILA::EventData eventData;
  outputTree->Branch("TriggerTime", &eventData.triggerTime, "TriggerTime/D");
  outputTree->Branch("EventDataVec", &eventData.eventDataVec);
  outputTree->SetDirectory(outputFile.get());

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

    auto file = DELILA::MakeTFile(fileName.c_str(), "READ");
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

    // CHUNKED PROCESSING: Process file in chunks to limit memory usage
    // Instead of loading all 174M entries (6.9 GB), process 10M at a time (350 MB)
    std::vector<std::unique_ptr<RawData_t>> overlapBuffer;  // Events from previous chunk

    const Long64_t numChunks = (nEntries + CHUNK_SIZE - 1) / CHUNK_SIZE;

    {
      std::lock_guard<std::mutex> lock(fFileListMutex);
      std::cout << "Thread " << threadID << ": Processing " << nEntries
                << " entries in " << numChunks << " chunks" << std::endl;
    }

    for (Long64_t chunkStart = 0; chunkStart < nEntries; chunkStart += CHUNK_SIZE) {
      // Check if cancelled
      if (fCancelled.load()) {
        std::lock_guard<std::mutex> lock(fFileListMutex);
        std::cout << "Thread " << threadID << " cancelled during chunked processing." << std::endl;
        break;
      }

      // Calculate chunk boundaries with overlap for coincidence window
      Long64_t readStart = (chunkStart > OVERLAP_SIZE) ? (chunkStart - OVERLAP_SIZE) : 0;
      Long64_t readEnd = std::min(nEntries, chunkStart + CHUNK_SIZE + OVERLAP_SIZE);

      // Load this chunk from file
      std::vector<std::unique_ptr<RawData_t>> rawDataVec;
      rawDataVec.reserve(readEnd - readStart);

      for (Long64_t iEve = readStart; iEve < readEnd; iEve++) {
        tree->GetEntry(iEve);

        // Bounds checking to prevent segmentation fault
        if (mod >= fChSettingsVec.size() || ch >= fChSettingsVec[mod].size()) {
          continue;
        }

        // Check if mod/ch are within bounds for time settings array
        if (mod >= fTimeSettingsVec[fRefMod][fRefCh].size() ||
            ch >= fTimeSettingsVec[fRefMod][fRefCh][mod].size()) {
          continue;
        }

        if (chargeLong > fChSettingsVec[mod][ch].thresholdADC) {
          auto ts = fineTS / 1000.;  // ps to ns
          ts -= fTimeSettingsVec[fRefMod][fRefCh][mod][ch];
          auto rawData =
              new RawData_t(false, mod, ch, chargeLong, chargeShort, ts);
          rawDataVec.emplace_back(rawData);
        }
      }

      // Sort chunk
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

            // Bounds checking to prevent segmentation fault
            if (mod >= fChSettingsVec.size() || ch >= fChSettingsVec[mod].size()) {
              continue;
            }

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

      // Clear this chunk's data and release memory
      rawDataVec.clear();
      rawDataVec.shrink_to_fit();

      {
        std::lock_guard<std::mutex> lock(fFileListMutex);
        std::cout << "Thread " << threadID << ": Chunk "
                  << (chunkStart / CHUNK_SIZE + 1) << "/" << numChunks
                  << " complete" << std::endl;
      }
    }  // End chunk loop

    overlapBuffer.clear();  // Clear overlap buffer between files
  }

  outputFile->cd();
  outputTree->Write();
  // outputFile will be automatically closed and deleted
  {
    std::lock_guard<std::mutex> lock(fFileListMutex);
    std::cout << "Thread " << threadID << " finished writing data."
              << std::endl;
    std::cout << "Thread " << threadID << " finished." << std::endl;
  }
}