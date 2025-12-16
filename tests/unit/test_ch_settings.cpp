#include <gtest/gtest.h>

#include "ChSettings.hpp"

#include <fstream>

using namespace DELILA;

class ChSettingsTest : public ::testing::Test {
 protected:
  void SetUp() override {}
  void TearDown() override {
    // Clean up test files
    std::remove("test_settings.json");
  }
};

TEST_F(ChSettingsTest, DefaultConstructor) {
  ChSettings settings;

  EXPECT_EQ(settings.isEventTrigger, false);
  EXPECT_EQ(settings.ID, 0);
  EXPECT_EQ(settings.mod, 0);
  EXPECT_EQ(settings.ch, 0);
  EXPECT_EQ(settings.thresholdADC, 0);
  EXPECT_EQ(settings.hasAC, false);
  EXPECT_EQ(settings.ACMod, 0);
  EXPECT_EQ(settings.ACCh, 0);
  EXPECT_DOUBLE_EQ(settings.phi, 0.0);
  EXPECT_DOUBLE_EQ(settings.theta, 0.0);
  EXPECT_DOUBLE_EQ(settings.distance, 0.0);
  EXPECT_DOUBLE_EQ(settings.x, 0.0);
  EXPECT_DOUBLE_EQ(settings.y, 0.0);
  EXPECT_DOUBLE_EQ(settings.z, 0.0);
  EXPECT_DOUBLE_EQ(settings.p0, 0.0);
  EXPECT_DOUBLE_EQ(settings.p1, 1.0);
  EXPECT_DOUBLE_EQ(settings.p2, 0.0);
  EXPECT_DOUBLE_EQ(settings.p3, 0.0);
  EXPECT_EQ(settings.detectorType, "");
  EXPECT_EQ(settings.tags.size(), 0);
}

TEST_F(ChSettingsTest, SetAllFields) {
  ChSettings settings;

  settings.isEventTrigger = true;
  settings.ID = 42;
  settings.mod = 5;
  settings.ch = 12;
  settings.thresholdADC = 1000;
  settings.hasAC = true;
  settings.ACMod = 3;
  settings.ACCh = 8;
  settings.phi = 45.0;
  settings.theta = 30.0;
  settings.distance = 100.5;
  settings.x = 10.0;
  settings.y = 20.0;
  settings.z = 30.0;
  settings.p0 = 1.0;
  settings.p1 = 2.0;
  settings.p2 = 3.0;
  settings.p3 = 4.0;
  settings.detectorType = "HPGe";
  settings.tags = {"tag1", "tag2", "tag3"};

  EXPECT_EQ(settings.isEventTrigger, true);
  EXPECT_EQ(settings.ID, 42);
  EXPECT_EQ(settings.mod, 5);
  EXPECT_EQ(settings.ch, 12);
  EXPECT_DOUBLE_EQ(settings.phi, 45.0);
  EXPECT_EQ(settings.tags.size(), 3);
}

TEST_F(ChSettingsTest, GetDetectorTypeAC) {
  EXPECT_EQ(ChSettings::GetDetectorType("ac"), DetectorType::AC);
  EXPECT_EQ(ChSettings::GetDetectorType("AC"), DetectorType::AC);
  EXPECT_EQ(ChSettings::GetDetectorType("Ac"), DetectorType::AC);
  EXPECT_EQ(ChSettings::GetDetectorType("aC"), DetectorType::AC);
}

TEST_F(ChSettingsTest, GetDetectorTypePMT) {
  EXPECT_EQ(ChSettings::GetDetectorType("pmt"), DetectorType::PMT);
  EXPECT_EQ(ChSettings::GetDetectorType("PMT"), DetectorType::PMT);
  EXPECT_EQ(ChSettings::GetDetectorType("Pmt"), DetectorType::PMT);
}

TEST_F(ChSettingsTest, GetDetectorTypeHPGe) {
  EXPECT_EQ(ChSettings::GetDetectorType("hpge"), DetectorType::HPGe);
  EXPECT_EQ(ChSettings::GetDetectorType("HPGE"), DetectorType::HPGe);
  EXPECT_EQ(ChSettings::GetDetectorType("HpGe"), DetectorType::HPGe);
}

TEST_F(ChSettingsTest, GetDetectorTypeSi) {
  EXPECT_EQ(ChSettings::GetDetectorType("si"), DetectorType::Si);
  EXPECT_EQ(ChSettings::GetDetectorType("SI"), DetectorType::Si);
  EXPECT_EQ(ChSettings::GetDetectorType("Si"), DetectorType::Si);
}

TEST_F(ChSettingsTest, GetDetectorTypeUnknown) {
  EXPECT_EQ(ChSettings::GetDetectorType(""), DetectorType::Unknown);
  EXPECT_EQ(ChSettings::GetDetectorType("invalid"), DetectorType::Unknown);
  EXPECT_EQ(ChSettings::GetDetectorType("XYZ"), DetectorType::Unknown);
  EXPECT_EQ(ChSettings::GetDetectorType("123"), DetectorType::Unknown);
}

TEST_F(ChSettingsTest, DetectorTypeEnumValues) {
  EXPECT_EQ(static_cast<int>(DetectorType::Unknown), 0);
  EXPECT_EQ(static_cast<int>(DetectorType::AC), 1);
  EXPECT_EQ(static_cast<int>(DetectorType::PMT), 2);
  EXPECT_EQ(static_cast<int>(DetectorType::HPGe), 3);
  EXPECT_EQ(static_cast<int>(DetectorType::Si), 4);
}

