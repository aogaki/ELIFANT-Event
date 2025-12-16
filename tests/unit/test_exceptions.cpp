#include <gtest/gtest.h>

#include "DELILAExceptions.hpp"

#include <atomic>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

using namespace DELILA;

class ExceptionsTest : public ::testing::Test {
 protected:
  void SetUp() override {}
  void TearDown() override {}
};

//=============================================================================
// DELILAException (Base) Tests
//=============================================================================

TEST_F(ExceptionsTest, DELILAExceptionBasic) {
  try {
    throw DELILAException("Test error message");
  } catch (const DELILAException &e) {
    EXPECT_STREQ(e.what(), "Test error message");
  } catch (...) {
    FAIL() << "Wrong exception type caught";
  }
}

TEST_F(ExceptionsTest, DELILAExceptionInheritsFromRuntimeError) {
  try {
    throw DELILAException("Test");
  } catch (const std::runtime_error &e) {
    SUCCEED();
  } catch (...) {
    FAIL() << "DELILAException should inherit from std::runtime_error";
  }
}

TEST_F(ExceptionsTest, DELILAExceptionEmptyMessage) {
  try {
    throw DELILAException("");
  } catch (const DELILAException &e) {
    EXPECT_STREQ(e.what(), "");
  }
}

TEST_F(ExceptionsTest, DELILAExceptionLongMessage) {
  std::string longMsg(1000, 'A');
  try {
    throw DELILAException(longMsg);
  } catch (const DELILAException &e) {
    EXPECT_EQ(std::string(e.what()), longMsg);
  }
}

//=============================================================================
// FileException Tests
//=============================================================================

TEST_F(ExceptionsTest, FileExceptionBasic) {
  try {
    throw FileException("File not found");
  } catch (const FileException &e) {
    EXPECT_STREQ(e.what(), "File not found");
  } catch (...) {
    FAIL() << "Wrong exception type";
  }
}

TEST_F(ExceptionsTest, FileExceptionCatchAsDELILAException) {
  try {
    throw FileException("File error");
  } catch (const DELILAException &e) {
    EXPECT_STREQ(e.what(), "File error");
    SUCCEED();
  } catch (...) {
    FAIL() << "Should be catchable as DELILAException";
  }
}

TEST_F(ExceptionsTest, FileExceptionCatchAsRuntimeError) {
  try {
    throw FileException("File error");
  } catch (const std::runtime_error &e) {
    SUCCEED();
  }
}

TEST_F(ExceptionsTest, FileExceptionTypicalMessages) {
  const char *messages[] = {
      "File not found: data.root",
      "Permission denied: /protected/file.dat",
      "Cannot open file for writing",
      "File is corrupted",
      "I/O error during read",
  };

  for (const char *msg : messages) {
    try {
      throw FileException(msg);
    } catch (const FileException &e) {
      EXPECT_STREQ(e.what(), msg);
    }
  }
}

//=============================================================================
// ConfigException Tests
//=============================================================================

TEST_F(ExceptionsTest, ConfigExceptionBasic) {
  try {
    throw ConfigException("Invalid configuration");
  } catch (const ConfigException &e) {
    EXPECT_STREQ(e.what(), "Invalid configuration");
  }
}

TEST_F(ExceptionsTest, ConfigExceptionCatchAsBase) {
  try {
    throw ConfigException("Config error");
  } catch (const DELILAException &e) {
    SUCCEED();
  } catch (...) {
    FAIL();
  }
}

TEST_F(ExceptionsTest, ConfigExceptionTypicalMessages) {
  try {
    throw ConfigException("Module index out of range: 255");
    FAIL() << "Should have thrown";
  } catch (const ConfigException &e) {
    std::string msg(e.what());
    EXPECT_TRUE(msg.find("Module") != std::string::npos);
    EXPECT_TRUE(msg.find("255") != std::string::npos);
  }
}

//=============================================================================
// JSONException Tests
//=============================================================================

TEST_F(ExceptionsTest, JSONExceptionBasic) {
  try {
    throw JSONException("JSON parse error");
  } catch (const JSONException &e) {
    EXPECT_STREQ(e.what(), "JSON parse error");
  }
}

TEST_F(ExceptionsTest, JSONExceptionCatchAsBase) {
  try {
    throw JSONException("Invalid JSON");
  } catch (const DELILAException &e) {
    SUCCEED();
  }
}

TEST_F(ExceptionsTest, JSONExceptionTypicalMessages) {
  const char *messages[] = {
      "Failed to parse JSON file",
      "Missing required field: 'Module'",
      "Invalid JSON syntax at line 42",
      "Type mismatch: expected number, got string",
  };

  for (const char *msg : messages) {
    try {
      throw JSONException(msg);
    } catch (const JSONException &e) {
      EXPECT_STREQ(e.what(), msg);
    }
  }
}

//=============================================================================
// ValidationException Tests
//=============================================================================

TEST_F(ExceptionsTest, ValidationExceptionBasic) {
  try {
    throw ValidationException("Validation failed");
  } catch (const ValidationException &e) {
    EXPECT_STREQ(e.what(), "Validation failed");
  }
}

TEST_F(ExceptionsTest, ValidationExceptionCatchAsBase) {
  try {
    throw ValidationException("Invalid input");
  } catch (const DELILAException &e) {
    SUCCEED();
  }
}

