# ELIFANT-Event Test Suite

Comprehensive test suite for the ELIFANT-Event event builder.

## Structure

```
tests/
├── CMakeLists.txt              # Master test configuration
├── unit/                       # Unit tests
│   ├── test_event_data.cpp     # EventData & RawData tests (25 tests)
│   ├── test_ch_settings.cpp    # ChSettings tests (30 tests)
│   ├── test_exceptions.cpp     # Exception hierarchy tests (40 tests)
│   ├── test_tfile_raii.cpp     # TFile RAII tests (35 tests)
│   └── test_builders.cpp       # Builder classes tests (50 tests)
├── integration/                # Integration tests
│   └── test_pipeline.cpp       # Full pipeline tests (20 tests)
└── benchmarks/                 # Performance benchmarks
    └── bench_performance.cpp   # Performance measurements (15 benchmarks)
```

## Building Tests

```bash
cd tests
mkdir build
cd build
cmake ..
make -j4
```

## Running Tests

### Run All Tests with Summary
```bash
ctest --output-on-failure --verbose
```

or

```bash
make test_all
```

### Run Specific Test Suites
```bash
# Unit tests only
./unit/unit_tests

# Integration tests only
./integration/integration_tests

# Benchmarks
./benchmarks/benchmark_tests
```

### Run Specific Test Cases
```bash
# Run tests matching a pattern
./unit/unit_tests --gtest_filter="EventDataTest.*"
./unit/unit_tests --gtest_filter="*Exception*"
```

### List All Tests
```bash
./unit/unit_tests --gtest_list_tests
```

## Test Coverage

### Unit Tests (~180 tests)
- **EventData** (25 tests): Constructor, copy/move semantics, memory management, thread safety
- **ChSettings** (30 tests): Configuration, detector types, calibration parameters, geometry
- **Exceptions** (40 tests): All exception types, hierarchy, thread safety
- **TFileRAII** (35 tests): RAII pattern, file operations, move semantics, exception safety
- **Builders** (50 tests): L1/L2/TimeAlignment basic operations, thread safety, edge cases

### Integration Tests (~20 tests)
- Full pipeline setup and execution
- Multi-stage data flow
- Resource management across components
- Configuration propagation
- Data integrity checks

### Benchmarks (~15 tests)
- EventData creation performance
- Copy vs move performance
- Builder construction overhead
- Memory efficiency
- Parallel execution performance

## Test Output

Tests produce detailed output including:
- Pass/fail status for each test
- Performance metrics (for benchmarks)
- Memory usage estimates
- Thread safety verification

## Philosophy

Tests follow the KISS (Keep It Simple, Stupid) principle:
- Test what matters: correctness, memory safety, thread safety
- Simple, focused test cases
- No over-engineering
- Fast execution

## Continuous Integration

These tests are designed to be run:
- Locally during development
- Before committing changes
- In CI/CD pipelines (future)
