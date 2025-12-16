# Integration Tests CMakeLists.txt

file(GLOB INTEGRATION_TEST_SOURCES "*.cpp")

add_executable(integration_tests ${INTEGRATION_TEST_SOURCES})

target_link_libraries(integration_tests
    EveBuilder
    ${ROOT_LIBRARIES}
    gtest
    gtest_main
    pthread
)

gtest_discover_tests(integration_tests)
