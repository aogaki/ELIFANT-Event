# ELIFANT-Event Code Improvement TODO List

This directory contains detailed improvement recommendations for the ELIFANT-Event codebase, organized by priority.

## Priority Levels

- **HIGH**: Critical issues that affect correctness, safety, or reliability
- **MEDIUM**: Important improvements for maintainability and performance
- **LOW**: Nice-to-have enhancements and best practices

## Files by Priority

### HIGH Priority
1. ✅ [Memory Management Issues](COMPLETED_1_Memory_Management_Issues.md) - **COMPLETED** - Fixed memory leaks and unsafe pointer usage
2. [Error Handling and Exception Safety](2_Error_Handling_and_Exception_Safety.md) - Implement proper error handling
3. [Thread Safety Issues](3_Thread_Safety_Issues.md) - Fix race conditions and synchronization problems

### MEDIUM Priority
4. [Resource Management RAII](4_Resource_Management_RAII.md) - Implement RAII patterns for all resources
5. [Code Organization and Architecture](5_Code_Organization_and_Architecture.md) - Refactor for better structure
6. [Performance Optimizations](6_Performance_Optimizations.md) - Optimize data structures and algorithms
7. [Modern C++ Best Practices](7_Modern_CPP_Best_Practices.md) - Update to modern C++ standards

### LOW Priority
8. [Specific Code Issues](8_Specific_Code_Issues.md) - Fix code duplication and naming inconsistencies
9. [Testing and Documentation](9_Testing_and_Documentation.md) - Add tests and documentation

## Completion Status

### Completed TODOs
- ✅ **TODO #1 - Memory Management Issues** (Completed using TDD approach)
  - Fixed memory leaks in L1EventBuilder.cpp and TimeAlignment.cpp
  - Verified EventData class has proper memory management
  - Created 26 unit tests, all passing
  - Maintained ROOT compatibility

### In Progress
- None currently

### Remaining
- 8 TODOs (2 HIGH, 4 MEDIUM, 2 LOW priority)

## Getting Started

1. Start with HIGH priority issues as they affect system stability
2. Each file contains:
   - Detailed issue descriptions
   - Code examples of problems
   - Recommended solutions with code snippets
   - Action items checklist

3. Consider creating feature branches for each major improvement
4. Add tests as you fix issues to prevent regression

## Notes

- Many improvements are interconnected (e.g., RAII helps with exception safety)
- Some solutions require C++17 or C++20 features
- ROOT-specific considerations are noted where applicable
- Performance improvements should be measured before and after implementation