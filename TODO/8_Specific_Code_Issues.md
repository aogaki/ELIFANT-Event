# Specific Code Issues

## Priority: LOW

### Issues Identified

1. **Code Duplication**
   - `GetFileList` function duplicated in main.cpp and L2EventBuilder.cpp
   - Similar file handling patterns repeated across classes

2. **Inconsistent Naming Conventions**
   - Mix of camelCase and PascalCase for variables
   - Some members prefixed with 'f', others not
   - Inconsistent parameter naming

3. **Hardcoded File Paths**
   - Fixed filenames like "timeAlignment.root"
   - No configuration for output paths
   - Hardcoded settings file names

4. **No Input Validation**
   - Missing bounds checking for array accesses
   - No validation of module/channel indices
   - No range checks for numeric inputs

5. **Direct Console Output**
   - Using std::cout/std::cerr directly
   - No logging framework
   - No log levels or filtering

### Recommended Solutions

1. **Consolidate Duplicate Code**
   ```cpp
   // Create FileUtils.hpp/cpp
   namespace DELILA::FileUtils {
       std::vector<std::string> GetFileList(const std::string& directory,
                                           uint32_t runNumber,
                                           uint32_t startVersion,
                                           uint32_t endVersion) {
           // Single implementation
       }
       
       std::vector<std::string> FindROOTFiles(const std::string& directory,
                                              const std::string& pattern) {
           // Reusable file finding logic
       }
   }
   ```

2. **Establish Consistent Naming Convention**
   ```cpp
   // Document and follow consistently:
   class EventBuilder {
   private:
       // Member variables: prefix with 'm_'
       std::vector<ChSettings> m_channelSettings;
       double m_timeWindow;
       
       // Static members: prefix with 's_'
       static constexpr size_t s_defaultBufferSize = 1024;
       
   public:
       // Methods: PascalCase for public, camelCase for private
       void ProcessEvent(const EventData& data);
       
   private:
       void validateInput(const EventData& data);
   };
   ```

3. **Configuration Management**
   ```cpp
   class PathConfiguration {
   private:
       std::string m_outputDirectory;
       std::string m_timeAlignmentFile;
       std::string m_settingsFile;
       
   public:
       PathConfiguration() {
           // Load from config file or use defaults
           LoadFromFile("paths.json");
       }
       
       [[nodiscard]] std::string GetTimeAlignmentPath() const {
           return m_outputDirectory + "/" + m_timeAlignmentFile;
       }
       
       void SetOutputDirectory(const std::string& dir) {
           if (!std::filesystem::exists(dir)) {
               std::filesystem::create_directories(dir);
           }
           m_outputDirectory = dir;
       }
   };
   ```

4. **Add Input Validation**
   ```cpp
   class SafeChannelAccess {
   public:
       [[nodiscard]] bool IsValidChannel(uint8_t mod, uint8_t ch) const {
           return mod < fChSettingsVec.size() && 
                  ch < fChSettingsVec[mod].size();
       }
       
       [[nodiscard]] std::optional<ChSettings> GetSettings(uint8_t mod, uint8_t ch) const {
           if (!IsValidChannel(mod, ch)) {
               LOG_ERROR("Invalid channel access: mod={}, ch={}", mod, ch);
               return std::nullopt;
           }
           return fChSettingsVec[mod][ch];
       }
       
       void ValidateTimeWindow(double window) {
           if (window <= 0 || window > MAX_TIME_WINDOW) {
               throw std::invalid_argument("Time window out of range");
           }
       }
   };
   ```

5. **Implement Logging Framework**
   ```cpp
   // Simple logging interface
   enum class LogLevel {
       Debug = 0,
       Info = 1,
       Warning = 2,
       Error = 3
   };
   
   class Logger {
   private:
       static LogLevel s_currentLevel;
       static std::ofstream s_logFile;
       
   public:
       template<typename... Args>
       static void Log(LogLevel level, const std::string& format, Args... args) {
           if (level < s_currentLevel) return;
           
           auto timestamp = GetTimestamp();
           auto message = fmt::format(format, args...);
           
           // Log to file and console
           s_logFile << timestamp << " [" << ToString(level) << "] " << message << std::endl;
           if (level >= LogLevel::Warning) {
               std::cerr << timestamp << " [" << ToString(level) << "] " << message << std::endl;
           }
       }
   };
   
   // Macros for convenience
   #define LOG_DEBUG(...) Logger::Log(LogLevel::Debug, __VA_ARGS__)
   #define LOG_INFO(...) Logger::Log(LogLevel::Info, __VA_ARGS__)
   #define LOG_WARNING(...) Logger::Log(LogLevel::Warning, __VA_ARGS__)
   #define LOG_ERROR(...) Logger::Log(LogLevel::Error, __VA_ARGS__)
   ```

6. **Remove Form() Usage**
   ```cpp
   // Replace ROOT's Form() with std::format (C++20) or fmt::format
   // Before:
   std::string searchKey = Form("run%04d_%04d_", runNumber, i);
   
   // After (C++20):
   std::string searchKey = std::format("run{:04d}_{:04d}_", runNumber, i);
   
   // Or with fmt library:
   std::string searchKey = fmt::format("run{:04d}_{:04d}_", runNumber, i);
   ```

### Action Items

- [ ] Create FileUtils module to eliminate duplication
- [ ] Document and enforce naming conventions
- [ ] Implement configuration management system
- [ ] Add comprehensive input validation
- [ ] Replace console output with logging framework
- [ ] Create path configuration system
- [ ] Add bounds checking to all array/vector access
- [ ] Replace Form() with modern string formatting
- [ ] Add validation unit tests