#include <gtest/gtest.h>

#include "L2Conditions.hpp"

#include <thread>
#include <vector>

using namespace DELILA;

//=============================================================================
// L2Counter Tests
//=============================================================================

class L2CounterTest : public ::testing::Test {
 protected:
  void SetUp() override {}
  void TearDown() override {}
};

TEST_F(L2CounterTest, DefaultConstructor) {
  L2Counter counter;

  EXPECT_EQ(counter.name, "");
  EXPECT_EQ(counter.counter, 0);
}

TEST_F(L2CounterTest, ParameterizedConstructorNameOnly) {
  L2Counter counter("TestCounter");

  EXPECT_EQ(counter.name, "TestCounter");
  EXPECT_EQ(counter.counter, 0);
}

TEST_F(L2CounterTest, ParameterizedConstructorWithValue) {
  L2Counter counter("TestCounter", 42);

  EXPECT_EQ(counter.name, "TestCounter");
  EXPECT_EQ(counter.counter, 42);
}

TEST_F(L2CounterTest, SetConditionTableEmpty) {
  L2Counter counter("test");
  std::vector<std::vector<bool>> table;

  EXPECT_NO_THROW(counter.SetConditionTable(table));
}

TEST_F(L2CounterTest, SetConditionTableSimple) {
  L2Counter counter("test");
  std::vector<std::vector<bool>> table = {{true, false}, {false, true}};

  counter.SetConditionTable(table);
  // Table is set, counter should still be 0
  EXPECT_EQ(counter.counter, 0);
}

TEST_F(L2CounterTest, CheckWithMatchingCondition) {
  L2Counter counter("test");
  std::vector<std::vector<bool>> table = {{true, false}, {false, true}};

  counter.SetConditionTable(table);
  counter.Check(0, 0); // Should increment (table[0][0] = true)

  EXPECT_EQ(counter.counter, 1);
}

TEST_F(L2CounterTest, CheckWithNonMatchingCondition) {
  L2Counter counter("test");
  std::vector<std::vector<bool>> table = {{true, false}, {false, true}};

  counter.SetConditionTable(table);
  counter.Check(0, 1); // Should NOT increment (table[0][1] = false)

  EXPECT_EQ(counter.counter, 0);
}

TEST_F(L2CounterTest, CheckMultipleTimes) {
  L2Counter counter("test");
  std::vector<std::vector<bool>> table = {{true, true, true}, {true, false, true}};

  counter.SetConditionTable(table);

  counter.Check(0, 0); // Match
  counter.Check(0, 1); // Match
  counter.Check(0, 2); // Match
  counter.Check(1, 0); // Match
  counter.Check(1, 1); // No match
  counter.Check(1, 2); // Match

  EXPECT_EQ(counter.counter, 5);
}

TEST_F(L2CounterTest, CheckOutOfBoundsModule) {
  L2Counter counter("test");
  std::vector<std::vector<bool>> table = {{true}, {true}};

  counter.SetConditionTable(table);
  counter.Check(5, 0); // Module 5 doesn't exist

  EXPECT_EQ(counter.counter, 0); // Should not crash, counter stays 0
}

TEST_F(L2CounterTest, CheckOutOfBoundsChannel) {
  L2Counter counter("test");
  std::vector<std::vector<bool>> table = {{true, false}, {true}};

  counter.SetConditionTable(table);
  counter.Check(0, 5); // Channel 5 doesn't exist in module 0

  EXPECT_EQ(counter.counter, 0); // Should not crash, counter stays 0
}

TEST_F(L2CounterTest, CheckNegativeIndices) {
  L2Counter counter("test");
  std::vector<std::vector<bool>> table = {{true}};

  counter.SetConditionTable(table);
  counter.Check(-1, 0);
  counter.Check(0, -1);

  EXPECT_EQ(counter.counter, 0); // Should handle gracefully
}

TEST_F(L2CounterTest, ResetCounter) {
  L2Counter counter("test", 100);

  EXPECT_EQ(counter.counter, 100);

  counter.ResetCounter();

  EXPECT_EQ(counter.counter, 0);
}

TEST_F(L2CounterTest, ResetAfterCounting) {
  L2Counter counter("test");
  std::vector<std::vector<bool>> table = {{true, true, true}};

  counter.SetConditionTable(table);
  counter.Check(0, 0);
  counter.Check(0, 1);
  counter.Check(0, 2);

  EXPECT_EQ(counter.counter, 3);

  counter.ResetCounter();

  EXPECT_EQ(counter.counter, 0);
}

