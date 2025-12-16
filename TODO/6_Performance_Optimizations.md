# Performance Optimizations

## Priority: MEDIUM

### Issues Identified

1. **Unnecessary Copies**
   - String parameters passed by value in some functions
   - Vector copies in loops
   - EventData copied unnecessarily

2. **Inefficient Container Usage**
   - Deep nested vectors: `std::vector<std::vector<std::vector<std::vector<double_t>>>>`
   - No reserve() calls before known insertions
   - Frequent reallocations possible

3. **Missing Move Semantics**
   - No custom move constructors where beneficial
   - Not using std::move for temporary objects
   - Return by value without RVO optimization

4. **Repeated Lookups and Calculations**
   - No caching of frequently accessed data
   - Redundant calculations in loops
   - Multiple map/vector lookups for same key

5. **Suboptimal Threading**
   - No work stealing between threads
   - Fixed thread pool size regardless of workload
   - Potential false sharing in shared data

### Recommended Solutions

1. **Pass by Const Reference**
   ```cpp
   // Before:
   void LoadChSettings(std::string fileName);
   
   // After:
   void LoadChSettings(const std::string& fileName);
   ```

2. **Reserve Container Capacity**
   ```cpp
   void LoadFileList(const std::vector<std::string>& files) {
       fFileList.reserve(files.size());  // Avoid reallocations
       for (const auto& file : files) {
           fFileList.push_back(file);
       }
   }
   
   // Or better:
   void LoadFileList(std::vector<std::string> files) {
       fFileList = std::move(files);  // Move entire vector
   }
   ```

3. **Optimize Nested Container Access**
   ```cpp
   // Instead of deep nested vectors
   class TimeSettingsOptimized {
   private:
       struct Key {
           uint8_t refMod, refCh, mod, ch;
           bool operator<(const Key& other) const {
               return std::tie(refMod, refCh, mod, ch) < 
                      std::tie(other.refMod, other.refCh, other.mod, other.ch);
           }
       };
       std::map<Key, double> timeOffsets;
       
   public:
       double GetOffset(uint8_t refMod, uint8_t refCh, 
                       uint8_t mod, uint8_t ch) const {
           auto it = timeOffsets.find({refMod, refCh, mod, ch});
           return it != timeOffsets.end() ? it->second : 0.0;
       }
   };
   ```

4. **Implement Move Semantics**
   ```cpp
   class EventData {
   public:
       EventData(EventData&& other) noexcept
           : triggerTime(other.triggerTime),
             eventDataVec(std::move(other.eventDataVec)) {
           other.triggerTime = 0;
       }
       
       EventData& operator=(EventData&& other) noexcept {
           if (this != &other) {
               triggerTime = other.triggerTime;
               eventDataVec = std::move(other.eventDataVec);
               other.triggerTime = 0;
           }
           return *this;
       }
   };
   ```

5. **Cache Frequently Used Data**
   ```cpp
   class ChSettingsCache {
   private:
       mutable std::unordered_map<uint32_t, ChSettings> cache;
       
       static uint32_t MakeKey(uint8_t mod, uint8_t ch) {
           return (uint32_t(mod) << 8) | ch;
       }
       
   public:
       const ChSettings& Get(uint8_t mod, uint8_t ch) const {
           auto key = MakeKey(mod, ch);
           auto it = cache.find(key);
           if (it != cache.end()) {
               return it->second;
           }
           // Load and cache
           return cache[key] = LoadSettings(mod, ch);
       }
   };
   ```

6. **Use String Views for Read-Only Access**
   ```cpp
   void ProcessFileName(std::string_view filename) {
       // No copy made, just a view
       if (filename.find("run") != std::string_view::npos) {
           // Process
       }
   }
   ```

7. **Optimize Thread Work Distribution**
   ```cpp
   class WorkStealingThreadPool {
   private:
       std::vector<std::deque<std::function<void()>>> queues;
       std::vector<std::mutex> queueMutexes;
       
       bool TryStealWork(size_t thiefId, std::function<void()>& task);
       
   public:
       void Submit(std::function<void()> task);
       void RunWorker(size_t workerId);
   };
   ```

### Action Items

- [ ] Audit all function parameters for pass-by-value issues
- [ ] Add reserve() calls for vectors with known sizes
- [ ] Implement move constructors/assignments where beneficial
- [ ] Replace nested vectors with flat maps where appropriate
- [ ] Add caching layer for frequently accessed data
- [ ] Use string_view for read-only string operations
- [ ] Profile code to identify actual bottlenecks
- [ ] Consider memory pooling for frequent allocations
- [ ] Optimize thread work distribution
- [ ] Add performance benchmarks

### ⚠️ KISS Principle Reminder

When optimizing performance:
- **ALWAYS profile first**: Don't optimize what you haven't measured
- **Fix the right problem**: 90% of time is spent in 10% of code - find that 10%
- **Readable beats fast**: Premature optimization makes code hard to maintain
- **Simple wins**: Often a simple algorithm change beats micro-optimizations
- **Question complexity**: Work-stealing thread pools sound cool, but do you really need them?
- **Measure everything**: Add benchmarks before and after - did it actually help?