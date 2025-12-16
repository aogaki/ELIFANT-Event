#include <gtest/gtest.h>

#include "DELILAExceptions.hpp"
#include "L1EventBuilder.hpp"
#include "L2EventBuilder.hpp"
#include "TimeAlignment.hpp"

using namespace DELILA;

//=============================================================================
// L1EventBuilder Tests
//=============================================================================

class L1EventBuilderTest : public ::testing::Test {
 protected:
  void SetUp() override {}
  void TearDown() override {}
};

TEST_F(L1EventBuilderTest, Constructor) {
  EXPECT_NO_THROW({ L1EventBuilder builder; });
}

TEST_F(L1EventBuilderTest, Destructor) {
  EXPECT_NO_THROW({
    L1EventBuilder *builder = new L1EventBuilder();
    delete builder;
  });
}

TEST_F(L1EventBuilderTest, SetTimeWindow) {
  L1EventBuilder builder;
  EXPECT_NO_THROW(builder.SetTimeWindow(1000.0));
}

TEST_F(L1EventBuilderTest, SetCoincidenceWindow) {
  L1EventBuilder builder;
  EXPECT_NO_THROW(builder.SetCoincidenceWindow(50.0));
}

TEST_F(L1EventBuilderTest, SetRefMod) {
  L1EventBuilder builder;
  EXPECT_NO_THROW(builder.SetRefMod(0));
  EXPECT_NO_THROW(builder.SetRefMod(5));
  EXPECT_NO_THROW(builder.SetRefMod(255));
}

TEST_F(L1EventBuilderTest, SetRefCh) {
  L1EventBuilder builder;
  EXPECT_NO_THROW(builder.SetRefCh(0));
  EXPECT_NO_THROW(builder.SetRefCh(8));
  EXPECT_NO_THROW(builder.SetRefCh(255));
}

TEST_F(L1EventBuilderTest, LoadEmptyFileList) {
  L1EventBuilder builder;
  std::vector<std::string> emptyList;
  // Should throw ValidationException since empty file list is invalid
  EXPECT_THROW(builder.LoadFileList(emptyList), ValidationException);
}

TEST_F(L1EventBuilderTest, LoadFileListWithMultipleFiles) {
  L1EventBuilder builder;
  std::vector<std::string> fileList = {"file1.root", "file2.root",
                                        "file3.root"};
  EXPECT_NO_THROW(builder.LoadFileList(fileList));
}

TEST_F(L1EventBuilderTest, CancelOperation) {
  L1EventBuilder builder;
  EXPECT_NO_THROW(builder.Cancel());
}

TEST_F(L1EventBuilderTest, MultipleSetOperations) {
  L1EventBuilder builder;
  builder.SetTimeWindow(2000.0);
  builder.SetCoincidenceWindow(100.0);
  builder.SetRefMod(3);
  builder.SetRefCh(5);

  // Should not crash
  SUCCEED();
}

TEST_F(L1EventBuilderTest, SetZeroTimeWindow) {
  L1EventBuilder builder;
  EXPECT_NO_THROW(builder.SetTimeWindow(0.0));
}

TEST_F(L1EventBuilderTest, SetNegativeTimeWindow) {
  L1EventBuilder builder;
  EXPECT_NO_THROW(builder.SetTimeWindow(-100.0));
}

TEST_F(L1EventBuilderTest, LoadFileListMultipleTimes) {
  L1EventBuilder builder;
  std::vector<std::string> list1 = {"f1.root"};
  std::vector<std::string> list2 = {"f2.root", "f3.root"};

  builder.LoadFileList(list1);
  builder.LoadFileList(list2);  // Should replace previous list

  SUCCEED();
}

//=============================================================================
// L2EventBuilder Tests
//=============================================================================

class L2EventBuilderTest : public ::testing::Test {
 protected:
  void SetUp() override {}
  void TearDown() override {}
};

TEST_F(L2EventBuilderTest, Constructor) {
  EXPECT_NO_THROW({ L2EventBuilder builder; });
}

