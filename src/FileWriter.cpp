#include "FileWriter.hpp"

#include <iostream>

namespace DELILA
{

FileWriter::FileWriter(std::string fileName)
{
  fOutputFile = new TFile(fileName.c_str(), "RECREATE");
  fTree = new TTree("Event_Tree", "Data tree");
  fTree->Branch("Module", &fModule);
  fTree->Branch("Channel", &fChannel);
  fTree->Branch("Timestamp", &fTimestamp);
  fTree->Branch("Energy", &fEnergy);
  fTree->Branch("EnergyShort", &fEnergyShort);
  fTree->SetDirectory(fOutputFile);

  fRawData = std::make_unique<std::vector<EventData>>();

  fWriteDataThread = std::thread(&FileWriter::WriteData, this);
}

FileWriter::~FileWriter()
{
  // delete fOutputFile;
}

void FileWriter::SetData(std::unique_ptr<std::vector<EventData>> &data)
{
  fMutex.lock();
  fRawData->insert(fRawData->end(), data->begin(), data->end());
  fMutex.unlock();
}

void FileWriter::Write()
{
  while (true) {
    fMutex.lock();
    if (fRawData->size() == 0) {
      fWritingFlag = false;
      fMutex.unlock();
      break;
    }
    fMutex.unlock();
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
  fWriteDataThread.join();

  fMutex.lock();
  fOutputFile->cd();
  fTree->Write();
  fOutputFile->Write();
  fOutputFile->Close();
  fMutex.unlock();
}

void FileWriter::WriteData()
{
  fWritingFlag = true;

  while (fWritingFlag) {
    fMutex.lock();
    auto size = fRawData->size();
    fMutex.unlock();

    if (size > 0) {
      auto localData = std::make_unique<std::vector<EventData>>();
      fMutex.lock();
      localData->insert(localData->end(), fRawData->begin(), fRawData->end());
      fRawData->clear();
      fMutex.unlock();

      for (auto &event : *localData) {
        fModule.clear();
        fChannel.clear();
        fTimestamp.clear();
        fEnergy.clear();
        fEnergyShort.clear();
        for (auto &hit : event.HitDataVec) {
          fModule.push_back(hit.Module);
          fChannel.push_back(hit.Channel);
          fTimestamp.push_back(hit.Timestamp);
          fEnergy.push_back(hit.Energy);
          fEnergyShort.push_back(hit.EnergyShort);
        }
        fTree->Fill();
      }
    } else {
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
  }
}

}  // namespace DELILA