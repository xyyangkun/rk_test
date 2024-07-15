message("in src level:")

set(osee_player_src Mp4Play.cpp rk_h264_decode.c mb_get.cpp Decoder.cpp Decoder.h)
add_library(rvplayer STATIC  ${osee_player_src})
target_include_directories(rvplayer PRIVATE  easymedia)

ADD_DEFINITIONS(-DZLOG)
set(LINK_LIB_LIST m pthread asound avformat avdevice avcodec avfilter swresample avutil SDL2 SDL2_ttf rga drm
        rockchip_mpp  z asound v4l2 v4lconvert jpeg easymedia  ${LINK_ASAN} zlog fdk-aac jemalloc png)

###################################################################################
# rk3568 sample code
add_executable(test_mp4 test_mp4.cpp)
target_link_libraries(test_mp4 ${LINK_LIB_LIST} rvplayer)
#编译目标设置-DTEST宏
#https://stackoverflow.com/questions/5096881/does-set-target-properties-in-cmake-override-cmake-cxx-flags
set_target_properties(test_mp4 PROPERTIES  COMPILE_FLAGS "-DTEST") # ok
add_custom_command(TARGET test_mp4
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy "test_mp4" "/nfs/rk3568/osee_live/"
        COMMAND ${CMAKE_COMMAND} -E md5sum "test_mp4"
        )