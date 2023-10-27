cmake_minimum_required (VERSION 3.13..3.18)
project(x_stitch_editor)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")

add_subdirectory(ext/nanogui)
include_directories(ext/nanogui/include)
include_directories(${NANOGUI_EXTRA_INCS})
add_definitions(${NANOGUI_EXTRA_DEFS})

# include_directories(lib/embed-resource)
# add_subdirectory(lib/embed-resource)

# embed_resources(MyResources shader/vert.glsl shader/frag.glsl)

# add_executable(mytest test.cpp ${MyResources})
add_executable(XStitchEditor main.cpp)
target_link_libraries(XStitchEditor nanogui ${NANOGUI_EXTRA_LIBS})