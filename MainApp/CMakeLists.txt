project(copyToolExe VERSION 0.1.0 LANGUAGES C CXX)

find_package(Boost REQUIRED COMPONENTS program_options)

set(SOURCES
    main.cpp
    ProgramOptions.cpp
    ProgramOptions.h
)

# W4 or Werror Wall TODO

add_executable(copyTool ${SOURCES})

target_link_libraries(copyTool 
PRIVATE 
    CopyTool.Static 
    Boost::program_options
)

include(CTest)
enable_testing()

