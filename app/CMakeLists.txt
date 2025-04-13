# glad
FILE(GLOB SRC ${CMAKE_SOURCE_DIR}/third_party/glad/src/*.c)

LIST(APPEND SRC ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp
                ${CMAKE_CURRENT_SOURCE_DIR}/src/application.cpp)

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/inc
                    "E:/workspace/glfw-3.4.bin.WIN64/include"
                    ${CMAKE_SOURCE_DIR}/third_party/glad/include)

ADD_EXECUTABLE(${PROJECT_NAME} ${SRC})

TARGET_LINK_LIBRARIES(${PROJECT_NAME} "E:/workspace/glfw-3.4.bin.WIN64/lib-vc2022/glfw3.lib" 
                                      opengl32.lib)

SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES FOLDER "app")