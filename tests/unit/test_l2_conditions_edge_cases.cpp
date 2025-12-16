#include <gtest/gtest.h>

#include "L2Conditions.hpp"

#include <limits>

using namespace DELILA;

//=============================================================================
// L2Counter Edge Cases - Testing Signed/Unsigned Issues
//=============================================================================

class L2CounterEdgeCaseTest : public ::testing::Test {
 protected:
  void SetUp() override {}
  void TearDown() override {}
};

TEST_F(L2CounterEdgeCaseTest, CheckWithNegativeModule) {
  L2Counter counter("test");
  std::vector<std::vector<bool>> table = {{true, true}, {true, true}};
  counter.SetConditionTable(table);

  // This should NOT increment because mod is negative
  counter.Check(-1, 0);
  EXPECT_EQ(counter.counter, 0);

  // Verify it still works with valid indices
  counter.Check(0, 0);
  EXPECT_EQ(counter.counter, 1);
}

TEST_F(L2CounterEdgeCaseTest, CheckWithNegativeChannel) {
  L2Counter counter("test");
  std::vector<std::vector<bool>> table = {{true, true}, {true, true}};
  counter.SetConditionTable(table);

  // This should NOT increment because ch is negative
  counter.Check(0, -1);
  EXPECT_EQ(counter.counter, 0);

  // Verify it still works with valid indices
  counter.Check(0, 0);
  EXPECT_EQ(counter.counter, 1);
}

TEST_F(L2CounterEdgeCaseTest, CheckWithBothNegative) {
  L2Counter counter("test");
  std::vector<std::vector<bool>> table = {{true, true}, {true, true}};
  counter.SetConditionTable(table);

  // Neither should increment
  counter.Check(-1, -1);
  EXPECT_EQ(counter.counter, 0);
}

TEST_F(L2CounterEdgeCaseTest, CheckWithLargeNegativeValues) {
  L2Counter counter("test");
  std::vector<std::vector<bool>> table = {{true}};
  counter.SetConditionTable(table);

  // These should NOT increment
  counter.Check(-100, 0);
  counter.Check(0, -100);
  counter.Check(std::numeric_limits<int32_t>::min(), 0);
  counter.Check(0, std::numeric_limits<int32_t>::min());

  EXPECT_EQ(counter.counter, 0);
}

TEST_F(L2CounterEdgeCaseTest, EmptyInnerVector) {
  L2Counter counter("test");
  std::vector<std::vector<bool>> table = {{}};  // One module with zero channels
  counter.SetConditionTable(table);

  // Should not crash, should not increment
  counter.Check(0, 0);
  EXPECT_EQ(counter.counter, 0);
}

TEST_F(L2CounterEdgeCaseTest, UnevenInnerVectors) {
  L2Counter counter("test");
  std::vector<std::vector<bool>> table = {
      {true, true, true},  // Module 0: 3 channels
      {true},               // Module 1: 1 channel
      {true, true}          // Module 2: 2 channels
  };
  counter.SetConditionTable(table);

  // Valid accesses
  counter.Check(0, 2);  // Should work
  EXPECT_EQ(counter.counter, 1);

  counter.Check(1, 0);  // Should work
  EXPECT_EQ(counter.counter, 2);

  // Out of bounds for module 1 (only has 1 channel)
  counter.Check(1, 1);  // Should not increment
  EXPECT_EQ(counter.counter, 2);

  counter.Check(1, 2);  // Should not increment
  EXPECT_EQ(counter.counter, 2);
}

//=============================================================================
// L2Flag Edge Cases - Testing Type Mismatch Issues
//=============================================================================

class L2FlagEdgeCaseTest : public ::testing::Test {
 protected:
  void SetUp() override {}
  void TearDown() override {}
};

TEST_F(L2FlagEdgeCaseTest, CompareWithNegativeValue) {
  L2Flag flag("TestFlag", "Counter1", "==", -5);

  std::vector<L2Counter> counters;
  counters.emplace_back("Counter1", 0);

  flag.Check(counters);
  // counter.counter is uint64_t (0), fValue is int32_t (-5)
  // When comparing, -5 becomes a huge uint64_t
  // So 0 == (huge number) should be false
  EXPECT_FALSE(flag.flag);
}

