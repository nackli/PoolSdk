
#Written by Nack li <nackli@163.com>
#Copyright (c) 2024. All Rights Reserved.

#version
cmake_minimum_required(VERSION 3.16)

project(PoolSdk VERSION 2.5.0)

set(PACKAGE_VERSION ${CMAKE_PROJECT_VERSION})
set(PACKAGE_STRING "${CMAKE_PROJECT_NAME} ${CMAKE_PROJECT_VERSION}")

option(ENABLE_POOL_SDK_TEST "Enable test" OFF)
option(ENABLE_MEM_POOL "mem pool enable" ON)
option(USE_LIBFMT_FORMAT "use fmtlib" ON)


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

add_definitions(-D_CRT_SECURE_NO_WARNINGS)

# default user option
if(ENABLE_POOL_SDK_TEST)
  add_definitions(-DENABLE_POOL_SDK_TEST)
endif()

if(ENABLE_MEM_POOL)
  add_definitions(-DMEM_POOL_OPEN)
endif()

if(USE_LIBFMT_FORMAT)
  add_definitions(-DENABLE_LIB_FMT)
  add_definitions(-DFMT_UNICODE=0)
endif()

# set msvc info
macro(add_msvc_options target)
  if(MSVC)
    target_compile_options(${target} PRIVATE
    /W3					# 设置警告级别为 3
    #/WX               	# 将警告视为错误
    /MP                	# 启用多处理器编译
    /permissive-       	# 禁用不严格的语言 conformance
    /Zc:__cplusplus    	# 启用正确的 __cplusplus 宏值
    /Zc:inline         	# 移除未使用的函数
    #/Gm-               # 已弃用。 启用最小重新生成。
	/Gy				   	# 启用函数级链接。
	/sdl				# 启用更多安全功能和警告。
    )
  endif()
endmacro()




if(MSVC)
  set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})
endif()


file(GLOB_RECURSE DEV_SOURCES_EXE ./libSdk/*.cpp ./libSdk/*.cc)
file(GLOB_RECURSE REMOVE_SOURCES ./libSdk/test/*.cpp)
list(REMOVE_ITEM DEV_SOURCES_EXE ${REMOVE_SOURCES})

add_library(${PROJECT_NAME} ${DEV_SOURCES_EXE})
add_msvc_options(${PROJECT_NAME})


add_executable(${PROJECT_NAME}_test .//libSdk/test/log_Test.cpp)


target_link_libraries(${PROJECT_NAME}_test ${PROJECT_NAME})


include(BuildPage.cmake)
use_pack_fun(${PROJECT_NAME} "ZIP")