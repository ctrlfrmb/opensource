# CMakeList.txt: 顶层 CMake 项目文件，在此处执行全局配置
# 并包含子项目。
#
cmake_minimum_required (VERSION 3.8)

# 如果支持，请为 MSVC 编译器启用热重载。
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("fksoft")

message(STATUS ${CMAKE_HOST_SYSTEM_NAME})
message(STATUS ${CMAKE_HOST_SYSTEM_PROCESSOR})
 
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        message(STATUS "Configuring on/for Linux")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    message(STATUS "Configuring on/for macOS")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    message(STATUS "Configuring on/for Windows")
elseif(CMAKE_SYSTEM_NAME STREQUAL "AIX")
    message(STATUS "Configuring on/for IBM AIX")
else()
    message(STATUS "Configuring on/for ${CMAKE_SYSTEM_NAME}")
endif()

if(CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "AMD64")
    message(STATUS "aarch64 architecture detected")
elseif(CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "aarch64")
    message(STATUS "aarch64 architecture detected")
elseif(CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "i386")
    message(STATUS "i386 architecture detected")
elseif(CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "i686")
    message(STATUS "i686 architecture detected")
elseif(CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "x86_64")
    message(STATUS "x86_64 architecture detected")
else()
    message(FATAL_ERROR "host processor architecture is unknown")
endif()

option(BUILD_WITH_DEBUG_MODE "build with debug mode?" OFF)
if (BUILD_WITH_DEBUG_MODE)
    set(CMAKE_BUILD_TYPE "Debug")
else()
    set(CMAKE_BUILD_TYPE "Release")
endif()

# 包含子项目。
add_subdirectory ("ipcap")
