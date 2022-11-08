
include_directories(inc)

AUX_SOURCE_DIRECTORY(. SRCS)
add_executable(MPPEncode ${SRCS} MppEncoder.cpp MppEncoder.h main.cpp)

LINK_DIRECTORIES(${CMAKE_CURRENT_LIST_DIR}/mpp_encode/lib)
target_link_libraries(MPPEncode rockchip_mpp utils pthread)
