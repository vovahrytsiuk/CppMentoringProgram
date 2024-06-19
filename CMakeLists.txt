cmake_minimum_required(VERSION 3.22.0)
project(copyTool VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)


set(SOURCES
    ICopyTool.h
    main.cpp
    SingleThreadedCopyTool.cpp
    StlCopyTool.cpp
    TwoThreadedCopyTool.cpp
)

add_executable(copyTool ${SOURCES})

include(CTest)
enable_testing()