TEST_F(L2CounterTest, LargeConditionTable) {
  L2Counter counter("test");

  // Create 10x16 table (typical detector configuration)
  std::vector<std::vector<bool>> table(10, std::vector<bool>(16, false));
  table[5][8] = true;
  table[3][12] = true;

  counter.SetConditionTable(table);

  for (int mod = 0; mod < 10; mod++) {
    for (int ch = 0; ch < 16; ch++) {
      counter.Check(mod, ch);
    }
  }

  EXPECT_EQ(counter.counter, 2); // Only 2 positions were true
}

TEST_F(L2CounterTest, CounterOverflow) {
  L2Counter counter("test", UINT64_MAX - 5);
  std::vector<std::vector<bool>> table = {{true}};

  counter.SetConditionTable(table);

  for (int i = 0; i < 10; i++) {
    counter.Check(0, 0);
  }

  // Counter should overflow (wraps around to small number)
  EXPECT_LT(counter.counter, 100);
}

TEST_F(L2CounterTest, CopySemantics) {
  L2Counter original("original", 42);
  std::vector<std::vector<bool>> table = {{true}};
  original.SetConditionTable(table);

  L2Counter copy = original;

  EXPECT_EQ(copy.name, "original");
  EXPECT_EQ(copy.counter, 42);
}

//=============================================================================
// L2Flag Tests
//=============================================================================

class L2FlagTest : public ::testing::Test {
 protected:
  void SetUp() override {}
  void TearDown() override {}
};

TEST_F(L2FlagTest, DefaultConstructor) {
  L2Flag flag;

  EXPECT_EQ(flag.name, "");
  EXPECT_EQ(flag.flag, false);
}

TEST_F(L2FlagTest, ParameterizedConstructor) {
  L2Flag flag("TestFlag", "CounterA", "==", 10);

  EXPECT_EQ(flag.name, "TestFlag");
  EXPECT_EQ(flag.flag, false);
}

TEST_F(L2FlagTest, CheckEqualConditionMatch) {
  L2Flag flag("TestFlag", "Counter1", "==", 5);

  std::vector<L2Counter> counters;
  counters.emplace_back("Counter1", 5);

  flag.Check(counters);

  EXPECT_TRUE(flag.flag);
}

TEST_F(L2FlagTest, CheckEqualConditionNoMatch) {
  L2Flag flag("TestFlag", "Counter1", "==", 5);

  std::vector<L2Counter> counters;
  counters.emplace_back("Counter1", 3);

  flag.Check(counters);

  EXPECT_FALSE(flag.flag);
}

TEST_F(L2FlagTest, CheckLessThanCondition) {
  L2Flag flag("TestFlag", "Counter1", "<", 10);

  std::vector<L2Counter> counters;
  counters.emplace_back("Counter1", 5);

  flag.Check(counters);

  EXPECT_TRUE(flag.flag);

  // Test boundary
  counters[0].counter = 10;
  flag.Check(counters);
  EXPECT_FALSE(flag.flag);
}

TEST_F(L2FlagTest, CheckGreaterThanCondition) {
  L2Flag flag("TestFlag", "Counter1", ">", 10);

  std::vector<L2Counter> counters;
  counters.emplace_back("Counter1", 15);

  flag.Check(counters);

  EXPECT_TRUE(flag.flag);

  // Test boundary
  counters[0].counter = 10;
  flag.Check(counters);
  EXPECT_FALSE(flag.flag);
}

TEST_F(L2FlagTest, CheckLessOrEqualCondition) {
  L2Flag flag("TestFlag", "Counter1", "<=", 10);

  std::vector<L2Counter> counters;
  counters.emplace_back("Counter1", 10);

  flag.Check(counters);
  EXPECT_TRUE(flag.flag);

  counters[0].counter = 9;
  flag.Check(counters);
  EXPECT_TRUE(flag.flag);

  counters[0].counter = 11;
  flag.Check(counters);
  EXPECT_FALSE(flag.flag);
}

TEST_F(L2FlagTest, CheckGreaterOrEqualCondition) {
  L2Flag flag("TestFlag", "Counter1", ">=", 10);

  std::vector<L2Counter> counters;
  counters.emplace_back("Counter1", 10);

  flag.Check(counters);
  EXPECT_TRUE(flag.flag);

  counters[0].counter = 11;
  flag.Check(counters);
  EXPECT_TRUE(flag.flag);

  counters[0].counter = 9;
  flag.Check(counters);
  EXPECT_FALSE(flag.flag);
}

