project(MainAppTest)

find_package(GTest REQUIRED)

include_directories( ${GTEST_INCLUDE_DIRS})

set(TEST_SOURCES
    ProgramOptionsTest.cpp
)

add_executable(${PROJECT_NAME} ${TEST_SOURCES})

target_link_libraries(${PROJECT_NAME} 
PRIVATE 
    GTest::gtest 
    GTest::gmock 
    GTest::gtest_main 
    GTest::gmock_main
    MainApp.Static
)
target_compile_definitions(${PROJECT_NAME} PRIVATE WITH_GMOCK)