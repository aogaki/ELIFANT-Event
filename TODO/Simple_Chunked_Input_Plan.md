# Simple Chunked Input Processing - Plan

**Date:** 2025-12-17
**Branch:** feature/simple-chunked-input
**Goal:** Reduce memory usage by processing input files in chunks

---

## Problem Analysis

### Root Cause Identified

User tested with **7 threads vs 14 threads**:
- **14 threads:** Memory problems, swap usage ❌
- **7 threads:** No problems ✅

**Memory calculation:**
```
48 GB / 7 threads ≈ 6.8 GB per thread
```

This matches exactly:
```
174M entries × 40 bytes × 87% selection ratio = 6.9 GB per file
```

**Conclusion:** Each thread loads the **entire input file** into memory before processing!

### Current Code Issue

**Location:** [src/L1EventBuilder.cpp:248-272](../src/L1EventBuilder.cpp#L248-L272)

```cpp
const auto nEntries = tree->GetEntries();  // 174M entries
std::vector<std::unique_ptr<RawData_t>> rawDataVec;
rawDataVec.reserve(nEntries);  // Reserve for ALL entries!

for (Long64_t iEve = 0; iEve < nEntries; iEve++) {
  tree->GetEntry(iEve);
  if (chargeLong > fChSettingsVec[mod][ch].thresholdADC) {
    rawDataVec.emplace_back(...);  // Accumulates ALL passing events
  }
}
// rawDataVec now has 151M events = 6.9 GB!

// THEN process all at once
std::sort(rawDataVec.begin(), rawDataVec.end(), ...);
// ... event building ...
```

**Problem:** Loads entire file before processing.

**With 14 threads:**
- 14 × 6.9 GB = 96 GB needed
- Only have 48 GB available
- Result: OOM / swap

**With 7 threads:**
- 7 × 6.9 GB = 48 GB needed
- Fits exactly (but at limit)

---

## Solution: Simple Chunked Input

### Strategy

Instead of:
1. Load ALL entries → huge rawDataVec (6.9 GB)
2. Sort all
3. Process all
4. Clear

Do:
1. Load CHUNK of entries → small rawDataVec (350 MB)
2. Sort chunk
3. Process chunk
4. Clear chunk
5. Repeat for next chunk

### Implementation Plan

**Key insight:** The event building logic needs a **time window** of events to look forward/backward for coincidences. We handle this with **overlapping chunks**.

```
File: 174M entries

Chunk 1: entries [0, 10M + overlap]
  → Process entries [0, 10M]
  → Keep overlap for next chunk

Chunk 2: entries [10M - overlap, 20M + overlap]
  → Merge with previous overlap
  → Process entries [10M, 20M]
  → Keep overlap for next chunk

Chunk 3: entries [20M - overlap, 30M + overlap]
  → Merge with previous overlap
  → Process entries [20M, 30M]
  → Keep overlap for next chunk

...
```

**Overlap size:** Based on coincidence window (e.g., 1000 ns = ~100 events at typical rate)

---

## Implementation Steps

### Step 1: Add Chunk Configuration

**File:** [include/L1EventBuilder.hpp](../include/L1EventBuilder.hpp)

Add private members:
```cpp
private:
  // Chunked processing configuration
  static constexpr Long64_t CHUNK_SIZE = 10000000;  // 10M entries per chunk
  static constexpr Long64_t OVERLAP_SIZE = 10000;    // 10k entries overlap
```

### Step 2: Refactor DataReader to Use Chunks

**File:** [src/L1EventBuilder.cpp](../src/L1EventBuilder.cpp)

**Current structure:**
```cpp
void DataReader(int threadID, std::vector<std::string> fileList) {
  // Create output tree

  for (each file) {
    // Load entire file
    std::vector<std::unique_ptr<RawData_t>> rawDataVec;
    for (all entries) {
      rawDataVec.emplace_back(...);
    }

    // Sort all
    std::sort(rawDataVec.begin(), rawDataVec.end(), ...);

    // Process all
    for (each rawData) {
      // Event building logic
    }

    rawDataVec.clear();
  }

  // Write output
}
```

**New structure:**
```cpp
void DataReader(int threadID, std::vector<std::string> fileList) {
  // Create output tree

  std::vector<std::unique_ptr<RawData_t>> overlapBuffer;  // Carry between chunks

  for (each file) {
    const Long64_t nEntries = tree->GetEntries();

    for (Long64_t chunkStart = 0; chunkStart < nEntries; chunkStart += CHUNK_SIZE) {
      Long64_t readStart = (chunkStart > OVERLAP_SIZE) ?
                           (chunkStart - OVERLAP_SIZE) : 0;
      Long64_t readEnd = std::min(nEntries, chunkStart + CHUNK_SIZE + OVERLAP_SIZE);

      // Load chunk
      std::vector<std::unique_ptr<RawData_t>> rawDataVec;
      for (Long64_t iEve = readStart; iEve < readEnd; iEve++) {
        tree->GetEntry(iEve);
        if (passes_threshold) {
          rawDataVec.emplace_back(...);
        }
      }

      // Merge with overlap from previous chunk
      if (!overlapBuffer.empty()) {
        rawDataVec.insert(rawDataVec.begin(),
                         std::make_move_iterator(overlapBuffer.begin()),
                         std::make_move_iterator(overlapBuffer.end()));
        overlapBuffer.clear();
      }

      // Sort merged data
      std::sort(rawDataVec.begin(), rawDataVec.end(), ...);

      // Process chunk (same logic as before)
      for (each rawData in processing range) {
        // Event building logic (unchanged)
      }

      // Save overlap for next chunk
      SaveOverlapForNextChunk(rawDataVec, overlapBuffer, chunkStart + CHUNK_SIZE);

      // Clear chunk
      rawDataVec.clear();
      rawDataVec.shrink_to_fit();
    }

    overlapBuffer.clear();  // Clear between files
  }

  // Write output
}
```

### Step 3: Implement Helper Function

**File:** [src/L1EventBuilder.cpp](../src/L1EventBuilder.cpp)

```cpp
void SaveOverlapForNextChunk(
    const std::vector<std::unique_ptr<RawData_t>>& rawDataVec,
    std::vector<std::unique_ptr<RawData_t>>& overlapBuffer,
    Long64_t nextChunkStart)
{
  overlapBuffer.clear();

  // Find events in last OVERLAP_SIZE range
  // These might be needed for coincidence with next chunk's events
  Long64_t overlapStartIdx = rawDataVec.size() > OVERLAP_SIZE ?
                             rawDataVec.size() - OVERLAP_SIZE : 0;

  for (Long64_t i = overlapStartIdx; i < rawDataVec.size(); i++) {
    overlapBuffer.push_back(
      std::make_unique<RawData_t>(*rawDataVec[i])  // Deep copy
    );
  }
}
```

---

## Memory Analysis

### Before Fix (Current Code)

**Per thread per file:**
```
174M entries × 40 bytes × 87% = 6.9 GB
```

**With 14 threads:**
```
14 threads × 6.9 GB = 96 GB needed
```

**Result:** OOM / swap ❌

### After Fix (Chunked Input)

**Per thread per chunk:**
```
10M entries × 40 bytes × 87% = 350 MB (input)
+ 10k entries overlap = 3.5 MB
= 353 MB per chunk
```

**With 14 threads:**
```
14 threads × 353 MB = 4.9 GB total
```

**Result:** Fits comfortably in 48 GB! ✅

**Reduction:** 96 GB → 4.9 GB = **95% memory reduction**

---

## Testing Plan

### Unit Test

Create test to verify chunked processing produces same results as full-file:

```cpp
TEST(ChunkedInput, ProducesSameResults) {
  // Generate test file with 100k events
  auto testFile = CreateTestFile(100000);

  // Process with old method (load all)
  auto results1 = ProcessFullFile(testFile);

  // Process with new method (chunks)
  auto results2 = ProcessChunked(testFile, 10000);  // 10k per chunk

  // Compare
  EXPECT_EQ(results1.size(), results2.size());
  for (size_t i = 0; i < results1.size(); i++) {
    EXPECT_NEAR(results1[i].triggerTime, results2[i].triggerTime, 1e-9);
    EXPECT_EQ(results1[i].eventDataVec.size(),
              results2[i].eventDataVec.size());
  }
}
```

### Integration Test

**Test with real data:**
```bash
# Small file (should work either way)
./eve-builder -l1 -i small_file.root -s settings.json -t timeSettings.json -n 8

# Large file (174M entries - this is the test!)
./eve-builder -l1 -i large_file.root -s settings.json -t timeSettings.json -n 14

# Monitor memory:
watch -n 1 'ps aux | grep eve-builder | grep -v grep | awk "{print \$6/1024\" MB\"}"'
```

**Expected memory pattern:**
```
Start: ~500 MB baseline
Chunk 1: ~5 GB peak (14 threads × 350 MB)
Chunk 2: ~5 GB peak (memory oscillates, doesn't grow)
Chunk 3: ~5 GB peak
...
```

---

## Key Differences from Previous Approach

### What We Tried Before (Failed)

1. **Focus:** Output tree memory management
2. **Complexity:** Multiple layers (memory analysis, config calculation, mode selection)
3. **Root cause:** Misidentified - thought output was the problem
4. **Result:** Complex code that didn't solve the real issue

### What We're Doing Now (Simple)

1. **Focus:** Input data loading
2. **Complexity:** Single change - chunk the input loop
3. **Root cause:** Correctly identified - input accumulation
4. **Result:** Simple, focused fix for the actual problem

---

## Implementation Checklist

- [ ] Add CHUNK_SIZE and OVERLAP_SIZE constants to header
- [ ] Modify DataReader to loop over chunks
- [ ] Implement chunk loading logic
- [ ] Implement overlap merging
- [ ] Implement overlap saving for next chunk
- [ ] Add shrink_to_fit() after clearing each chunk
- [ ] Test with small file (verify correctness)
- [ ] Test with large file (verify memory usage)
- [ ] Measure performance impact
- [ ] Update documentation

---

## Expected Impact

**Memory:**
- ✅ 95% reduction (96 GB → 4.9 GB)
- ✅ Can run with 14+ threads
- ✅ No swap usage
- ✅ No OOM errors

**Performance:**
- ⚠️ Slightly slower due to:
  - Multiple sort operations (per chunk instead of once)
  - Overlap copying between chunks
- Estimated: 5-10% slower
- **Worth it:** Enables processing that was impossible before

**Code Complexity:**
- ✅ Simple: ~50 lines of code change
- ✅ Focused: Single file modification
- ✅ Maintainable: Clear logic, well-commented

---

## Future Enhancements (Optional)

### 1. Adaptive Chunk Size

```cpp
// Adjust chunk size based on available memory
size_t availableMemory = GetAvailableMemory();
Long64_t chunkSize = (availableMemory / numThreads / 40) * 0.75;
chunkSize = std::min(chunkSize, 50000000LL);  // Cap at 50M
```

### 2. Progress Reporting

```cpp
std::cout << "Thread " << threadID
          << ": Processing chunk " << (chunkNum)
          << "/" << (totalChunks)
          << " (" << (100 * chunkNum / totalChunks) << "%)"
          << std::endl;
```

### 3. Parallel Chunk Processing

Process multiple chunks in parallel (advanced, not needed now).

---

**Status:** Ready to implement

This is a simple, focused solution to the real problem. Let's implement it step by step.
