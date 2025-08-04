# Code Organization and Architecture

## Priority: MEDIUM

### Issues Identified

1. **Large Classes with Multiple Responsibilities**
   - L1EventBuilder handles file I/O, event building, and thread management
   - L2EventBuilder mixes configuration, processing, and file operations
   - Violation of Single Responsibility Principle

2. **Mixed Concerns**
   - Business logic intertwined with I/O operations
   - Configuration parsing mixed with processing logic
   - No clear separation of layers

3. **Code Duplication**
   - GetFileList function duplicated in main.cpp and L2EventBuilder.cpp
   - Similar file handling patterns repeated across classes

4. **Poor Dependency Management**
   - Implementation headers included in header files
   - Circular dependency risks
   - Heavy header dependencies slow compilation

5. **Commented Dead Code**
   - Large commented block in ChSettings.hpp (lines 176-236)
   - Should be removed or moved to separate file

### Recommended Solutions

1. **Extract Interfaces and Separate Concerns**
   ```cpp
   // IEventBuilder.hpp
   class IEventBuilder {
   public:
       virtual ~IEventBuilder() = default;
       virtual void BuildEvent(uint32_t nThreads) = 0;
       virtual void LoadConfiguration(const std::string& configFile) = 0;
   };
   
   // IDataReader.hpp
   class IDataReader {
   public:
       virtual ~IDataReader() = default;
       virtual std::unique_ptr<EventData> ReadNext() = 0;
       virtual bool HasMore() const = 0;
   };
   
   // IEventProcessor.hpp
   class IEventProcessor {
   public:
       virtual ~IEventProcessor() = default;
       virtual void Process(const EventData& data) = 0;
   };
   ```

2. **Create Utility Classes**
   ```cpp
   // FileUtils.hpp
   namespace DELILA::Utils {
       std::vector<std::string> GetFileList(const std::string& directory,
                                           uint32_t runNumber,
                                           uint32_t startVersion,
                                           uint32_t endVersion);
       
       bool ValidateFile(const std::string& path);
   }
   
   // ConfigurationManager.hpp
   class ConfigurationManager {
   public:
       void LoadFromFile(const std::string& path);
       ChSettings GetChannelSettings(uint32_t mod, uint32_t ch) const;
       TimeSettings GetTimeSettings() const;
   };
   ```

3. **Implement Builder Pattern for Complex Objects**
   ```cpp
   class EventBuilderFactory {
   public:
       class Builder {
       public:
           Builder& WithConfig(const std::string& path);
           Builder& WithThreads(uint32_t count);
           Builder& WithTimeWindow(double window);
           std::unique_ptr<IEventBuilder> Build();
       };
       
       static Builder Create() { return Builder{}; }
   };
   ```

4. **Separate Processing Pipeline**
   ```cpp
   class ProcessingPipeline {
   private:
       std::vector<std::unique_ptr<IEventProcessor>> processors;
       
   public:
       void AddProcessor(std::unique_ptr<IEventProcessor> proc) {
           processors.push_back(std::move(proc));
       }
       
       void Process(const EventData& data) {
           for (auto& proc : processors) {
               proc->Process(data);
           }
       }
   };
   ```

5. **Move Implementation Details to Source Files**
   ```cpp
   // Instead of in header:
   void SetTimeWindow(double_t timeWindow) { fTimeWindow = timeWindow; }
   
   // In header:
   void SetTimeWindow(double_t timeWindow);
   
   // In source:
   void L1EventBuilder::SetTimeWindow(double_t timeWindow) {
       fTimeWindow = timeWindow;
   }
   ```

### Directory Structure Recommendation
```
ELIFANT-Event/
├── include/
│   ├── Core/
│   │   ├── EventData.hpp
│   │   ├── ChSettings.hpp
│   │   └── Interfaces/
│   │       ├── IEventBuilder.hpp
│   │       └── IDataReader.hpp
│   ├── EventBuilders/
│   │   ├── L1EventBuilder.hpp
│   │   └── L2EventBuilder.hpp
│   ├── Utils/
│   │   ├── FileUtils.hpp
│   │   └── ConfigManager.hpp
│   └── Processing/
│       └── Pipeline.hpp
├── src/
│   └── [matching structure]
└── tests/
    └── [unit tests]
```

### Action Items

- [ ] Define clear interfaces for major components
- [ ] Extract utility functions to separate namespace/classes
- [ ] Implement dependency injection for better testability
- [ ] Remove commented dead code
- [ ] Create factory classes for complex object creation
- [ ] Separate I/O operations from business logic
- [ ] Move implementation to source files
- [ ] Reorganize directory structure
- [ ] Add unit tests for each component