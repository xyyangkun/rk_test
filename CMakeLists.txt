project(test)

add_executable(test test.c)
target_link_libraries(test rga drm drm pthread z v4l2 v4l2rds  rockchip_mpp m)
#target_link_libraries(set-plane-set rga drm pthread z v4l2 v4l2rds  rockchip_mpp -lm)

