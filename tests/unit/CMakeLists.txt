# Unit Tests CMakeLists.txt

# Collect all unit test files
file(GLOB UNIT_TEST_SOURCES "*.cpp")

# Create unit test executable
add_executable(unit_tests ${UNIT_TEST_SOURCES})

# Link against production library and Google Test
target_link_libraries(unit_tests
    EveBuilder
    ${ROOT_LIBRARIES}
    gtest
    gtest_main
    pthread
)

# Discover tests for CTest
gtest_discover_tests(unit_tests)
