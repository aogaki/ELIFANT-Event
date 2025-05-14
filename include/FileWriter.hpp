#ifndef FileWriter_hpp
#define FileWriter_hpp 1

#include <TFile.h>
#include <TTree.h>

#include <memory>
#include <mutex>
#include <thread>
#include <vector>

#include "EventData.hpp"

namespace DELILA
{

class FileWriter
{
 public:
  FileWriter(std::string fileName);
  ~FileWriter();

  void SetData(std::unique_ptr<std::vector<EventData>> &data);

  void Write();

 private:
  void WriteData();
  std::thread fWriteDataThread;
  std::mutex fMutex;
  bool fWritingFlag;

  std::unique_ptr<std::vector<EventData>> fRawData;
  TFile *fOutputFile;
  TTree *fTree;

  // For tree branches
  std::vector<uint8_t> fModule;
  std::vector<uint8_t> fChannel;
  std::vector<double_t> fTimestamp;
  std::vector<uint16_t> fEnergy;
  std::vector<uint16_t> fEnergyShort;
};

}  // namespace DELILA
#endif