#set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR arm)

#set(TOOLCHAIN_DIR  "/data/rk356x/buildroot/output/rockchip_rk3568/host")
#set(TOOLCHAIN_DIR  "/data/works/rk3568/rk356x-linux-20210809/buildroot/output/rockchip_rk3568/host")
#set(TOOLCHAIN_DIR  "/data/works/rk3568/rk356x_linux_release_v1_2_0/rk356x/buildroot/output/rockchip_rk3568/host")
##set(TOOLCHAIN_DIR  "/data/works/rk3568/rk356x_v1_2_3/rk356x/buildroot/output/rockchip_rk3568/host")
#set(TOOLCHAIN_DIR  "/works/rk3568/rk3568_v1_2/buildroot/output/rockchip_rk3568/host")
set(TOOLCHAIN_DIR  "/data/rk/rv1126/buildroot/output/rockchip_rv1126_rv1109_weston_qt/host")

#set(SYSROOT_PATH  ${TOOLCHAIN_DIR}/aarch64-buildroot-linux-gnu/sysroot)
set(SYSROOT_PATH  ${TOOLCHAIN_DIR}/arm-buildroot-linux-gnueabihf/sysroot)
set(CMAKE_SYSROOT "${SYSROOT_PATH}")



#SET(CMAKE_C_COMPILER ${TOOLCHAIN_DIR}/bin/aarch64-linux-gcc)
#SET(CMAKE_CXX_COMPILER ${TOOLCHAIN_DIR}/bin/aarch64-linux-g++)
SET(CMAKE_C_COMPILER ${TOOLCHAIN_DIR}/bin/arm-linux-gnueabihf-gcc)
SET(CMAKE_CXX_COMPILER ${TOOLCHAIN_DIR}/bin/arm-linux-gnueabihf-g++)
SET(CMAKE_FIND_ROOT_PATH  ${SYSROOT_PATH})



#set(SYSROOT_PATH  ${tools_path}/host/aarch64-buildroot-linux-gnu/sysroot)
#set(CMAKE_SYSROOT ${SYSROOT_PATH})