TEST_F(L2EventBuilderTest, Destructor) {
  EXPECT_NO_THROW({
    L2EventBuilder *builder = new L2EventBuilder();
    delete builder;
  });
}

TEST_F(L2EventBuilderTest, SetCoincidenceWindow) {
  L2EventBuilder builder;
  EXPECT_NO_THROW(builder.SetCoincidenceWindow(50.0));
  EXPECT_NO_THROW(builder.SetCoincidenceWindow(0.0));
  EXPECT_NO_THROW(builder.SetCoincidenceWindow(1000.0));
}

TEST_F(L2EventBuilderTest, CancelOperation) {
  L2EventBuilder builder;
  EXPECT_NO_THROW(builder.Cancel());
}

TEST_F(L2EventBuilderTest, MultipleCancel) {
  L2EventBuilder builder;
  builder.Cancel();
  builder.Cancel();  // Should be safe to call multiple times
  SUCCEED();
}

//=============================================================================
// TimeAlignment Tests
//=============================================================================

class TimeAlignmentTest : public ::testing::Test {
 protected:
  void SetUp() override {}
  void TearDown() override {}
};

TEST_F(TimeAlignmentTest, Constructor) {
  EXPECT_NO_THROW({ TimeAlignment alignment; });
}

TEST_F(TimeAlignmentTest, Destructor) {
  EXPECT_NO_THROW({
    TimeAlignment *alignment = new TimeAlignment();
    delete alignment;
  });
}

TEST_F(TimeAlignmentTest, SetTimeWindow) {
  TimeAlignment alignment;
  EXPECT_NO_THROW(alignment.SetTimeWindow(1000.0));
  EXPECT_NO_THROW(alignment.SetTimeWindow(0.0));
  EXPECT_NO_THROW(alignment.SetTimeWindow(-50.0));
}

TEST_F(TimeAlignmentTest, LoadEmptyFileList) {
  TimeAlignment alignment;
  std::vector<std::string> emptyList;
  // Should throw ValidationException since empty file list is invalid
  EXPECT_THROW(alignment.LoadFileList(emptyList), ValidationException);
}

TEST_F(TimeAlignmentTest, LoadFileListWithFiles) {
  TimeAlignment alignment;
  std::vector<std::string> fileList = {"a.root", "b.root"};
  EXPECT_NO_THROW(alignment.LoadFileList(fileList));
}

TEST_F(TimeAlignmentTest, CancelOperation) {
  TimeAlignment alignment;
  EXPECT_NO_THROW(alignment.Cancel());
}

TEST_F(TimeAlignmentTest, InitHistograms) {
  TimeAlignment alignment;
  // InitHistograms needs LoadChSettings first, so it might throw
  // Just test it doesn't crash the program
  try {
    alignment.InitHistograms();
  } catch (...) {
    // Expected to throw without proper setup
    SUCCEED();
  }
}

TEST_F(TimeAlignmentTest, Constants) {
  EXPECT_EQ(kTimeAlignmentFileName, "timeAlignment.root");
  EXPECT_EQ(kTimeSettingsFileName, "timeSettings.json");
}

TEST_F(TimeAlignmentTest, ConstantsAreNonEmpty) {
  EXPECT_FALSE(kTimeAlignmentFileName.empty());
  EXPECT_FALSE(kTimeSettingsFileName.empty());
}

TEST_F(TimeAlignmentTest, MultipleSetTimeWindow) {
  TimeAlignment alignment;
  alignment.SetTimeWindow(100.0);
  alignment.SetTimeWindow(200.0);
  alignment.SetTimeWindow(300.0);
  // Should not crash
  SUCCEED();
}

//=============================================================================
// Integration-like Tests (Basic)
//=============================================================================

TEST(BuilderIntegrationTest, CreateAllBuilders) {
  EXPECT_NO_THROW({
    L1EventBuilder l1;
    L2EventBuilder l2;
    TimeAlignment ta;
  });
}

