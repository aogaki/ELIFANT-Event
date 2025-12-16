# Modern C++ Best Practices

## Priority: MEDIUM

### Issues Identified

1. **Old-Style Type Definitions**
   - Using `typedef` instead of `using`
   - C-style casts instead of C++ casts
   - No type aliases for complex types

2. **Missing Modern Attributes**
   - No `[[nodiscard]]` on important return values
   - No `[[maybe_unused]]` for optional parameters
   - Missing `override` keyword for virtual functions

3. **Const Correctness Issues**
   - Many getter methods not marked const
   - Missing const on parameters
   - Mutable members not properly used

4. **No Use of Modern Language Features**
   - Not using structured bindings
   - No constexpr where applicable
   - Missing auto type deduction in appropriate places

5. **Magic Numbers and Hardcoded Values**
   - Hardcoded array sizes and limits
   - No named constants for configuration values
   - Magic numbers in algorithms

### Recommended Solutions

1. **Replace typedef with using**
   ```cpp
   // Before:
   typedef ChSettings ChSettings_t;
   typedef EventData EventData_t;
   
   // After:
   using ChSettings_t = ChSettings;
   using EventData_t = EventData;
   
   // For complex types:
   using TimeSettingsMap = std::unordered_map<uint32_t, TimeSettings>;
   using EventCallback = std::function<void(const EventData&)>;
   ```

2. **Add Modern Attributes**
   ```cpp
   class EventBuilder {
   public:
       [[nodiscard]] bool IsValid() const noexcept;
       [[nodiscard]] size_t GetEventCount() const noexcept;
       
       void Process([[maybe_unused]] int debugLevel = 0);
       
       // For virtual functions:
       void BuildEvent(uint32_t nThreads) override;
   };
   ```

3. **Improve Const Correctness**
   ```cpp
   class ChSettings {
   public:
       // Mark getters as const
       [[nodiscard]] bool isEventTrigger() const noexcept { return fIsEventTrigger; }
       [[nodiscard]] uint32_t GetModule() const noexcept { return mod; }
       
       // Const parameters
       void SetThreshold(const uint32_t threshold) noexcept { thresholdADC = threshold; }
       
       // Const member functions
       void Print() const;  // Should be const since it doesn't modify state
   };
   ```

4. **Use Structured Bindings**
   ```cpp
   // Before:
   for (const auto& pair : settingsMap) {
       auto key = pair.first;
       auto value = pair.second;
       // Use key and value
   }
   
   // After:
   for (const auto& [key, value] : settingsMap) {
       // Direct use of key and value
   }
   ```

5. **Replace Magic Numbers with Constants**
   ```cpp
   namespace DELILA::Constants {
       inline constexpr size_t MAX_MODULES = 16;
       inline constexpr size_t MAX_CHANNELS = 16;
       inline constexpr double DEFAULT_TIME_WINDOW = 1000.0;
       inline constexpr uint32_t DEFAULT_THRESHOLD = 50;
       inline constexpr size_t HISTOGRAM_BINS = 32000;
       inline constexpr int PROGRESS_UPDATE_INTERVAL = 1000;
   }
   ```

6. **Use Enum Classes**
   ```cpp
   // Already good in the code:
   enum class DetectorType { Unknown = 0, AC = 1, PMT = 2, HPGe = 3, Si = 4 };
   
   // Add more enum classes:
   enum class ProcessingStatus {
       Idle,
       Processing,
       Completed,
       Error
   };
   
   enum class LogLevel {
       Debug,
       Info,
       Warning,
       Error
   };
   ```

7. **Smart Type Aliases**
   ```cpp
   // Strong types for better type safety
   template<typename T, typename Tag>
   class StrongType {
       T value;
   public:
       explicit StrongType(T val) : value(val) {}
       T get() const { return value; }
   };
   
   using ModuleId = StrongType<uint8_t, struct ModuleIdTag>;
   using ChannelId = StrongType<uint8_t, struct ChannelIdTag>;
   ```

8. **Use std::optional for Optional Values**
   ```cpp
   [[nodiscard]] std::optional<double> GetTimeOffset(uint8_t mod, uint8_t ch) const {
       if (IsValidChannel(mod, ch)) {
           return fTimeOffsets[mod][ch];
       }
       return std::nullopt;
   }
   ```

9. **Proper Cast Usage**
   ```cpp
   // Before:
   int value = (int)doubleValue;
   
   // After:
   int value = static_cast<int>(doubleValue);
   
   // For polymorphic types:
   if (auto* derived = dynamic_cast<DerivedClass*>(base)) {
       // Use derived
   }
   ```

### Action Items

- [ ] Replace all typedef with using declarations
- [ ] Add [[nodiscard]] to functions returning important values
- [ ] Mark all non-modifying member functions as const
- [ ] Replace C-style casts with appropriate C++ casts
- [ ] Define named constants for all magic numbers
- [ ] Use structured bindings where appropriate
- [ ] Add override to all virtual function implementations
- [ ] Consider using std::optional for nullable returns
- [ ] Implement strong types for type safety
- [ ] Use constexpr for compile-time constants

### ⚠️ KISS Principle Reminder

When modernizing C++ code:
- **Modern doesn't mean complex**: `using` is simpler than `typedef`, but don't add complexity
- **[[nodiscard]] sparingly**: Only add it where ignoring the return is truly a bug
- **Don't overuse features**: Not every function needs `constexpr`, not every type needs to be strong-typed
- **Readable first**: `auto` can make code shorter but also harder to understand
- **Progressive updates**: Convert to modern C++ incrementally during normal maintenance
- **Keep it working**: Don't change working code just to make it "more modern"