TEST_F(L2FlagEdgeCaseTest, CompareWithLargePositiveValue) {
  L2Flag flag("TestFlag", "Counter1", "<", 1000000);

  std::vector<L2Counter> counters;
  counters.emplace_back("Counter1", 100);

  flag.Check(counters);
  EXPECT_TRUE(flag.flag);  // 100 < 1000000
}

TEST_F(L2FlagEdgeCaseTest, MultipleCountersWithSameName) {
  L2Flag flag("TestFlag", "Counter1", "==", 5);

  std::vector<L2Counter> counters;
  counters.emplace_back("Counter1", 5);   // Matches
  counters.emplace_back("Counter1", 10);  // Also named Counter1, doesn't match

  flag.Check(counters);

  // Bug: The flag will be set to true by first counter, then to false by
  // second counter Current implementation uses the LAST matching counter's
  // result
  EXPECT_FALSE(flag.flag);  // Last match wins (10 != 5)
}

TEST_F(L2FlagEdgeCaseTest, MultipleCountersAllMatch) {
  L2Flag flag("TestFlag", "Counter1", ">=", 5);

  std::vector<L2Counter> counters;
  counters.emplace_back("Counter1", 10);  // Matches
  counters.emplace_back("Counter1", 7);   // Matches

  flag.Check(counters);

  // Last match should still be true
  EXPECT_TRUE(flag.flag);
}

TEST_F(L2FlagEdgeCaseTest, EmptyCounterVector) {
  L2Flag flag("TestFlag", "Counter1", "==", 5);

  std::vector<L2Counter> counters;  // Empty

  flag.Check(counters);

  // No matching counter found, flag should remain false
  EXPECT_FALSE(flag.flag);
}

TEST_F(L2FlagEdgeCaseTest, CounterWithZeroValue) {
  L2Flag flag("TestFlag", "Counter1", "==", 0);

  std::vector<L2Counter> counters;
  counters.emplace_back("Counter1", 0);

  flag.Check(counters);
  EXPECT_TRUE(flag.flag);
}

TEST_F(L2FlagEdgeCaseTest, GreaterThanZeroWhenCounterIsZero) {
  L2Flag flag("TestFlag", "Counter1", ">", 0);

  std::vector<L2Counter> counters;
  counters.emplace_back("Counter1", 0);

  flag.Check(counters);
  EXPECT_FALSE(flag.flag);  // 0 > 0 is false
}

TEST_F(L2FlagEdgeCaseTest, LessThanZeroWithUnsignedCounter) {
  L2Flag flag("TestFlag", "Counter1", "<", 0);

  std::vector<L2Counter> counters;
  counters.emplace_back("Counter1", 100);  // uint64_t value

  flag.Check(counters);
  // counter is 100, comparing with -0 which is 0
  EXPECT_FALSE(flag.flag);  // 100 < 0 is false
}

TEST_F(L2FlagEdgeCaseTest, VeryLargeCounterValue) {
  L2Flag flag("TestFlag", "Counter1", ">", 1000);

  std::vector<L2Counter> counters;
  counters.emplace_back("Counter1",
                        std::numeric_limits<uint64_t>::max());  // Maximum value

  flag.Check(counters);
  EXPECT_TRUE(flag.flag);  // max_uint64 > 1000 is true
}

//=============================================================================
// L2DataAcceptance Edge Cases
//=============================================================================

class L2DataAcceptanceEdgeCaseTest : public ::testing::Test {
 protected:
  void SetUp() override {}
  void TearDown() override {}
};

TEST_F(L2DataAcceptanceEdgeCaseTest, EmptyMonitorVector) {
  std::vector<std::string> monitors;  // Empty
  L2DataAcceptance acceptance(monitors, "AND");

  std::vector<L2Flag> flags;
  flags.emplace_back("Flag1", "C1", "==", 1);
  flags[0].flag = true;

  bool result = acceptance.Check(flags);

  // No monitors specified, checkCounter will be 0
  // For AND: returns false when checkCounter == 0
  EXPECT_FALSE(result);
}

