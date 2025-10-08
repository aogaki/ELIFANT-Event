#include <TFile.h>
#include <TSystem.h>
#include <TTree.h>

#include <algorithm>
#include <iostream>
#include <map>
#include <vector>

class TreeData
{  // no getter setter.  using public member variables.
 public:
  TreeData() {};
  TreeData(unsigned char mod, unsigned char ch, uint64_t timeStamp,
           double fineTS, uint16_t chargeLong, uint16_t chargeShort,
           uint32_t nSamples)
      : Mod(mod),
        Ch(ch),
        TimeStamp(timeStamp),
        FineTS(fineTS),
        ChargeLong(chargeLong),
        ChargeShort(chargeShort)
  {
    RecordLength = nSamples;
    Trace1.resize(nSamples);
    Trace2.resize(nSamples);
    DTrace1.resize(nSamples);
    DTrace2.resize(nSamples);
  };

  TreeData(uint32_t nSamples)
  {
    RecordLength = nSamples;
    Trace1.resize(nSamples);
    Trace2.resize(nSamples);
    DTrace1.resize(nSamples);
    DTrace2.resize(nSamples);
  };

  ~TreeData() {};

  unsigned char Mod;
  unsigned char Ch;
  uint64_t TimeStamp;
  double FineTS;
  uint16_t ChargeLong;
  uint16_t ChargeShort;
  uint32_t Extras;
  uint32_t RecordLength;
  std::vector<uint16_t> Trace1;
  std::vector<uint16_t> Trace2;
  std::vector<uint8_t> DTrace1;
  std::vector<uint8_t> DTrace2;

  static const uint16_t OneHitSize =
      sizeof(Mod) + sizeof(Ch) + sizeof(TimeStamp) + sizeof(FineTS) +
      sizeof(ChargeLong) + sizeof(ChargeShort) + sizeof(RecordLength);
};
typedef TreeData TreeData_t;

void WriteData(TString outFileName, std::vector<TreeData_t> &data)
{
  // File has to be stored resort directory
  gSystem->mkdir("resort", true);
  gSystem->cd("resort");

  TFile *outFile = TFile::Open(outFileName, "RECREATE");
  if (!outFile || outFile->IsZombie()) {
    std::cerr << "Error creating file: " << outFileName << std::endl;
    return;
  }
  TTree *tree = new TTree("ELIADE_Tree", "Resorted ELIADE data");

  UChar_t Mod, Ch;
  ULong64_t TimeStamp;
  Double_t FineTS;
  UShort_t ChargeLong;
  UShort_t ChargeShort;
  UInt_t RecordLength;
  UShort_t Signal[100000]{0};
  tree->Branch("Mod", &Mod, "Mod/b");
  tree->Branch("Ch", &Ch, "Ch/b");
  tree->Branch("TimeStamp", &TimeStamp, "TimeStamp/l");
  tree->Branch("FineTS", &FineTS, "Finets/D");
  tree->Branch("ChargeLong", &ChargeLong, "ChargeLong/s");
  tree->Branch("ChargeShort", &ChargeShort, "ChargeShort/s");
  tree->Branch("RecordLength", &RecordLength, "RecordLength/i");
  tree->Branch("Signal", Signal, "Signal[RecordLength]/s");

  for (const auto &d : data) {
    Mod = d.Mod;
    Ch = d.Ch;
    TimeStamp = d.TimeStamp;
    FineTS = d.FineTS;
    ChargeLong = d.ChargeLong;
    ChargeShort = d.ChargeShort;
    RecordLength = d.RecordLength;
    std::copy(d.Trace1.begin(), d.Trace1.end(), Signal);
    tree->Fill();
  }

  tree->Write();
  outFile->Close();

  gSystem->cd("..");
  std::cout << "Written resort/" << outFileName << " with " << data.size()
            << " entries." << std::endl;
}

void ReSorter(const UInt_t runNumber)
{
  constexpr uint32_t nFiles = 2;  // number of files to be merged
  auto fileCounter = 0;
  auto version = 0;
  auto outVersion = 0;
  std::vector<TreeData_t> data;
  while (true) {
    auto fileName =
        TString::Format("run%04d_%04d_p_91Zr.root", runNumber, version++);
    if (gSystem->AccessPathName(fileName)) {  // No such file
      if (fileCounter == 0) {
        std::cout << "No more files found. Exiting." << std::endl;
        return;
      }
      std::sort(data.begin(), data.end(),
                [](const TreeData_t &a, const TreeData_t &b) {
                  return a.FineTS < b.FineTS;
                });
      auto outFileName = TString::Format("run%04d_%04d_p_91Zr_resort.root",
                                         runNumber, outVersion++);
      WriteData(outFileName, data);
      break;
    }
    // Load file
    TFile *file = TFile::Open(fileName, "READ");
    if (!file || file->IsZombie()) {
      std::cerr << "Error opening file: " << fileName << std::endl;
      continue;
    }
    TTree *tree = (TTree *)file->Get("ELIADE_Tree");
    if (!tree) {
      std::cerr << "Tree not found in file: " << fileName << std::endl;
      file->Close();
      continue;
    }
    UChar_t Mod, Ch;
    ULong64_t TimeStamp;
    Double_t FineTS;
    UShort_t ChargeLong;
    UShort_t ChargeShort;
    UInt_t RecordLength;
    tree->SetBranchAddress("Mod", &Mod);
    tree->SetBranchAddress("Ch", &Ch);
    tree->SetBranchAddress("TimeStamp", &TimeStamp);
    tree->SetBranchAddress("FineTS", &FineTS);
    tree->SetBranchAddress("ChargeLong", &ChargeLong);
    tree->SetBranchAddress("ChargeShort", &ChargeShort);
    tree->SetBranchAddress("RecordLength", &RecordLength);
    // Get number of entries
    Long64_t nEntries = tree->GetEntries();
    if (nEntries == 0) {
      std::cout << "Warning: " << fileName << " has no entries" << std::endl;
      file->Close();
      continue;
    }
    // Read entries
    std::cout << "Reading file: " << fileName << " with " << nEntries
              << " entries." << std::endl;
    for (Long64_t i = 0; i < nEntries; i++) {
      tree->GetEntry(i);
      data.emplace_back(Mod, Ch, TimeStamp, FineTS, ChargeLong, ChargeShort,
                        RecordLength);
    }
    file->Close();
    fileCounter++;

    if (fileCounter == nFiles + 1) {
      std::sort(data.begin(), data.end(),
                [](const TreeData_t &a, const TreeData_t &b) {
                  return a.FineTS < b.FineTS;
                });
      // Separate data into 2 files. One is nFiles, the other is the rest.
      auto border = data.size() * nFiles / (nFiles + 1);
      std::vector<TreeData_t> outData(data.begin(), data.begin() + border);

      auto outFileName = TString::Format("run%04d_%04d_p_91Zr_resort.root",
                                         runNumber, outVersion++);
      WriteData(outFileName, outData);
      data.erase(data.begin(), data.begin() + border);
      fileCounter = 1;
    }
  }
}