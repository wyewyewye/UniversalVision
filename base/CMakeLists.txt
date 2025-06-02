MESSAGE(STATUS "ENABLE_BOOST_LOG=${ENABLE_BOOST_LOG}")
MESSAGE(STATUS "ENABLE_POCO_LOG=${ENABLE_POCO_LOG}")

IF(NOT ENABLE_POCO_LOG AND NOT ENABLE_BOOST_LOG)
    MESSAGE(FATAL_ERROR "Please enable one of the log system: ENABLE_POCO_LOG or ENABLE_BOOST_LOG in config.py")
ENDIF()

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/inc)

LIST(APPEND BASE_SRC ${CMAKE_CURRENT_SOURCE_DIR}/inc/logging.h
                     ${CMAKE_CURRENT_SOURCE_DIR}/src/logging.cpp)

ADD_LIBRARY(base ${BASE_SRC})
# target_compile_definitions(base PRIVATE ${LOG_MACRO})
TARGET_LINK_LIBRARIES(base
                      Poco::Foundation
                      ${Boost_LIBRARIES}
)