TEST_F(L2FlagTest, CheckNotEqualCondition) {
  L2Flag flag("TestFlag", "Counter1", "!=", 10);

  std::vector<L2Counter> counters;
  counters.emplace_back("Counter1", 5);

  flag.Check(counters);
  EXPECT_TRUE(flag.flag);

  counters[0].counter = 10;
  flag.Check(counters);
  EXPECT_FALSE(flag.flag);
}

TEST_F(L2FlagTest, CheckUnknownCondition) {
  L2Flag flag("TestFlag", "Counter1", "unknown", 10);

  std::vector<L2Counter> counters;
  counters.emplace_back("Counter1", 5);

  testing::internal::CaptureStderr();
  flag.Check(counters);
  std::string output = testing::internal::GetCapturedStderr();

  EXPECT_FALSE(flag.flag);
  EXPECT_FALSE(output.empty()); // Should print error message
}

TEST_F(L2FlagTest, CheckWithMultipleCounters) {
  L2Flag flag("TestFlag", "Counter2", "==", 20);

  std::vector<L2Counter> counters;
  counters.emplace_back("Counter1", 10);
  counters.emplace_back("Counter2", 20);
  counters.emplace_back("Counter3", 30);

  flag.Check(counters);

  EXPECT_TRUE(flag.flag);
}

TEST_F(L2FlagTest, CheckWithNoMatchingCounter) {
  L2Flag flag("TestFlag", "NonExistent", "==", 10);

  std::vector<L2Counter> counters;
  counters.emplace_back("Counter1", 10);
  counters.emplace_back("Counter2", 20);

  flag.Check(counters);

  EXPECT_FALSE(flag.flag); // Should remain false
}

TEST_F(L2FlagTest, FlagResetsOnEachCheck) {
  L2Flag flag("TestFlag", "Counter1", "==", 10);

  std::vector<L2Counter> counters;
  counters.emplace_back("Counter1", 10);

  flag.Check(counters);
  EXPECT_TRUE(flag.flag);

  // Change counter value and check again
  counters[0].counter = 5;
  flag.Check(counters);
  EXPECT_FALSE(flag.flag); // Should reset to false
}

TEST_F(L2FlagTest, ZeroValueConditions) {
  L2Flag flag("TestFlag", "Counter1", "==", 0);

  std::vector<L2Counter> counters;
  counters.emplace_back("Counter1", 0);

  flag.Check(counters);

  EXPECT_TRUE(flag.flag);
}

TEST_F(L2FlagTest, NegativeValueConditions) {
  L2Flag flag("TestFlag", "Counter1", "<", 0);

  std::vector<L2Counter> counters;
  counters.emplace_back("Counter1", 0); // uint64_t can't be negative

  flag.Check(counters);

  EXPECT_FALSE(flag.flag);
}

//=============================================================================
// L2DataAcceptance Tests
//=============================================================================

class L2DataAcceptanceTest : public ::testing::Test {
 protected:
  void SetUp() override {}
  void TearDown() override {}
};

TEST_F(L2DataAcceptanceTest, DefaultConstructor) {
  L2DataAcceptance acceptance;
  // Should not crash
  SUCCEED();
}

TEST_F(L2DataAcceptanceTest, ParameterizedConstructor) {
  std::vector<std::string> monitors = {"Flag1", "Flag2"};
  L2DataAcceptance acceptance(monitors, "AND");

  // Should not crash
  SUCCEED();
}

TEST_F(L2DataAcceptanceTest, CheckAND_AllTrue) {
  std::vector<std::string> monitors = {"Flag1", "Flag2", "Flag3"};
  L2DataAcceptance acceptance(monitors, "AND");

  std::vector<L2Flag> flags;
  flags.emplace_back("Flag1", "C1", "==", 1);
  flags[0].flag = true;
  flags.emplace_back("Flag2", "C2", "==", 2);
  flags[1].flag = true;
  flags.emplace_back("Flag3", "C3", "==", 3);
  flags[2].flag = true;

  bool result = acceptance.Check(flags);

  EXPECT_TRUE(result);
}

TEST_F(L2DataAcceptanceTest, CheckAND_OneFalse) {
  std::vector<std::string> monitors = {"Flag1", "Flag2", "Flag3"};
  L2DataAcceptance acceptance(monitors, "AND");

  std::vector<L2Flag> flags;
  flags.emplace_back("Flag1", "C1", "==", 1);
  flags[0].flag = true;
  flags.emplace_back("Flag2", "C2", "==", 2);
  flags[1].flag = false; // One false
  flags.emplace_back("Flag3", "C3", "==", 3);
  flags[2].flag = true;

  bool result = acceptance.Check(flags);

  EXPECT_FALSE(result);
}