TEST_F(L2DataAcceptanceEdgeCaseTest, EmptyMonitorVectorOR) {
  std::vector<std::string> monitors;  // Empty
  L2DataAcceptance acceptance(monitors, "OR");

  std::vector<L2Flag> flags;
  flags.emplace_back("Flag1", "C1", "==", 1);
  flags[0].flag = true;

  bool result = acceptance.Check(flags);

  // For OR: returns false when checkCounter == 0
  EXPECT_FALSE(result);
}

TEST_F(L2DataAcceptanceEdgeCaseTest, EmptyFlagVector) {
  std::vector<std::string> monitors = {"Flag1", "Flag2"};
  L2DataAcceptance acceptance(monitors, "AND");

  std::vector<L2Flag> flags;  // Empty

  bool result = acceptance.Check(flags);

  // No flags to check, checkCounter stays 0
  EXPECT_FALSE(result);
}

TEST_F(L2DataAcceptanceEdgeCaseTest, DuplicateMonitorNamesAND) {
  std::vector<std::string> monitors = {"Flag1", "Flag1"};  // Duplicate
  L2DataAcceptance acceptance(monitors, "AND");

  std::vector<L2Flag> flags;
  flags.emplace_back("Flag1", "C1", "==", 1);
  flags[0].flag = true;

  bool result = acceptance.Check(flags);

  // The same flag will be checked twice for the duplicate monitor name
  // Both checks will pass, checkCounter will be 2
  EXPECT_TRUE(result);
}

TEST_F(L2DataAcceptanceEdgeCaseTest, DuplicateMonitorNamesOR) {
  std::vector<std::string> monitors = {"Flag1", "Flag1"};  // Duplicate
  L2DataAcceptance acceptance(monitors, "OR");

  std::vector<L2Flag> flags;
  flags.emplace_back("Flag1", "C1", "==", 1);
  flags[0].flag = false;

  bool result = acceptance.Check(flags);

  // Both checks will fail, but it's OR logic with all false
  EXPECT_FALSE(result);
}

TEST_F(L2DataAcceptanceEdgeCaseTest, CaseSensitiveOperator) {
  std::vector<std::string> monitors = {"Flag1"};
  L2DataAcceptance acceptance1(monitors, "and");  // lowercase

  std::vector<L2Flag> flags;
  flags.emplace_back("Flag1", "C1", "==", 1);
  flags[0].flag = true;

  bool result1 = acceptance1.Check(flags);

  // "and" != "AND", so it will be treated as unknown operator
  EXPECT_FALSE(result1);  // Unknown operator returns false
}

TEST_F(L2DataAcceptanceEdgeCaseTest, InvalidOperator) {
  std::vector<std::string> monitors = {"Flag1"};
  L2DataAcceptance acceptance(monitors, "XOR");  // Invalid

  std::vector<L2Flag> flags;
  flags.emplace_back("Flag1", "C1", "==", 1);
  flags[0].flag = true;

  bool result = acceptance.Check(flags);

  // XOR is not supported, should return false
  EXPECT_FALSE(result);
}

TEST_F(L2DataAcceptanceEdgeCaseTest, VeryLargeNumberOfFlags) {
  std::vector<std::string> monitors;
  for (int i = 0; i < 100; i++) {
    monitors.push_back("Flag" + std::to_string(i));
  }

  L2DataAcceptance acceptance(monitors, "AND");

  std::vector<L2Flag> flags;
  for (int i = 0; i < 100; i++) {
    flags.emplace_back("Flag" + std::to_string(i), "C" + std::to_string(i),
                       "==", 1);
    flags.back().flag = true;
  }

  bool result = acceptance.Check(flags);

  // All 100 flags are true with AND
  EXPECT_TRUE(result);
}

TEST_F(L2DataAcceptanceEdgeCaseTest, VeryLargeNumberOfFlagsOneFlase) {
  std::vector<std::string> monitors;
  for (int i = 0; i < 100; i++) {
    monitors.push_back("Flag" + std::to_string(i));
  }

  L2DataAcceptance acceptance(monitors, "AND");

  std::vector<L2Flag> flags;
  for (int i = 0; i < 100; i++) {
    flags.emplace_back("Flag" + std::to_string(i), "C" + std::to_string(i),
                       "==", 1);
    flags.back().flag = (i != 50);  // Flag50 is false
  }

  bool result = acceptance.Check(flags);

  // One false flag with AND should make result false
  EXPECT_FALSE(result);
}
