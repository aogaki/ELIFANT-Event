# Memory Management Issues

## Priority: HIGH

### Status: IN PROGRESS 🔄

### Issues Identified

1. **Raw Pointer in EventData Class**
   - Location: `include/EventData.hpp:49`
   - Problem: Using raw `new` without corresponding `delete`
   - Code: `eventDataVec = new std::vector<RawData_t>();`
   - Impact: Memory leak risk
   - **Resolution**: Kept raw pointer due to ROOT compatibility issues with `std::unique_ptr`. The class already has proper copy/move constructors, assignment operators, and destructor that correctly manage the memory.

2. **Manual Memory Management for ROOT Objects**
   - Locations:
     - `src/L1EventBuilder.cpp:277`
     - `src/TimeAlignment.cpp:120, 194, 313`
   - Problem: Manual `delete` without null checks
   - Impact: Potential crashes if delete is called on null pointers
   - **Resolution**: Fixed all instances by adding proper cleanup before early returns and continues

3. **Missing RAII for File Resources**
   - Problem: ROOT TFile objects managed manually
   - Impact: Resource leaks if exceptions occur
   - **Resolution**: TFileRAII class already implemented with `std::unique_ptr`

### Implemented Solutions

1. **EventData Class**
   - Verified existing implementation has proper memory management
   - Copy constructor performs deep copy
   - Move constructor transfers ownership correctly
   - Assignment operators handle self-assignment and cleanup
   - Destructor properly deletes the raw pointer
   - All 7 tests pass

2. **Fixed Memory Leaks in TimeAlignment.cpp**
   - Added `delete file` in SaveHistograms() before early return (line 85)
   - Added `delete file` in FillHistograms() before continue (line 163)
   - Added `delete file` in CalculateTimeAlignment() before return (line 253)

3. **Fixed Memory Leaks in L1EventBuilder.cpp**
   - Added validation check for output file creation with cleanup on failure
   - Added `delete file` when input file can't be opened (line 136)
   - Added `delete file` when tree is not found (line 144)
   - Added `delete file` after normal file processing (line 183)

4. **TFileRAII Implementation**
   - Already implemented in `include/TFileRAII.hpp`
   - Uses `std::unique_ptr<TFile>` for automatic cleanup
   - Provides move semantics and const-correct methods
   - Throws FileException on errors
   - All 7 tests pass

### Test Coverage

Created comprehensive test suite using TDD approach:
- **EventDataTest.cpp**: 7 tests for memory management, copy/move semantics
- **TFileRAIITest.cpp**: 7 tests for RAII file handling
- **L1EventBuilderTest.cpp**: 5 tests demonstrating memory issues and fixes
- **TimeAlignmentTest.cpp**: 7 tests for zombie file handling and memory leaks

**Total: 26 tests, all passing ✓**

### Action Items

- [x] ~~Replace raw pointer in EventData with direct member or smart pointer~~ (Not needed - proper management exists)
- [x] ~~Convert all manual ROOT object management to use smart pointers~~ (Fixed with proper cleanup)
- [x] Add null checks before delete operations
- [x] Create RAII wrappers for commonly used ROOT resources (TFileRAII implemented)
- [x] Review and fix all instances of manual memory management

### Notes

- Raw pointers are kept where necessary for ROOT compatibility
- All memory leaks have been fixed by ensuring proper cleanup in error paths
- Exception safety is provided through TFileRAII for new code
- Existing code has been made safer with minimal changes to maintain compatibility

### ⚠️ KISS Principle Applied

This TODO followed KISS principles:
- ✅ **Used existing patterns**: Worked with ROOT's ownership model instead of fighting it
- ✅ **Minimal changes**: Fixed memory leaks without rewriting entire classes
- ✅ **Simple solutions**: Added `delete` statements where needed, not complex RAII everywhere
- ✅ **Maintained compatibility**: Kept raw pointers where ROOT requires them
- ✅ **Incremental improvements**: Fixed specific issues without over-engineering