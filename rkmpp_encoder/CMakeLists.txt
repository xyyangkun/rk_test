

add_executable(rkmppenc rkmppmain.cpp RkMppEncoder.cpp )
target_link_libraries(rkmppenc rga drm rockchip_mpp easymedia utils ${LINK_ASAN})

add_executable(test_dec_enc test_dec_enc.cpp RkMppEncoder.cpp ../h264_file_parse.c)
target_link_libraries(test_dec_enc rga drm rockchip_mpp easymedia utils ${LINK_ASAN})
