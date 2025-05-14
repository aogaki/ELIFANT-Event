#ifndef EventData_hpp
#define EventData_hpp 1

#include <tuple>
#include <vector>

namespace DELILA
{

class HitData
{
 public:
  uint8_t Module;
  uint8_t Channel;
  double_t Timestamp;
  uint16_t Energy;
  uint16_t EnergyShort;

  HitData() {};
  HitData(uint8_t Module, uint8_t Channel, double_t Timestamp, uint16_t Energy,
          uint16_t EnergyShort)
      : Channel(Channel),
        Timestamp(Timestamp),
        Module(Module),
        Energy(Energy),
        EnergyShort(EnergyShort) {};
  virtual ~HitData() {};
};
typedef HitData HitData_t;

class EventData
{
 public:
  EventData() {};
  virtual ~EventData() {};

  std::vector<HitData> HitDataVec;

 private:
};

}  // namespace DELILA
#endif