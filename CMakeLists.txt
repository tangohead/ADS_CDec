cmake_minimum_required(VERSION 3.8)
project(ADS_CDec)

SET(CMAKE_PREFIX_PATH "/home/matt/Qt5.9.2/5.9.2/gcc_64")

#include_directories(/home/matt/Qt5.9.2/5.9.2/gcc_64/include/)
#link_directories(/home/matt/Qt5.9.2/5.9.2/gcc_64/lib)

SET(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt5Core)

SET(CMAKE_CXX_FLAGS "-fPIC -DQT_NO_VERSION_TAGGING")
SET(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} ${CMAKE_CXX_EXTRAS})
set(CMAKE_CXX_STANDARD 11)



set(SOURCE_FILES main.cpp main.h)
add_executable(ADS_CDec ${SOURCE_FILES})

target_link_libraries(ADS_CDec Qt5::Core)