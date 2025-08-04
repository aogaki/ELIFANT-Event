# Error Handling and Exception Safety

## Priority: HIGH

### Status: IN PROGRESS 🔄

### Issues Identified

1. **No Exception Handling**
   - Problem: No try-catch blocks throughout the codebase
   - Impact: Unhandled exceptions will crash the application
   - Critical areas: File I/O, JSON parsing, memory allocation

2. **Inconsistent Error Reporting**
   - Problem: Mix of std::cerr messages and return values
   - No unified error handling strategy
   - No error codes or exception hierarchy

3. **Missing Input Validation**
   - JSON parsing without validation
   - File operations without checks
   - Array access without bounds checking

4. **Resource Cleanup on Error**
   - No guarantee of cleanup if errors occur mid-operation
   - File handles may leak on errors

### Recommended Solutions

1. **Implement Exception Hierarchy**
   ```cpp
   namespace DELILA {
   class DELILAException : public std::runtime_error {
   public:
       explicit DELILAException(const std::string& msg) : std::runtime_error(msg) {}
   };
   
   class FileException : public DELILAException {
   public:
       explicit FileException(const std::string& msg) : DELILAException(msg) {}
   };
   
   class ConfigException : public DELILAException {
   public:
       explicit ConfigException(const std::string& msg) : DELILAException(msg) {}
   };
   }
   ```

2. **Add Try-Catch Blocks in Main Functions**
   ```cpp
   void BuildEvent(const uint32_t nThreads) {
       try {
           // Existing code
       } catch (const FileException& e) {
           std::cerr << "File error: " << e.what() << std::endl;
           throw;
       } catch (const std::exception& e) {
           std::cerr << "Unexpected error: " << e.what() << std::endl;
           throw;
       }
   }
   ```

3. **Validate JSON Input**
   ```cpp
   void LoadChSettings(const std::string& fileName) {
       std::ifstream ifs(fileName);
       if (!ifs) {
           throw FileException("Cannot open file: " + fileName);
       }
       
       nlohmann::json j;
       try {
           ifs >> j;
       } catch (const nlohmann::json::exception& e) {
           throw ConfigException("Invalid JSON in " + fileName + ": " + e.what());
       }
       
       // Validate required fields
       if (!j.contains("RequiredField")) {
           throw ConfigException("Missing required field in " + fileName);
       }
   }
   ```

4. **Use RAII for Automatic Cleanup**
   ```cpp
   class ThreadPool {
       std::vector<std::thread> threads;
   public:
       ~ThreadPool() {
           for (auto& t : threads) {
               if (t.joinable()) t.join();
           }
       }
   };
   ```

### Implemented Solutions

1. **Enhanced Exception Hierarchy** (`DELILAExceptions.hpp`)
   - Extended base `DELILAException` class  
   - Added `JSONException` for JSON parsing errors
   - Added `ValidationException` for input validation
   - Added `RangeException` for bounds checking
   - Added `ProcessingException` for data processing errors

2. **ChSettings Validation** (`ChSettingsValidated.hpp`)
   - Created `GetChSettingsWithValidation()` function
   - Validates all numeric ranges (module 0-255, channel 0-15)
   - Checks for required fields in JSON
   - Proper type checking with informative error messages
   - Context-aware error messages with array indices

3. **File Operations Error Handling** (`L1EventBuilderValidated.hpp`)
   - Created `L1EventBuilderImproved` class
   - Validates thread count (1-128)
   - Checks file existence and type
   - Validates file extensions
   - Empty file list validation

4. **Comprehensive Test Coverage**
   - ChSettingsErrorHandlingTest.cpp - 9 tests for configuration validation
   - FileOperationsErrorTest.cpp - 5 tests for file operations
   - JSONParsingExceptionTest.cpp - 10 tests for JSON parsing
   - **Total: 24 tests, all passing ✓**

### Action Items

- [x] Define exception hierarchy for different error types
- [x] Replace error messages with appropriate exceptions  
- [x] Add try-catch blocks in all public API functions
- [x] Implement input validation for all external data
- [x] Add bounds checking for array/vector access
- [x] Create error codes enum for non-exceptional errors (used exceptions instead)
- [x] Document error handling strategy
- [x] Add unit tests for error conditions
- [x] Modify original classes to throw exceptions
- [x] Implement RAII for thread management
- [x] Add exception handling to file operations

### Notes

- Error handling needs to be implemented throughout the codebase
- Input validation is missing in many areas
- Resource cleanup on error paths needs attention