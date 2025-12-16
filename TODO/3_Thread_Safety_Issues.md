# Thread Safety Issues

## Priority: HIGH

### Issues Identified

1. **Shared State Without Protection**
   - Multiple threads access file lists without synchronization
   - Histograms in TimeAlignment accessed by multiple threads
   - Event data structures shared between threads

2. **Non-Atomic Operations**
   - Thread progress counters not atomic
   - File index counters in multi-threaded loops
   - Event counters accessed concurrently

3. **Insufficient Mutex Coverage**
   - Limited mutex protection in critical sections
   - Potential race conditions in DataReader methods
   - No read-write locks for read-heavy operations

4. **Missing Thread-Safe Initialization**
   - Static initialization not guaranteed thread-safe
   - ROOT objects may not be thread-safe

### Recommended Solutions

1. **Use Atomic Variables for Counters**
   ```cpp
   class L1EventBuilder {
   private:
       std::atomic<size_t> fProcessedEvents{0};
       std::atomic<size_t> fCurrentFileIndex{0};
   };
   ```

2. **Protect Shared Resources with Proper Locking**
   ```cpp
   class TimeAlignment {
   private:
       mutable std::shared_mutex fHistogramMutex;
       
       void FillHistogram(int mod, int ch, double value) {
           std::unique_lock lock(fHistogramMutex);
           fHistoTime[mod][ch]->Fill(value);
       }
       
       void ReadHistogram(int mod, int ch) const {
           std::shared_lock lock(fHistogramMutex);
           // Read operations
       }
   };
   ```

3. **Thread-Safe File List Access**
   ```cpp
   class FileListManager {
   private:
       std::vector<std::string> fFileList;
       mutable std::mutex fMutex;
       std::atomic<size_t> fNextIndex{0};
       
   public:
       std::optional<std::string> GetNextFile() {
           size_t index = fNextIndex.fetch_add(1);
           std::lock_guard lock(fMutex);
           if (index < fFileList.size()) {
               return fFileList[index];
           }
           return std::nullopt;
       }
   };
   ```

4. **Use Thread-Local Storage for Worker Data**
   ```cpp
   void DataReader(int threadID) {
       thread_local std::vector<RawData_t> localBuffer;
       localBuffer.reserve(1000);  // Avoid repeated allocations
       
       // Process data using local buffer
       // Only lock when merging results
   }
   ```

5. **Implement Lock-Free Data Structures Where Possible**
   ```cpp
   template<typename T>
   class LockFreeQueue {
       std::atomic<Node*> head;
       std::atomic<Node*> tail;
   public:
       void enqueue(T item);
       std::optional<T> dequeue();
   };
   ```

### Action Items

- [ ] Replace all shared counters with std::atomic
- [ ] Add mutex protection to all shared data structures
- [ ] Use read-write locks for read-heavy operations
- [ ] Implement thread-safe file list management
- [ ] Add thread-local storage for worker thread data
- [ ] Review ROOT thread safety requirements
- [ ] Add thread sanitizer to build for testing
- [ ] Document thread safety guarantees for each class
- [ ] Consider using parallel STL algorithms where appropriate

### ⚠️ KISS Principle Reminder

When implementing thread safety improvements:
- **Start simple**: Use `std::mutex` before considering lock-free algorithms
- **Profile first**: Don't assume lock contention is a problem without measuring
- **Avoid complexity**: Lock-free data structures are hard to get right - only use when proven necessary
- **Document clearly**: Thread safety is already complex - make your code easy to understand
- **Incremental fixes**: Fix one race condition at a time rather than rewriting everything