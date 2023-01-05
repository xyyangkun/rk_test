

add_executable(rkmppenc rkmppmain.cpp RkMppEncoder.cpp )
target_link_libraries(rkmppenc rga drm rockchip_mpp easymedia utils ${LINK_ASAN})

add_executable(test_dec_enc test_dec_enc.cpp RkMppEncoder.cpp ../h264_file_parse.c)
target_link_libraries(test_dec_enc rga drm rockchip_mpp easymedia utils zlog ${LINK_ASAN})
set_target_properties(test_dec_enc PROPERTIES  COMPILE_FLAGS "-DZLOG -DRKMPPENCODE_DEBUG=1 -DYKH264_PARSE_DEBUG=1")
