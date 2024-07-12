project(copyToolExe VERSION 0.1.0 LANGUAGES C CXX)

find_package(Boost REQUIRED COMPONENTS program_options)


set(SOURCES
    ProgramOptions.cpp
    include/MainApp/ProgramOptions.h
)

set(LIB_TARGET MainApp.Static)

add_library(${LIB_TARGET} STATIC ${HEADERS} ${SOURCES})

target_include_directories(${LIB_TARGET}
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

target_link_libraries(${LIB_TARGET} PUBLIC Boost::program_options)

add_executable(copyTool main.cpp)

target_link_libraries(copyTool 
PRIVATE 
    ${LIB_TARGET}
    CopyTool.Static 
)

include(CTest)
enable_testing()

add_subdirectory(test)

