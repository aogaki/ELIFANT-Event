#include <gtest/gtest.h>

#include "EventData.hpp"
#include "L1EventBuilder.hpp"
#include "L2EventBuilder.hpp"
#include "TimeAlignment.hpp"

using namespace DELILA;

//=============================================================================
// Pipeline Integration Tests
//=============================================================================

class PipelineTest : public ::testing::Test {
 protected:
  void SetUp() override {}
  void TearDown() override {}
};

TEST_F(PipelineTest, CreateFullPipeline) {
  EXPECT_NO_THROW({
    TimeAlignment timeAlign;
    L1EventBuilder l1Builder;
    L2EventBuilder l2Builder;
  });
}

TEST_F(PipelineTest, PipelineSequence) {
  // Test that the three-stage pipeline can be instantiated
  TimeAlignment timeAlign;
  timeAlign.SetTimeWindow(1000.0);

  L1EventBuilder l1Builder;
  l1Builder.SetTimeWindow(1000.0);
  l1Builder.SetCoincidenceWindow(50.0);
  l1Builder.SetRefMod(0);
  l1Builder.SetRefCh(0);

  L2EventBuilder l2Builder;
  l2Builder.SetCoincidenceWindow(50.0);

  SUCCEED();
}

TEST_F(PipelineTest, CancelAllStages) {
  TimeAlignment timeAlign;
  L1EventBuilder l1Builder;
  L2EventBuilder l2Builder;

  timeAlign.Cancel();
  l1Builder.Cancel();
  l2Builder.Cancel();

  SUCCEED();
}

//=============================================================================
// EventData Flow Tests
//=============================================================================

TEST(EventDataFlowTest, CreateAndManipulateEvent) {
  EventData event;
  event.triggerTime = 123.456;

  for (int i = 0; i < 10; i++) {
    RawData_t data(true, i % 4, i % 16, i * 100, i * 50, i * 1.5);
    event.eventDataVec->push_back(data);
  }

  EXPECT_EQ(event.eventDataVec->size(), 10);
  EXPECT_DOUBLE_EQ(event.triggerTime, 123.456);
}

TEST(EventDataFlowTest, MultipleEventsProcessing) {
  std::vector<EventData> events;

  for (int i = 0; i < 100; i++) {
    EventData event;
    event.triggerTime = i * 10.0;

    for (int j = 0; j < 5; j++) {
      RawData_t data(true, 0, j, j * 10, j * 5, j * 0.1);
      event.eventDataVec->push_back(data);
    }

    events.push_back(std::move(event));
  }

  EXPECT_EQ(events.size(), 100);
  for (const auto &evt : events) {
    EXPECT_EQ(evt.eventDataVec->size(), 5);
  }
}

//=============================================================================
// Multi-threaded Pipeline Tests
//=============================================================================

TEST(MultiThreadPipelineTest, ConcurrentBuilderOperations) {
  std::vector<std::thread> threads;

  threads.emplace_back([]() {
    L1EventBuilder builder;
    builder.SetTimeWindow(1000.0);
  });

  threads.emplace_back([]() {
    L2EventBuilder builder;
    builder.SetCoincidenceWindow(50.0);
  });

  threads.emplace_back([]() {
    TimeAlignment alignment;
    alignment.SetTimeWindow(1000.0);
  });

  for (auto &t : threads) {
    t.join();
  }

  SUCCEED();
}

TEST(MultiThreadPipelineTest, ParallelEventCreation) {
  std::vector<std::thread> threads;
  std::vector<EventData> events(10);

  for (int i = 0; i < 10; i++) {
    threads.emplace_back([&events, i]() {
      events[i].triggerTime = i * 100.0;
      for (int j = 0; j < 50; j++) {
        RawData_t data(true, i, j % 16, j * 10, j * 5, j * 0.1);
        events[i].eventDataVec->push_back(data);
      }
    });
  }

  for (auto &t : threads) {
    t.join();
  }

  for (int i = 0; i < 10; i++) {
    EXPECT_EQ(events[i].eventDataVec->size(), 50);
    EXPECT_DOUBLE_EQ(events[i].triggerTime, i * 100.0);
  }
}

//=============================================================================
// Resource Management Integration Tests
//=============================================================================

TEST(ResourceManagementTest, RAIIWithBuilders) {
  // Test RAII pattern throughout the pipeline
  {
    L1EventBuilder l1;
    L2EventBuilder l2;
    TimeAlignment ta;

    std::vector<EventData> events;
    for (int i = 0; i < 100; i++) {
      events.emplace_back();
      events.back().triggerTime = i;
    }
  }  // Everything should be cleaned up here

  SUCCEED();
}

