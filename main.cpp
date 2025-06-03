#include <TROOT.h>
#include <TString.h>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <nlohmann/json.hpp>
#include <string>
#include <thread>
#include <vector>

#include "ChSettings.hpp"
#include "L1EventBuilder.hpp"
#include "L2EventBuilder.hpp"
#include "TimeAlignment.hpp"

std::vector<std::string> GetFileList(const std::string &directory,
                                     const uint32_t runNumber,
                                     const uint32_t startVersion,
                                     const uint32_t endVersion)
{
  std::vector<std::string> fileList = {};
  // Check if the directory exists
  if (!std::filesystem::exists(directory)) {
    std::cerr << "Directory not found: " << directory << std::endl;
    return fileList;
  }

  // Make all file list
  std::vector<std::string> allFileList;
  for (const auto &entry : std::filesystem::directory_iterator(directory)) {
    if (entry.is_regular_file()) {
      allFileList.push_back(entry.path().string());
    }
  }

  // Filter the file list based on the run number and version
  for (auto i = startVersion; i <= endVersion; i++) {
    std::string searchKey = Form("run%04d_%04d_", runNumber, i);
    std::string searchKeyOld = Form("run%d_%d_", runNumber, i);
    for (const auto &file : allFileList) {
      if ((file.find(searchKey) != std::string::npos) ||
          (file.find(searchKeyOld) != std::string::npos)) {
        // Check if the file is a ROOT file
        if (file.find(".root") == std::string::npos) {
          continue;
        }
        fileList.push_back(file);
        break;
      }
    }
  }

  return fileList;
}

enum class BuildType {
  Init,
  Time,
  L1,
  L2,
};

void PrintHelp()
{
  std::cout << "Usage: eve-builder [options]" << std::endl;
  std::cout << "Options:" << std::endl;
  std::cout << "  -h         Show this help message" << std::endl;
  std::cout << "  -i         Initialize the event builder" << std::endl;
  std::cout << "  -t         Generating time allignment file." << std::endl;
  std::cout << "  -l1        Making files by L1 trigger settings" << std::endl;
  std::cout << "  -l2        Making files by L2 trigger settings" << std::endl;
}

