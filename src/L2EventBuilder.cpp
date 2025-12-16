#include "L2EventBuilder.hpp"

#include <TChain.h>
#include <TFile.h>
#include <TROOT.h>
#include <TTree.h>

#include <csignal>
#include <filesystem>

// Global pointer for signal handler access
static DELILA::L2EventBuilder* g_l2EventBuilder = nullptr;

// Signal handler for Ctrl-C
void l2SignalHandler(int signal) {
  if (signal == SIGINT) {
    std::cout << "\n\nReceived Ctrl-C! Stopping L2 threads gracefully..." << std::endl;
    if (g_l2EventBuilder != nullptr) {
      g_l2EventBuilder->Cancel();
    }
  }
}

DELILA::L2EventBuilder::L2EventBuilder() {}
DELILA::L2EventBuilder::~L2EventBuilder() {}

void DELILA::L2EventBuilder::LoadChSettings(const std::string &fileName)
{
  fChSettingsVec = ChSettings::GetChSettings(fileName);
  if (fChSettingsVec.size() == 0) {
    std::cerr << "Error: No channel settings found in file: " << fileName
              << std::endl;
    return;
  }
}

void DELILA::L2EventBuilder::LoadL2Settings(const std::string &fileName)
{
  if (fChSettingsVec.size() == 0) {
    std::cerr << "Error: No channel settings found in file: " << fileName
              << std::endl;
    return;
  }

  std::ifstream file(fileName);
  if (!file.is_open()) {
    std::cerr << "Error: Could not open L2 settings file: " << fileName
              << std::endl;
    return;
  }
  nlohmann::json j;
  file >> j;
  file.close();

  fCounterVec.clear();

  std::cout << j.size() << " Conditions" << std::endl;
  for (auto condition : j) {
    auto name = condition["Name"];
    auto type = condition["Type"];
    if (type == "Counter") {
      std::vector<std::string> tags = condition["Tags"];
      std::cout << "Counter setting: " << name << std::endl;
      std::cout << "Tags: ";
      for (auto &tag : tags) {
        std::cout << tag << " ";
      }
      std::cout << std::endl;

      std::vector<std::vector<bool>> conditionTable;
      conditionTable.resize(fChSettingsVec.size());
      for (auto i = 0; i < fChSettingsVec.size(); i++) {
        conditionTable[i].resize(fChSettingsVec[i].size());
        for (auto j = 0; j < conditionTable[i].size(); ++j) {
          conditionTable[i][j] = false;
          auto definedTags = fChSettingsVec[i][j].tags;
          for (auto &tag : tags) {
            if (std::find(definedTags.begin(), definedTags.end(), tag) !=
                definedTags.end()) {
              conditionTable[i][j] = true;
              break;
            }
          }
        }
      }

      L2Counter counter(name);
      counter.SetConditionTable(conditionTable);
      fCounterVec.push_back(counter);

      std::cout << name << " condition table: " << std::endl;
      for (auto i = 0; i < conditionTable.size(); i++) {
        std::cout << "Mod " << i << ": ";
        for (auto j = 0; j < conditionTable[i].size(); j++) {
          std::cout << conditionTable[i][j] << " ";
        }
        std::cout << std::endl;
      }
      std::cout << std::endl;
    } else if (type == "Flag") {
      auto monitorName = condition["Monitor"];
      auto conditionOp = condition["Operator"];
      auto value = condition["Value"];
      std::cout << "Flag setting: " << name << std::endl;
      std::cout << "Monitor: " << monitorName << std::endl;
      std::cout << "Operator: " << conditionOp << std::endl;
      std::cout << "Value: " << value << std::endl;
      L2Flag flag(name, monitorName, conditionOp, value);
      fFlagVec.push_back(flag);
      std::cout << std::endl;
    } else if (type == "Accept") {
      std::vector<std::string> monitors = condition["Monitor"];
      auto logic = condition["Operator"];
      std::cout << "Accept setting: " << name << std::endl;
      std::cout << "Monitors: ";
      for (auto &monitor : monitors) {
        std::cout << monitor << " ";
      }
      std::cout << std::endl;
      std::cout << "Operator: " << logic << std::endl;
      auto tmp = L2DataAcceptance(monitors, logic);
      fDataAcceptanceVec.push_back(tmp);
    } else {
      std::cerr << "Error: Unknown type: " << type << std::endl;
    }
  }
}

