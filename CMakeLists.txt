
cmake_minimum_required(VERSION 2.8 FATAL_ERROR)
project(ThreadSharedResources)

include_directories(
  ${PROJECT_SOURCE_DIR}
  "C:/Program Files/PostgreSQL/9.3/include")

link_directories(
  "C:/Program Files/PostgreSQL/9.3/lib")

add_executable(ThreadSharedResources
  ThreadSharedResources.cpp)

add_executable(Test_CountThreadNum
  Test_CountThreadNum.cpp)

add_executable(Sample_PG
  Sample_PG.cpp)

target_link_libraries(Sample_PG
  libpq)