TEST(ResourceManagementTest, ExceptionSafetyInPipeline) {
  try {
    L1EventBuilder builder;
    builder.SetTimeWindow(1000.0);
    throw std::runtime_error("Simulated error");
  } catch (const std::runtime_error &e) {
    // Builder should be properly destructed
    SUCCEED();
  }
}

//=============================================================================
// Configuration Propagation Tests
//=============================================================================

TEST(ConfigPropagationTest, ParameterConsistency) {
  const double timeWindow = 1500.0;
  const double coincWindow = 75.0;

  L1EventBuilder l1;
  l1.SetTimeWindow(timeWindow);
  l1.SetCoincidenceWindow(coincWindow);

  L2EventBuilder l2;
  l2.SetCoincidenceWindow(coincWindow);

  TimeAlignment ta;
  ta.SetTimeWindow(timeWindow);

  // Parameters set consistently across pipeline
  SUCCEED();
}

//=============================================================================
// Data Integrity Tests
//=============================================================================

TEST(DataIntegrityTest, EventDataPreserved) {
  EventData original;
  original.triggerTime = 999.888;

  for (int i = 0; i < 20; i++) {
    original.eventDataVec->push_back(RawData_t(true, 3, i, i * 50, i * 25, i));
  }

  EventData copy = original;

  EXPECT_DOUBLE_EQ(copy.triggerTime, original.triggerTime);
  EXPECT_EQ(copy.eventDataVec->size(), original.eventDataVec->size());

  for (size_t i = 0; i < original.eventDataVec->size(); i++) {
    EXPECT_EQ((*copy.eventDataVec)[i].mod, (*original.eventDataVec)[i].mod);
    EXPECT_EQ((*copy.eventDataVec)[i].ch, (*original.eventDataVec)[i].ch);
    EXPECT_EQ((*copy.eventDataVec)[i].chargeLong,
              (*original.eventDataVec)[i].chargeLong);
  }
}

TEST(DataIntegrityTest, LargeDatasetIntegrity) {
  const int N = 10000;
  EventData event;

  for (int i = 0; i < N; i++) {
    event.eventDataVec->push_back(RawData_t(i % 2 == 0, i % 256, i % 256, i,
                                             i / 2, i * 0.01));
  }

  EXPECT_EQ(event.eventDataVec->size(), N);

  // Spot check some entries
  EXPECT_EQ((*event.eventDataVec)[0].chargeLong, 0);
  EXPECT_EQ((*event.eventDataVec)[100].chargeLong, 100);
  EXPECT_EQ((*event.eventDataVec)[N - 1].chargeLong, N - 1);
}

//=============================================================================
// Performance Sanity Tests
//=============================================================================

TEST(PerformanceSanityTest, BuilderCreationSpeed) {
  auto start = std::chrono::high_resolution_clock::now();

  for (int i = 0; i < 1000; i++) {
    L1EventBuilder builder;
  }

  auto end = std::chrono::high_resolution_clock::now();
  auto duration =
      std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

  // Should complete in reasonable time (< 1 second for 1000 constructions)
  EXPECT_LT(duration.count(), 1000);
}

TEST(PerformanceSanityTest, EventDataCreationSpeed) {
  auto start = std::chrono::high_resolution_clock::now();

  for (int i = 0; i < 10000; i++) {
    EventData event;
    for (int j = 0; j < 10; j++) {
      event.eventDataVec->push_back(RawData_t(true, 0, j, 100, 50, 1.0));
    }
  }

  auto end = std::chrono::high_resolution_clock::now();
  auto duration =
      std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

  // Should be fast (< 2 seconds for 10k events with 10 data points each)
  EXPECT_LT(duration.count(), 2000);
}

//=============================================================================
// Smoke Tests for Full Workflow
//=============================================================================

TEST(WorkflowSmokeTest, CompleteWorkflowSetup) {
  // Simulate setting up a complete analysis workflow
  TimeAlignment timeAlign;
  timeAlign.SetTimeWindow(2000.0);
  std::vector<std::string> files = {"dummy1.root", "dummy2.root"};
  timeAlign.LoadFileList(files);

  L1EventBuilder l1Builder;
  l1Builder.SetTimeWindow(2000.0);
  l1Builder.SetCoincidenceWindow(100.0);
  l1Builder.SetRefMod(0);
  l1Builder.SetRefCh(0);
  l1Builder.LoadFileList(files);

  L2EventBuilder l2Builder;
  l2Builder.SetCoincidenceWindow(100.0);

  SUCCEED();
}
