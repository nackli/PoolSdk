
#Written by Nack li <nackli@163.com>
#Copyright (c) 2024. All Rights Reserved.

#version
cmake_minimum_required(VERSION 3.16)

project(libSdk VERSION 2.5.0)

set(PACKAGE_VERSION ${CMAKE_PROJECT_VERSION})
set(PACKAGE_STRING "${CMAKE_PROJECT_NAME} ${CMAKE_PROJECT_VERSION}")

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

# set C++ standard
set(CMAKE_CXX_STANDARD 14)
# set(CMAKE_CXX_STANDARD_REQUIRED ON)
# set(CMAKE_CXX_EXTENSIONS OFF)
#set(CMAKE_BINARY_DIR ${CMAKE_CURRENT_SOURCE_DIR}/build)
set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "" FORCE)

include_directories(./libSdk)


if(MSVC) 
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd /Zi")
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT /Zi")

	set(CMAKE_GENERATOR_PLATFORM Win32)
  add_compile_options(/W3 /utf-8 /bigobj /MP)
else()
	 set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS}  -Wall -O3 -fPIC")
	 set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -O3 -fPIC")
endif()	

# default user option
option(ENABLE_POOL_SDK_TEST "Enable test" OFF)
if(ENABLE_POOL_SDK_TEST)
  add_definitions(-DENABLE_POOL_SDK_TEST -D_CRT_SECURE_NO_WARNINGS)
endif()

# set msvc info
macro(add_msvc_options target)
  if(MSVC)
    target_compile_options(${target} PRIVATE
    /W4                # 设置警告级别为 4
    /WX                # 将警告视为错误
    /MP                # 启用多处理器编译
    /permissive-       # 禁用不严格的语言 conformance
    /Zc:__cplusplus    # 启用正确的 __cplusplus 宏值
    /Zc:inline         # 移除未使用的函数
    /Gm-               # 禁用最小生成（minimal rebuild）
    #/EHsc              # 指定异常处理模型
    )
  endif()
endmacro()




if(MSVC)
  set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT "netWorkBroad")
endif()


file(GLOB_RECURSE DEV_SOURCES_EXE ./libSdk/*.cpp)
file(GLOB_RECURSE REMOVE_SOURCES ./libSdk/test/*.cpp)
list(REMOVE_ITEM DEV_SOURCES_EXE ${REMOVE_SOURCES})

add_library(${PROJECT_NAME} ${DEV_SOURCES_EXE})



add_executable(${PROJECT_NAME}_test ./PoolSdk/PoolSdk.cpp)

target_link_libraries(${PROJECT_NAME}_test ${PROJECT_NAME})
