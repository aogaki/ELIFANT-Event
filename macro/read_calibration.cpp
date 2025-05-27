#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <string>

#include "ChSettings.hpp"

void read_calibration()
{
  auto jsonFile = "chSettings.json";
  auto chSettingsVec = DELILA::ChSettings::GetChSettings(jsonFile);

  auto calibrationFile = "ELIFANT2025.dat";
  std::ifstream ifs(calibrationFile);
  if (!ifs) {
    std::cerr << "File not found: " << calibrationFile << std::endl;
    return;
  }

  int mod = 0;
  int ch = 0;
  double p0 = 0.;
  double p1 = 0.;
  while (true) {
    ifs >> mod >> ch >> p0 >> p1;
    if (ifs.eof()) {
      break;  // End of file reached
    }
    if (ifs.fail()) {
      std::cerr << "Error reading from file: " << calibrationFile << std::endl;
      break;
    }

    if (mod < 0 || mod >= chSettingsVec.size() || ch < 0 ||
        ch >= chSettingsVec[mod].size()) {
      std::cerr << "Invalid module or channel index: " << mod << ", " << ch
                << std::endl;
      continue;
    }

    auto &chSetting = chSettingsVec[mod][ch];
    chSetting.p0 = p0;
    chSetting.p1 = p1;

    std::cout << "Module: " << mod << "\tChannel: " << ch << "\tp0: " << p0
              << "\tp1: " << p1 << std::endl;
  }

  // Save the updated settings back to the JSON file
  std::ofstream ofs("tmp.json");
  if (!ofs) {
    std::cerr << "Error opening file for writing: " << jsonFile << std::endl;
    return;
  }
  nlohmann::json j;
  for (const auto &mod : chSettingsVec) {
    nlohmann::json modJson = nlohmann::json::array();
    for (const auto &ch : mod) {
      nlohmann::json chJson;
      chJson["IsEventTrigger"] = ch.isEventTrigger;
      chJson["ID"] = ch.ID;
      chJson["Module"] = ch.mod;
      chJson["Channel"] = ch.ch;
      chJson["HasAC"] = ch.hasAC;
      chJson["ACModule"] = ch.ACMod;
      chJson["ACChannel"] = ch.ACCh;
      chJson["Phi"] = ch.phi;
      chJson["Theta"] = ch.theta;
      chJson["Distance"] = ch.distance;
      chJson["ThresholdADC"] = ch.thresholdADC;
      chJson["x"] = ch.x;
      chJson["y"] = ch.y;
      chJson["z"] = ch.z;
      chJson["p0"] = ch.p0;
      chJson["p1"] = ch.p1;
      chJson["p2"] = ch.p2;
      chJson["p3"] = ch.p3;
      chJson["DetectorType"] = ch.detectorType;
      chJson["Tags"] = nlohmann::json(ch.tags);
      modJson.push_back(chJson);
    }
    j.push_back(modJson);
  }
  ofs << j.dump(4) << std::endl;
  ofs.close();
  std::cout << "Calibration data read and updated successfully." << std::endl;
}