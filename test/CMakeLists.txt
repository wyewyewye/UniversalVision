FILE(GLOB SOURCES 
          ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp 
          ${CMAKE_CURRENT_SOURCE_DIR}/*.h)

ADD_EXECUTABLE(test ${SOURCES})

TARGET_LINK_LIBRARIES(test GTest::gtest GTest::gtest_main GTest::gmock GTest::gmock_main Poco::Foundation)