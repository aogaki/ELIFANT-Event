#include <gtest/gtest.h>

#include "EventData.hpp"

#include <thread>
#include <vector>

using namespace DELILA;

//=============================================================================
// RawData_t Tests
//=============================================================================

class RawDataTest : public ::testing::Test {
 protected:
  void SetUp() override {}
  void TearDown() override {}
};

TEST_F(RawDataTest, DefaultConstructor) {
  RawData_t data;
  // Default values are undefined, just test it compiles and doesn't crash
  SUCCEED();
}

TEST_F(RawDataTest, ParameterizedConstructor) {
  RawData_t data(true, 5, 10, 1000, 500, 123.456);

  EXPECT_EQ(data.isWithAC, true);
  EXPECT_EQ(data.mod, 5);
  EXPECT_EQ(data.ch, 10);
  EXPECT_EQ(data.chargeLong, 1000);
  EXPECT_EQ(data.chargeShort, 500);
  EXPECT_DOUBLE_EQ(data.fineTS, 123.456);
}

TEST_F(RawDataTest, CopyConstructor) {
  RawData_t original(true, 5, 10, 1000, 500, 123.456);
  RawData_t copy(original);

  EXPECT_EQ(copy.isWithAC, original.isWithAC);
  EXPECT_EQ(copy.mod, original.mod);
  EXPECT_EQ(copy.ch, original.ch);
  EXPECT_EQ(copy.chargeLong, original.chargeLong);
  EXPECT_EQ(copy.chargeShort, original.chargeShort);
  EXPECT_DOUBLE_EQ(copy.fineTS, original.fineTS);
}

TEST_F(RawDataTest, AssignmentOperator) {
  RawData_t original(true, 5, 10, 1000, 500, 123.456);
  RawData_t copy;
  copy = original;

  EXPECT_EQ(copy.isWithAC, original.isWithAC);
  EXPECT_EQ(copy.mod, original.mod);
  EXPECT_EQ(copy.ch, original.ch);
  EXPECT_EQ(copy.chargeLong, original.chargeLong);
  EXPECT_EQ(copy.chargeShort, original.chargeShort);
  EXPECT_DOUBLE_EQ(copy.fineTS, original.fineTS);
}

TEST_F(RawDataTest, MaxValues) {
  RawData_t data(true, 255, 255, 65535, 65535, 999999.999);

  EXPECT_EQ(data.mod, 255);
  EXPECT_EQ(data.ch, 255);
  EXPECT_EQ(data.chargeLong, 65535);
  EXPECT_EQ(data.chargeShort, 65535);
  EXPECT_DOUBLE_EQ(data.fineTS, 999999.999);
}

TEST_F(RawDataTest, MinValues) {
  RawData_t data(false, 0, 0, 0, 0, 0.0);

  EXPECT_EQ(data.isWithAC, false);
  EXPECT_EQ(data.mod, 0);
  EXPECT_EQ(data.ch, 0);
  EXPECT_EQ(data.chargeLong, 0);
  EXPECT_EQ(data.chargeShort, 0);
  EXPECT_DOUBLE_EQ(data.fineTS, 0.0);
}

TEST_F(RawDataTest, NegativeTimestamp) {
  RawData_t data(false, 0, 0, 100, 50, -123.456);
  EXPECT_DOUBLE_EQ(data.fineTS, -123.456);
}

//=============================================================================
// EventData Tests
//=============================================================================

class EventDataTest : public ::testing::Test {
 protected:
  void SetUp() override {}
  void TearDown() override {}
};

TEST_F(EventDataTest, DefaultConstructor) {
  EventData event;
  EXPECT_NE(event.eventDataVec, nullptr);
  EXPECT_EQ(event.eventDataVec->size(), 0);
  EXPECT_DOUBLE_EQ(event.triggerTime, 0.0);
}

TEST_F(EventDataTest, AddSingleData) {
  EventData event;
  RawData_t data(true, 1, 2, 100, 50, 123.456);
  event.eventDataVec->push_back(data);

  EXPECT_EQ(event.eventDataVec->size(), 1);
  EXPECT_EQ((*event.eventDataVec)[0].mod, 1);
  EXPECT_EQ((*event.eventDataVec)[0].ch, 2);
}

TEST_F(EventDataTest, AddMultipleData) {
  EventData event;
  for (int i = 0; i < 100; i++) {
    RawData_t data(true, i % 10, i % 16, i * 100, i * 50, i * 1.5);
    event.eventDataVec->push_back(data);
  }

  EXPECT_EQ(event.eventDataVec->size(), 100);
}

TEST_F(EventDataTest, SetTriggerTime) {
  EventData event;
  event.triggerTime = 987.654;
  EXPECT_DOUBLE_EQ(event.triggerTime, 987.654);
}

TEST_F(EventDataTest, ClearMethod) {
  EventData event;
  event.triggerTime = 123.456;
  for (int i = 0; i < 10; i++) {
    event.eventDataVec->push_back(RawData_t(true, 1, 2, 100, 50, i * 1.0));
  }

  event.Clear();

  EXPECT_DOUBLE_EQ(event.triggerTime, 0.0);
  EXPECT_EQ(event.eventDataVec->size(), 0);
}

