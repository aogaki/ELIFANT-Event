# Resource Management and RAII Compliance

## Priority: MEDIUM

### Issues Identified

1. **Manual File Handle Management**
   - ROOT TFile objects managed with new/delete
   - No automatic cleanup on exceptions
   - File close operations scattered throughout code

2. **Missing RAII Wrappers**
   - No custom RAII classes for ROOT resources
   - Thread management without RAII
   - Mutex locking without lock guards in some places

3. **Resource Acquisition Without Guards**
   - Files opened without ensuring closure
   - Threads created without join guarantees
   - Memory allocated without smart pointers

### Recommended Solutions

1. **Create RAII Wrapper for ROOT Files**
   ```cpp
   namespace DELILA {
   class TFileRAII {
   private:
       std::unique_ptr<TFile> file;
       
   public:
       explicit TFileRAII(const std::string& name, const char* option = "READ") {
           file.reset(TFile::Open(name.c_str(), option));
           if (!file || file->IsZombie()) {
               throw FileException("Failed to open ROOT file: " + name);
           }
       }
       
       TFile* operator->() { return file.get(); }
       const TFile* operator->() const { return file.get(); }
       TFile& operator*() { return *file; }
       const TFile& operator*() const { return *file; }
       
       void Write() { if (file) file->Write(); }
       bool IsValid() const { return file && !file->IsZombie(); }
   };
   }
   ```

2. **Thread Pool with RAII**
   ```cpp
   class ThreadPoolRAII {
   private:
       std::vector<std::thread> threads;
       std::atomic<bool> shouldStop{false};
       
   public:
       template<typename Func>
       void AddThread(Func&& func) {
           threads.emplace_back(std::forward<Func>(func));
       }
       
       void Stop() { shouldStop = true; }
       
       ~ThreadPoolRAII() {
           Stop();
           for (auto& t : threads) {
               if (t.joinable()) t.join();
           }
       }
   };
   ```

3. **Scoped Timer for Performance Measurement**
   ```cpp
   class ScopedTimer {
   private:
       std::string name;
       std::chrono::high_resolution_clock::time_point start;
       
   public:
       explicit ScopedTimer(const std::string& name) 
           : name(name), start(std::chrono::high_resolution_clock::now()) {}
           
       ~ScopedTimer() {
           auto end = std::chrono::high_resolution_clock::now();
           auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
           std::cout << name << " took " << duration.count() << " ms" << std::endl;
       }
   };
   ```

4. **ROOT Object Manager**
   ```cpp
   template<typename T>
   class ROOTObjectRAII {
   private:
       std::unique_ptr<T> obj;
       
   public:
       template<typename... Args>
       explicit ROOTObjectRAII(Args&&... args) 
           : obj(std::make_unique<T>(std::forward<Args>(args)...)) {}
           
       T* operator->() { return obj.get(); }
       const T* operator->() const { return obj.get(); }
       T& operator*() { return *obj; }
       const T& operator*() const { return *obj; }
       
       T* release() { return obj.release(); }
       void reset(T* ptr = nullptr) { obj.reset(ptr); }
   };
   ```

5. **Lock Guard Usage**
   ```cpp
   void ProcessData() {
       // Instead of manual lock/unlock
       // fMutex.lock();
       // ... code ...
       // fMutex.unlock();
       
       // Use lock_guard
       {
           std::lock_guard<std::mutex> lock(fMutex);
           // ... critical section ...
       } // Automatic unlock
   }
   ```

### Action Items

- [ ] Implement RAII wrappers for all ROOT objects
- [ ] Replace manual new/delete with smart pointers
- [ ] Create thread pool class with automatic cleanup
- [ ] Add scoped timers for performance monitoring
- [ ] Use lock_guard/unique_lock consistently
- [ ] Implement file manager class with RAII
- [ ] Add resource leak detection in tests
- [ ] Document RAII patterns used in the project

### ⚠️ KISS Principle Reminder

When implementing RAII patterns:
- **Wrap what matters**: Not every resource needs a custom RAII class - use standard smart pointers first
- **One responsibility**: Each RAII wrapper should manage exactly one resource
- **Don't over-abstract**: A simple `std::unique_ptr<TFile>` is better than a complex custom wrapper
- **ROOT compatibility**: ROOT has its own ownership model - respect it, don't fight it
- **Progressive adoption**: Convert to RAII incrementally, starting with the most problematic areas