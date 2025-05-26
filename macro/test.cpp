#include <TChain.h>
#include <TFile.h>
#include <TH1.h>
#include <TH2.h>
#include <TSystem.h>

#include <iostream>

#include "EventData.hpp"

TH2D *hist;

void test()
{
  gSystem->Load("./libEveBuilder.dylib");

  hist = new TH2D("hist", "Test Histogram", 1600, 0, 16000, 1600, 0, 16000);

  auto chain = new TChain("L2EventData");
  chain->Add("L2_0.root");
  chain->Add("L2_1.root");
  chain->Add("L2_2.root");
  chain->Add("L2_3.root");
  chain->Add("L2_4.root");
  chain->Add("L2_5.root");
  chain->Add("L2_6.root");
  chain->Add("L2_7.root");
  chain->Add("L2_8.root");
  chain->Add("L2_9.root");
  chain->Add("L2_10.root");
  chain->Add("L2_11.root");
  chain->Add("L2_12.root");
  chain->Add("L2_13.root");

  std::cout << "Number of entries in chain: " << chain->GetEntries()
            << std::endl;

  DELILA::EventData eventData;
  chain->SetBranchAddress("TriggerTime", &eventData.triggerTime);
  chain->SetBranchAddress("EventDataVec", &eventData.eventDataVec);

  ULong64_t E_Sector_Counter = 0;
  chain->SetBranchAddress("E_Sector_Counter", &E_Sector_Counter);

  ULong64_t dE_Sector_Counter = 0;
  chain->SetBranchAddress("dE_Sector_Counter", &dE_Sector_Counter);

  const auto nEntries = chain->GetEntries();
  for (auto i = 0; i < nEntries; i++) {
    chain->GetEntry(i);
    if (i % 1000000 == 0) {
      std::cout << "Processing event " << i << " / " << nEntries << std::endl;
    }
    if (dE_Sector_Counter > 0 && E_Sector_Counter > 0) {
      //   std::cout << "Event " << i << ": TriggerTime = " << eventData.triggerTime
      //             << ", E_Sector_Counter = " << E_Sector_Counter
      //             << ", dE_Sector_Counter = " << dE_Sector_Counter
      //             << ", EventDataVec size = " << eventData.eventDataVec->size()
      //             << std::endl;

      // reference is 0
      auto E_Ene = 0.;
      auto dE_Ene = 0.;
      for (const auto &rawData : *eventData.eventDataVec) {
        if (rawData.mod == 4 && rawData.ch == 0) {
          E_Ene = rawData.chargeLong;
        } else if (rawData.mod == 0) {
          dE_Ene = rawData.chargeLong;
        }

        if (E_Ene > 0 && dE_Ene > 0) {
          hist->Fill(E_Ene, dE_Ene);
        }
      }
    }
  }

  hist->Draw("COLZ");
}