TEST_F(ChSettingsTest, PrintMethod) {
  ChSettings settings;
  settings.mod = 1;
  settings.ch = 5;
  settings.ID = 10;

  // Print should not crash
  testing::internal::CaptureStdout();
  settings.Print();
  std::string output = testing::internal::GetCapturedStdout();

  EXPECT_FALSE(output.empty());
}

TEST_F(ChSettingsTest, TagsManipulation) {
  ChSettings settings;

  settings.tags.push_back("detector1");
  settings.tags.push_back("front");
  settings.tags.push_back("calibrated");

  EXPECT_EQ(settings.tags.size(), 3);
  EXPECT_EQ(settings.tags[0], "detector1");
  EXPECT_EQ(settings.tags[1], "front");
  EXPECT_EQ(settings.tags[2], "calibrated");

  settings.tags.clear();
  EXPECT_EQ(settings.tags.size(), 0);
}

TEST_F(ChSettingsTest, CalibrationParametersLinear) {
  ChSettings settings;
  settings.p0 = 0.0;
  settings.p1 = 2.0;
  settings.p2 = 0.0;
  settings.p3 = 0.0;

  // Linear calibration: E = p0 + p1*ADC
  double adc = 100.0;
  double energy = settings.p0 + settings.p1 * adc;
  EXPECT_DOUBLE_EQ(energy, 200.0);
}

TEST_F(ChSettingsTest, CalibrationParametersQuadratic) {
  ChSettings settings;
  settings.p0 = 1.0;
  settings.p1 = 2.0;
  settings.p2 = 0.5;
  settings.p3 = 0.0;

  // Quadratic calibration: E = p0 + p1*ADC + p2*ADC^2
  double adc = 10.0;
  double energy = settings.p0 + settings.p1 * adc + settings.p2 * adc * adc;
  EXPECT_DOUBLE_EQ(energy, 71.0);  // 1 + 20 + 50
}

TEST_F(ChSettingsTest, MaxModuleChannel) {
  ChSettings settings;
  settings.mod = UINT32_MAX;
  settings.ch = UINT32_MAX;

  EXPECT_EQ(settings.mod, UINT32_MAX);
  EXPECT_EQ(settings.ch, UINT32_MAX);
}

TEST_F(ChSettingsTest, GeometrySpherical) {
  ChSettings settings;
  settings.phi = 45.0;
  settings.theta = 60.0;
  settings.distance = 10.0;

  // Test spherical coordinates are stored correctly
  EXPECT_DOUBLE_EQ(settings.phi, 45.0);
  EXPECT_DOUBLE_EQ(settings.theta, 60.0);
  EXPECT_DOUBLE_EQ(settings.distance, 10.0);
}

TEST_F(ChSettingsTest, GeometryCartesian) {
  ChSettings settings;
  settings.x = 5.5;
  settings.y = -3.2;
  settings.z = 12.8;

  EXPECT_DOUBLE_EQ(settings.x, 5.5);
  EXPECT_DOUBLE_EQ(settings.y, -3.2);
  EXPECT_DOUBLE_EQ(settings.z, 12.8);
}

TEST_F(ChSettingsTest, ACAssociation) {
  ChSettings settings;
  settings.hasAC = true;
  settings.ACMod = 2;
  settings.ACCh = 15;

  EXPECT_TRUE(settings.hasAC);
  EXPECT_EQ(settings.ACMod, 2);
  EXPECT_EQ(settings.ACCh, 15);
}

TEST_F(ChSettingsTest, ThresholdADCBoundary) {
  ChSettings settings;

  settings.thresholdADC = 0;
  EXPECT_EQ(settings.thresholdADC, 0);

  settings.thresholdADC = UINT32_MAX;
  EXPECT_EQ(settings.thresholdADC, UINT32_MAX);

  settings.thresholdADC = 4095;  // 12-bit ADC max
  EXPECT_EQ(settings.thresholdADC, 4095);
}

TEST_F(ChSettingsTest, CopySemantics) {
  ChSettings original;
  original.ID = 42;
  original.mod = 5;
  original.ch = 10;
  original.detectorType = "HPGe";
  original.tags = {"tag1", "tag2"};

  ChSettings copy = original;

  EXPECT_EQ(copy.ID, original.ID);
  EXPECT_EQ(copy.mod, original.mod);
  EXPECT_EQ(copy.ch, original.ch);
  EXPECT_EQ(copy.detectorType, original.detectorType);
  EXPECT_EQ(copy.tags.size(), original.tags.size());
}

TEST_F(ChSettingsTest, NegativeIDValues) {
  ChSettings settings;
  settings.ID = -1;
  EXPECT_EQ(settings.ID, -1);

  settings.ID = INT32_MIN;
  EXPECT_EQ(settings.ID, INT32_MIN);
}

TEST_F(ChSettingsTest, DetectorTypeStoredAsString) {
  ChSettings settings;
  settings.detectorType = "custom_detector_type";
  EXPECT_EQ(settings.detectorType, "custom_detector_type");

  // GetDetectorType doesn't modify the stored string
  auto type = ChSettings::GetDetectorType(settings.detectorType);
  EXPECT_EQ(type, DetectorType::Unknown);
  EXPECT_EQ(settings.detectorType, "custom_detector_type");
}