void DELILA::L2EventBuilder::BuildEvent(uint32_t nThreads)
{
  // For sequential data read and multi threading by user code.
  // Disable implicit multi-threading is faster now. ROOT 6.34.08
  ROOT::DisableImplicitMT();
  ROOT::EnableThreadSafety();

  // Setup signal handler for Ctrl-C
  g_l2EventBuilder = this;
  fCancelled.store(false);
  ::signal(SIGINT, l2SignalHandler);

  GetFileList("L1");

  if (nThreads < fFileList.size()) {
    std::cout << "Number of threads is less than the number of files. "
                 "Using the same number of files: "
              << fFileList.size() << std::endl;
  }
  nThreads = fFileList.size();

  std::vector<std::thread> threads;
  for (uint32_t i = 0; i < nThreads; i++) {
    auto localCounterVec = fCounterVec;
    auto localFlagVec = fFlagVec;
    auto localDataAcceptanceVec = fDataAcceptanceVec;
    threads.emplace_back(&DELILA::L2EventBuilder::ProcessData, this, i,
                         fFileList[i], localCounterVec, localFlagVec,
                         localDataAcceptanceVec);
  }

  for (auto &thread : threads) {
    thread.join();
  }

  // MergeFiles();

  // // delete files
  // GetFileList("L2");
  // for (const auto &file : fFileList) {
  //   std::cout << "Deleting file: " << file << std::endl;
  //   std::filesystem::remove(file);
  // }
}

void DELILA::L2EventBuilder::MergeFiles()
{
  std::cout << "Merging files..." << std::endl;
  //   ROOT::EnableImplicitMT();

  auto chain = new TChain("L2EventData");
  GetFileList("L2");
  if (fFileList.size() == 0) {
    std::cerr << "Error: No L2 files found." << std::endl;
    return;
  }
  for (const auto &file : fFileList) {
    chain->Add(file.c_str());
  }
  //   DELILA::EventData originalData;
  DELILA::EventData eventData;
  chain->SetBranchAddress("TriggerTime", &eventData.triggerTime);
  chain->SetBranchAddress("EventDataVec", &eventData.eventDataVec);

  auto outputFile = new TFile("L2Event.root", "RECREATE");
  auto outputTree = new TTree("L2EventData", "L2EventData");

  //   DELILA::EventData eventData;
  outputTree->Branch("TriggerTime", &eventData.triggerTime, "TriggerTime/D");
  outputTree->Branch("EventDataVec", &eventData.eventDataVec);
  outputTree->SetDirectory(outputFile);

  auto startTime = std::chrono::high_resolution_clock::now();
  auto lastTime = startTime;
  const auto nEntries = chain->GetEntries();
  for (Long64_t iEve = 0; iEve < nEntries; iEve++) {
    chain->GetEntry(iEve);
    if (iEve % 1000 == 0) {
      auto now = std::chrono::high_resolution_clock::now();
      auto duration =
          std::chrono::duration_cast<std::chrono::milliseconds>(now - lastTime);
      if (duration.count() > 1000) {
        auto finishedEvents = iEve;
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                           now - startTime)
                           .count();
        auto remainingTime =
            (nEntries - finishedEvents) * elapsed / finishedEvents / 1.e3;
        lastTime = now;
        std::cout << "\b\r" << "Processing event " << finishedEvents << " / "
                  << nEntries << ", " << int(remainingTime) << "s  \b\b"
                  << std::flush;
      }
    }
    // eventData = originalData;
    outputTree->Fill();
  }
  std::cout << "\b\r" << "Processing event " << nEntries << " / " << nEntries
            << ", finished." << std::endl;

  {
    std::lock_guard<std::mutex> lock(fMutex);
    std::cout << "Merging files finished." << std::endl;
  }

  outputFile->cd();
  outputTree->Write();
  outputFile->Close();
}

