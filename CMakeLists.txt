cmake_minimum_required(VERSION 3.28)

project(x-stitch-editor)

set(NANOGUI_BUILD_EXAMPLES OFF CACHE BOOL " " FORCE)
set(NANOGUI_BUILD_PYTHON   OFF CACHE BOOL " " FORCE)
set(NANOGUI_INSTALL        OFF CACHE BOOL " " FORCE)

add_subdirectory(third_party/nanogui)
set_property(TARGET glfw glfw_objects nanogui PROPERTY FOLDER "dependencies")
add_definitions(${NANOGUI_EXTRA_DEFS})
include_directories(${NANOGUI_EXTRA_INCS})

add_subdirectory(third_party/tinyxml2)
add_subdirectory(third_party/fmt)
add_subdirectory(third_party/glm)

file(GLOB_RECURSE SOURCES src/*.cpp)

add_executable(${PROJECT_NAME} ${SOURCES})
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 20)

target_link_libraries(${PROJECT_NAME} PRIVATE nanogui ${NANOGUI_EXTRA_LIBS})
target_link_libraries(${PROJECT_NAME} PRIVATE tinyxml2)
target_link_libraries(${PROJECT_NAME} PRIVATE fmt)
target_link_libraries(${PROJECT_NAME} PRIVATE glm::glm-header-only)
