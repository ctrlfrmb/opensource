cmake_minimum_required(VERSION 3.10)

# 设置项目名称和版本
project(ipcap VERSION 1.0)

# 指定C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# 引用头文件路径
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/include/npcap1.13/include)

# 添加库目录
link_directories(${CMAKE_SOURCE_DIR}/include/npcap1.13/lib/${CMAKE_HOST_SYSTEM_PROCESSOR})
LIST(APPEND LINK_LIBS "wpcap")
LIST(APPEND LINK_LIBS "Packet")
LIST(APPEND LINK_LIBS "ws2_32")

file(GLOB_RECURSE IPCAP_SRC ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

# 指定可执行文件和源文件
add_executable(${PROJECT_NAME} ${IPCAP_SRC})

# 链接WinPcap库
target_link_libraries(${PROJECT_NAME} ${LINK_LIBS})
