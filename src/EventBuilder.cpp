#include "EventBuilder.hpp"

#include <TFile.h>
#include <TTree.h>

#include <algorithm>
#include <iostream>

namespace DELILA
{

EventBuilder::EventBuilder(
    const std::string &fileName, const double_t timeWindow,
    const std::vector<std::vector<ChSettings>> &chSettings,
    const std::vector<std::vector<TimeSettings>> &timeSettings)
    : fFileName(fileName),
      fTimeWindow(timeWindow),
      fChSettings(chSettings),
      fTimeSettings(timeSettings)
{
}

EventBuilder::~EventBuilder() {}

uint32_t EventBuilder::LoadHits()
{
  fHitData.clear();

  auto file = TFile::Open(fFileName.c_str(), "READ");
  if (!file) {
    std::cout << "File not found: " << fFileName << std::endl;
    return 0;
  }
  auto tree = dynamic_cast<TTree *>(file->Get("ELIADE_Tree"));
  if (!tree) {
    std::cout << "Tree not found: " << fFileName << std::endl;
    return 0;
  }
  tree->SetBranchStatus("*", kFALSE);

  HitData_t hit;

  tree->SetBranchStatus("Ch", kTRUE);
  tree->SetBranchAddress("Ch", &hit.Channel);

  tree->SetBranchStatus("Mod", kTRUE);
  tree->SetBranchAddress("Mod", &hit.Module);

  tree->SetBranchStatus("FineTS", kTRUE);
  tree->SetBranchAddress("FineTS", &hit.Timestamp);

  tree->SetBranchStatus("ChargeLong", kTRUE);
  tree->SetBranchAddress("ChargeLong", &hit.Energy);

  tree->SetBranchStatus("ChargeShort", kTRUE);
  tree->SetBranchAddress("ChargeShort", &hit.EnergyShort);

  const auto nEntries = tree->GetEntries();
  for (auto i = 0; i < nEntries; i++) {
    tree->GetEntry(i);
    hit.Timestamp /= 1000.0;  // ps -> ns
    // hit.Timestamp += fSettings.at(hit.Module).at(hit.Channel).timeOffset;
    fHitData.push_back(hit);
  }

  CheckHitData();

  file->Close();

  return fHitData.size();
}

void EventBuilder::CheckHitData()
{
  const double_t timeOffset = (pow(2, 47) - 1);
  const auto firstTS = fHitData.at(0).Timestamp;
  const auto lastTS = fHitData.at(fHitData.size() - 1).Timestamp;
  if (lastTS - firstTS > timeOffset) {
    std::cout << "Timestamp overflow detected: " << fFileName;
    std::cout << "\nFirst timestamp: " << firstTS;
    std::cout << "\nLast timestamp: " << lastTS << std::endl;

    for (auto i = 0; i < fHitData.size() - 1; i++) {
      auto originalTS = fHitData.at(i).Timestamp;
      if (fHitData.at(i).Module == 0 || fHitData.at(i).Module == 1) {
        fHitData.at(i).Timestamp += timeOffset * 4;
      } else {
        fHitData.at(i).Timestamp += timeOffset * 2;
      }

      if (fHitData.at(i + 1).Timestamp - originalTS > timeOffset) {
        break;
      }
    }
  }

  std::sort(fHitData.begin(), fHitData.end(),
            [](const HitData_t &a, const HitData_t &b) {
              return a.Timestamp < b.Timestamp;
            });
}

uint32_t EventBuilder::EventBuild()
{
  if (fHitData.size() == 0) {
    std::cout << "No hits loaded." << std::endl;
    return 0;
  }

  fEventData = std::make_unique<std::vector<EventData>>();

  const int32_t nHits = fHitData.size();
  for (auto iHit = 0; iHit < nHits; iHit++) {
    auto hit = fHitData.at(iHit);
    if (fChSettings.at(hit.Module).at(hit.Channel).isEventTrigger) {
      EventData eventData;
      uint16_t frontADC = 0;
      uint16_t backADC = 0;

      auto triggerTime = hit.Timestamp;
      hit.Timestamp -= triggerTime;
      eventData.HitDataVec.push_back(hit);

      bool fillFlag = true;

      for (auto jHit = iHit + 1; (jHit < nHits) && fillFlag; jHit++) {
        auto nextHit = fHitData.at(jHit);
        if (fChSettings.at(nextHit.Module).at(nextHit.Channel).isEventTrigger) {
        }

        nextHit.Timestamp -= triggerTime;
        if (nextHit.Timestamp > fTimeWindow) {
          break;
        }

        eventData.HitDataVec.push_back(nextHit);
      }

      for (auto jHit = iHit - 1; (jHit >= 0) && fillFlag; jHit--) {
        auto prevHit = fHitData.at(jHit);
        if (fChSettings.at(prevHit.Module).at(prevHit.Channel).isEventTrigger) {
        }

        prevHit.Timestamp -= triggerTime;
        if (prevHit.Timestamp < -fTimeWindow) {
          break;
        }

        eventData.HitDataVec.push_back(prevHit);
      }

      if (fillFlag) {
        ;
      }
    }
  }

  return fEventData->size();
}

}  // namespace DELILA