
cmake_minimum_required(VERSION 2.8 FATAL_ERROR)
project(ThreadSharedResources)

include_directories(${PROJECT_SOURCE_DIR})

add_executable(ThreadSharedResources
  main.cpp)

add_executable(Test_CountThreadNum
  Test_CountThreadNum.cpp)
