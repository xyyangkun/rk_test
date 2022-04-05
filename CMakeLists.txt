project(test)

add_executable(test test.c)
target_link_libraries(test rga drm drm pthread z v4l2 v4l2rds  rockchip_mpp m)
#target_link_libraries(set-plane-set rga drm pthread z v4l2 v4l2rds  rockchip_mpp -lm)

add_executable(test_rga test_rga.cpp rkRgaApi.cpp)
target_link_libraries(test_rga rga drm easymedia drm pthread z v4l2 v4l2rds  rockchip_mpp m )

add_executable(test_im2d test_im2d.cpp )
target_link_libraries(test_im2d rga drm easymedia drm pthread z v4l2 v4l2rds  rockchip_mpp m )

