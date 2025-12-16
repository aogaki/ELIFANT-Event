#include <gtest/gtest.h>

#include "EventData.hpp"
#include "L1EventBuilder.hpp"
#include "L2EventBuilder.hpp"
#include "TimeAlignment.hpp"

#include <algorithm>
#include <chrono>
#include <iomanip>
#include <random>
#include <thread>

using namespace DELILA;

// Helper to print benchmark results
void PrintBenchmark(const std::string& name, double timeMs, int iterations,
                    const std::string& unit = "ops") {
  std::cout << std::setw(50) << std::left << name << ": "
            << std::setw(10) << std::right << std::fixed << std::setprecision(3)
            << timeMs << " ms";

  if (iterations > 0) {
    double perOp = (timeMs * 1000.0) / iterations; // Convert to microseconds
    std::cout << " | " << std::setw(8) << std::fixed << std::setprecision(3)
              << perOp << " μs/" << unit;

    double throughput = (iterations * 1000.0) / timeMs; // ops per second
    std::cout << " | " << std::setw(10) << std::fixed << std::setprecision(0)
              << throughput << " " << unit << "/s";
  }
  std::cout << std::endl;
}

//=============================================================================
// EventData Performance Benchmarks
//=============================================================================

class EventDataBenchmark : public ::testing::Test {
 protected:
  void SetUp() override { std::cout << "\n=== EventData Benchmarks ===" << std::endl; }
};

TEST_F(EventDataBenchmark, CreateSmallEvents_10K) {
  const int numEvents = 10000;
  const int dataPerEvent = 10;

  auto start = std::chrono::high_resolution_clock::now();

  std::vector<EventData> events;
  events.reserve(numEvents);

  for (int i = 0; i < numEvents; i++) {
    events.emplace_back();
    events.back().triggerTime = i * 0.001;
    events.back().eventDataVec->reserve(dataPerEvent);

    for (int j = 0; j < dataPerEvent; j++) {
      events.back().eventDataVec->emplace_back(
          true, j % 4, j % 16, j * 100, j * 50, j * 0.1);
    }
  }

  auto end = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

  PrintBenchmark("Create 10K small events (10 hits each)",
                 duration.count() / 1000.0, numEvents, "evt");

  EXPECT_EQ(events.size(), numEvents);
}

TEST_F(EventDataBenchmark, CreateMediumEvents_10K) {
  const int numEvents = 10000;
  const int dataPerEvent = 100;

  auto start = std::chrono::high_resolution_clock::now();

  std::vector<EventData> events;
  events.reserve(numEvents);

  for (int i = 0; i < numEvents; i++) {
    events.emplace_back();
    events.back().triggerTime = i * 0.001;
    events.back().eventDataVec->reserve(dataPerEvent);

    for (int j = 0; j < dataPerEvent; j++) {
      events.back().eventDataVec->emplace_back(
          true, j % 8, j % 16, j * 10, j * 5, j * 0.01);
    }
  }

  auto end = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

  PrintBenchmark("Create 10K medium events (100 hits each)",
                 duration.count() / 1000.0, numEvents, "evt");

  EXPECT_EQ(events.size(), numEvents);
}

TEST_F(EventDataBenchmark, CreateLargeEvents_1K) {
  const int numEvents = 1000;
  const int dataPerEvent = 1000;

  auto start = std::chrono::high_resolution_clock::now();

  std::vector<EventData> events;
  events.reserve(numEvents);

  for (int i = 0; i < numEvents; i++) {
    events.emplace_back();
    events.back().eventDataVec->reserve(dataPerEvent);
    events.back().triggerTime = i * 0.001;

    for (int j = 0; j < dataPerEvent; j++) {
      events.back().eventDataVec->emplace_back(
          true, j % 256, j % 256, j, j / 2, j * 0.001);
    }
  }

  auto end = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

  PrintBenchmark("Create 1K large events (1000 hits each)",
                 duration.count(), numEvents, "evt");

  EXPECT_EQ(events.size(), numEvents);
}

TEST_F(EventDataBenchmark, CreateHugeEvents_100) {
  const int numEvents = 100;
  const int dataPerEvent = 10000;

  auto start = std::chrono::high_resolution_clock::now();

  std::vector<EventData> events;
  events.reserve(numEvents);

  for (int i = 0; i < numEvents; i++) {
    events.emplace_back();
    events.back().eventDataVec->reserve(dataPerEvent);
    events.back().triggerTime = i * 0.001;

    for (int j = 0; j < dataPerEvent; j++) {
      events.back().eventDataVec->emplace_back(
          true, j % 256, j % 256, j, j / 2, j * 0.0001);
    }
  }

  auto end = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

  PrintBenchmark("Create 100 huge events (10K hits each)",
                 duration.count(), numEvents, "evt");

  EXPECT_EQ(events.size(), numEvents);
}