TEST_F(EventDataTest, CopyConstructor) {
  EventData original;
  original.triggerTime = 123.456;
  original.eventDataVec->push_back(RawData_t(true, 1, 2, 100, 50, 10.0));

  EventData copy(original);

  EXPECT_DOUBLE_EQ(copy.triggerTime, original.triggerTime);
  EXPECT_EQ(copy.eventDataVec->size(), original.eventDataVec->size());
}

TEST_F(EventDataTest, CopyAssignmentOperator) {
  EventData original;
  original.triggerTime = 123.456;
  original.eventDataVec->push_back(RawData_t(true, 1, 2, 100, 50, 10.0));

  EventData copy;
  copy = original;

  EXPECT_DOUBLE_EQ(copy.triggerTime, original.triggerTime);
  EXPECT_EQ(copy.eventDataVec->size(), original.eventDataVec->size());
}

TEST_F(EventDataTest, MoveConstructor) {
  EventData original;
  original.triggerTime = 123.456;
  original.eventDataVec->push_back(RawData_t(true, 1, 2, 100, 50, 10.0));

  EventData moved(std::move(original));

  EXPECT_DOUBLE_EQ(moved.triggerTime, 123.456);
  EXPECT_EQ(moved.eventDataVec->size(), 1);
}

TEST_F(EventDataTest, MoveAssignmentOperator) {
  EventData original;
  original.triggerTime = 123.456;
  original.eventDataVec->push_back(RawData_t(true, 1, 2, 100, 50, 10.0));

  EventData moved;
  moved = std::move(original);

  EXPECT_DOUBLE_EQ(moved.triggerTime, 123.456);
  EXPECT_EQ(moved.eventDataVec->size(), 1);
}

TEST_F(EventDataTest, NoMemoryLeakOnDestruction) {
  // Test that destructor properly cleans up
  {
    EventData event;
    for (int i = 0; i < 1000; i++) {
      event.eventDataVec->push_back(RawData_t(true, 1, 2, 100, 50, i * 1.0));
    }
  }  // EventData goes out of scope here
  // If there's a leak, valgrind/sanitizer would catch it
  SUCCEED();
}

TEST_F(EventDataTest, LargeDataSet) {
  EventData event;
  const int N = 10000;
  for (int i = 0; i < N; i++) {
    event.eventDataVec->push_back(RawData_t(i % 2 == 0, i % 256, i % 256,
                                             i, i / 2, i * 0.1));
  }

  EXPECT_EQ(event.eventDataVec->size(), N);
  EXPECT_EQ((*event.eventDataVec)[0].chargeLong, 0);
  EXPECT_EQ((*event.eventDataVec)[N - 1].chargeLong, N - 1);
}

TEST_F(EventDataTest, ConcurrentAccess) {
  // Test thread safety for separate instances
  std::vector<std::thread> threads;
  std::vector<EventData> events(10);

  for (int t = 0; t < 10; t++) {
    threads.emplace_back([&events, t]() {
      for (int i = 0; i < 100; i++) {
        events[t].eventDataVec->push_back(
            RawData_t(true, t, i, i * 10, i * 5, i * 0.1));
      }
    });
  }

  for (auto &thread : threads) {
    thread.join();
  }

  for (int t = 0; t < 10; t++) {
    EXPECT_EQ(events[t].eventDataVec->size(), 100);
  }
}

TEST_F(EventDataTest, RepeatedClearOperations) {
  EventData event;

  for (int iteration = 0; iteration < 100; iteration++) {
    event.triggerTime = iteration * 1.5;
    for (int i = 0; i < 50; i++) {
      event.eventDataVec->push_back(RawData_t(true, 1, 2, i, i, i * 0.1));
    }

    EXPECT_EQ(event.eventDataVec->size(), 50);
    event.Clear();
    EXPECT_EQ(event.eventDataVec->size(), 0);
    EXPECT_DOUBLE_EQ(event.triggerTime, 0.0);
  }
}

TEST_F(EventDataTest, VectorReserve) {
  EventData event;
  event.eventDataVec->reserve(1000);

  EXPECT_GE(event.eventDataVec->capacity(), 1000);
  EXPECT_EQ(event.eventDataVec->size(), 0);
}

TEST_F(EventDataTest, EmplaceBack) {
  EventData event;
  event.eventDataVec->emplace_back(true, 5, 10, 1000, 500, 123.456);

  EXPECT_EQ(event.eventDataVec->size(), 1);
  EXPECT_EQ((*event.eventDataVec)[0].mod, 5);
  EXPECT_EQ((*event.eventDataVec)[0].ch, 10);
}

TEST_F(EventDataTest, EdgeCaseTriggerTimes) {
  EventData event;

  // Very large positive
  event.triggerTime = 1e15;
  EXPECT_DOUBLE_EQ(event.triggerTime, 1e15);

  // Very small positive
  event.triggerTime = 1e-15;
  EXPECT_DOUBLE_EQ(event.triggerTime, 1e-15);

  // Negative
  event.triggerTime = -123.456;
  EXPECT_DOUBLE_EQ(event.triggerTime, -123.456);

  // Zero
  event.triggerTime = 0.0;
  EXPECT_DOUBLE_EQ(event.triggerTime, 0.0);
}
