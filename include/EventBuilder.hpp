#ifndef EventBuilder_hpp
#define EventBuilder_hpp 1

#include <memory>
#include <string>
#include <vector>

#include "ChSettings.hpp"
#include "EventData.hpp"

namespace DELILA
{

class EventBuilder
{
 public:
  EventBuilder(const std::string &fileName, const double_t timeWindow,
               const std::vector<std::vector<ChSettings_t>> &settings,
               const std::vector<std::vector<TimeSettings_t>> &timeSettings);
  ~EventBuilder();

  uint32_t LoadHits();
  uint32_t EventBuild();

  std::unique_ptr<std::vector<EventData>> GetEventData()
  {
    return std::move(fEventData);
  }

  void SetTimeWindow(double_t timeWindow) { fTimeWindow = timeWindow; }

 private:
  std::vector<HitData> fHitData;
  void CheckHitData();

  std::unique_ptr<std::vector<EventData>> fEventData;
  std::string fFileName;
  std::vector<std::vector<ChSettings>> fChSettings;
  std::vector<std::vector<TimeSettings>> fTimeSettings;
  double_t fTimeWindow = 0.0;  // ns
};

}  // namespace DELILA

#endif