TEST_F(EventDataBenchmark, CopyVsMoveVsPushback_1K) {
  EventData template_event;
  for (int i = 0; i < 1000; i++) {
    template_event.eventDataVec->push_back(RawData_t(true, 0, i % 16, i, i, i));
  }

  const int iterations = 1000;

  // Benchmark copy
  auto copyStart = std::chrono::high_resolution_clock::now();
  std::vector<EventData> copied;
  copied.reserve(iterations);
  for (int i = 0; i < iterations; i++) {
    copied.push_back(template_event);
  }
  auto copyEnd = std::chrono::high_resolution_clock::now();
  auto copyDuration = std::chrono::duration_cast<std::chrono::microseconds>(copyEnd - copyStart);

  // Benchmark move
  auto moveStart = std::chrono::high_resolution_clock::now();
  std::vector<EventData> moved;
  moved.reserve(iterations);
  for (int i = 0; i < iterations; i++) {
    EventData temp = template_event; // Copy first
    moved.push_back(std::move(temp));
  }
  auto moveEnd = std::chrono::high_resolution_clock::now();
  auto moveDuration = std::chrono::duration_cast<std::chrono::microseconds>(moveEnd - moveStart);

  // Benchmark emplace_back
  auto emplaceStart = std::chrono::high_resolution_clock::now();
  std::vector<EventData> emplaced;
  emplaced.reserve(iterations);
  for (int i = 0; i < iterations; i++) {
    emplaced.emplace_back(template_event);
  }
  auto emplaceEnd = std::chrono::high_resolution_clock::now();
  auto emplaceDuration = std::chrono::duration_cast<std::chrono::microseconds>(emplaceEnd - emplaceStart);

  PrintBenchmark("Copy 1K events (1K hits each)", copyDuration.count() / 1000.0, iterations, "evt");
  PrintBenchmark("Move 1K events (1K hits each)", moveDuration.count() / 1000.0, iterations, "evt");
  PrintBenchmark("Emplace 1K events (1K hits each)", emplaceDuration.count() / 1000.0, iterations, "evt");

  double speedup = (double)copyDuration.count() / moveDuration.count();
  std::cout << "  → Move is " << std::fixed << std::setprecision(2) << speedup << "x faster than copy" << std::endl;
}

TEST_F(EventDataBenchmark, ClearOperation_10K) {
  const int iterations = 10000;
  EventData event;
  event.eventDataVec->reserve(100);

  auto start = std::chrono::high_resolution_clock::now();

  for (int i = 0; i < iterations; i++) {
    event.triggerTime = i;
    for (int j = 0; j < 100; j++) {
      event.eventDataVec->emplace_back(true, 0, j, j, j, j);
    }
    event.Clear();
  }

  auto end = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

  PrintBenchmark("Fill & clear 10K times (100 hits)", duration.count(), iterations, "cycles");
}

TEST_F(EventDataBenchmark, ReserveVsNoReserve_10K) {
  const int N = 10000;

  // Without reserve
  auto start1 = std::chrono::high_resolution_clock::now();
  {
    EventData event;
    for (int i = 0; i < N; i++) {
      event.eventDataVec->push_back(RawData_t(true, 0, i % 16, i, i, i));
    }
  }
  auto end1 = std::chrono::high_resolution_clock::now();
  auto duration1 = std::chrono::duration_cast<std::chrono::microseconds>(end1 - start1);

  // With reserve
  auto start2 = std::chrono::high_resolution_clock::now();
  {
    EventData event;
    event.eventDataVec->reserve(N);
    for (int i = 0; i < N; i++) {
      event.eventDataVec->push_back(RawData_t(true, 0, i % 16, i, i, i));
    }
  }
  auto end2 = std::chrono::high_resolution_clock::now();
  auto duration2 = std::chrono::duration_cast<std::chrono::microseconds>(end2 - start2);

  PrintBenchmark("Push 10K hits WITHOUT reserve", duration1.count() / 1000.0, N, "hits");
  PrintBenchmark("Push 10K hits WITH reserve", duration2.count() / 1000.0, N, "hits");

  double speedup = (double)duration1.count() / duration2.count();
  std::cout << "  → reserve() is " << std::fixed << std::setprecision(2) << speedup << "x faster" << std::endl;
}

