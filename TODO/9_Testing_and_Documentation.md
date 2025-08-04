# Testing and Documentation

## Priority: LOW

### Issues Identified

1. **No Unit Tests**
   - No test framework integrated
   - No test coverage metrics
   - Manual testing only

2. **Missing API Documentation**
   - No Doxygen comments for public interfaces
   - Unclear method purposes and parameters
   - No usage examples

3. **No Integration Tests**
   - No end-to-end testing
   - No performance benchmarks
   - No regression tests

4. **Missing Architecture Documentation**
   - No high-level design documents
   - Data flow not documented
   - Threading model not explained

### Recommended Solutions

1. **Set Up Testing Framework**
   ```cmake
   # In CMakeLists.txt
   include(FetchContent)
   FetchContent_Declare(
     googletest
     URL https://github.com/google/googletest/archive/release-1.12.1.zip
   )
   FetchContent_MakeAvailable(googletest)
   
   enable_testing()
   add_subdirectory(tests)
   ```

2. **Create Unit Tests**
   ```cpp
   // tests/ChSettingsTest.cpp
   #include <gtest/gtest.h>
   #include "ChSettings.hpp"
   
   class ChSettingsTest : public ::testing::Test {
   protected:
       void SetUp() override {
           // Create test configuration file
       }
       
       void TearDown() override {
           // Clean up test files
       }
   };
   
   TEST_F(ChSettingsTest, LoadValidConfiguration) {
       auto settings = ChSettings::GetChSettings("test_config.json");
       ASSERT_FALSE(settings.empty());
       EXPECT_EQ(settings[0][0].mod, 0);
       EXPECT_EQ(settings[0][0].ch, 0);
   }
   
   TEST_F(ChSettingsTest, LoadInvalidConfiguration) {
       EXPECT_THROW(ChSettings::GetChSettings("nonexistent.json"), 
                    FileException);
   }
   
   TEST_F(ChSettingsTest, DetectorTypeConversion) {
       EXPECT_EQ(ChSettings::GetDetectorType("hpge"), DetectorType::HPGe);
       EXPECT_EQ(ChSettings::GetDetectorType("HPGE"), DetectorType::HPGe);
       EXPECT_EQ(ChSettings::GetDetectorType("unknown"), DetectorType::Unknown);
   }
   ```

3. **Add Doxygen Documentation**
   ```cpp
   /**
    * @brief L1 Event Builder for first-level trigger processing
    * 
    * This class handles the construction of physics events based on 
    * time correlation between detector hits. It processes raw data
    * files in parallel and outputs correlated events.
    * 
    * @note Thread-safe for parallel processing
    * @see L2EventBuilder for second-level processing
    */
   class L1EventBuilder {
   public:
       /**
        * @brief Load channel settings from JSON configuration
        * 
        * @param fileName Path to the channel settings JSON file
        * @throws FileException if file cannot be opened
        * @throws ConfigException if JSON is invalid
        * 
        * @code
        * L1EventBuilder builder;
        * builder.LoadChSettings("config/channels.json");
        * @endcode
        */
       void LoadChSettings(const std::string& fileName);
       
       /**
        * @brief Set the time window for event correlation
        * 
        * @param timeWindow Time window in nanoseconds
        * @pre timeWindow > 0
        * @post Event building will use the new time window
        */
       void SetTimeWindow(double timeWindow);
   };
   ```

4. **Create Integration Tests**
   ```cpp
   // tests/IntegrationTest.cpp
   class EventProcessingIntegrationTest : public ::testing::Test {
   protected:
       void SetUp() override {
           // Create test data files
           CreateTestROOTFiles();
       }
       
       void TearDown() override {
           // Clean up test files
           CleanupTestFiles();
       }
   };
   
   TEST_F(EventProcessingIntegrationTest, FullProcessingPipeline) {
       // Time alignment
       TimeAlignment timeAlign;
       timeAlign.LoadChSettings("test_config.json");
       timeAlign.LoadFileList(GetTestFiles());
       timeAlign.FillHistograms(1);
       timeAlign.CalculateTimeAlignment();
       
       // L1 building
       L1EventBuilder l1Builder;
       l1Builder.LoadChSettings("test_config.json");
       l1Builder.LoadTimeSettings("timeSettings.json");
       l1Builder.LoadFileList(GetTestFiles());
       l1Builder.BuildEvent(1);
       
       // Verify output
       ASSERT_TRUE(std::filesystem::exists("L1_test.root"));
   }
   ```

5. **Performance Benchmarks**
   ```cpp
   // benchmarks/EventBuilderBenchmark.cpp
   #include <benchmark/benchmark.h>
   
   static void BM_EventBuilding(benchmark::State& state) {
       L1EventBuilder builder;
       builder.LoadChSettings("config.json");
       auto files = GetBenchmarkFiles();
       
       for (auto _ : state) {
           builder.LoadFileList(files);
           builder.BuildEvent(state.range(0));  // Thread count
       }
       
       state.SetItemsProcessed(state.iterations() * files.size());
   }
   
   BENCHMARK(BM_EventBuilding)->Range(1, 16)->UseRealTime();
   ```

6. **Architecture Documentation**
   ```markdown
   # ELIFANT-Event Architecture
   
   ## Overview
   The system follows a three-stage processing pipeline:
   
   1. **Time Alignment**: Calculates time offsets between detectors
   2. **L1 Event Building**: First-level trigger based on time correlation
   3. **L2 Event Building**: Second-level selection with complex conditions
   
   ## Data Flow
   ```mermaid
   graph LR
       A[Raw Data Files] --> B[Time Alignment]
       B --> C[Time Settings]
       A --> D[L1 Event Builder]
       C --> D
       D --> E[L1 Events]
       E --> F[L2 Event Builder]
       F --> G[Final Events]
   ```
   
   ## Threading Model
   - Each stage uses thread pool for parallel file processing
   - Thread-safe file list management with mutex protection
   - Local buffers per thread to minimize contention
   ```

### Action Items

- [ ] Integrate Google Test framework
- [ ] Write unit tests for all public interfaces
- [ ] Add Doxygen configuration and generate docs
- [ ] Create integration test suite
- [ ] Set up continuous integration with test runs
- [ ] Add code coverage reporting
- [ ] Write architecture documentation
- [ ] Create user guide with examples
- [ ] Add performance benchmarks
- [ ] Document threading model and safety guarantees