#ifndef L2Counter_hpp
#define L2Counter_hpp 1

#include <cstdint>
#include <string>
#include <vector>

#include "ChSettings.hpp"

namespace DELILA
{

class L2Counter
{
 public:
  L2Counter() : name(""), counter(0) {};
  L2Counter(const std::string &name, const uint64_t counter = 0)
      : name(name), counter(counter) {};
  ~L2Counter() = default;

  void SetConditionTable(std::vector<std::vector<bool>> table)
  {
    fConditionTable = table;
  };
  void Check(int32_t mod, int32_t ch)
  {
    if (mod < fConditionTable.size() && ch < fConditionTable[mod].size()) {
      if (fConditionTable[mod][ch]) {
        counter++;
      }
    }
  };
  void ResetCounter() { counter = 0; };

  std::string name;
  uint64_t counter;

 private:
  std::vector<std::vector<bool>> fConditionTable;
};

class L2Flag
{
 public:
  L2Flag() : name(""), flag(false) {};
  L2Flag(const std::string &name, const std::string &monitorName,
         const std::string &condition, const int32_t value)
      : name(name),
        fMonitorName(monitorName),
        fCondition(condition),
        fValue(value),
        flag(false) {};
  ~L2Flag() = default;

  std::string name;
  bool flag;

  void Check(std::vector<L2Counter> &counterVec)
  {
    flag = false;

    for (auto &counter : counterVec) {
      if (counter.name == fMonitorName) {
        auto counterVal = counter.counter;
        if (fCondition == "==") {
          flag = counterVal == fValue;
        } else if (fCondition == "<") {
          flag = counterVal < fValue;
        } else if (fCondition == ">") {
          flag = counterVal > fValue;
        } else if (fCondition == "<=") {
          flag = counterVal <= fValue;
        } else if (fCondition == ">=") {
          flag = counterVal >= fValue;
        } else if (fCondition == "!=") {
          flag = counterVal != fValue;
        } else {
          std::cerr << "Error: Unknown condition: " << fCondition << std::endl;
          flag = false;
        }
      }
    }
  };

 private:
  std::string fMonitorName;
  std::string fCondition;
  int32_t fValue;
};

class L2DataAcceptance
{
 public:
  L2DataAcceptance() { fMonitorVec = {}, fLogicalOperator = ""; };
  L2DataAcceptance(const std::vector<std::string> &monitorVec,
                   const std::string &logicalOperator)
      : fMonitorVec(monitorVec), fLogicalOperator(logicalOperator) {};
  ~L2DataAcceptance() = default;

  bool Check(std::vector<L2Flag> &flagVec)
  {
    if (fLogicalOperator == "AND") {
      for (auto &monitor : fMonitorVec) {
        for (auto &flag : flagVec) {
          if (flag.name == monitor) {
            if (!flag.flag) {
              return false;
            }
          }
        }
      }
    } else if (fLogicalOperator == "OR") {
      for (auto &monitor : fMonitorVec) {
        for (auto &flag : flagVec) {
          if (flag.name == monitor) {
            if (flag.flag) {
              return true;
            }
          }
        }
      }
    } else {
      std::cerr << "Error: Unknown logical operator: " << fLogicalOperator
                << std::endl;
      return false;
    }

    return true;
  };

 private:
  std::vector<std::string> fMonitorVec;
  std::string fLogicalOperator;
};

}  // namespace DELILA
#endif