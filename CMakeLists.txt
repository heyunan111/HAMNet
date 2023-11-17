cmake_minimum_required(VERSION 3.22.0)
project(HAMNet VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
find_package(GTest REQUIRED)
include(CTest)
enable_testing()


set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)

include_directories(${PROJECT_SOURCE_DIR}/src/include)
#include_directories(${PROJECT_SOURCE_DIR}/example)
link_directories(${PROJECT_SOURCE_DIR}/lib)

add_subdirectory(src)
add_subdirectory(tests)
#add_subdirectory(example)


set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
