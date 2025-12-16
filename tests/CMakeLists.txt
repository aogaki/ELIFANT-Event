# ----------------------------------------------------------------------------
# ELIFANT-Event Test Suite
# ----------------------------------------------------------------------------
cmake_minimum_required(VERSION 3.5 FATAL_ERROR)
project(EveBuilderTests)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ---Locate the ROOT package
list(APPEND CMAKE_PREFIX_PATH $ENV{ROOTSYS})
find_package(ROOT REQUIRED COMPONENTS RIO Net)
include(${ROOT_USE_FILE})

add_compile_options(-pthread -O2)

# ----------------------------------------------------------------------------
# Include directories - point to production code
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
    ${ROOT_INCLUDE_DIR}
)
link_directories(${ROOT_LIBRARY_DIR})

# ----------------------------------------------------------------------------
# Build production library that tests will link against
file(GLOB headers ${CMAKE_CURRENT_SOURCE_DIR}/../include/*.hpp)
file(GLOB sources ${CMAKE_CURRENT_SOURCE_DIR}/../src/*.cpp)

# Copy config files to build directory
file(COPY ${headers} DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

# ROOT Dictionary
set(MY_LINKDEF ${CMAKE_CURRENT_SOURCE_DIR}/../dictionary/LinkDef.h)
set(MY_DICTIONARY "G__EveBuilder")
ROOT_GENERATE_DICTIONARY(${MY_DICTIONARY} ${headers} LINKDEF ${MY_LINKDEF})

# Production library
add_library(EveBuilder SHARED ${sources} ${headers} "${MY_DICTIONARY}.cxx")
target_link_libraries(EveBuilder ${ROOT_LIBRARIES} RHTTP Spectrum)

# ----------------------------------------------------------------------------
# Google Test
include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/release-1.12.1.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()
include(GoogleTest)

# ----------------------------------------------------------------------------
# Add test subdirectories
add_subdirectory(unit)
add_subdirectory(integration)
add_subdirectory(benchmarks)

# ----------------------------------------------------------------------------
# Custom target to run all tests with summary
add_custom_target(test_all
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure --verbose
    DEPENDS unit_tests integration_tests
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running all tests with summary..."
)
