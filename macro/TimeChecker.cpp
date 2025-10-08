#include <TFile.h>
#include <TSystem.h>
#include <TTree.h>

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <map>
#include <vector>

struct RunInfo {
  int runNumber;
  int version;
  Double_t minTime;
  Double_t maxTime;
  Double_t duration;
  std::string fileName;
  Long64_t entries;
};

void TimeChecker()
{
  // Get list of all ROOT files
  void *dirp = gSystem->OpenDirectory(".");
  const char *entry;
  std::vector<std::string> fileList;

  while ((entry = gSystem->GetDirEntry(dirp))) {
    std::string fname(entry);
    if (fname.find("run") == 0 && fname.find(".root") != std::string::npos) {
      fileList.push_back(fname);
    }
  }
  gSystem->FreeDirectory(dirp);

  std::sort(fileList.begin(), fileList.end());

  // Store run information
  std::vector<RunInfo> runInfoList;

  // Process each file
  for (const auto &fname : fileList) {
    // Parse filename: runXXXX_YYYY_p_91Zr.root
    int runNum = 0, version = 0;
    if (sscanf(fname.c_str(), "run%d_%d", &runNum, &version) == 2) {
      TFile *file = TFile::Open(fname.c_str(), "READ");
      if (!file || file->IsZombie()) {
        std::cerr << "Error opening file: " << fname << std::endl;
        continue;
      }

      TTree *tree = (TTree *)file->Get("ELIADE_Tree");
      if (!tree) {
        std::cerr << "Tree not found in file: " << fname << std::endl;
        file->Close();
        continue;
      }

      Double_t fineTS;
      tree->SetBranchAddress("FineTS", &fineTS);

      Long64_t nEntries = tree->GetEntries();
      if (nEntries == 0) {
        std::cout << "Warning: " << fname << " has no entries" << std::endl;
        file->Close();
        continue;
      }

      // Get min and max time
      Double_t minTime = 1e20, maxTime = -1e20;
      for (Long64_t i = 0; i < nEntries; i++) {
        tree->GetEntry(i);
        if (fineTS < minTime) minTime = fineTS;
        if (fineTS > maxTime) maxTime = fineTS;
      }

      RunInfo info;
      info.runNumber = runNum;
      info.version = version;
      info.minTime = minTime;
      info.maxTime = maxTime;
      info.duration = maxTime - minTime;
      info.fileName = fname;
      info.entries = nEntries;

      runInfoList.push_back(info);

      file->Close();
    }
  }

  // Sort by run number and version
  std::sort(runInfoList.begin(), runInfoList.end(),
            [](const RunInfo &a, const RunInfo &b) {
              if (a.runNumber != b.runNumber) return a.runNumber < b.runNumber;
              return a.version < b.version;
            });

  // Print summary table
  std::cout << "\n========== TIME DURATION SUMMARY ==========\n" << std::endl;
  std::cout << std::setw(8) << "Run" << std::setw(8) << "Version"
            << std::setw(15) << "Min Time (ps)" << std::setw(15)
            << "Max Time (ps)" << std::setw(18) << "Duration (ps)"
            << std::setw(15) << "Duration (s)" << std::setw(10) << "Entries"
            << std::endl;
  std::cout << std::string(95, '-') << std::endl;

  for (const auto &info : runInfoList) {
    std::cout << std::setw(8) << info.runNumber << std::setw(8) << info.version
              << std::setw(15) << std::scientific << std::setprecision(3)
              << info.minTime << std::setw(15) << info.maxTime << std::setw(18)
              << info.duration << std::setw(15) << std::fixed
              << std::setprecision(2) << info.duration / 1e12 << std::setw(10)
              << info.entries << std::endl;
  }

  // Check overlaps between runs
  std::cout << "\n========== OVERLAP ANALYSIS ==========\n" << std::endl;

  // Group by run number
  std::map<int, std::vector<RunInfo>> runGroups;
  for (const auto &info : runInfoList) {
    runGroups[info.runNumber].push_back(info);
  }

  // Print per-run summary
  std::cout << "\n--- Per-Run Summary ---\n" << std::endl;
  for (const auto &pair : runGroups) {
    int runNum = pair.first;
    const auto &versions = pair.second;

    Double_t runMin = 1e20, runMax = -1e20;
    for (const auto &v : versions) {
      if (v.minTime < runMin) runMin = v.minTime;
      if (v.maxTime > runMax) runMax = v.maxTime;
    }

    std::cout << "Run " << std::setw(4) << runNum << ": Versions "
              << std::setw(4) << versions.front().version << " - "
              << std::setw(4) << versions.back().version << " ("
              << versions.size() << " files)"
              << ", Duration: " << std::fixed << std::setprecision(2)
              << (runMax - runMin) / 1e12 << " s" << std::endl;
  }

  // Check overlaps between consecutive runs
  std::cout << "\n--- Overlaps Between Runs ---\n" << std::endl;

  std::vector<int> runNumbers;
  for (const auto &pair : runGroups) {
    runNumbers.push_back(pair.first);
  }
  std::sort(runNumbers.begin(), runNumbers.end());

  for (size_t i = 0; i < runNumbers.size() - 1; i++) {
    int run1 = runNumbers[i];
    int run2 = runNumbers[i + 1];

    const auto &versions1 = runGroups[run1];
    const auto &versions2 = runGroups[run2];

    Double_t run1_max = -1e20;
    for (const auto &v : versions1) {
      if (v.maxTime > run1_max) run1_max = v.maxTime;
    }

    Double_t run2_min = 1e20;
    for (const auto &v : versions2) {
      if (v.minTime < run2_min) run2_min = v.minTime;
    }

    Double_t gap = run2_min - run1_max;

    std::cout << "Run " << std::setw(4) << run1 << " -> Run " << std::setw(4)
              << run2 << ": ";
    if (gap > 0) {
      std::cout << "Gap = " << std::fixed << std::setprecision(4) << gap / 1e9
                << " ms" << std::endl;
    } else {
      std::cout << "OVERLAP = " << std::fixed << std::setprecision(4)
                << -gap / 1e9 << " ms" << std::endl;
    }
  }

  // Check overlaps within same run (between versions)
  std::cout << "\n--- Overlaps Between Versions (Same Run) ---\n" << std::endl;

  for (const auto &pair : runGroups) {
    int runNum = pair.first;
    const auto &versions = pair.second;

    if (versions.size() > 1) {
      std::cout << "\nRun " << runNum << ":" << std::endl;
      for (size_t i = 0; i < versions.size() - 1; i++) {
        const auto &v1 = versions[i];
        const auto &v2 = versions[i + 1];

        Double_t gap = v2.minTime - v1.maxTime;

        std::cout << "  Version " << std::setw(4) << v1.version
                  << " -> Version " << std::setw(4) << v2.version << ": ";
        if (gap > 0) {
          std::cout << "Gap = " << std::fixed << std::setprecision(4)
                    << gap / 1e9 << " ms" << std::endl;
        } else {
          std::cout << "OVERLAP = " << std::fixed << std::setprecision(4)
                    << -gap / 1e9 << " ms" << std::endl;
        }
      }
    }
  }

  std::cout << "\n========== END OF ANALYSIS ==========\n" << std::endl;
}
