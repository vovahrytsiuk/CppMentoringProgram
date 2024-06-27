project(CopyTool LANGUAGES CXX)

set(HEADERS
    include/CopyTool/ICopyTool.h
)

set(SOURCES
    SingleThreadedCopyTool.cpp
    StlCopyTool.cpp
    TwoThreadedCopyTool.cpp
)

set(LIB_TARGET ${PROJECT_NAME}.Static)

add_library(${LIB_TARGET} STATIC ${HEADERS} ${SOURCES})

target_include_directories(${LIB_TARGET}
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)