TEST_F(ExceptionsTest, ValidationExceptionTypicalMessages) {
  try {
    throw ValidationException("Thread count must be > 0");
    FAIL();
  } catch (const ValidationException &e) {
    std::string msg(e.what());
    EXPECT_TRUE(msg.find("Thread count") != std::string::npos);
  }
}

//=============================================================================
// RangeException Tests
//=============================================================================

TEST_F(ExceptionsTest, RangeExceptionBasic) {
  try {
    throw RangeException("Index out of bounds");
  } catch (const RangeException &e) {
    EXPECT_STREQ(e.what(), "Index out of bounds");
  }
}

TEST_F(ExceptionsTest, RangeExceptionCatchAsBase) {
  try {
    throw RangeException("Out of range");
  } catch (const DELILAException &e) {
    SUCCEED();
  }
}

TEST_F(ExceptionsTest, RangeExceptionTypicalMessages) {
  const char *messages[] = {
      "Module index 10 exceeds maximum 8",
      "Channel 16 out of range [0-15]",
      "Array access violation at index 100",
      "Vector subscript out of range",
  };

  for (const char *msg : messages) {
    try {
      throw RangeException(msg);
    } catch (const RangeException &e) {
      EXPECT_STREQ(e.what(), msg);
    }
  }
}

//=============================================================================
// ProcessingException Tests
//=============================================================================

TEST_F(ExceptionsTest, ProcessingExceptionBasic) {
  try {
    throw ProcessingException("Data processing failed");
  } catch (const ProcessingException &e) {
    EXPECT_STREQ(e.what(), "Data processing failed");
  }
}

TEST_F(ExceptionsTest, ProcessingExceptionCatchAsBase) {
  try {
    throw ProcessingException("Processing error");
  } catch (const DELILAException &e) {
    SUCCEED();
  }
}

TEST_F(ExceptionsTest, ProcessingExceptionTypicalMessages) {
  const char *messages[] = {
      "Event building failed for file: data.root",
      "Time alignment computation error",
      "Corrupted data detected in event 12345",
      "Thread synchronization error",
  };

  for (const char *msg : messages) {
    try {
      throw ProcessingException(msg);
    } catch (const ProcessingException &e) {
      EXPECT_STREQ(e.what(), msg);
    }
  }
}

//=============================================================================
// Exception Hierarchy Tests
//=============================================================================

TEST_F(ExceptionsTest, ExceptionHierarchy) {
  // All derived exceptions should be catchable as DELILAException
  std::vector<DELILAException> exceptions;

  try {
    throw FileException("test");
  } catch (const DELILAException &e) {
    exceptions.push_back(e);
  }

  try {
    throw ConfigException("test");
  } catch (const DELILAException &e) {
    exceptions.push_back(e);
  }

  try {
    throw JSONException("test");
  } catch (const DELILAException &e) {
    exceptions.push_back(e);
  }

  try {
    throw ValidationException("test");
  } catch (const DELILAException &e) {
    exceptions.push_back(e);
  }

  try {
    throw RangeException("test");
  } catch (const DELILAException &e) {
    exceptions.push_back(e);
  }

  try {
    throw ProcessingException("test");
  } catch (const DELILAException &e) {
    exceptions.push_back(e);
  }

  EXPECT_EQ(exceptions.size(), 6);
}

TEST_F(ExceptionsTest, CatchSpecificExceptionType) {
  bool caughtCorrectType = false;

  try {
    throw FileException("file error");
  } catch (const FileException &e) {
    caughtCorrectType = true;
  } catch (const ConfigException &e) {
    FAIL() << "Caught wrong exception type";
  } catch (const DELILAException &e) {
    FAIL() << "Should catch most derived type first";
  }

  EXPECT_TRUE(caughtCorrectType);
}

TEST_F(ExceptionsTest, RethrowException) {
  try {
    try {
      throw ValidationException("validation error");
    } catch (const DELILAException &e) {
      // Rethrow
      throw;
    }
  } catch (const ValidationException &e) {
    EXPECT_STREQ(e.what(), "validation error");
  } catch (...) {
    FAIL() << "Should preserve exception type when rethrowing";
  }
}

TEST_F(ExceptionsTest, ExceptionInMultiThreadContext) {
  std::atomic<int> exceptionsCaught{0};
  std::vector<std::thread> threads;

  for (int i = 0; i < 10; i++) {
    threads.emplace_back([&exceptionsCaught, i]() {
      try {
        if (i % 2 == 0) {
          throw FileException("error");
        } else {
          throw ConfigException("error");
        }
      } catch (const DELILAException &e) {
        exceptionsCaught++;
      }
    });
  }

  for (auto &t : threads) {
    t.join();
  }

  EXPECT_EQ(exceptionsCaught.load(), 10);
}

TEST_F(ExceptionsTest, NestedExceptions) {
  try {
    try {
      throw FileException("Inner exception");
    } catch (const FileException &e) {
      throw ConfigException("Outer exception wrapping file error");
    }
  } catch (const ConfigException &e) {
    std::string msg(e.what());
    EXPECT_TRUE(msg.find("Outer") != std::string::npos);
  }
}

TEST_F(ExceptionsTest, ExceptionCopyConstructor) {
  FileException original("original message");
  FileException copy(original);

  EXPECT_STREQ(copy.what(), "original message");
}

TEST_F(ExceptionsTest, ExceptionAssignmentOperator) {
  FileException ex1("message 1");
  FileException ex2("message 2");

  try {
    throw ex1;
  } catch (FileException &e) {
    EXPECT_STREQ(e.what(), "message 1");
  }
}