TEST_F(L2DataAcceptanceTest, CheckOR_AllTrue) {
  std::vector<std::string> monitors = {"Flag1", "Flag2"};
  L2DataAcceptance acceptance(monitors, "OR");

  std::vector<L2Flag> flags;
  flags.emplace_back("Flag1", "C1", "==", 1);
  flags[0].flag = true;
  flags.emplace_back("Flag2", "C2", "==", 2);
  flags[1].flag = true;

  bool result = acceptance.Check(flags);

  EXPECT_TRUE(result);
}

TEST_F(L2DataAcceptanceTest, CheckOR_OneTrue) {
  std::vector<std::string> monitors = {"Flag1", "Flag2", "Flag3"};
  L2DataAcceptance acceptance(monitors, "OR");

  std::vector<L2Flag> flags;
  flags.emplace_back("Flag1", "C1", "==", 1);
  flags[0].flag = false;
  flags.emplace_back("Flag2", "C2", "==", 2);
  flags[1].flag = true; // At least one true
  flags.emplace_back("Flag3", "C3", "==", 3);
  flags[2].flag = false;

  bool result = acceptance.Check(flags);

  EXPECT_TRUE(result);
}

TEST_F(L2DataAcceptanceTest, CheckOR_AllFalse) {
  std::vector<std::string> monitors = {"Flag1", "Flag2"};
  L2DataAcceptance acceptance(monitors, "OR");

  std::vector<L2Flag> flags;
  flags.emplace_back("Flag1", "C1", "==", 1);
  flags[0].flag = false;
  flags.emplace_back("Flag2", "C2", "==", 2);
  flags[1].flag = false;

  bool result = acceptance.Check(flags);

  EXPECT_FALSE(result); // OR with all false = false
}

TEST_F(L2DataAcceptanceTest, CheckUnknownOperator) {
  std::vector<std::string> monitors = {"Flag1"};
  L2DataAcceptance acceptance(monitors, "XOR");

  std::vector<L2Flag> flags;
  flags.emplace_back("Flag1", "C1", "==", 1);
  flags[0].flag = true;

  testing::internal::CaptureStderr();
  bool result = acceptance.Check(flags);
  std::string output = testing::internal::GetCapturedStderr();

  EXPECT_FALSE(result);
  EXPECT_FALSE(output.empty());
}

TEST_F(L2DataAcceptanceTest, CheckWithNoMatchingMonitors_AND) {
  std::vector<std::string> monitors = {"NonExistent"};
  L2DataAcceptance acceptance(monitors, "AND");

  std::vector<L2Flag> flags;
  flags.emplace_back("Flag1", "C1", "==", 1);
  flags[0].flag = true;

  testing::internal::CaptureStderr();
  bool result = acceptance.Check(flags);
  std::string output = testing::internal::GetCapturedStderr();

  EXPECT_FALSE(result);
  EXPECT_FALSE(output.empty()); // Should print error
}

TEST_F(L2DataAcceptanceTest, CheckWithNoMatchingMonitors_OR) {
  std::vector<std::string> monitors = {"NonExistent"};
  L2DataAcceptance acceptance(monitors, "OR");

  std::vector<L2Flag> flags;
  flags.emplace_back("Flag1", "C1", "==", 1);
  flags[0].flag = true;

  testing::internal::CaptureStderr();
  bool result = acceptance.Check(flags);
  std::string output = testing::internal::GetCapturedStderr();

  EXPECT_FALSE(result);
  EXPECT_FALSE(output.empty());
}

TEST_F(L2DataAcceptanceTest, CheckWithManyFlags) {
  std::vector<std::string> monitors;
  for (int i = 0; i < 10; i++) {
    monitors.push_back("Flag" + std::to_string(i));
  }
  L2DataAcceptance acceptance(monitors, "AND");

  std::vector<L2Flag> flags;
  for (int i = 0; i < 10; i++) {
    flags.emplace_back("Flag" + std::to_string(i), "C", "==", i);
    flags.back().flag = true;
  }

  bool result = acceptance.Check(flags);

  EXPECT_TRUE(result);
}

//=============================================================================
// Integration Tests - L2 Trigger Logic
//=============================================================================

class L2TriggerIntegrationTest : public ::testing::Test {
 protected:
  void SetUp() override {}
  void TearDown() override {}
};