TEST_F(EventDataBenchmark, RandomAccessPattern_100K) {
  const int numEvents = 1000;
  const int hitsPerEvent = 100;

  // Create events
  std::vector<EventData> events;
  events.reserve(numEvents);
  for (int i = 0; i < numEvents; i++) {
    events.emplace_back();
    events.back().eventDataVec->reserve(hitsPerEvent);
    events.back().triggerTime = i;
    for (int j = 0; j < hitsPerEvent; j++) {
      events.back().eventDataVec->emplace_back(true, j % 8, j % 16, j, j, j * 0.1);
    }
  }

  // Random access benchmark
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> eventDist(0, numEvents - 1);
  std::uniform_int_distribution<> hitDist(0, hitsPerEvent - 1);

  const int numAccesses = 100000;
  uint64_t checksum = 0;

  auto start = std::chrono::high_resolution_clock::now();

  for (int i = 0; i < numAccesses; i++) {
    int evtIdx = eventDist(gen);
    int hitIdx = hitDist(gen);
    checksum += (*events[evtIdx].eventDataVec)[hitIdx].chargeLong;
  }

  auto end = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

  PrintBenchmark("Random access 100K hits", duration.count() / 1000.0, numAccesses, "access");

  EXPECT_GT(checksum, 0); // Prevent optimization
}

//=============================================================================
// Builder Performance Benchmarks
//=============================================================================

class BuilderBenchmark : public ::testing::Test {
 protected:
  void SetUp() override { std::cout << "\n=== Builder Benchmarks ===" << std::endl; }
};

TEST_F(BuilderBenchmark, L1Constructor_10K) {
  const int iterations = 10000;

  auto start = std::chrono::high_resolution_clock::now();

  for (int i = 0; i < iterations; i++) {
    L1EventBuilder builder;
  }

  auto end = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

  PrintBenchmark("L1EventBuilder construction", duration.count() / 1000.0, iterations, "obj");
}

TEST_F(BuilderBenchmark, L2Constructor_10K) {
  const int iterations = 10000;

  auto start = std::chrono::high_resolution_clock::now();

  for (int i = 0; i < iterations; i++) {
    L2EventBuilder builder;
  }

  auto end = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

  PrintBenchmark("L2EventBuilder construction", duration.count() / 1000.0, iterations, "obj");
}

TEST_F(BuilderBenchmark, TimeAlignmentConstructor_10K) {
  const int iterations = 10000;

  auto start = std::chrono::high_resolution_clock::now();

  for (int i = 0; i < iterations; i++) {
    TimeAlignment alignment;
  }

  auto end = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

  PrintBenchmark("TimeAlignment construction", duration.count() / 1000.0, iterations, "obj");
}

TEST_F(BuilderBenchmark, SetParameters_1M) {
  const int iterations = 1000000;
  L1EventBuilder builder;

  auto start = std::chrono::high_resolution_clock::now();

  for (int i = 0; i < iterations; i++) {
    builder.SetTimeWindow(i * 0.1);
    builder.SetCoincidenceWindow(i * 0.01);
    builder.SetRefMod(i % 256);
    builder.SetRefCh(i % 256);
  }

  auto end = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

  PrintBenchmark("Set 4 parameters 1M times", duration.count(), iterations, "ops");
}

TEST_F(BuilderBenchmark, LoadFileList_10K) {
  const int iterations = 10000;
  L1EventBuilder builder;

  std::vector<std::string> fileList;
  fileList.reserve(100);
  for (int i = 0; i < 100; i++) {
    fileList.push_back("file_" + std::to_string(i) + ".root");
  }

  auto start = std::chrono::high_resolution_clock::now();

  for (int i = 0; i < iterations; i++) {
    builder.LoadFileList(fileList);
  }

  auto end = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

  PrintBenchmark("Load file list (100 files) 10K times", duration.count(), iterations, "loads");
}

TEST_F(BuilderBenchmark, ConcurrentBuilderCreation) {
  const int numThreads = std::thread::hardware_concurrency();
  const int buildersPerThread = 1000;

  auto start = std::chrono::high_resolution_clock::now();

  std::vector<std::thread> threads;
  threads.reserve(numThreads);

  for (int t = 0; t < numThreads; t++) {
    threads.emplace_back([buildersPerThread]() {
      for (int i = 0; i < buildersPerThread; i++) {
        L1EventBuilder builder;
        builder.SetTimeWindow(i * 10.0);
      }
    });
  }

  for (auto &t : threads) {
    t.join();
  }

  auto end = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

  int totalBuilders = numThreads * buildersPerThread;
  PrintBenchmark("Concurrent builder creation (" + std::to_string(numThreads) + " threads)",
                 duration.count(), totalBuilders, "obj");
}

//=============================================================================
// Memory & Throughput Benchmarks
//=============================================================================

