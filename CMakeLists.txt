project(ffmpegtest)
cmake_minimum_required(VERSION 3.1.3)
#使能c++11
set(CMAKE_CXX_STANDARD 11)
#加载自定义模块
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set(CMAKE_BUILD_TYPE "Debug")
#set(CMAKE_BUILD_TYPE "Release")

set(SYS_BUILD_TYPE "ARM64")

if(${SYS_BUILD_TYPE} MATCHES "ARM64")
	set(CMAKE_SYSTEM_PROCESSOR arm)

	set(TOOLCHAIN_DIR  "/works/rk3568/rk3568_v1_2_4/buildroot/output/rockchip_rk3568/host")

	set(SYSROOT_PATH  ${TOOLCHAIN_DIR}/aarch64-buildroot-linux-gnu/sysroot)
	set(CMAKE_SYSROOT "${SYSROOT_PATH}")



	SET(CMAKE_C_COMPILER ${TOOLCHAIN_DIR}/bin/aarch64-linux-gcc)
	SET(CMAKE_CXX_COMPILER ${TOOLCHAIN_DIR}/bin/aarch64-linux-g++)
	SET(CMAKE_FIND_ROOT_PATH  ${SYSROOT_PATH})
endif()

if(${CMAKE_BUILD_TYPE} MATCHES "Release")
    message(STATUS "Release版本")
    set(BuildType "Release")

else()
    set(BuildType "Debug")
    message(STATUS "Debug版本")
	#add debug operation
	set(ENABLE_ASAN true)
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

include_directories(include include/rkmedia rkmedia easymedia include/libdrm include/zlm)
link_directories(lib lib/zlm lib/mali lib/zlog)

ADD_DEFINITIONS(-DARM)
set(LINK_LIB_LIST m pthread asound avformat avdevice avcodec avfilter swresample avutil  rga drm rockchip_mpp  z asound v4l2 v4lconvert jpeg SDL2 SDL2_ttf easymedia freetype mali-bifrost-g52-g2p0-dummy ${LINK_ASAN} fdk-aac jemalloc png)


add_executable(rktest h264_dec.c  test.cpp vpu_decode.c rk_h264_decode.c)
target_link_libraries(rktest easymedia rga drm rockchip_mpp avformat avcodec swresample avutil pthread z asound v4l2 v4lconvert jpeg freetype ${LINK_ASAN})


#add_executable(rkmedia_vi_get_frame_test rkmedia_vi_get_frame_test.c)
#target_link_libraries(rkmedia_vi_get_frame_test easymedia mk_api rga drm rockchip_mpp avformat avcodec swresample avutil pthread z asound v4l2 v4lconvert jpeg freetype mali-bifrost-g52-g2p0-dummy ${LINK_ASAN})

add_executable(test_write test_write.c)

add_executable(test_sd_event test_sd_event.c uevent.c)

add_executable(testc testc.c)

