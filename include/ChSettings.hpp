#ifndef ChSettings_hpp
#define ChSettings_hpp 1

#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

namespace DELILA
{

enum class DetectorType {
  Unknown = 0,
  AC = 1,
  PMT = 2,
  HPGe = 3,
};

class ChSettings
{
 public:
  ChSettings() {};
  ~ChSettings() {};

  bool isEventTrigger = false;
  int32_t ID = 0;
  uint32_t mod = 0;
  uint32_t ch = 0;
  uint32_t thresholdADC = 0;

  bool hasAC = false;
  uint32_t ACMod = 0;
  uint32_t ACCh = 0;

  double_t phi = 0.;
  double_t theta = 0.;
  double_t distance = 0.;

  double_t x = 0.;
  double_t y = 0.;
  double_t z = 0.;

  double_t p0 = 0.;
  double_t p1 = 1.;
  double_t p2 = 0.;
  double_t p3 = 0.;

  std::string detectorType = "";

  std::vector<std::string> tags = {};

  static DetectorType GetDetectorType(std::string type)
  {
    std::transform(type.begin(), type.end(), type.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    if (type == "ac") {
      return DetectorType::AC;
    } else if (type == "pmt") {
      return DetectorType::PMT;
    } else if (type == "hpge") {
      return DetectorType::HPGe;
    } else {
      return DetectorType::Unknown;
    }
  };

  void Print()
  {
    std::cout << "Module: " << mod << "\tChannel: " << ch << std::endl;
    std::cout << "\tIs Event Trigger: " << isEventTrigger << std::endl;
    std::cout << "\tID: " << ID << std::endl;
    std::cout << "\tHas AC: " << hasAC << std::endl;
    std::cout << "\tAC Module: " << ACMod << "\tAC Channel: " << ACCh
              << std::endl;
    std::cout << "\tPhi: " << phi << "\tTheta: " << theta
              << "\tDistance: " << distance << std::endl;
    std::cout << "\tx: " << x << "\ty: " << y << "\tz: " << z << std::endl;
    std::cout << "\tp0: " << p0 << "\tp1: " << p1 << "\tp2: " << p2
              << "\tp3: " << p3 << std::endl;
    std::cout << "\tThreshold ADC: " << thresholdADC << std::endl;
    std::cout << std::endl;
  };

  static void GenerateTemplate(std::vector<uint32_t> nChsInMod,
                               std::string fileName)
  {
    nlohmann::json result;
    auto idCounter = 0;
    for (uint32_t i = 0; i < nChsInMod.size(); i++) {
      nlohmann::json mod = nlohmann::json::array();
      for (uint32_t j = 0; j < nChsInMod[i]; j++) {
        nlohmann::json ch;
        ch["IsEventTrigger"] = false;
        ch["ID"] = idCounter++;
        ch["Module"] = i;
        ch["Channel"] = j;
        ch["HasAC"] = false;
        ch["ACModule"] = 128;
        ch["ACChannel"] = 128;
        ch["Phi"] = 0.;
        ch["Theta"] = 0.;
        ch["Distance"] = 0.;
        ch["ThresholdADC"] = 0;
        ch["x"] = 0.;
        ch["y"] = 0.;
        ch["z"] = 0.;
        ch["p0"] = 0.;
        ch["p1"] = 1.;
        ch["p2"] = 0.;
        ch["p3"] = 0.;
        ch["DetectorType"] = "";
        ch["Tags"] = nlohmann::json::array();
        mod.push_back(ch);
      }
      result.push_back(mod);
    }

    std::ofstream ofs(fileName);
    ofs << result.dump(4) << std::endl;
    ofs.close();
  };

  static std::vector<std::vector<ChSettings>> GetChSettings(
      const std::string fileName)
  {
    std::vector<std::vector<ChSettings>> chSettingsVec;

    std::ifstream ifs(fileName);
    if (!ifs) {
      std::cerr << "File not found: " << fileName << std::endl;
      return chSettingsVec;
    }

    nlohmann::json j;
    ifs >> j;

    for (const auto &mod : j) {
      std::vector<ChSettings> chSettings;
      for (const auto &ch : mod) {
        ChSettings chSetting;
        chSetting.isEventTrigger = ch["IsEventTrigger"];
        chSetting.ID = ch["ID"];
        chSetting.mod = ch["Module"];
        chSetting.ch = ch["Channel"];
        chSetting.hasAC = ch["HasAC"];
        chSetting.ACMod = ch["ACModule"];
        chSetting.ACCh = ch["ACChannel"];
        chSetting.phi = ch["Phi"];
        chSetting.theta = ch["Theta"];
        chSetting.thresholdADC = ch["ThresholdADC"];
        chSetting.distance = ch["Distance"];
        chSetting.x = ch["x"];
        chSetting.y = ch["y"];
        chSetting.z = ch["z"];
        chSetting.p0 = ch["p0"];
        chSetting.p1 = ch["p1"];
        chSetting.p2 = ch["p2"];
        chSetting.p3 = ch["p3"];
        chSetting.detectorType = ch["DetectorType"];
        chSetting.tags = ch["Tags"].get<std::vector<std::string>>();

        chSettings.push_back(chSetting);
      }
      chSettingsVec.push_back(chSettings);
    }

    return chSettingsVec;
  };
};
typedef ChSettings ChSettings_t;

class TimeSettings
{
 public:
  TimeSettings() {};

  ~TimeSettings() {};

  double_t TimeOffset = 0.;
  double_t TimeWindowLeftEdge = 0.;
  double_t TimeWindowRightEdge = 0.;

  static std::vector<std::vector<TimeSettings>> GetTimeSettings(
      const std::string fileName)
  {
    std::vector<std::vector<TimeSettings>> timeSettingsVec;

    std::ifstream ifs(fileName);
    if (!ifs) {
      std::cerr << "File not found: " << fileName << std::endl;
      return timeSettingsVec;
    }

    nlohmann::json j;
    ifs >> j;

    for (const auto &mod : j) {
      std::vector<TimeSettings> timeSettings;
      for (const auto &ch : mod) {
        TimeSettings timeSetting;
        timeSetting.TimeOffset = ch["TimeOffset"];
        timeSetting.TimeWindowLeftEdge = ch["TimeWindowLeftEdge"];
        timeSetting.TimeWindowRightEdge = ch["TimeWindowRightEdge"];

        timeSettings.push_back(timeSetting);
      }
      timeSettingsVec.push_back(timeSettings);
    }

    return timeSettingsVec;
  };

  void Print()
  {
    std::cout << "Time Offset: " << TimeOffset << std::endl;
    std::cout << "Time Window Left Edge: " << TimeWindowLeftEdge << std::endl;
    std::cout << "Time Window Right Edge: " << TimeWindowRightEdge << std::endl;
  };
};
typedef TimeSettings TimeSettings_t;

}  // namespace DELILA

#endif