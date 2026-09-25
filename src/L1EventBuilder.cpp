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
#include <limits>
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

  // The files of this thread form ONE continuous stream (entries of the files
  // concatenated), broken only by a timestamp reset. Each hit remembers its
  // stream entry index. A hit is evaluated as a trigger candidate exactly once:
  // in the pass where ownStart <= entry < ownEnd. The OVERLAP_SIZE entries on
  // either side of a candidate are context only (dedup check + event content).
  // At the end of every chunk (and so of every file) the last OVERLAP_SIZE
  // entries are deferred to the next pass, where they get their forward
  // context; the stream end is flushed.
  struct StreamHit {
    RawData_t data;
    Long64_t entry;  // index in the thread's stream
  };
  std::vector<StreamHit> hitVec;  // carried context + deferred hits + chunk
  hitVec.reserve(CHUNK_SIZE + 2 * OVERLAP_SIZE);  // carry <= 2 * OVERLAP_SIZE
  Long64_t streamOffset = 0;      // stream index of the current file's entry 0
  Long64_t ownStart = 0;          // first stream entry not yet evaluated

  // Self-check of the context assumption: every hit within the coincidence
  // window of a candidate lies within OVERLAP_SIZE entries of it.
  constexpr Double_t kNoTime = std::numeric_limits<Double_t>::lowest();
  Double_t lastCandidateTime = kNoTime;  // latest evaluated candidate
  Double_t lastDroppedTime = kNoTime;    // latest hit dropped from the carry
  Long64_t nOutOfContext = 0;

  auto buildEvents = [&](Long64_t ownEnd) {
    const Long64_t nHits = hitVec.size();
    for (Long64_t iEve = 0; iEve < nHits; iEve++) {
      auto &rawData = hitVec[iEve].data;
      auto trgMod = rawData.mod;
      auto trgCh = rawData.ch;
      auto entry = hitVec[iEve].entry;

      if (entry >= ownStart && entry < ownEnd &&
          fChSettingsVec[trgMod][trgCh].isEventTrigger) {
        if (rawData.fineTS - fCoincidenceWindow <= lastDroppedTime) {
          nOutOfContext++;
        }
        lastCandidateTime = std::max(lastCandidateTime, rawData.fineTS);

        auto triggerID = fChSettingsVec[trgMod][trgCh].ID;
        eventData.Clear();
        eventData.triggerTime = rawData.fineTS;
        eventData.eventDataVec->emplace_back(
            rawData.isWithAC, trgMod, trgCh, rawData.chargeLong,
            rawData.chargeShort, rawData.fineTS - eventData.triggerTime);
        bool fillFlag = true;

        // Precedence: the lower ID wins; for an equal ID the earlier hit wins.
        for (auto jEve = iEve + 1; (jEve < nHits) && fillFlag; jEve++) {
          auto &rawData2 = hitVec[jEve].data;
          auto ts = rawData2.fineTS - eventData.triggerTime;
          if (ts > fCoincidenceWindow) {
            break;
          }
          auto mod = rawData2.mod;
          auto ch = rawData2.ch;
          auto id = fChSettingsVec[mod][ch].ID;
          auto isEventTrigger = fChSettingsVec[mod][ch].isEventTrigger;
          if (isEventTrigger && id < triggerID && ts < fCoincidenceWindow) {
            // a later trigger with a lower ID wins: skip this event
            fillFlag = false;
            break;
          }
          auto hit = rawData2;
          hit.fineTS -= eventData.triggerTime;
          eventData.eventDataVec->emplace_back(hit.isWithAC, mod, ch,
                                               hit.chargeLong, hit.chargeShort,
                                               hit.fineTS);
        }
        for (auto jEve = iEve - 1; (jEve >= 0) && fillFlag; jEve--) {
          auto &rawData2 = hitVec[jEve].data;
          auto ts = rawData2.fineTS - eventData.triggerTime;
          if (ts < -fCoincidenceWindow) {
            break;
          }
          auto mod = rawData2.mod;
          auto ch = rawData2.ch;
          auto id = fChSettingsVec[mod][ch].ID;
          auto isEventTrigger = fChSettingsVec[mod][ch].isEventTrigger;
          if (isEventTrigger && id <= triggerID && ts > -fCoincidenceWindow) {
            // an earlier trigger with a lower or equal ID wins: skip this event
            fillFlag = false;
            break;
          }
          auto hit = rawData2;
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
    ownStart = std::max(ownStart, ownEnd);

    // Keep OVERLAP_SIZE entries of backward context for the next candidates
    const Long64_t keepFrom = ownStart - OVERLAP_SIZE;
    auto keepEnd = std::remove_if(hitVec.begin(), hitVec.end(),
                                  [&](const StreamHit &h) {
                                    if (h.entry >= keepFrom) return false;
                                    lastDroppedTime =
                                        std::max(lastDroppedTime, h.data.fineTS);
                                    return true;
                                  });
    hitVec.erase(keepEnd, hitVec.end());
  };

  // Track last timestamp to detect acquisition restarts (timestamp resets)
  // If first event of new file has earlier timestamp than last event of previous file,
  // it indicates a new acquisition: the stream is flushed and restarted
  Double_t lastFileLastTimestamp = -1.0;

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

    // Check first event timestamp to detect acquisition restart
    if (nEntries > 0 && lastFileLastTimestamp > 0) {
      tree->GetEntry(0);
      Double_t firstTimestamp = fineTS / 1000.;  // ps to ns

      // Timestamp reset detection with 10-second threshold
      // Electronics/DAQ can have small timing variations, so only consider it a reset
      // if timestamp jumped backwards by more than 10 seconds
      constexpr Double_t TIMESTAMP_RESET_THRESHOLD = 10e9;  // 10 seconds in ns

      if ((firstTimestamp + TIMESTAMP_RESET_THRESHOLD) < lastFileLastTimestamp) {
        // Significant timestamp jump backwards - new acquisition detected:
        // flush the old stream (no forward context follows) and restart
        buildEvents(streamOffset);
        hitVec.clear();
        lastCandidateTime = kNoTime;
        lastDroppedTime = kNoTime;
        std::lock_guard<std::mutex> lock(fFileListMutex);
        std::cout << "Thread " << threadID
                  << ": Timestamp reset detected (new acquisition), flushing the previous stream"
                  << std::endl;
        std::cout << "         Previous file last timestamp: " << lastFileLastTimestamp / 1e9 << " s"
                  << std::endl;
        std::cout << "         Current file first timestamp: " << firstTimestamp / 1e9 << " s"
                  << std::endl;
      }
    }

    // CHUNKED PROCESSING: Process file in chunks to limit memory usage
    // Instead of loading all 174M entries (6.9 GB), process 10M at a time (350 MB)

    // Performance profiling: measure read vs process time
    Double_t totalReadTime = 0.0;
    Double_t totalProcessTime = 0.0;

    for (Long64_t chunkStart = 0; chunkStart < nEntries; chunkStart += CHUNK_SIZE) {
      // Check if cancelled
      if (fCancelled.load()) {
        std::lock_guard<std::mutex> lock(fFileListMutex);
        std::cout << "Thread " << threadID << " cancelled during chunked processing." << std::endl;
        break;
      }

      // Each entry is read once; the context comes from the carried hits
      Long64_t chunkEnd = std::min(nEntries, chunkStart + CHUNK_SIZE);

      // === Timing: Start Read Phase ===
      auto readPhaseStart = std::chrono::high_resolution_clock::now();

      for (Long64_t iEve = chunkStart; iEve < chunkEnd; iEve++) {
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
          if (ts - fCoincidenceWindow <= lastCandidateTime) {
            nOutOfContext++;  // arrived after a candidate it belongs to
          }
          hitVec.push_back(
              {RawData_t(false, mod, ch, chargeLong, chargeShort, ts),
               streamOffset + iEve});
        }
      }

      // Sort by time; equal times keep the stream order
      std::sort(hitVec.begin(), hitVec.end(),
                [](const StreamHit &a, const StreamHit &b) {
                  if (a.data.fineTS != b.data.fineTS) {
                    return a.data.fineTS < b.data.fineTS;
                  }
                  return a.entry < b.entry;
                });

      // === Timing: End Read Phase, Start Process Phase ===
      auto readPhaseEnd = std::chrono::high_resolution_clock::now();
      totalReadTime += std::chrono::duration<double>(readPhaseEnd - readPhaseStart).count();

      // Evaluate the owned candidates; defer the last OVERLAP_SIZE entries
      buildEvents(std::max(ownStart, streamOffset + chunkEnd - OVERLAP_SIZE));

      // === Timing: End Process Phase ===
      auto processPhaseEnd = std::chrono::high_resolution_clock::now();
      totalProcessTime += std::chrono::duration<double>(processPhaseEnd - readPhaseEnd).count();
    }  // End chunk loop
    // Note: hitVec is NOT cleared here to maintain cross-file continuity
    streamOffset += nEntries;

    // Update last timestamp from this file for next file's acquisition restart detection
    if (!hitVec.empty()) {
      lastFileLastTimestamp = hitVec.back().data.fineTS;
    }

    {
      std::lock_guard<std::mutex> lock(fFileListMutex);
      std::cout << "Thread " << threadID << ": Finished processing "
                << fileName << std::endl;
      std::cout << "         Read time:    " << totalReadTime << " s ("
                << (totalReadTime / (totalReadTime + totalProcessTime) * 100) << "%)" << std::endl;
      std::cout << "         Process time: " << totalProcessTime << " s ("
                << (totalProcessTime / (totalReadTime + totalProcessTime) * 100) << "%)" << std::endl;
      std::cout << "         Total time:   " << (totalReadTime + totalProcessTime) << " s" << std::endl;
    }
  }

  // End of the stream: flush the deferred hits
  buildEvents(streamOffset);
  if (nOutOfContext > 0) {
    std::lock_guard<std::mutex> lock(fFileListMutex);
    std::cerr << "Thread " << threadID << ": WARNING: " << nOutOfContext
              << " hits lie out of time order by more than OVERLAP_SIZE ("
              << OVERLAP_SIZE << ") entries;"
              << " events near chunk/file boundaries may be incomplete."
              << std::endl;
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