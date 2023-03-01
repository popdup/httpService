# CMake 最低版本号要求
cmake_minimum_required (VERSION 2.8)

# 项目信息
project (demo2)

find_package(CURL)
if(CURL_FOUND)
  message(STATUS "found curl")
endif()

# 指定生成目标
add_executable(demo demo.cpp HttpThread.cpp)
target_link_libraries(demo curl pthread)