TEST(BuilderIntegrationTest, BuilderLifecycle) {
  {
    L1EventBuilder builder;
    builder.SetTimeWindow(1000.0);
    builder.SetCoincidenceWindow(50.0);
    builder.SetRefMod(0);
    builder.SetRefCh(0);
  }  // Destructor called
  SUCCEED();
}

TEST(BuilderIntegrationTest, MultipleBuilderInstances) {
  L1EventBuilder b1, b2, b3;
  b1.SetTimeWindow(100.0);
  b2.SetTimeWindow(200.0);
  b3.SetTimeWindow(300.0);

  SUCCEED();
}

TEST(BuilderIntegrationTest, CancelAllBuilders) {
  L1EventBuilder l1;
  L2EventBuilder l2;
  TimeAlignment ta;

  l1.Cancel();
  l2.Cancel();
  ta.Cancel();

  SUCCEED();
}

//=============================================================================
// Thread Safety Smoke Tests
//=============================================================================

TEST(BuilderThreadSafetyTest, ConcurrentL1Builders) {
  std::vector<std::thread> threads;

  for (int i = 0; i < 10; i++) {
    threads.emplace_back([i]() {
      L1EventBuilder builder;
      builder.SetTimeWindow(i * 100.0);
      builder.SetCoincidenceWindow(i * 10.0);
    });
  }

  for (auto &t : threads) {
    t.join();
  }

  SUCCEED();
}

TEST(BuilderThreadSafetyTest, ConcurrentL2Builders) {
  std::vector<std::thread> threads;

  for (int i = 0; i < 10; i++) {
    threads.emplace_back([i]() {
      L2EventBuilder builder;
      builder.SetCoincidenceWindow(i * 10.0);
    });
  }

  for (auto &t : threads) {
    t.join();
  }

  SUCCEED();
}

TEST(BuilderThreadSafetyTest, ConcurrentTimeAlignment) {
  std::vector<std::thread> threads;

  for (int i = 0; i < 10; i++) {
    threads.emplace_back([i]() {
      TimeAlignment alignment;
      alignment.SetTimeWindow(i * 100.0);
    });
  }

  for (auto &t : threads) {
    t.join();
  }

  SUCCEED();
}

//=============================================================================
// Stress Tests
//=============================================================================

TEST(BuilderStressTest, ManyL1Builders) {
  const int N = 100;
  std::vector<std::unique_ptr<L1EventBuilder>> builders;

  for (int i = 0; i < N; i++) {
    builders.push_back(std::make_unique<L1EventBuilder>());
    builders.back()->SetTimeWindow(i * 10.0);
  }

  EXPECT_EQ(builders.size(), N);
}

TEST(BuilderStressTest, RapidCreateDestroy) {
  for (int i = 0; i < 1000; i++) {
    L1EventBuilder *b = new L1EventBuilder();
    delete b;
  }
  SUCCEED();
}

TEST(BuilderStressTest, LargeFileList) {
  L1EventBuilder builder;
  std::vector<std::string> largeList;

  for (int i = 0; i < 10000; i++) {
    largeList.push_back("file_" + std::to_string(i) + ".root");
  }

  EXPECT_NO_THROW(builder.LoadFileList(largeList));
}

//=============================================================================
// Edge Case Tests
//=============================================================================

TEST(BuilderEdgeCaseTest, VeryLargeTimeWindow) {
  L1EventBuilder builder;
  EXPECT_NO_THROW(builder.SetTimeWindow(1e15));
}

TEST(BuilderEdgeCaseTest, VerySmallTimeWindow) {
  L1EventBuilder builder;
  EXPECT_NO_THROW(builder.SetTimeWindow(1e-15));
}

TEST(BuilderEdgeCaseTest, MaxUint8RefMod) {
  L1EventBuilder builder;
  EXPECT_NO_THROW(builder.SetRefMod(255));
}

TEST(BuilderEdgeCaseTest, MaxUint8RefCh) {
  L1EventBuilder builder;
  EXPECT_NO_THROW(builder.SetRefCh(255));
}
