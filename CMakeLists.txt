cmake_minimum_required(VERSION 3.5.0)

get_filename_component(BASE_FOLDER ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(PROJECT_NAME ${BASE_FOLDER})
project(${PROJECT_NAME} C CXX ASM)

file(GLOB_RECURSE SRC_FILES "Src/*.c" "Src/*.cpp")
add_executable(${PROJECT_NAME} ${SRC_FILES})


target_include_directories(${PROJECT_NAME} PRIVATE 
${CMAKE_CURRENT_SOURCE_DIR}/Inc 
)

#SET( CMAKE_BUILD_TYPE Debug )