project(rk_test)
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR arm)

set(CROSS_COMPILE 1)
set(TOOLCHAIN_DIR  "/works/rk3568/rk3568_v1_2_4/buildroot/output/rockchip_rk3568/host")
set(TOOLCHAIN_DIR  "/works/rk3568/rk3568_v1_2_4/buildroot/output/rockchip_rk3568/host")
set(SYSROOT_PATH  ${TOOLCHAIN_DIR}/aarch64-buildroot-linux-gnu/sysroot)
set(CMAKE_SYSROOT "${SYSROOT_PATH}")
SET(CMAKE_C_COMPILER ${TOOLCHAIN_DIR}/bin/aarch64-linux-gcc)
SET(CMAKE_CXX_COMPILER ${TOOLCHAIN_DIR}/bin/aarch64-linux-g++)
SET(CMAKE_FIND_ROOT_PATH  ${SYSROOT_PATH})

add_executable(hello hello.c)
target_link_libraries(hello rga drm drm pthread z v4l2 v4l2rds  rockchip_mpp m)
#target_link_libraries(set-plane-set rga drm pthread z v4l2 v4l2rds  rockchip_mpp -lm)

add_executable(test_rga test_rga.cpp rkRgaApi.cpp)
target_link_libraries(test_rga rga drm easymedia drm pthread z v4l2 v4l2rds  rockchip_mpp m )

add_executable(test_im2d test_im2d.cpp )
target_link_libraries(test_im2d rga drm easymedia drm pthread z v4l2 v4l2rds  rockchip_mpp m )