int main(int argc, char *argv[])
{
  BuildType buildType = BuildType::Init;
  if (argc < 2) {
    std::cout << "No options provided. Initialize mode." << std::endl;
  } else {
    for (int i = 1; i < argc; ++i) {
      if (std::string(argv[i]) == "-h") {
        PrintHelp();
        return 0;
      } else if (std::string(argv[i]) == "-i") {
        buildType = BuildType::Init;
      } else if (std::string(argv[i]) == "-t") {
        buildType = BuildType::Time;
      } else if (std::string(argv[i]) == "-l1") {
        buildType = BuildType::L1;
      } else if (std::string(argv[i]) == "-l2") {
        buildType = BuildType::L2;
      }
    }
  }

  std::string fileDir = "";
  auto runNumber = 0;
  auto startVersion = 0;
  auto endVersion = 0;
  auto timeWindow = 1000.;
  auto coincidenceWindow = 1000.;
  std::string chSettingsFileName = "chSettings.json";
  std::string l2SettingsFileName = "L2Settings.json";
  auto nThread = 0;
  auto refMod = 9;
  auto refCh = 0;

  auto settings = std::ifstream("settings.json");
  if (!settings) {
    std::cerr << "File not found: settings.json" << std::endl;
    std::cout << "Using default settings." << std::endl;
  } else {
    nlohmann::json j;
    settings >> j;
    fileDir = j["Directory"];
    runNumber = j["RunNumber"];
    startVersion = j["StartVersion"];
    endVersion = j["EndVersion"];
    timeWindow = j["TimeWindow"];
    coincidenceWindow = j["CoincidenceWindow"];
    chSettingsFileName = j["ChannelSettings"];
    l2SettingsFileName = j["L2Settings"];
    nThread = j["NumberOfThread"];
    refMod = j["TimeReferenceMod"];
    refCh = j["TimeReferenceCh"];
  }
  if (nThread == 0) {
    nThread = std::thread::hardware_concurrency();
  }

  if (buildType == BuildType::Init) {
    std::cout << "Initializing the event builder..." << std::endl;
    // Initialize the event builder
    std::cout << "Please enter the following information:" << std::endl;
    std::cout << "What is the data directory? (default: " << fileDir << "): ";
    std::string bufString;
    std::getline(std::cin, bufString);
    if (bufString != "") {
      fileDir = bufString;
    }

    std::cout << "What is the run number? (default: " << runNumber << "): ";
    std::getline(std::cin, bufString);
    if (bufString != "") {
      runNumber = std::stoi(bufString);
    }
    std::cout << "What is the start version? (default: " << startVersion
              << "): ";
    std::getline(std::cin, bufString);
    if (bufString != "") {
      startVersion = std::stoi(bufString);
    }
    std::cout << "What is the end version? (default: " << endVersion << "): ";
    std::getline(std::cin, bufString);
    if (bufString != "") {
      endVersion = std::stoi(bufString);
    }
    std::cout << "What is the time window? (default: " << timeWindow << "): ";
    std::getline(std::cin, bufString);
    if (bufString != "") {
      timeWindow = std::stoi(bufString);
    }
    std::cout << "What is the coincidence window? (default: "
              << coincidenceWindow << "): ";
    std::getline(std::cin, bufString);
    if (bufString != "") {
      coincidenceWindow = std::stoi(bufString);
    }

    uint32_t nMods = 11;
    uint32_t nChs = 32;
    std::cout << "How many modules? (default: " << nMods << "): ";
    std::getline(std::cin, bufString);
    if (bufString != "") {
      nMods = std::stoi(bufString);
    }
    auto nChsInMod = std::vector<uint32_t>(nMods);
    for (auto i = 0; i < nMods; i++) {
      std::cout << "How many channels of module " << i << "? (default: " << nChs
                << "): ";
      std::getline(std::cin, bufString);
      if (bufString != "") {
        nChs = std::stoi(bufString);
      }
      nChsInMod[i] = nChs;
    }

    int refMod = 9;
    int refCh = 0;
    std::cout << "What is the time reference module? (default: " << refMod
              << "): ";
    std::getline(std::cin, bufString);
    if (bufString != "") {
      refMod = std::stoi(bufString);
    }
    std::cout << "What is the time reference channel? (default: " << refCh
              << "): ";
    std::getline(std::cin, bufString);
    if (bufString != "") {
      refCh = std::stoi(bufString);
    }

    std::string chSettingsFileName = "chSettings.json";
    std::cout << "What is the channel settings file name? (default: "
              << chSettingsFileName << "): ";
    std::getline(std::cin, bufString);
    if (bufString != "") {
      chSettingsFileName = bufString;
    }

    std::string l2SettingsFileName = "L2Settings.json";
    std::cout << "What is the L2 settings file name? (default: "
              << l2SettingsFileName << "): ";
    std::getline(std::cin, bufString);
    if (bufString != "") {
      l2SettingsFileName = bufString;
    }

    std::cout << "Generating settings template..." << std::endl;

    // Making settings.json
    nlohmann::json settings;
    settings["Directory"] = fileDir;
    settings["RunNumber"] = runNumber;
    settings["StartVersion"] = startVersion;
    settings["EndVersion"] = endVersion;
    settings["TimeWindow"] = timeWindow;
    settings["ChannelSettings"] = chSettingsFileName;
    settings["NumberOfThread"] = 0;
    settings["TimeReferenceMod"] = refMod;
    settings["TimeReferenceCh"] = refCh;
    settings["CoincidenceWindow"] = coincidenceWindow;
    settings["L2Settings"] = l2SettingsFileName;

    std::ofstream ofs("settings.json");
    ofs << settings.dump(4) << std::endl;
    ofs.close();
    std::cout << "settings.json generated." << std::endl;

    // Making chSettings.json
    DELILA::ChSettings::GenerateTemplate(nChsInMod, chSettingsFileName);
    std::cout << chSettingsFileName << " generated." << std::endl;

    std::cout << "Initialization completed." << std::endl;

    return 0;
  }

  auto fileList = GetFileList(fileDir, runNumber, startVersion, endVersion);
  if (fileList.empty()) {
    std::cerr << "No files found." << std::endl;
    return 1;
  }
  // std::cout << "Found files:" << std::endl;
  // for (const auto &file : fileList) {
  //   std::cout << file << std::endl;
  // }
  std::cout << "Total files: " << fileList.size() << std::endl;

  if (fileList.size() < nThread) {
    nThread = fileList.size();
  }

  auto start = std::chrono::high_resolution_clock::now();

  if (buildType == BuildType::Time) {
    std::cout << "Generating time alignment information..." << std::endl;
    auto timeAlign = std::make_unique<DELILA::TimeAlignment>();
    timeAlign->LoadChSettings(chSettingsFileName);
    timeAlign->LoadFileList(fileList);
    timeAlign->SetTimeWindow(timeWindow);
    timeAlign->InitHistograms();
    timeAlign->FillHistograms(nThread);
    timeAlign->CalculateTimeAlignment();
    std::cout << "Time alignment information generated." << std::endl;
    return 0;
  } else if (buildType == BuildType::L1) {
    std::cout << "Generating L1 trigger information..." << std::endl;
    auto l1EventBuilder = std::make_unique<DELILA::L1EventBuilder>();
    l1EventBuilder->LoadChSettings(chSettingsFileName);
    l1EventBuilder->LoadFileList(fileList);
    l1EventBuilder->LoadTimeSettings(DELILA::kTimeSettingsFileName);
    l1EventBuilder->SetRefMod(refMod);
    l1EventBuilder->SetRefCh(refCh);
    l1EventBuilder->SetTimeWindow(timeWindow);
    l1EventBuilder->SetCoincidenceWindow(coincidenceWindow);
    l1EventBuilder->BuildEvent(nThread);
    std::cout << "L1 trigger event file generated." << std::endl;
  } else if (buildType == BuildType::L2) {
    std::cout << "Generating L2 trigger information..." << std::endl;
    auto l2EventBuilder = std::make_unique<DELILA::L2EventBuilder>();
    l2EventBuilder->LoadChSettings(chSettingsFileName);
    l2EventBuilder->SetCoincidenceWindow(coincidenceWindow);
    l2EventBuilder->LoadL2Settings(l2SettingsFileName);
    l2EventBuilder->BuildEvent(nThread);
    std::cout << "L2 trigger event file generated." << std::endl;
  }

  auto end = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::seconds>(end - start);
  std::cout << "Time taken: " << duration.count() << " seconds." << std::endl;

  return 0;
}