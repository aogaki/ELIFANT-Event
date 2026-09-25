#include <gtest/gtest.h>

#include <TFile.h>
#include <TTree.h>
#include <unistd.h>

#include <filesystem>
#include <fstream>
#include <memory>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

#include "ChSettings.hpp"
#include "EventData.hpp"
#include "L1EventBuilder.hpp"

using namespace DELILA;

//=============================================================================
// L1 trigger rules and cross-file continuity on tiny synthetic raw files.
// One module, three channels: ch0 trigger ID 1, ch1 trigger ID 2,
// ch2 not a trigger. Coincidence window 100 ns, all time offsets 0.
//=============================================================================

namespace
{

struct Hit {
  uint8_t ch;
  double tNs;
};

struct L1Event {
  double triggerTime;
  uint8_t triggerCh;
  size_t nHits;
};

class L1RulesTest : public ::testing::Test
{
 protected:
  std::filesystem::path fOldCwd;
  std::filesystem::path fDir;

  void SetUp() override
  {
    fOldCwd = std::filesystem::current_path();
    fDir = std::filesystem::temp_directory_path() /
           ("eve_l1_rules_" + std::to_string(::getpid()));
    std::filesystem::create_directories(fDir);
    std::filesystem::current_path(fDir);  // L1_<thread>.root is written to cwd

    ChSettings::GenerateTemplate({3}, "chSettings.json");
    nlohmann::json chs;
    {
      std::ifstream ifs("chSettings.json");
      ifs >> chs;
    }
    for (int ch = 0; ch < 3; ch++) {
      chs[0][ch]["IsEventTrigger"] = (ch < 2);
      chs[0][ch]["ID"] = ch + 1;
      chs[0][ch]["ThresholdADC"] = 0;
    }
    std::ofstream("chSettings.json") << chs.dump(4);

    // [refMod][refCh][mod][ch]["TimeOffset"]
    nlohmann::json offsets = nlohmann::json::array();
    for (int ch = 0; ch < 3; ch++) offsets.push_back({{"TimeOffset", 0.}});
    nlohmann::json perRefCh = nlohmann::json::array();
    for (int refCh = 0; refCh < 3; refCh++) {
      perRefCh.push_back(nlohmann::json::array({offsets}));
    }
    std::ofstream("timeSettings.json")
        << nlohmann::json::array({perRefCh}).dump();
  }

  void TearDown() override
  {
    std::filesystem::current_path(fOldCwd);
    std::filesystem::remove_all(fDir);
  }

  static void WriteRaw(const std::string &name, const std::vector<Hit> &hits)
  {
    TFile file(name.c_str(), "RECREATE");
    TTree tree("ELIADE_Tree", "ELIADE_Tree");
    UChar_t mod = 0, ch = 0;
    Double_t fineTS = 0.;
    UShort_t chargeLong = 100, chargeShort = 50;
    tree.Branch("Mod", &mod, "Mod/b");
    tree.Branch("Ch", &ch, "Ch/b");
    tree.Branch("FineTS", &fineTS, "FineTS/D");
    tree.Branch("ChargeLong", &chargeLong, "ChargeLong/s");
    tree.Branch("ChargeShort", &chargeShort, "ChargeShort/s");
    for (const auto &hit : hits) {
      ch = hit.ch;
      fineTS = hit.tNs * 1000.;  // ns to ps
      tree.Fill();
    }
    tree.Write();
  }

  static std::vector<L1Event> RunL1(const std::vector<std::string> &files)
  {
    {
      L1EventBuilder builder;
      builder.LoadChSettings("chSettings.json");
      builder.LoadTimeSettings("timeSettings.json");
      builder.LoadFileList(files);
      builder.SetCoincidenceWindow(100.);
      builder.SetRefMod(0);
      builder.SetRefCh(0);
      builder.BuildEvent(1);
    }

    std::vector<L1Event> events;
    TFile file("L1_0.root", "READ");
    auto tree = static_cast<TTree *>(file.Get("L1EventData"));
    if (!tree) return events;
    Double_t triggerTime = 0.;
    std::vector<RawData_t> *hits = nullptr;
    tree->SetBranchAddress("TriggerTime", &triggerTime);
    tree->SetBranchAddress("EventDataVec", &hits);
    for (Long64_t i = 0; i < tree->GetEntries(); i++) {
      tree->GetEntry(i);
      events.push_back({triggerTime, hits->at(0).ch, hits->size()});
    }
    return events;
  }
};

}  // namespace

TEST_F(L1RulesTest, EqualIdEarlierWinsAndLowerIdWins)
{
  WriteRaw("raw_a.root", {
                             {0, 1000.},  // ch0 twice: the earlier one wins
                             {0, 1050.},
                             {1, 5000.},  // ID 2 loses to the later ID 1
                             {0, 5040.},
                             {2, 5060.},  // not a trigger: content only
                         });
  auto events = RunL1({"raw_a.root"});

  ASSERT_EQ(events.size(), 2u);
  EXPECT_DOUBLE_EQ(events[0].triggerTime, 1000.);
  EXPECT_EQ(events[0].triggerCh, 0);
  EXPECT_EQ(events[0].nHits, 2u);
  EXPECT_DOUBLE_EQ(events[1].triggerTime, 5040.);
  EXPECT_EQ(events[1].triggerCh, 0);
  EXPECT_EQ(events[1].nHits, 3u);
}

TEST_F(L1RulesTest, TimeInterleavedFilesFormOneStream)
{
  // The second file starts before the first one ends (as DELILA segments do)
  WriteRaw("raw_a.root", {{2, 10000.}, {0, 20000.}, {2, 20060.}});
  WriteRaw("raw_b.root", {{2, 19980.}, {0, 30000.}});
  auto events = RunL1({"raw_a.root", "raw_b.root"});

  // Each trigger exactly once; the event at 20000 sees the hit of the next file
  ASSERT_EQ(events.size(), 2u);
  EXPECT_DOUBLE_EQ(events[0].triggerTime, 20000.);
  EXPECT_EQ(events[0].nHits, 3u);
  EXPECT_DOUBLE_EQ(events[1].triggerTime, 30000.);
  EXPECT_EQ(events[1].nHits, 1u);
}
