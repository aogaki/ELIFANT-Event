# ----------------------------------------------------------------------------
# Setup the project
cmake_minimum_required(VERSION 3.5 FATAL_ERROR)
project(eve-builder)
set(LIB_NAME EveBuilder)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 20)

# ---Locate the ROOT package and defines a number of variables (e.g. ROOT_INCLUDE_DIRS)
list(APPEND CMAKE_PREFIX_PATH $ENV{ROOTSYS})

# set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "$ENV{ROOTSYS}/ect/cmake")
find_package(ROOT REQUIRED COMPONENTS RIO Net)
include(${ROOT_USE_FILE})

set(CMAKE_CXX_FLAGS_DEBUG_INIT "-Wall")
set(CMAKE_CXX_FLAGS_RELEASE_INIT "-Wall")

add_compile_options(-pthread -O2)

# ----------------------------------------------------------------------------
include_directories(${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${ROOT_INCLUDE_DIR})
link_directories(${ROOT_LIBRARY_DIR})
file(GLOB headers ${PROJECT_SOURCE_DIR}/include/*.hpp)
file(GLOB sources ${PROJECT_SOURCE_DIR}/src/*.cpp)

# Copy config files *.conf to build directory
file(GLOB conf_files ${PROJECT_SOURCE_DIR}/include/*.hpp)
file(COPY ${conf_files} DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

# Generating Dictionary for ROOT
set(MY_LINKDEF ${PROJECT_SOURCE_DIR}/dictionary/LinkDef.h)
set(MY_DICTIONARY "G__${LIB_NAME}")
ROOT_GENERATE_DICTIONARY(${MY_DICTIONARY} ${headers} LINKDEF ${MY_LINKDEF})

# ----------------------------------------------------------------------------
set(CMAKE_SHARED_LIBRARY_SUFFIX ".so")
add_library(${LIB_NAME} SHARED ${sources} ${headers} "${MY_DICTIONARY}.cxx")
target_link_libraries(${LIB_NAME} ${ROOT_LIBRARIES} RHTTP Spectrum)

add_executable(${PROJECT_NAME} main.cpp)
target_link_libraries(${PROJECT_NAME} ${LIB_NAME})
