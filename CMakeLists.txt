# CMake最低版本要求
cmake_minimum_required(VERSION 3.16)

# 项目名称
project(RealTimeChart)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_BUILD_TYPE Release)

# 查找Qt包
find_package(Qt5 COMPONENTS Core Gui Widgets PrintSupport OpenGL REQUIRED)

# 查找其他依赖包
find_package(Eigen3 REQUIRED)
find_package(Boost COMPONENTS system thread REQUIRED)
find_package(TBB REQUIRED)
find_package(OpenGL REQUIRED)

# 添加自动生成的UI文件
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# 源文件
set(SOURCES
    CsvLoaderWindow.cc
    main.cpp
    mainwindow.cpp
    qcustomplot.cpp
    socket_share_mem/IOopperator.cc
    socket_share_mem/dz_sm_socket.cc
    socket_share_mem/dz_sm_socket_top.cc
    socket_share_mem/dzsem.cc
)

# 头文件
set(HEADERS
    dziostream.h
    mainwindow.h
    qcustomplot.h
    socket_share_mem/IOopperator.h
    socket_share_mem/dz_sm_socket.h
    socket_share_mem/dz_sm_socket_top.h
    socket_share_mem/dz_sm_socket_top_queue.hpp
    socket_share_mem/dzsem.hpp
)

# UI文件
set(FORMS
    mainwindow.ui
)

# 添加可执行文件
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS} ${FORMS})

# 包含目录
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
    socket_share_mem
    ${EIGEN3_INCLUDE_DIRS}
)

# 链接库
target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt5::Core
    Qt5::Gui
    Qt5::Widgets
    Qt5::PrintSupport
    Qt5::OpenGL
    ${OPENGL_LIBRARIES} # 添加OpenGL库
    GL # 显式链接libGL
    Boost::system
    Boost::thread
    TBB::tbb
    pthread
    rt
)

# 定义
target_compile_definitions(${PROJECT_NAME} PRIVATE
    QCUSTOMPLOT_USE_OPENGL
)

# 设置RPATH
set_target_properties(${PROJECT_NAME} PROPERTIES
    INSTALL_RPATH "/usr/lib/x86_64-linux-gnu/qt5/plugins"
    BUILD_WITH_INSTALL_RPATH TRUE
)