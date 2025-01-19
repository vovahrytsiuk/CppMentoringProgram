project(CopyTool LANGUAGES CXX)

find_package(Boost REQUIRED COMPONENTS)

set(HEADERS
    include/CopyTool/ICopyTool.h
)

set(SOURCES
    SharedMemoryCopyTool.cpp
    SingleThreadedCopyTool.cpp
    StlCopyTool.cpp
    TwoThreadedCopyTool.cpp
)

set(LIB_TARGET ${PROJECT_NAME}.Static)

add_library(${LIB_TARGET} STATIC ${HEADERS} ${SOURCES})

target_link_libraries(${LIB_TARGET} PUBLIC Boost::boost)

target_include_directories(${LIB_TARGET}
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

add_subdirectory(test)