class ThroughputBenchmark : public ::testing::Test {
 protected:
  void SetUp() override { std::cout << "\n=== Throughput Benchmarks ===" << std::endl; }
};

TEST_F(ThroughputBenchmark, MemoryFootprint) {
  const int numEvents = 100000;
  const int hitsPerEvent = 10;

  std::vector<EventData> events;
  events.reserve(numEvents);

  for (int i = 0; i < numEvents; i++) {
    events.emplace_back();
    events.back().eventDataVec->reserve(hitsPerEvent);
    for (int j = 0; j < hitsPerEvent; j++) {
      events.back().eventDataVec->emplace_back(true, 0, j, 100, 50, 1.0);
    }
  }

  size_t approxMemory = numEvents * (sizeof(EventData) + hitsPerEvent * sizeof(RawData_t));
  double memoryMB = approxMemory / (1024.0 * 1024.0);

  std::cout << std::setw(50) << std::left << "Memory for 100K events (10 hits each)"
            << ": ~" << std::fixed << std::setprecision(2) << memoryMB << " MB" << std::endl;

  EXPECT_EQ(events.size(), numEvents);
}

TEST_F(ThroughputBenchmark, ParallelEventCreation_MultiThread) {
  const int numThreads = std::thread::hardware_concurrency();
  const int eventsPerThread = 10000;
  const int hitsPerEvent = 50;

  auto start = std::chrono::high_resolution_clock::now();

  std::vector<std::thread> threads;
  std::vector<std::vector<EventData>> threadEvents(numThreads);

  for (int t = 0; t < numThreads; t++) {
    threads.emplace_back([&threadEvents, t, eventsPerThread, hitsPerEvent]() {
      threadEvents[t].reserve(eventsPerThread);
      for (int i = 0; i < eventsPerThread; i++) {
        threadEvents[t].emplace_back();
        threadEvents[t].back().eventDataVec->reserve(hitsPerEvent);
        threadEvents[t].back().triggerTime = i;
        for (int j = 0; j < hitsPerEvent; j++) {
          threadEvents[t].back().eventDataVec->emplace_back(
              true, t, j, i * j, i, j * 0.1);
        }
      }
    });
  }

  for (auto &t : threads) {
    t.join();
  }

  auto end = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

  int totalEvents = numThreads * eventsPerThread;
  PrintBenchmark("Parallel creation (" + std::to_string(numThreads) + " threads, 50 hits/evt)",
                 duration.count(), totalEvents, "evt");
}

TEST_F(ThroughputBenchmark, SustainedThroughput_1M_Hits) {
  const int totalHits = 1000000;
  const int eventsPerBatch = 100;
  const int hitsPerEvent = 100;
  const int numBatches = totalHits / (eventsPerBatch * hitsPerEvent);

  auto start = std::chrono::high_resolution_clock::now();

  for (int batch = 0; batch < numBatches; batch++) {
    std::vector<EventData> events;
    events.reserve(eventsPerBatch);

    for (int i = 0; i < eventsPerBatch; i++) {
      events.emplace_back();
      events.back().eventDataVec->reserve(hitsPerEvent);
      for (int j = 0; j < hitsPerEvent; j++) {
        events.back().eventDataVec->emplace_back(true, j % 8, j % 16, j, j, j);
      }
    }
  }

  auto end = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

  PrintBenchmark("Process 1M hits in batches", duration.count(), totalHits, "hits");
}

TEST_F(ThroughputBenchmark, DataCopyOverhead_Large) {
  const int numCopies = 100;
  const int hitsPerEvent = 10000;

  // Create a large event
  EventData largeEvent;
  largeEvent.eventDataVec->reserve(hitsPerEvent);
  for (int i = 0; i < hitsPerEvent; i++) {
    largeEvent.eventDataVec->emplace_back(true, i % 8, i % 16, i, i, i);
  }

  auto start = std::chrono::high_resolution_clock::now();

  std::vector<EventData> copies;
  copies.reserve(numCopies);
  for (int i = 0; i < numCopies; i++) {
    copies.push_back(largeEvent);
  }

  auto end = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

  PrintBenchmark("Copy large event (10K hits) 100 times", duration.count(), numCopies, "copies");

  size_t dataSize = hitsPerEvent * sizeof(RawData_t) * numCopies;
  double dataMB = dataSize / (1024.0 * 1024.0);
  double bandwidth = dataMB / (duration.count() / 1000.0); // MB/s

  std::cout << "  → Data copied: " << std::fixed << std::setprecision(2) << dataMB
            << " MB | Bandwidth: " << bandwidth << " MB/s" << std::endl;
}
