

###################################################################################
# rv1126 sample code
set(target "alsa_read_write")
add_executable(${target} ${target}.cpp alsa_log.c UnlockQueue.cpp time_utils.c alsa.cpp sound_card_get.c usb_camera_notify.c usb_camera_get.c usb_camer_audio.cpp)
target_link_libraries(${target} asound soxr rveffects_static)
#target_include_directories(${target} PRIVATE ${SYSROOT_PATH}/usr/include)
target_include_directories(${target} PRIVATE  /works/rk3568/rk3568_v1_2_4/buildroot/output/rockchip_rk3568/host/aarch64-buildroot-linux-gnu/sysroot/usr/include )
#编译目标设置-DTEST宏
#https://stackoverflow.com/questions/5096881/does-set-target-properties-in-cmake-override-cmake-cxx-flags
#set_target_properties(${target} PROPERTIES  COMPILE_FLAGS "-DTEST") # ok
#set(copy_target "../../release/linux/${BuildType}/${target}")
add_custom_command(TARGET ${target}
        POST_BUILD
        COMMAND pwd
        COMMAND ${CMAKE_COMMAND} -E copy "../../release/linux/${BuildType}/${target}" "/nfs/rk3568/"
        COMMAND ${CMAKE_COMMAND} -E md5sum "../../release/linux/${BuildType}/${target}"
        )



###################################################################################
# rv1126 sample code
set(target "alsa_hello")
add_executable(${target} ${target}.cpp UnlockQueue.cpp)
#target_link_libraries(${target} asound)
#编译目标设置-DTEST宏
#https://stackoverflow.com/questions/5096881/does-set-target-properties-in-cmake-override-cmake-cxx-flags
#set_target_properties(${target} PROPERTIES  COMPILE_FLAGS "-DTEST") # ok
set(copy_target "../../release/linux/${BuildType}/${target}")
add_custom_command(TARGET ${target}
        POST_BUILD
        COMMAND pwd
        COMMAND ${CMAKE_COMMAND} -E copy "${copy_target}" "/nfs/rk3568/"
        COMMAND ${CMAKE_COMMAND} -E md5sum "${copy_target}"
        )



###################################################################################
# rv1126 sample code
set(target "test_soxr")
add_executable(${target} ${target}.cpp)
target_link_libraries(${target} soxr)
#target_include_directories(${target} PRIVATE ${SYSROOT_PATH}/usr/include)
target_include_directories(${target} PRIVATE  /works/rk3568/rk3568_v1_2_4/buildroot/output/rockchip_rk3568/host/aarch64-buildroot-linux-gnu/sysroot/usr/include )

#编译目标设置-DTEST宏
#https://stackoverflow.com/questions/5096881/does-set-target-properties-in-cmake-override-cmake-cxx-flags
#set_target_properties(${target} PROPERTIES  COMPILE_FLAGS "-DTEST") # ok
set(copy_target "../../release/linux/${BuildType}/${target}")
add_custom_command(TARGET ${target}
        POST_BUILD
        COMMAND pwd
        COMMAND ${CMAKE_COMMAND} -E copy "${copy_target}" "/nfs/rk3568/"
        COMMAND ${CMAKE_COMMAND} -E md5sum "${copy_target}"
        )