TEST_F(L2TriggerIntegrationTest, SimpleMultiplicityTrigger) {
  // Setup: Count hits in specific detectors
  L2Counter counter("Multiplicity");
  std::vector<std::vector<bool>> table(4, std::vector<bool>(16, false));
  table[0][0] = true;
  table[0][1] = true;
  table[1][0] = true;
  counter.SetConditionTable(table);

  // Simulate 5 hits
  counter.Check(0, 0);
  counter.Check(0, 1);
  counter.Check(1, 0);
  counter.Check(0, 0);
  counter.Check(2, 5); // Not in table

  EXPECT_EQ(counter.counter, 4);

  // Create flag: require multiplicity >= 3
  L2Flag flag("MultiFlag", "Multiplicity", ">=", 3);
  std::vector<L2Counter> counters = {counter};
  flag.Check(counters);

  EXPECT_TRUE(flag.flag);

  // Data acceptance
  L2DataAcceptance acceptance({"MultiFlag"}, "AND");
  std::vector<L2Flag> flags = {flag};

  bool accept = acceptance.Check(flags);

  EXPECT_TRUE(accept);
}

TEST_F(L2TriggerIntegrationTest, ComplexTriggerWithMultipleConditions) {
  // Two counters: Forward and Backward detectors
  L2Counter fwdCounter("Forward");
  L2Counter bwdCounter("Backward");

  std::vector<std::vector<bool>> fwdTable(2, std::vector<bool>(8, false));
  fwdTable[0][0] = true;
  fwdTable[0][1] = true;
  fwdCounter.SetConditionTable(fwdTable);

  std::vector<std::vector<bool>> bwdTable(2, std::vector<bool>(8, false));
  bwdTable[1][5] = true;
  bwdTable[1][6] = true;
  bwdCounter.SetConditionTable(bwdTable);

  // Simulate hits
  fwdCounter.Check(0, 0);
  fwdCounter.Check(0, 1);
  bwdCounter.Check(1, 5);

  // Flags: require >= 1 in each
  L2Flag fwdFlag("FwdFlag", "Forward", ">=", 1);
  L2Flag bwdFlag("BwdFlag", "Backward", ">=", 1);

  std::vector<L2Counter> counters = {fwdCounter, bwdCounter};
  fwdFlag.Check(counters);
  bwdFlag.Check(counters);

  EXPECT_TRUE(fwdFlag.flag);
  EXPECT_TRUE(bwdFlag.flag);

  // Acceptance: require BOTH (AND logic)
  L2DataAcceptance acceptance({"FwdFlag", "BwdFlag"}, "AND");
  std::vector<L2Flag> flags = {fwdFlag, bwdFlag};

  bool accept = acceptance.Check(flags);

  EXPECT_TRUE(accept);
}

TEST_F(L2TriggerIntegrationTest, VetoLogic) {
  L2Counter signal("Signal");
  L2Counter veto("Veto");

  std::vector<std::vector<bool>> table(1, std::vector<bool>(2, true));
  signal.SetConditionTable(table);
  veto.SetConditionTable(table);

  // Signal present, no veto
  signal.Check(0, 0);
  signal.Check(0, 1);
  // veto.Check() not called - count stays 0

  L2Flag signalFlag("SignalFlag", "Signal", ">=", 2);
  L2Flag vetoFlag("VetoFlag", "Veto", "==", 0); // Veto must be zero

  std::vector<L2Counter> counters = {signal, veto};
  signalFlag.Check(counters);
  vetoFlag.Check(counters);

  EXPECT_TRUE(signalFlag.flag);
  EXPECT_TRUE(vetoFlag.flag);

  L2DataAcceptance acceptance({"SignalFlag", "VetoFlag"}, "AND");
  std::vector<L2Flag> flags = {signalFlag, vetoFlag};

  bool accept = acceptance.Check(flags);

  EXPECT_TRUE(accept); // Accept: signal present, no veto
}

TEST_F(L2TriggerIntegrationTest, MultiEventProcessing) {
  L2Counter counter("EventCounter");
  std::vector<std::vector<bool>> table = {{true}};
  counter.SetConditionTable(table);

  // Process 3 events
  for (int event = 0; event < 3; event++) {
    counter.ResetCounter();

    // Event-specific hit pattern
    for (int i = 0; i <= event; i++) {
      counter.Check(0, 0);
    }

    L2Flag flag("MultiFlag", "EventCounter", ">=", 2);
    std::vector<L2Counter> counters = {counter};
    flag.Check(counters);

    L2DataAcceptance acceptance({"MultiFlag"}, "AND");
    std::vector<L2Flag> flags = {flag};

    bool accept = acceptance.Check(flags);

    if (event < 1) {
      EXPECT_FALSE(accept); // Events 0-1: < 2 hits
    } else {
      EXPECT_TRUE(accept); // Events 2+: >= 2 hits
    }
  }
}
