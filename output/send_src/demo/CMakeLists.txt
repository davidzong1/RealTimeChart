# 设置 CMake 最低版本要求
cmake_minimum_required(VERSION 3.10)

# 设置项目名称和版本
project(MyProject VERSION 1.0)

# 设置构建类型
set(CMAKE_BUILD_TYPE Debug)

find_package(Eigen3 REQUIRED)

# 指定 C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

find_library(Boost REQUIRED)
include_directories(${CMAKE_SOURCE_DIR})
include_directories(${CMAKE_SOURCE_DIR}/lib/cppipc/include)
include_directories(${EIGEN3_INCLUDE_DIR})
add_executable(test2
    ./main1.cc
    ./dz_sm_socket.cc
    ./dz_sm_socket_top.cc
    ./dzsem.cc
)
target_link_libraries(test2 pthread ${Boost_LIBRARIES} ipc rt)