void DELILA::L2EventBuilder::ProcessData(
    const uint32_t threadID, const std::string &fileName,
    std::vector<L2Counter> localCounterVec, std::vector<L2Flag> localFlagVec,
    std::vector<L2DataAcceptance> localDataAcceptanceVec)
{
  // Process the data in the file here
  {
    std::lock_guard<std::mutex> lock(fMutex);
    std::cout << "Thread " << threadID << " processing file: " << fileName
              << std::endl;
  }

  auto inputFile = new TFile(fileName.c_str(), "READ");
  auto inputTree = static_cast<TTree *>(inputFile->Get("L1EventData"));
  if (!inputTree) {
    std::cerr << "Error: Could not find tree in file: " << fileName
              << std::endl;
    inputFile->Close();
    return;
  }
  DELILA::EventData eventData;
  inputTree->SetBranchAddress("TriggerTime", &eventData.triggerTime);
  inputTree->SetBranchAddress("EventDataVec", &eventData.eventDataVec);

  auto outputName = Form("L2_%d.root", threadID);
  auto outputFile = new TFile(outputName, "RECREATE");
  auto outputTree = new TTree("L2EventData", "L2EventData");
  outputTree->SetDirectory(outputFile);

  for (auto &flag : localFlagVec) {
    outputTree->Branch(flag.name.c_str(), &flag.flag,
                       (flag.name + "/O").c_str());
  }
  for (auto &counter : localCounterVec) {
    outputTree->Branch(counter.name.c_str(), &counter.counter,
                       (counter.name + "/l").c_str());
  }
  outputTree->Branch("TriggerTime", &eventData.triggerTime, "TriggerTime/D");
  outputTree->Branch("EventDataVec", &eventData.eventDataVec);

  auto startTime = std::chrono::high_resolution_clock::now();
  auto lastTime = startTime;
  const auto nEntries = inputTree->GetEntries();
  for (Long64_t iEve = 0; iEve < nEntries; iEve++) {
    // Check if cancelled
    if (fCancelled.load()) {
      std::lock_guard<std::mutex> lock(fMutex);
      std::cout << "Thread " << threadID << " cancelled by user after processing "
                << iEve << "/" << nEntries << " events." << std::endl;
      break;
    }

    inputTree->GetEntry(iEve);
    if ((threadID == 0) && (iEve % 1000 == 0)) {
      auto now = std::chrono::high_resolution_clock::now();
      auto duration =
          std::chrono::duration_cast<std::chrono::milliseconds>(now - lastTime);
      if (duration.count() > 1000) {
        auto finishedEvents = iEve;
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                           now - startTime)
                           .count();
        auto remainingTime =
            (nEntries - finishedEvents) * elapsed / finishedEvents / 1.e3;
        lastTime = now;
        std::lock_guard<std::mutex> lock(fMutex);
        std::cout << "\b\rThread " << threadID << ": Processing event "
                  << finishedEvents << " / " << nEntries << ", "
                  << int(remainingTime) << "s  \b\b" << std::flush;
      }
    }

    if (eventData.eventDataVec->size() == 0) {
      continue;
    }

    for (auto &counter : localCounterVec) {
      counter.ResetCounter();
      for (auto &rawData : *eventData.eventDataVec) {
        counter.Check(rawData.mod, rawData.ch);
      }
    }

    for (auto &flag : localFlagVec) {
      flag.Check(localCounterVec);
    }

    auto fillFlag = false;
    for (auto &accept : localDataAcceptanceVec) {
      fillFlag |= accept.Check(localFlagVec);
    }

    // if (localDataAcceptanceVec.size() == 0) {
    //   fillFlag = true;  // If no acceptance is defined, fill the tree
    // }
    if (fillFlag) {
      outputTree->Fill();
    }
  }

  {
    std::lock_guard<std::mutex> lock(fMutex);
    std::cout << "\b\rThread " << threadID << ": Processing event " << nEntries
              << " / " << nEntries << ", finished." << std::endl;
  }

  outputFile->cd();
  outputTree->Write();
  outputFile->Close();

  inputFile->Close();

  {
    std::lock_guard<std::mutex> lock(fMutex);
    std::cout << "Thread " << threadID << " finished writing data."
              << std::endl;
  }
}

void DELILA::L2EventBuilder::GetFileList(std::string key)
{
  std::string directory = "./";

  std::vector<std::string> fileList = {};
  // Check if the directory exists
  if (!std::filesystem::exists(directory)) {
    std::cerr << "Directory not found: " << directory << std::endl;
  }

  // Make all file list
  std::vector<std::string> allFileList;
  for (const auto &entry : std::filesystem::directory_iterator(directory)) {
    if (entry.is_regular_file()) {
      allFileList.push_back(entry.path().string());
    }
  }

  for (auto i = 0; i < 1024; i++) {
    std::string searchKey = key + Form("_%d.root", i);
    auto keyFound = false;
    for (const auto &file : allFileList) {
      if (file.find(searchKey) != std::string::npos) {
        fileList.push_back(file);
        keyFound = true;
        break;
      }
    }

    if (keyFound == false) {
      break;
    }
  }

  fFileList = fileList;
}