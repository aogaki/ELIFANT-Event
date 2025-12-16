# Benchmark Tests CMakeLists.txt

file(GLOB BENCHMARK_SOURCES "*.cpp")

# Only build benchmarks if there are source files
if(BENCHMARK_SOURCES)
    add_executable(benchmark_tests ${BENCHMARK_SOURCES})

    target_link_libraries(benchmark_tests
        EveBuilder
        ${ROOT_LIBRARIES}
        gtest
        gtest_main
        pthread
    )

    gtest_discover_tests(benchmark_tests)
else()
    # Create a dummy target if no benchmarks yet
    add_custom_target(benchmark_tests
        COMMAND ${CMAKE_COMMAND} -E echo "No benchmark tests implemented yet"
    )
endif()
