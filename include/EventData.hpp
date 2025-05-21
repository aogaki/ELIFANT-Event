#ifndef EventData_hpp
#define EventData_hpp 1

#include <TROOT.h>

#include <memory>
#include <tuple>
#include <vector>

namespace DELILA
{

class RawData_t
{
 public:
  RawData_t() {};
  RawData_t(const bool isWithAC, const uint8_t mod, const uint8_t ch,
            const uint16_t chargeLong, const uint16_t chargeShort,
            const double_t fineTS)
      : isWithAC(isWithAC),
        mod(mod),
        ch(ch),
        chargeLong(chargeLong),
        chargeShort(chargeShort),
        fineTS(fineTS) {};
  RawData_t(const RawData_t &other)
      : isWithAC(other.isWithAC),
        mod(other.mod),
        ch(other.ch),
        chargeLong(other.chargeLong),
        chargeShort(other.chargeShort),
        fineTS(other.fineTS) {};
  RawData_t &operator=(const RawData_t &other) = default;
  ~RawData_t() = default;

  bool isWithAC;
  uint8_t mod;
  uint8_t ch;
  uint16_t chargeLong;
  uint16_t chargeShort;
  double_t fineTS;

  ClassDef(RawData_t, 1);
};

class EventData
{
 public:
  EventData() { eventDataVec = new std::vector<RawData_t>(); };
  EventData(const EventData &other) = default;
  EventData(EventData &&other) = default;
  EventData &operator=(const EventData &other) = default;
  EventData &operator=(EventData &&other) = default;
  ~EventData() = default;

  void Clear()
  {
    triggerTime = 0.;
    eventDataVec->clear();
  };

  double_t triggerTime = 0.;
  std::vector<RawData_t> *eventDataVec;

  ClassDef(EventData, 1);
};
typedef EventData EventData_t;

}  // namespace DELILA
#endif