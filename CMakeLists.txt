cmake_minimum_required(VERSION 3.10)
project(rk_test)
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR arm64)

#使能c++17
set(CMAKE_CXX_STANDARD 17)
#加载自定义模块
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
set(TOOLCHAIN_DIR  "/works/rk3568/rk3568_v1_2_4/buildroot/output/rockchip_rk3568/host/")
set(SYSROOT_PATH  ${TOOLCHAIN_DIR}/aarch64-buildroot-linux-gnu/sysroot)
set(CMAKE_SYSROOT "${SYSROOT_PATH}")

SET(CMAKE_C_COMPILER ${TOOLCHAIN_DIR}/bin/aarch64-linux-gcc)
SET(CMAKE_CXX_COMPILER ${TOOLCHAIN_DIR}/bin/aarch64-linux-g++)
SET(CMAKE_FIND_ROOT_PATH  ${SYSROOT_PATH})

#include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)

#set(C_FLAGS " -mfpu=neon -mfloat-abi=hard ")
set(CMAKE_C_FLAGS "${C_FLAGS} -w")


set(CMAKE_BUILD_TYPE "Debug")
#set(CMAKE_BUILD_TYPE "Release")

add_compile_options( -D_GNU_SOURCE -D_XOPEN_SOURCE=600 -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 -Wno-psabi)

if(${CMAKE_BUILD_TYPE} MATCHES "Release")
    message(STATUS "Release版本")
    set(BuildType "Release")
    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -s")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -s")
    #add_compile_options(-O3 -mfpu=neon -mfloat-abi=hard)
else()
    set(BuildType "Debug")
    message(STATUS "Debug版本")
    #add debug operation
    set(ENABLE_ASAN true)
    #set(CMAKE_C_FLAGS_DEBUG "-O0 -g2 -ggdb")
endif()

if(ENABLE_ASAN)
    # 以下问内存泄漏检测
    add_compile_options(-g)
    add_compile_options(-fsanitize=address)
    add_compile_options(-fno-omit-frame-pointer)
    #add_compile_options(-Wl,--no-undefined)
    #add_compile_options(-O1)
    #添加链接选项, 只能添加一行
    SET(CMAKE_EXE_LINKER_FLAGS "-fsanitize=address")
    SET(LINK_ASAN asan)
endif()

# 忽略deprecated-declarations 警告与错误
add_compile_options(-Wno-deprecated-declarations)
add_compile_options(-Wno-error=deprecated-declarations)

#设置bin和lib库目录
set(RELEASE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/release)
if (CMAKE_SYSTEM_NAME MATCHES "Linux")
    SET(LIBRARY_OUTPUT_PATH ${RELEASE_DIR}/linux/${BuildType})
    SET(EXECUTABLE_OUTPUT_PATH ${RELEASE_DIR}/linux/${BuildType})
    add_compile_options(-fPIC)
elseif (CMAKE_SYSTEM_NAME MATCHES "Windows")
    SET(LIBRARY_OUTPUT_PATH ${RELEASE_DIR}/windows/${BuildType})
    SET(EXECUTABLE_OUTPUT_PATH ${RELEASE_DIR}/windows/${BuildType})
elseif (CMAKE_SYSTEM_NAME MATCHES "Darwin")
    SET(LIBRARY_OUTPUT_PATH ${RELEASE_DIR}/mac/${BuildType})
    SET(EXECUTABLE_OUTPUT_PATH ${RELEASE_DIR}/mac/${BuildType})
endif ()

LINK_DIRECTORIES(${LIBRARY_OUTPUT_PATH})
link_directories(lib)
include_directories(include)

add_executable(hello hello.c)
target_link_libraries(hello rga drm drm pthread z v4l2 v4l2rds  rockchip_mpp m)
#target_link_libraries(set-plane-set rga drm pthread z v4l2 v4l2rds  rockchip_mpp -lm)

add_executable(test_rga test_rga.cpp rkRgaApi.cpp)
target_link_libraries(test_rga rga drm easymedia drm pthread z v4l2 v4l2rds  rockchip_mpp m )

#add_executable(alphaTest alphaTest.cpp rkRgaApi.cpp)
#target_link_libraries(alphaTest rga drm easymedia drm pthread z v4l2 v4l2rds  rockchip_mpp m )

add_executable(test_rga_yuv_blend test_rga_yuv_blend.cpp rkRgaApi.cpp)
target_link_libraries(test_rga_yuv_blend rga drm easymedia drm pthread z v4l2 v4l2rds  rockchip_mpp m )

add_executable(test_yuv2rgb test_yuv2rgb.cpp rkRgaApi.cpp)
target_link_libraries(test_yuv2rgb rga drm easymedia drm pthread z v4l2 v4l2rds  rockchip_mpp m )

add_executable(test_rgbcopy test_rgbcopy.cpp rkRgaApi.cpp)
target_link_libraries(test_rgbcopy rga drm easymedia drm pthread z v4l2 v4l2rds  rockchip_mpp m )

add_executable(test_im2d test_im2d.cpp )
target_link_libraries(test_im2d rga drm easymedia drm pthread z v4l2 v4l2rds  rockchip_mpp m )

add_subdirectory(player)


add_subdirectory(alsa)